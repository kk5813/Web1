using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using System.Xml.Linq;
using System.Net.Http;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.IO;





namespace WpfApp1
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        public DateTime time_get_token;
        public string status_string;
        public string device_sn;
        public WxHTTP client;

        public MainWindow()
        {            
            InitializeComponent();
        }
        

        public class WxTocken
        {
            public WxTocken() { }
            public string access_token { get; set; }
            public long expires_in { get; set; }
        };
        public class WxDevice
        {
            public WxDevice() { }

            public string name { get; set; }
            public string code { get; set; }
            public string model { get; set; }
            public string status { get; set; }
            public string compnay { get; set; }
            public DateTime lastRun { get; set; }
        };
        private async Task GetToken()
        {
            using (HttpClient client = new HttpClient())
            {
                string uri = "https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&appid=wx54418a224baf3bc7&secret=a85a12b2d651c16ba17a304d62d1e82d";

                try
                {
                    client.Timeout = TimeSpan.FromSeconds(10);
                    HttpResponseMessage response = await client.GetAsync(uri);

                    response.EnsureSuccessStatusCode();

                    string responseBody = await response.Content.ReadAsStringAsync();

                    ReturnData.Text = responseBody;

                    WxTocken tk = JsonConvert.DeserializeObject<WxTocken>(responseBody);

                    if (tk != null)
                    {
                        Results.Text = tk.access_token;

                        TimeSpan dt = new TimeSpan(0, 0, int.Parse(tk.expires_in.ToString()));

                        tkExpires.Text = "Token Expire: " + dt.ToString();
                        time_get_token = DateTime.Now;
                    }
                }
                catch (HttpRequestException e)
                {
                    ReturnData.Text = e.Message;
                }
            }
        }
        private async Task GetDevs()
        {
            if (Results.Text == null || Results.Text.Length <= 10)
                return;

            using (HttpClient client = new HttpClient())
            {
                string uriQuery = "https://api.weixin.qq.com/tcb/databasequery?access_token=" + Results.Text;
                string QueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'devices\').field({\'_id\':false,\'sn\':true,\'status\':true}).limit(10).get()\"}";

                try
                {
                    client.Timeout = TimeSpan.FromSeconds(10);
                    
                    HttpContent t = new StringContent(QueryText);
                    t.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/json");

                    //查询
                    HttpResponseMessage response = await client.PostAsync(uriQuery, t);
                    string responseBody = await response.Content.ReadAsStringAsync();
                    JObject data = JObject.Parse(responseBody);

                    ParsePOSTData(data);
                }
                catch (HttpRequestException e)
                {
                }
            }
        }
        private async Task ChangeStatus(string devSN,string st)
        {
            if (Results.Text == null || Results.Text.Length <= 10)
                return;

            using (HttpClient client = new HttpClient())
            {
                string uriUpdate = "https://api.weixin.qq.com/tcb/databaseupdate?access_token=" + Results.Text;
                string UpdateText =  "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'devices\').where({\'sn\':\'yyyyyy\'}).update({\'data\':{\'status\':\'xxxxxx\'}})\"}";

                UpdateText = UpdateText.Replace("xxxxxx", st);
                UpdateText = UpdateText.Replace("yyyyyy",devSN);

                try
                {
                    client.Timeout = TimeSpan.FromSeconds(10);

                    HttpContent u = new StringContent(UpdateText);
                    u.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/json");

                    HttpResponseMessage response = await client.PostAsync(uriUpdate, u);

                    string responseBody = await response.Content.ReadAsStringAsync();

                    ReturnData.Text = responseBody;

                }
                catch (HttpRequestException e)
                {
                }
            }
        }
        private async Task Get(string uri = "")
        {
            using (HttpClient client = new HttpClient())
            {
                try
                {
                    client.Timeout = TimeSpan.FromSeconds(10);
                    HttpResponseMessage response = await client.GetAsync(uri);

                    response.EnsureSuccessStatusCode();

                    string responseBody = await response.Content.ReadAsStringAsync();



                    WxTocken tk = JsonConvert.DeserializeObject<WxTocken>(responseBody);

                    if (tk != null)
                    {
                        Results.Text = tk.access_token;
                        tkExpires.Text = "Token Expire after seconds: " + tk.expires_in.ToString();
                        string uriQuery = "https://api.weixin.qq.com/tcb/databaseupdate?access_token=" + tk.access_token;

                        string UpdateText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'devices\').where({\'sn\':\'CAR_DT_2022001\'}).update({\'data\':{\'status\':\'PC Normal\'}})\"}";
                        HttpContent u = new StringContent(UpdateText);
                        u.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/json");

                        response = await client.PostAsync(uriQuery,u);

                        responseBody = await response.Content.ReadAsStringAsync();

                        uriQuery = "https://api.weixin.qq.com/tcb/databasequery?access_token=" + tk.access_token;
                        string QueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'devices\').field({\'_id\':false,\'sn\':true,\'status\':true}).limit(10).get()\"}";
                        

                        //Newtonsoft.Json.Linq.JObject data = Newtonsoft.Json.Linq.JObject.Parse(QueryText);
                        //string x = data.ToString();


                        HttpContent t = new StringContent(QueryText);
                        t.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/json");

                        //查询
                        response = await client.PostAsync(uriQuery, t);

                        responseBody = await response.Content.ReadAsStringAsync();

                        JObject data = JObject.Parse(responseBody);

                        ParsePOSTData(data);





                        //List<WxDevice> devs = JsonConvert.DeserializeObject<List<WxDevice>>(x);
                        //if (devs != null)
                        //{
                        //    foreach (WxDevice dev in devs)
                        //    {
                        //        Console.WriteLine(dev.code + " - " + dev.name + " - " + dev.lastRun.ToLongTimeString());
                        //    }
                        //}
                    }
                    else
                    {
                        Results.Text = "Failed to Parse Json: " + responseBody;
                    }
                }
                catch (HttpRequestException e)
                {
                    Console.WriteLine(e.Message);
                }
            }
        }
        private void ParsePOSTData(JObject jobj)
        {
            if (jobj == null)
                return;

            JToken data = jobj["data"];
            JToken code = jobj["errcode"];
            JToken msg = jobj["errmsg"];
            JToken pager = jobj["pager"];

            if (code != null)
                ReturnData.Text = "ErrCode: [" + code.ToString() + "] - " + msg.ToString();
            else
                ReturnData.Text = "No ErrCode and ErrMessage";

            devs.ItemsSource = null;
            if (data != null && data.Count<JToken>() > 0)
            {
                List<string> lst = new List<string>();

                for (int i = 0; i < data.Count<JToken>(); i++)
                {
                    JToken t = data[i];

                    if (t.Type == JTokenType.String)
                    {
                        JToken dev = JToken.Parse(t.ToString());

                        if (dev == null)
                            continue;

                        JToken sn = dev["sn"];
                        JToken st = dev["status"];

                        lst.Add(sn.ToString() + ":" + st.ToString());
                    }
                }

                devs.ItemsSource = lst;
            }
            
        }
        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (client != null && client.GetAccessToken() != "")
            {
                client.QueryDevices();
            }
            return;
            GetDevs();
            //string QueryText = "{\"env\":\"cloud1-3gaaz1erabec81cd\",\"query\":\"db.collection(\'devices\').limit(10).get()\"}";

            //Newtonsoft.Json.Linq.JObject data = Newtonsoft.Json.Linq.JObject.Parse(QueryText);
            //string x = data.ToString();

            // Newtonsoft.Json.Linq.JToken a = data["env"];

            //string at = a.ToString();

            //Task a = Get("https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&appid=wx54418a224baf3bc7&secret=a85a12b2d651c16ba17a304d62d1e82d");
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            if (client == null)
            {
                client = new WxHTTP();
                client.QueryAccessToken();
            }
            return;
            //Get Access Token
            TimeSpan dt = new TimeSpan(0);

            if(time_get_token != null)
                dt = DateTime.Now - time_get_token;

            if (dt.TotalSeconds > 7200)
                GetToken();
            else
            {
                int seconds = 7200 - dt.Seconds;
                dt = new TimeSpan(0,0,seconds);

                tkExpires.Text = "Token Expire: "  + dt.ToString();
            }
        }
        private void RadioButton_Checked(object sender, RoutedEventArgs e)
        {
            RadioButton rd = sender as RadioButton;
            status_string = rd.Content.ToString();
        }
        private void Button_Click_2(object sender, RoutedEventArgs e)
        {            
            if (status_string == null || status_string.Length <= 1 || devs.SelectedItem == null)
                return;           

            string []infos = devs.SelectedItem.ToString().Split(':');

            if (infos == null)
                return;           

            ChangeStatus(infos[0], status_string);
        }
    }
}


