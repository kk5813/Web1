using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Xml.Linq;
using System.Net.Http;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Net.Sockets;
using System.Net;


namespace WpfApp1
{    
    public class WxHTTP : HttpClient
    {
        public Record rcd;
        public enum WxHttpTask
        {
            http_idle,
            http_query_accesstoken,
            http_query,
            http_update,
            http_call,
            http_unkown,
            http_add
        };

        //回调Event参数
        public class WxHttpArg : EventArgs
        {
            public WxHttpArg(string res, string key, string infos,string err = "ok")
            {
                Results = res;
                KeyString = key;
                InfoString = infos;
                ErrMsg = err;
            }

            public string Results { set; get; }
            public string KeyString { get; set; }
            public string InfoString { get; set; }
            public string ErrMsg { get; set; }
        }

        //微信HTTP调用读写接口标志
        public class WxToken
        {
            public WxToken()
            {
                expires_in = 0;
                access_token = "";
            }
            public string access_token { get; set; }
            public long expires_in { get; set; }

            public DateTime last_query;           

            public bool IsExpire()
            {
                if (expires_in > 100 && access_token != "")
                {
                    TimeSpan ex = TimeSpan.FromSeconds(expires_in - 100);   //提前100秒作废access_token

                    return ex < DateTime.Now - last_query;
                }
                return true;
            }
            public TimeSpan Remains()
            {                
                return TimeSpan.FromSeconds(expires_in) - (DateTime.Now - last_query);
            }
        };

        /*当前HTTP调用任务类型*/
        private WxHttpTask HttpTask { get; set; }
        public WxToken AccessToken { get; set; }
        public event EventHandler<WxHttpArg> WxHttpEventHandler;//自定义参数回调事件

        ///每次只能一个任务，所以调用前必须锁定，如果锁定不成功则不继续调用
        public bool Lock(WxHttpTask t)  
        {
            if (HttpTask != WxHttpTask.http_idle)
            {
                return false;
            }

            HttpTask = t;
            
            return true;
        }
        private void Unlock()
        {
            HttpTask = WxHttpTask.http_idle;
        }

        public WxHTTP()
        {
            Timeout = TimeSpan.FromSeconds(30);   //30秒

            WxHttpEventHandler = new EventHandler<WxHttpArg>(WxHttpEvent);
        }

        //获取当前Access Token
        public string GetAccessToken()
        {
            if (AccessToken == null || AccessToken.IsExpire())
            {
                return null;
            }
            return AccessToken.access_token;
        }

        //调用成功的回调处理函数
        private void OnResponse(string res, string key, string infos, string err = "ok")
        {
            WxHttpEventHandler.Invoke(this, new WxHttpArg(res, key, infos, err));
        }

        /// <summary>
        /// 查询Access Token
        /// </summary>
        /// <returns></returns>
        public async Task QueryAccessToken()
        {
            //当前Access有效，不重复查询
            if (AccessToken != null && !AccessToken.IsExpire())
                return;
            Console.WriteLine("获取到Token");
            //AccessToken查询地址
            string uri = "https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&appid=wx54418a224baf3bc7&secret=a85a12b2d651c16ba17a304d62d1e82d";

            if (!Lock(WxHttpTask.http_query_accesstoken))
            {
                OnResponse(null, "Access_Token","busy", "lock failed!");
                return; //繁忙中
            }

            try
            {
                HttpResponseMessage response = await GetAsync(uri);

                response.EnsureSuccessStatusCode();

                string responseBody = await response.Content.ReadAsStringAsync();                

                //解析
                AccessToken = JsonConvert.DeserializeObject<WxToken>(responseBody);

                if (AccessToken != null)    //记录查询时间
                {
                    AccessToken.last_query = DateTime.Now;
                    OnResponse(responseBody, "Access_Token", AccessToken.access_token);
                }
                else
                {
                    OnResponse(responseBody, "Access_Token", "","Error Response DeserializeObject(WxToken)");
                }
            }
            catch (HttpRequestException e)
            {
                AccessToken = null;
                OnResponse(null, "Access_Token", null,e.Message);
            }
            Unlock();
        }

        public HttpContent StringToJsonContent(string json_string)
        {
            HttpContent t = new StringContent(json_string);
            t.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/json");

            return t;
        }


