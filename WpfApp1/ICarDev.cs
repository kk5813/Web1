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

namespace WpfApp1                //命名空间声明
{
    //设备信息
    public class Device
    {
        public string sn { get; set; }    //设备序列号
        public string status { get; set; }  //设备状态
        public string code { get; set; }    //设备类型编号
        public string name { get; set; }    //设备名称
        public string model { get; set; }    //产品型号
        public string remark { get; set; }  //设备说明
        public string company { get; set; }
        public string address { get; set; }

        public string userID { get; set; }  //当前使用该设备的用户
        public string userName { get; set; }  //

        public Device(string dev_sn, string dev_name)
        {
            sn = dev_sn;
            name = dev_name;
        }
    }
    //运行记录信息
    public class Record
    {
        public Record()
        {
            details = new List<RcdItem>();
        }
        public class RcdItem
        {
            public int code;
            public string name;
            public string step;

            public int offsec;          //偏移时间
            public double x, y, z;
            public int[] infos = new int[10];


        }

        public bool IsRuning = false;

        public string ID;
        public string userID;
        public string userName;
        public string courseID;    //课程ID
        public string sn;       //设备序列号
        public string date;
        public long seconds;    //训练时长
        public string type;
        public string name;     //课程类型和名称
        public DateTime dt;

        public long scores;     //总分
        public string assess;   //总评
        public string remark;   //备注

        public string Sdetails;  //训练细节

        public List<RcdItem> details;
    }
    public class CarDev
    {
        public WxHTTP WxClient;
        public Device dev;  //设备信息
        public Record rcd;  //当前记录信息
        public IPEndPoint addr { get; set; } //该设备所在的IP地址及端口号
        public ICarDev devs { get; set; }

        public int beat;
        public int oldbeat;
        public int beatcount;
        public bool isconnected;


        public CarDev(ICarDev idevs, string sn, string name)
        {
            devs = idevs;
            dev = new Device(sn, name);
            rcd = new Record();
            WxClient = new WxHTTP();

            beat = 0;
            oldbeat = 0;
            beatcount = 0;
            isconnected = false;
        }

        public void SetAddress(string ip, int port)       //联网
        {
            addr = new IPEndPoint(IPAddress.Parse(ip), port);
        }

        public async Task<bool> SetStatus(string str, bool bForeceToDev = false)
        {
            if (str == dev.status && !bForeceToDev)
                return true;    //不用处理

            ICarDev.DevStatus s = ICarDev.StatusFromString(str);    //标准化
            dev.status = ICarDev.StatusString(s);

            /*更改设备状态*/
            await WxClient.QueryAccessToken();                   //获取AccessToken
            WxClient.UpdateDevStatus(dev.sn, dev.status);        //上传修改状态

            //当设备状态更改后，发送状态更改指令给CarDev

            SendCmd("setstatus", dev.status);

            return true;
        }

        public async Task<bool> InitSetStatus(string str, bool bForeceToDev = false)
        {
            ICarDev.DevStatus s = ICarDev.StatusFromString(str);    //标准化
            dev.status = ICarDev.StatusString(s);
            /*更改设备状态*/
            await WxClient.QueryAccessToken();                   //获取AccessToken
            string where = "{\'sn\':\'" + dev.sn + "\'}";
            string fields = "{\'status\':\'" + dev.status + "\'" +
                "," + "\'userID\':\'" + "" + "\'" +
                "," + "\'userName\':\'" + "" + "\'}";
            WxClient.HttpUpdate(null, "devices", where, fields);
            return true;
        }