/*
  {{
  "errcode": 0, 
  "errmsg": "ok",
  "pager": 
    {
    "Offset": 0,
    "Limit": 10,
    "Total": 3
    },
    "data": 
    [
    "{\"_id\":\"617ef50c6231a0230be94b9600638ca9\",\"acceptBy\":\"苏虎\",\"accumulation\":\"120\",\"address\":\"犀浦-交大驾校\",\"admin\":\"王泽彬\",\"code\":\"CAR_DT\",\"compnay\":\"交大驾校\",\"contacts\":\"13980047908(黄),170000000（王）\",\"dateDeliver\":\"2022-3-15\",\"dateProduction\":\"2022-3-11\",\"installs\":\"2022年3月14日初装，15日安装测试完成\",\"lastRun\":\"2022-3-8-17:00:00\",\"model\":\"CAR_DT_MR\",\"name\":\"汽车驾驶考试模拟器(MR 手动)\",\"remark\":\"1号设备  MR，手动档\",\"sn\":\"CAR_DT_2022001\",\"status\":\"normal\",\"verifyBy\":\"黄艳\"}",
    "{\"_id\":\"41ae62ef6231a0930cbb89016f0826a4\",\"acceptBy\":\"苏虎\",\"accumulation\":\"120\",\"address\":\"犀浦-交大驾校\",\"admin\":\"王泽彬\",\"code\":\"CAR_DT\",\"compnay\":\"交大驾校\",\"contacts\":\"13980047908(黄),170000000（王）\",\"dateDeliver\":\"2022-3-15\",\"dateProduction\":\"2022-3-11\",\"installs\":\"2022年3月14日初装，15日安装测试完成\",\"lastRun\":\"2022-3-8-17:00:00\",\"model\":\"CAR_DT_SRC_AUTO\",\"name\":\"汽车驾驶考试模拟器(曲面屏 自动)\",\"remark\":\"2号设备  曲面屏，自动档\",\"sn\":\"CAR_DT_2022002\",\"status\":\"normal\",\"verifyBy\":\"黄艳\"}",  
    "{\"_id\":\"617ef50c6231a0ef0be967f60f02f3db\",\"acceptBy\":\"黄艳\",\"accumulation\":\"120\",\"address\":\"犀浦-交大驾校\",\"admin\":\"王泽彬\",\"code\":\"CAR_DT\",\"compnay\":\"交大驾校\",\"contacts\":\"13980047908(黄),170000000（王）\",\"dateDeliver\":\"2022/3/18 下午12:05:16\",\"dateProduction\":\"2022/3/18 下午12:04:41\",\"installs\":\"2022年3月14日初装，15日安装测试完成\",\"lastRun\":\"2022-3-8-17:00:00\",\"model\":\"CAR_DT_SRC\",\"name\":\"汽车驾驶考试模拟器(曲面屏 手动)\",\"remark\":\"3号设备  曲面屏，手动档\",\"sn\":\"CAR_DT_2022003\",\"status\":\"normal\",\"verifyBy\":\"黄艳\"}" 
    ]
  }}
 */