        /// <summary>
        /// 查询接口函数
        /// </summary>
        /// <param name="key">回调标记字符串</param>
        /// <param name="table">查询数据库表格名称</param>
        /// <param name="where">筛选条件，格式为Json字符串 "{\'name\':'张三'}"</param>
        /// <param name="fields">查询字段，格式为Json字符串"{\'Number\':true,\'name\':true}"</param>
        /// <param name="limits">本次查询结果限制数量</param>
        /// <returns></returns>
        public async Task HttpQuery(string key,string table, string where,string fields,int limits = 10)
        {
            Unlock();
            if (!Lock(WxHttpTask.http_query))
                return;

            string uri = "https://api.weixin.qq.com/tcb/databasequery?access_token=" + GetAccessToken();

            string QueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" + table + "\')";            

            //条件
            if (where != null && where != "")
                QueryText += ".where(" + where + ")";

            //范围
            if (fields != null && fields != "")
                QueryText += ".field(" + fields + ")";

            //数量限制
            if(limits > 0)
                QueryText += ".limit(" + limits.ToString() + ")";

            QueryText += ".get()\"}";
                        
            try
            {
                //查询
                HttpResponseMessage response = await PostAsync(uri, this.StringToJsonContent(QueryText));
                string responseBody = await response.Content.ReadAsStringAsync();
                OnResponse(responseBody, key, table);
            }
            catch (HttpRequestException e)
            {
                OnResponse(null, key, table,e.Message);
            }

            Unlock();
            return;
        }
        /// <summary>
        /// 更新接口函数
        /// </summary>
        /// <param name="key">回调标记字符串</param>
        /// <param name="table">查询数据库表格名称</param>
        /// <param name="where">选条件，格式为Json字符串 "{\'name\':'张三'}"</param>
        /// <param name="datas">更新内容，格式为Json字符串 "{\'name\':'李四'}"</param>
        /// <returns></returns>
        public async Task HttpUpdate(string key, string table, string where, string datas)
        {
            Unlock();
            if (!Lock(WxHttpTask.http_update))
                return;
            string url = "https://api.weixin.qq.com/tcb/databaseupdate?access_token=" + GetAccessToken();
            string UpdateText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" + 
                                    table + "\').where(" +
                                    where + ").update({\'data\':" +
                                    datas + "})\"}";
            try
            {
                //查询
                HttpResponseMessage response = await PostAsync(url, this.StringToJsonContent(UpdateText));
                string responseBody = await response.Content.ReadAsStringAsync();
                OnResponse(responseBody, key, table);
            }
            catch (HttpRequestException e)
            {
                OnResponse(null, key, table,e.Message);
            }
            Unlock();
        }
        /// <summary>
        /// 查询Devices数据表
        /// </summary>
        public async void QueryDevices()
        {
            string fields = "{\'_id\':false,\'sn\':true,\'status\':true}";
            HttpQuery("querydevices", "devices", null, fields);
        }

        /// <summary>
        /// 查询Devices数据表的user信息与status信息
        /// </summary>
        public async Task QueryDevices_user(string where, string fields,CarDev C, string table = "devices", int limits = 1)
        {
            Unlock();
            if (!Lock(WxHttpTask.http_query))
                return;
            string uri = "https://api.weixin.qq.com/tcb/databasequery?access_token=" + GetAccessToken();
            string QueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" + table + "\')";
            //条件
            if (where != null && where != "")
                QueryText += ".where(" + where + ")";
            //范围
            if (fields != null && fields != "")
                QueryText += ".field(" + fields + ")";
            //数量限制
            if (limits > 0)
                QueryText += ".limit(" + limits.ToString() + ")";
            QueryText += ".get()\"}";
            try
            {
                //查询
                HttpResponseMessage response = await PostAsync(uri, this.StringToJsonContent(QueryText));
                string responseBody = await response.Content.ReadAsStringAsync();
                OnResponse(responseBody, null, table);
                /*解析出指令中的status，userID，userName*/
                if (responseBody.Contains("busy"))           //手机扫码登陆后
                {
                    C.rcd.userID = responseBody.Substring(111, 11);
                    C.rcd.userName = responseBody.Substring(140, 8);
                    string[] infos = C.rcd.userName.Split('\\');
                    C.rcd.userName = infos[0];
                    C.SendCmd("join", C.rcd.userID + ";" + C.rcd.userName);
                }
                else
                {
                    Console.WriteLine("状态是ready");
                }
            }
            catch (HttpRequestException e)
            {
                OnResponse(null, null, table, e.Message);
            }
            Unlock();
            return;
        }
        /// <summary>
        /// 查询Devices数据表的user信息与status信息
        /// </summary>
        public async Task QueryDevices_Status(string where, string fields, CarDev C, string table = "devices", int limits = 1)
        {
            Unlock();
            if (!Lock(WxHttpTask.http_query))
                return;
            string uri = "https://api.weixin.qq.com/tcb/databasequery?access_token=" + GetAccessToken();
            string QueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" + table + "\')";
            //条件
            if (where != null && where != "")
                QueryText += ".where(" + where + ")";
            //范围
            if (fields != null && fields != "")
                QueryText += ".field(" + fields + ")";
            //数量限制
            if (limits > 0)
                QueryText += ".limit(" + limits.ToString() + ")";
            QueryText += ".get()\"}";
            try
            {
                //查询
                HttpResponseMessage response = await PostAsync(uri, this.StringToJsonContent(QueryText));
                string responseBody = await response.Content.ReadAsStringAsync();
                OnResponse(responseBody, null, table);
                /*解析出指令中的status，userID，userName*/
                if (responseBody.Contains("ready"))           //手机扫码登陆后
                {
                    C.OnCmd("logoff","");
                }
            }
            catch (HttpRequestException e)
            {
                OnResponse(null, null, table, e.Message);
            }
            Unlock();
            return;
        }
        /// <summary>
        /// 更新指定序列号设备状态
        /// </summary>
        /// <param name="sn">设备序列号</param>
        /// <param name="status">设备状态</param>
        public void UpdateDevStatus(string sn, string status)
        {
            string where  = "{\'sn\':\'" + sn + "\'}";
            string fields = "{\'status\':\'" + status + "\'}";
            HttpUpdate("updatedevices", "devices", where, fields);
        }