        void OnStartCourse(string param)             //记录开始训练的信息/*isRunning,ID,type,name,date*/
        {
            string[] infos = param.Split(';');  //ID,type,name

            if (infos.Length == 3)
            {
                rcd.IsRuning = true;
                rcd.ID = infos[0];
                rcd.type = infos[1];         //教学，练习，考试
                rcd.name = infos[2];
                rcd.dt = DateTime.Now;
                //rcd.date = rcd.dt.GetDateTimeFormats('f')[0].ToString();
                rcd.date = rcd.dt.ToLongDateString().ToString();
                rcd.date += " ";
                rcd.date += rcd.dt.ToLongTimeString().ToString();

                rcd.courseID = "1";
            }
            else if (infos.Length == 4)
            {
                rcd.IsRuning = true;
                rcd.ID = infos[0];
                rcd.type = infos[1];
                rcd.courseID = infos[2];
                rcd.name = infos[3];
                rcd.dt = DateTime.Now;
                //rcd.date = rcd.dt.GetDateTimeFormats('f')[0].ToString();
                rcd.date = rcd.dt.ToLongDateString().ToString();
                rcd.date += " ";
                rcd.date += rcd.dt.ToLongTimeString().ToString();
            }
        }
        public string[] SplitStringInBrackets(string data, char ch = ',')           //将字符串修改一下格式
        {
            data = data.Trim();

            int cnt = data.Last<char>() == ')' ? data.Length - 1 : data.Length;
            int idx0 = 0;
            if (data[0] == '(')
            {
                idx0 = 1;
                cnt--;
            }

            if (cnt > 0)
            {
                return data.Substring(idx0, cnt).Split(ch);
            }
            return null;
        }
        void OnEndCourse(string param)              //记录结束训练的信息
        {
            if (!rcd.IsRuning)            //结束训练
            {
                SendCmd("record", "fail:device is not running");
                return;
            }
            if (rcd.details.Count != 0)
                rcd.Sdetails = "[";
            int i = 0;
            foreach (Record.RcdItem element in rcd.details)
            {
                rcd.Sdetails += "{" + "code:" + element.code + "," + "name:" + element.name + "," +
                    "step:" + element.step + "," + "pos:" + "(" + element.x + "," + element.y + "," + element.z + ")" + ","
                     + "offsec:" + element.offsec + "," + "ops:" + "{" + "Eg:" + element.infos[0] + "," + "St:" +
                     element.infos[1] + "," + "Br:" + element.infos[2] + "," + "Ga:" + element.infos[3] + "," + "Cl:"
                     + element.infos[4] + "," + "Ge:" + element.infos[5] + "," + "HL:" + element.infos[6] + "," + "TL:"
                     + element.infos[7] + "," + "SB:" + element.infos[8] + "}" + "}";
                if (i < rcd.details.Count() - 1)
                {
                    rcd.Sdetails += "];";
                    i++;
                }
            }
            string[] infos = param.Split(';');

            if (infos.Length == 3)
            {
                rcd.scores = int.Parse(infos[0]);       //分数
                rcd.assess = infos[1];                  //评价
                rcd.remark = infos[2];                  //评价
            }
            rcd.seconds = (int)(DateTime.Now - rcd.dt).TotalSeconds;           //练习时长
            rcd.IsRuning = false;

            if (rcd.seconds > 60)              //要求训练记录大于一分钟
            {
                /*云数据库中添加一条训练记录*/
                WxClient.AddRecords(rcd, this);
                WxClient.AddUserRecord(this);
            }
            else
                SendCmd("record", "fail: record is too short(<60s)");
        }
        void OnAddFault(string param)       //添加评价
        {
            if (!rcd.IsRuning)
                return;

            string[] infos = param.Split(';');
            if (infos.Length >= 6)
            {
                Record.RcdItem item = new Record.RcdItem();

                item.code = int.Parse(infos[0]);
                item.name = infos[1];
                item.step = infos[2];
                item.offsec = int.Parse(infos[4]);

                string[] pos = SplitStringInBrackets(infos[3]);
                if (pos.Length == 3)
                {
                    item.x = float.Parse(pos[0]);
                    item.y = float.Parse(pos[1]);
                    item.z = float.Parse(pos[2]);
                }
                string[] xinfo = SplitStringInBrackets(infos[5]);
                if (xinfo.Length == 10)
                {
                    for (int i = 0; i < 10; i++)
                        item.infos[i] = int.Parse(xinfo[i]);
                }
                else
                {
                    for (int i = 0; i < 10; i++)
                        item.infos[i] = 0;
                }

                rcd.details.Add(item);
                SendCmd("answer", "ok;fault;add a fault");
            }
        }
        //格式[{number,score,remark},{number,score,remark},{number,score,remark},{number,score,remark}]转字符串数组
        public string[] StoArray(string data)
        {
            List<string> a = new List<string>();
            string Date = "";
            for (int i = 0; i < data.Length; i++)
            {
                if (data[i] == '{')             //遇到{就开始填入
                {
                    while (data[i + 1] != '}')
                    {
                        Date = Date + data[i + 1];
                        i++;
                    }
                    a.Add(Date);
                    Date = "";
                }
            }
            string[] info = a.ToArray();
            return info;
        }
        public async Task OnCmd(string key, string datas)                   //对解析后的key param 进行实际的cmd操作
        {
            //key = datas
            switch (key)
            {
                case "query":
                    {
                        if (datas.Contains("qrcode"))          //二维码扫码
                        {
                            Console.WriteLine("有人在扫码");
                            Console.WriteLine(rcd.sn);
                            /*扫码时间内，查询是否有人扫码登陆成功*/
                            string fields = "{\'_id\':false,\'status\':true,\'userID\':true,\'userName\':true}";
                            string where = "{\'sn\':\'" + rcd.sn + "\'}";
                            await WxClient.QueryDevices_user(where, fields, this);
                        }
                        if (datas.Contains("islogin"))          //手机注销
                        {
                            string fields = "{\'_id\':false,\'status\':true,\'userID\':true,\'userName\':true}";
                            string where = "{\'sn\':\'" + rcd.sn + "\'}";
                            await WxClient.QueryDevices_Status(where, fields, this);
                        }
                        SendCmd("answer", "ok;query");
                    }
                    break;
                case "register":    //注册是否成功
                    {
                        if (datas == dev.sn)
                        {
                            rcd.sn = datas;
                            await SetStatus("ready", true);   //实际需要向数据库提交状态，成功后再返回
                            isconnected = true;        //表示已经连接
                            beatcount = 0;
                        }
                        else
                        {
                            SendCmd("answer", "fail;register;use wrong device SN");
                        }
                    }
                    break;
                case "login":   //向云数据库发送登录请求(SN,User)
                    {
                        string[] infos = datas.Split(';');
                        WxHTTP WxClient;
                        WxClient = new WxHTTP();
                        // WxClient.QueryAccessToken();                  
                        await WxClient.UpdateDevStatus_InputLogin(infos[1], infos[0], this);
                        //测试程序直接返回成功

                    }
                    break;
                case "logoff":  //向云数据库发送注销请求
                    {
                        SendCmd("user.name", "no user");
                        SendCmd("join", ""); //
                        //await SetStatus("ready",true);                  //修改云数据库内容
                        string where_device = "{\'sn\':\'" + rcd.sn + "\'}";
                        string field_device = "{\'status\':\'" + "ready" + "\'" +
                            "," + "\'userID\':\'" + "" + "\'" +
                            "," + "\'userName\':\'" + "" + "\'}";
                        await WxClient.HttpUpdate(null, "devices", where_device, field_device);
                    }
                    break;
                case "unregister":
                case "close":       //设备关闭/关机，向云数据库发送注销请求
                    await InitSetStatus("normal");
                    break;
                case "run":
                    OnStartCourse(datas);
                    break;  //开始课程
                case "fault":
                    OnAddFault(datas);
                    break;   //添加一条故障                                    
                case "end":
                    OnEndCourse(datas);
                    break;  //结束课程
                case "query.rcds":
                    await WxClient.QueryUserRecord(this);
                    break;
                default:
                    break;
            }
        }
        public bool SendCmd(string key, string param)
        {
            return devs.SendString("<" + key + "=" + param + "/>", addr);             //json格式
        }
        public void OnUdpReceived(object sender, EventArgs e)
        {
            UdpArg arg = e as UdpArg;

            //解析string  <key = params/>
            string x = arg.data;

            int idx0 = -1;
            int idx1 = -1;
            for (int i = 0; i < x.Length; i++)
            {
                if (x[i] == '<' && idx0 == -1)
                    idx0 = i;
                else if (idx0 >= 0)
                {
                    if (x[i] == '=')
                        idx1 = i;
                    else if (x[i] == '>' && x[i - 1] == '/')
                    {
                        //<key = params />
                        string key = null;
                        string param = "";

                        if (idx1 > idx0 + 1)    //<...=/>
                        {
                            key = x.Substring(idx0 + 1, idx1 - idx0 - 1);
                            param = x.Substring(idx1 + 1, i - idx1 - 2);
                        }
                        else if (idx1 < 0)       //无等号 <key/>
                        {
                            key = x.Substring(idx0 + 1, i - idx0 - 2);
                        }
                        else //<=..../>只有一个等号，任务无效
                        {
                        }

                        if (key != null && key.Length > 0)
                        {
                            key = key.ToLower();  //转化成小写
                            key = key.Trim();
                            param = param.Trim();

                            //去掉参数两侧的引号
                            if (param.Length >= 2)
                            {
                                if ((param[0] == '\'' && param.Last<char>() == '\'') ||
                                    (param[0] == '\"' && param.Last<char>() == '\"'))   //去掉双引号，单引号
                                    param = param.Substring(1, param.Length - 2);

                            }

                            OnCmd(key, param);
                        }

                        idx0 = -1;  //重置       
                        idx1 = -1;
                    }
                }
            }
        }
    }
    public class UdpArg : EventArgs
    {
        public string data;   //接收到的字符串