        /// <summary>
        /// 默认回调处理函数
        /// </summary>
        /// <param name="sender">WxHTTP</param>
        /// <param name="e">WxhttpArg</param>
        private void WxHttpEvent(object sender, EventArgs e)
        {
            WxHttpArg arg = e as WxHttpArg;  
        }
        /// <summary>
        /// 数据库添加记录
        /// </summary>
        public async Task HttpAdd( string table, string datas , CarDev c)
        {
            Unlock();
            if (!Lock(WxHttpTask.http_add))
                return;
            string url = "https://api.weixin.qq.com/tcb/databaseadd?access_token=" + GetAccessToken();
            string AddText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" +
                                    table + "\').add({\'data\':" +
                                    datas + "})\"}";
            try
            {
                //添加
                HttpResponseMessage response = await PostAsync(url, this.StringToJsonContent(AddText));
                string responseBody = await response.Content.ReadAsStringAsync();
                OnResponse(responseBody, "add", table);
                if(responseBody.Contains("ok"))
                {
                    c.SendCmd("record", "ok:" + c.rcd.ID);
                }
                else
                {
                    c.SendCmd("record", "failed:" + c.rcd.ID);
                }
            }
            catch (HttpRequestException e)
            {
                OnResponse(null, null , table, e.Message);
            }

            Unlock();
        }

        public void AddRecords(Record rcd , CarDev c)
        {
            string data = "{\'_id\':\'" + rcd.ID + "\'" +                  //用户_id        Record.ID
                "," + "\'assess\':\'" + rcd.assess + "\'" +                //评估           Record.assess
                "," + "\'course\':\'" + rcd.name + "\'" +                  //科目           Record.name
                "," + "\'date\':\'" + rcd.date + "\'" +                    //日期           Record.date
                "," + "\'details\':\'" + rcd.Sdetails + "\'" +             //细节           Record.details
                "," + "\'device\':\'" + rcd.sn + "\'" +                    //设备           Record.sn
                "," + "\'faults\':\'" + rcd.details.Count() + "\'" +       //错误           Record.details.length
                "," + "\'remark\':\'" + rcd.remark + "\'" +                //评价           Record.remark
                "," + "\'score\':\'" + rcd.scores + "\'" +                 //得分           Record.scores
                "," + "\'seconds\':\'" + rcd.seconds + "\'" +              //训练时间       Record.seconds
                "," + "\'type\':\'" + rcd.type + "\'" +                    //模拟训练类型   Record.type
                "," + "\'userID\':\'" + rcd.userID + "\'" +                //用户ID         Record.userID
                "," + "\'userName\':\'" + rcd.userName + "\'" +            //用户姓名       Record.userName
                "," + "\'courseID\':\'" + rcd.courseID + "\'}";            //课程ID         Record.courseID
            HttpAdd("records", data , c);
            Unlock();
        }