        public IPEndPoint addr;    //数据来源IP

        public UdpArg(string s, IPEndPoint ep)
        {
            data = s;
            addr = ep;
        }
    }
    public class ICarDev : UdpClient
    {
        public IPEndPoint LastEndPoint;
        public event EventHandler<UdpArg> UdpReceiveHandler;//自定义参数回调事件
        public List<CarDev> devs;

        static Dictionary<DevStatus, string> status;
        public enum DevStatus
        {
            dev_unknown = -1,
            dev_producing = 0,
            dev_installing,
            dev_close,
            dev_maintain,
            dev_fault,
            dev_normal,
            dev_ready,
            dev_busy
        };
        static void InitStatusStrings()
        {
            if (status != null)
                return;

            status = new Dictionary<DevStatus, string>();

            status.Add(DevStatus.dev_unknown, "unknown");
            status.Add(DevStatus.dev_producing, "producing");
            status.Add(DevStatus.dev_installing, "installing");
            status.Add(DevStatus.dev_close, "close");
            status.Add(DevStatus.dev_maintain, "maintain");
            status.Add(DevStatus.dev_fault, "fault");
            status.Add(DevStatus.dev_normal, "normal");
            status.Add(DevStatus.dev_ready, "ready");
            status.Add(DevStatus.dev_busy, "busy");
        }
        static public DevStatus StatusFromString(string s)
        {
            InitStatusStrings();
            foreach (var a in status)
            {
                if (a.Value == s)
                    return a.Key;
            }
            return DevStatus.dev_unknown;
        }
        static public string StatusString(DevStatus s)
        {
            InitStatusStrings();

            return status[s];
        }
        private void OnUdpReceived(object sender, EventArgs e)
        {
            UdpArg arg = e as UdpArg;

            foreach (CarDev dev in devs)                    //判断是哪一台设备发出的信息
            {
                bool b1 = dev.addr == arg.addr;
                bool b2 = dev.addr.Address == arg.addr.Address;
                bool b3 = dev.addr.Port == arg.addr.Port;

                if (dev.addr.Address.Address == arg.addr.Address.Address &&
                    dev.addr.Port == arg.addr.Port)
                {
                    dev.OnUdpReceived(sender, e);
                    break;  //
                }
            }
        }
        public ICarDev(string ips, int port) :
            base(new System.Net.IPEndPoint(IPAddress.Parse(ips), port))
        {
            UdpReceiveHandler += new EventHandler<UdpArg>(OnUdpReceived);

            devs = new List<CarDev>();
            Task.Run(() => ReceiveString());
        }
        private async void ReceiveString()                     //接收字符串
        {
            while (true)
            {
                UdpReceiveResult result = await ReceiveAsync();

                Encoding gb312 = Encoding.GetEncoding("gb2312");    //简体中文

                string str = gb312 == null ? Encoding.UTF8.GetString(result.Buffer) : gb312.GetString(result.Buffer);

                LastEndPoint = result.RemoteEndPoint;
                UdpReceiveHandler.Invoke(this, new UdpArg(str, result.RemoteEndPoint));
            }
        }
        public bool SendString(string s, IPEndPoint dest)            //发送字符串sendCmd
        {
            if (s == null || s.Length <= 0 || dest == null)
                return false;

            Encoding gb312 = Encoding.GetEncoding("gb2312");    //简体中文
            byte[] data = (gb312 != null) ?
                            data = gb312.GetBytes(s) :
                            data = Encoding.UTF8.GetBytes(s);

            Task<int> ret = SendAsync(data, data.Length, dest);

            return true;
        }
        public bool SendString(string s, string destIP, int port)
        {
            return SendString(s, new IPEndPoint(IPAddress.Parse(destIP), port));
        }
    }
}