        /*训练结束users表添加记录*/
        public async Task AddUserRecord(CarDev c , int limits = 1)
        {
            Unlock();
            if (!Lock(WxHttpTask.http_query))
                return;
            Console.WriteLine(1);
            string where = "{\'name\':\'" + c.rcd.userName + "\'}";
            string fields = "{\'_id\':false,\'records\':true}";
            string uri = "https://api.weixin.qq.com/tcb/databasequery?access_token=" + GetAccessToken();
            string QueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" + "users" + "\')";
            //条件
            if (where != null && where != "")
                QueryText += ".where(" + where + ")";
            //范围
            if (fields != null && fields != "")
                QueryText += ".field(" + fields + ")";
            //数量限制
            if (limits > 0)
                QueryText += ".limit(" + limits.ToString() + ")";
            QueryText += ".get()\"}";
            try
            {
                //查询
                HttpResponseMessage response = await PostAsync(uri, this.StringToJsonContent(QueryText));
                string responseBody = await response.Content.ReadAsStringAsync();
                OnResponse(responseBody, null, "users");
                Console.WriteLine(responseBody);
                if (responseBody.Contains("ok"))
                {
                    Console.WriteLine("查得到信息");
                    string[] infos = responseBody.Split('\\');
                    string record = infos[3].Substring(1);
                    Console.WriteLine(record);
                    if (record.Contains("{" + c.rcd.courseID))    //先前有记录
                    {
                        Console.WriteLine("{" + c.rcd.courseID + ",");
                        Console.WriteLine("有这类记录");
                        string[] Rcd;
                        Rcd = c.StoArray(record);
                        for (int i = 0; i < Rcd.Length; i++)
                        {
                            string[] sp = Rcd[i].Split(',');
                            if (sp[0] == c.rcd.courseID)
                            {
                                if (int.Parse(sp[1]) < c.rcd.scores)
                                {
                                    sp[1] = c.rcd.scores.ToString();
                                }
                                if (sp[2].ToCharArray()[0] > c.rcd.remark.ToCharArray()[0])
                                {
                                    sp[2] = c.rcd.remark;
                                }
                                Rcd[i] = sp[0] + "," + sp[1] + "," + sp[2];
                            }
                        }
                        record = "[";
                        for (int i = 0; i < Rcd.Length; i++)
                        {
                            record = record + "{" + Rcd[i] + "}";
                            if (i != Rcd.Length - 1)
                                record = record + ",";
                            else
                                record = record + "]";
                        }
                        string where_Q = "{\'name\':\'" + c.rcd.userName + "\'}";
                        string fields_Q = "{\'records\':\'" + record + "\'}";
                        string url = "https://api.weixin.qq.com/tcb/databaseupdate?access_token=" + GetAccessToken();
                        string UpdateText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" +
                                                "users" + "\').where(" +
                                                where_Q + ").update({\'data\':" +
                                                fields_Q + "})\"}";
                        try
                        {
                            //添加
                            HttpResponseMessage resp = await PostAsync(url, this.StringToJsonContent(UpdateText));
                            string respBody = await resp.Content.ReadAsStringAsync();
                            OnResponse(respBody, "add", "users");
                            Console.WriteLine("添加用户记录的结果是：" + respBody);
                        }
                        catch (HttpRequestException e)
                        {
                            OnResponse(null, null, null, e.Message);
                        }
                    }
                    else                                         //先前没有记录的
                    {
                        Console.WriteLine("没有这类记录");
                        record = record.Insert(record.Length - 1, ",{" + c.rcd.courseID + "," + c.rcd.scores.ToString() + "," + c.rcd.remark + "}");
                        string where_Q = "{\'name\':\'" + c.rcd.userName + "\'}";
                        string fields_Q = "{\'records\':\'" + record + "\'}";
                        string url = "https://api.weixin.qq.com/tcb/databaseupdate?access_token=" + GetAccessToken();
                        string UpdateText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" +
                                                "users" + "\').where(" +
                                                where_Q + ").update({\'data\':" +
                                                fields_Q + "})\"}";
                        try
                        {
                            //添加
                            HttpResponseMessage resp = await PostAsync(url, this.StringToJsonContent(UpdateText));
                            string respBody = await resp.Content.ReadAsStringAsync();
                            OnResponse(respBody, "add", "users");
                            Console.WriteLine("添加用户记录的结果是：" + respBody);
                        }
                        catch (HttpRequestException e)
                        {
                            OnResponse(null, null, null, e.Message);
                        }
                    }
                }
            }
            catch (HttpRequestException e)
            {
                OnResponse(null, null, "users", e.Message);
            }
            Unlock();
        }

        /*查询用户记录*/
        public async Task QueryUserRecord(CarDev c , int limits = 1)
        {
            Unlock();
            if (!Lock(WxHttpTask.http_query))
                return;
            string where = "{\'name\':\'" + c.rcd.userName + "\'}";
            string fields = "{\'_id\':false,\'records\':true}";
            await HttpQuery("", "users", where, fields);
            string uri = "https://api.weixin.qq.com/tcb/databasequery?access_token=" + GetAccessToken();

            string QueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" + "users" + "\')";

            //条件
            if (where != null && where != "")
                QueryText += ".where(" + where + ")";
            //范围
            if (fields != null && fields != "")
                QueryText += ".field(" + fields + ")";
            //数量限制
            if (limits > 0)
                QueryText += ".limit(" + limits.ToString() + ")";
            QueryText += ".get()\"}";
            try
            {
                //查询
                HttpResponseMessage response = await PostAsync(uri, this.StringToJsonContent(QueryText));
                string responseBody = await response.Content.ReadAsStringAsync();
                OnResponse(responseBody, null, "users");
                if (responseBody.Contains("ok"))
                {
                    string[] infos = responseBody.Split('\\');
                    string record = infos[3].Substring(1);
                    c.SendCmd("查询结果" , record);

                }
                else
                    c.SendCmd("查询", "失败");
            }
            catch (HttpRequestException e)
            {
                OnResponse(null, null, "users", e.Message);
            }
            Unlock();
            return;
        }
        public async Task UpdateDevStatus_InputLogin(string sn, string phone,CarDev c)
        {
          //  this.QueryAccessToken();
            /***********************获取当前时间**************************/
            int time_h;
            int time_m;
            string date;
            string week;
            int period=11;

            date = DateTime.Now.ToLongDateString();//获取当前日期 xxxx年xx月xx日
            string[] weekdays = { "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六" };
            week = weekdays[Convert.ToInt32(DateTime.Now.DayOfWeek)];   //获取当前星期
            string dateweek = date + " " + week; //xx年xx月xx日 星期x
           
            time_h = DateTime.Now.Hour; //获取当前小时 
            time_m = DateTime.Now.Minute;//获取当前分钟
            /****************************判断合法时间**********************************/
            switch (time_h)
            {
                case 8: { if (time_m > 55 && time_m < 60) period = 0; else period = -1; break; }
                case 9: { if (time_m >= 0 && time_m <= 10) period = 0; else if (time_m > 25 && time_m <= 40) period = 1; else if (time_m > 55 && time_m < 60) period = 2; else period = -1; break; }
                case 10: { if (time_m >= 0 && time_m <= 10) period = 2; else if (time_m > 25 && time_m <= 40) period = 3; else if (time_m > 55 && time_m < 60) period = 4; else period = -1; break; }
                case 11: { if (time_m >= 0 && time_m <= 10) period = 4; else if (time_m > 25 && time_m <= 40) period = 5; else period = -1; break; }
                case 12: { if (time_m > 55 && time_m < 60) period = 6; else period = -1; break; }
                case 13: { if (time_m >= 0 && time_m <= 10) period = 6; else if (time_m > 25 && time_m <= 40) period = 7; else if (time_m > 55 && time_m < 60) period = 8; else period = -1; break; }
                case 14: { if (time_m >= 0 && time_m <= 10) period = 8; else if (time_m > 25 && time_m <= 40) period = 9; else if (time_m > 55 && time_m < 60) period = 10; else period = -1; break; }
                case 15: { if (time_m >= 0 && time_m <= 10) period = 10; else if (time_m > 25 && time_m <= 40) period = 11; else if (time_m > 55 && time_m < 60) period = 12; else period = -1; break; }
                case 16: { if (time_m >= 0 && time_m <= 10) period = 12; else if (time_m > 25 && time_m <= 40) period = 13; else if (time_m > 55 && time_m < 60) period = 14; else period = -1; break; }
                case 17: { if (time_m >= 0 && time_m <= 10) period = 14; else if (time_m > 25 && time_m <= 40) period = 15; else period = -1; break; }
                default: { period = -1; break; }
            }
            /*if (period == -1)
            {
                c.SendCmd("error", "仅开课前5分钟或开课后10分钟内才可登录");
                return;
            }*/
            /****************************获得AccessToken**********************************/
            string uriq = "https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&appid=wx54418a224baf3bc7&secret=a85a12b2d651c16ba17a304d62d1e82d";
            HttpResponseMessage responseq = await GetAsync(uriq);
            responseq.EnsureSuccessStatusCode();
            string responseBody = await responseq.Content.ReadAsStringAsync();
            AccessToken = JsonConvert.DeserializeObject<WxToken>(responseBody);
           
            /****************************根据手机号确定请求登陆者的openID**********************************/
            string fields = "{\'_id\':false , \'openID\':true}";
            string where = "{\'phone\':\'" + phone + "\'}";
            string key = "updatedevices";
            string table = "users";

            if (!Lock(WxHttpTask.http_query))
                return;
           
            string uri = "https://api.weixin.qq.com/tcb/databasequery?access_token=" + AccessToken.access_token;
            string openIDQueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" +
                table + "\').where(" +
                where + ").field(" +
                fields + ").get()\"}";
            HttpResponseMessage response = await PostAsync(uri, this.StringToJsonContent(openIDQueryText));  //查询
            string useropenID = await response.Content.ReadAsStringAsync();

            /****************************根据手机号确定请求登陆者的姓名**********************************/
            fields = "{\'_id\':false , \'name\':true}";
            where = "{\'phone\':\'" + phone + "\'}";
            table = "users";
            string nameQueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" +
                table + "\').where(" +
                where + ").field(" +
                fields + ").get()\"}";
            HttpResponseMessage response_1 = await PostAsync(uri, this.StringToJsonContent(nameQueryText));  //查询
            string username = await response_1.Content.ReadAsStringAsync();
            /**********************************查询当前时段该设备的预约记录*****************************************/


            fields = "{\'_id\':false , \'openID\':true}";
            where = "{\'sn\':\'" + sn + "\',\'date\':\'" + dateweek + "\',\'period\':" + period + "}"; 
            table = "appointments";
            string userQueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'" +
            table + "\').where(" +
            where + ").field(" +
            fields + ").get()\"}";
            HttpResponseMessage response_2 = await PostAsync(uri, this.StringToJsonContent(userQueryText));  //查询
            string appointmentopenID = await response_2.Content.ReadAsStringAsync();
            string where_device, field_device;
            try
            {
                if (useropenID.Length == 79)
                    c.SendCmd("error", "手机号未注册/手机号错误");
                else
                {
                    if (appointmentopenID.Length == useropenID.Length && useropenID.Substring(92, 28) == appointmentopenID.Substring(92, 28))  //预约者本人登录
                    {

                        //成功匹配

                        c.SendCmd("join", phone + ";" + username.Substring(90, username.Length - 96));
                        //  UpdateDevStatus(sn,"busy");//没有效果
                        c.rcd.sn=sn;
                        c.rcd.userID=phone;
                        c.rcd.userName=username.Substring(90, username.Length - 96);
                        where_device = "{\'sn\':\'" + sn + "\'}";
                        field_device = "{\'status\':\'" + "busy" + "\'" +
                            "," + "\'userID\':\'" + phone + "\'" + 
                            "," + "\'userName\':\'" + username.Substring(90, username.Length - 96) + "\'}";
                        //c.SetStatus("busy");
                        Unlock();
                        await QueryAccessToken();
                        HttpUpdate(null, "devices", where_device, field_device);
                    }
                    else if (appointmentopenID.Length == useropenID.Length && useropenID.Substring(92, 28) != appointmentopenID.Substring(92, 28))//当前时段其他人预约了
                    {

                        c.SendCmd("error", "当前时段其他人预约");
                    }
                    else //当前时段没有人预约，设备空闲
                    {

                        c.SendCmd("join", phone + ";" + username.Substring(90, username.Length - 96));
                        //  UpdateDevStatus(sn, "busy");
                        //c.SetStatus("busy");
                        c.rcd.sn = sn;
                        c.rcd.userID = phone;
                        c.rcd.userName = username.Substring(90, username.Length - 96);
                        where_device = "{\'sn\':\'" + sn + "\'}";
                        field_device = "{\'status\':\'" + "busy" + "\'" +
                            "," + "\'userID\':\'" + phone + "\'" +
                            "," + "\'userName\':\'" + username.Substring(90, username.Length - 96) + "\'}";
                        Unlock();
                        await QueryAccessToken();
                        HttpUpdate(null, "devices", where_device, field_device);
                    }
                }

           }
           catch (HttpRequestException e)
           {
               OnResponse(null, key, table, e.Message);
           }
           
            Unlock();    
            return;
        }
    }
}
