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

using System.Configuration;
using System.Windows.Threading;

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
        public WxHTTP WxClient;
        public ICarDev devOnLan;

        private DispatcherTimer mDataTimer = null; //定时器
        private long timerExeCount = 0; //定时器执行次数

        public MainWindow()
        {
            InitializeComponent();

            WxClient = new WxHTTP();
            WxClient.WxHttpEventHandler += new EventHandler<WxHTTP.WxHttpArg>(WxHttpEvent);
            try
            {
                string devinfo = ConfigurationManager.AppSettings["Device"];
                string[] devinfos = devinfo.Split(';');

                string carinfo;
                string[] carinfos;

                devOnLan = new ICarDev(devinfos[0], Convert.ToInt32(devinfos[1]));
                devOnLan.UdpReceiveHandler += new EventHandler<UdpArg>(WxUdpEvent);
                // CarDev car=new CarDev(devOnLan, "", "");
                int num = 0;
                while (true)
                {
                    carinfo = ConfigurationManager.AppSettings["Device" + num.ToString()];
                    num++;
                    if (carinfo != null)
                    {
                        carinfos = carinfo.Split(';');
                        CarDev car = new CarDev(devOnLan, carinfos[0], carinfos[1]);
                        car.SetAddress(carinfos[2], Convert.ToInt32(carinfos[3]));
                        devOnLan.devs.Add(car);

                    }
                    else
                        break;
                }
            }
            catch (Exception e)
            {
                System.Windows.MessageBox.Show(e.Message);
            }
            InitTimer();
            StartTimer();
        }

        private void InitTimer()
        {
            if (mDataTimer == null)
            {
                mDataTimer = new DispatcherTimer();
                mDataTimer.Tick += new EventHandler(DataTimer_Tick);
                mDataTimer.Interval = TimeSpan.FromSeconds(1);
            }
        }
        private async void DataTimer_Tick(object sender, EventArgs e)
        {

            s2 = DateTime.Now;
            s1 = DateTime.Now;
            ++timerExeCount;
            if (timerExeCount == 1)
            {
                foreach (var item in devOnLan.devs)
                {
                    await item.InitSetStatus("normal");
                }
                foreach (var item in devOnLan.devs)
                {
                    item.isconnected = false;
                }

            }
            else timerExeCount = 2;

            foreach (var item in devOnLan.devs)            //遍历每台设备
            {

                if (item.isconnected == true)               //如果设备已经连接过,没连接的设备不考虑
                {
                    Console.WriteLine("ok");
                    if (item.oldbeat == item.beat)               //没有收到响应
                        item.beatcount++;
                    else
                    { item.beatcount = 0; Console.WriteLine("响应" + item.dev.sn); }  //有响应的提示

                    if (item.beatcount == 10)                    //10s没有响应(由于计时器精度问题，实际上大于10秒)
                    {
                        Console.WriteLine("死机" + item.dev.sn);
                        await item.InitSetStatus("normal");
                        // Console.WriteLine("normal");
                    }
                    if (item.beatcount > 10)                     //10s后一直发死机消息提醒
                    {
                        Console.WriteLine("死机" + item.dev.sn);
                        item.beatcount = 11; //防越界
                    }

                    item.oldbeat = item.beat;
                }
            }

        }
        DateTime s1;
        DateTime s2;
        public void StartTimer()
        {
            if (mDataTimer != null && mDataTimer.IsEnabled == false)
            {
                mDataTimer.Start();
                s1 = DateTime.Now;
            }
        }
        public void StopTimer()
        {
            if (mDataTimer != null && mDataTimer.IsEnabled == true)
            {
                mDataTimer.Stop();
            }
        }

        private void WxUdpEvent(object sender, UdpArg e)
        {
            UdpArg arg = e as UdpArg;

            //if (FilterQuery.IsChecked == true && arg.data.Contains("<query"))
            //    return;

            string x = DateTime.Now.ToLocalTime() + ">> " + arg.addr.Address.ToString() + " - " + arg.data + "\n";

            Dispatcher.Invoke(new Action(() => {

                if (FilterQuery.IsChecked == true && arg.data.Contains("<query"))
                    return;

                if (UdpRecvs.Text != null)
                    UdpRecvs.Text = x + UdpRecvs.Text;
                else
                    UdpRecvs.Text = x;
            }));
            //    Console.WriteLine("a"+arg.addr.Address.ToString());
            //   Console.WriteLine("b"+devOnLan.devs.First().addr.Address.ToString());
            foreach (var item in devOnLan.devs)
            {
                if (item.addr.Address.ToString() == arg.addr.Address.ToString())
                {
                    if (item.beat == 1) item.beat = 0;
                    else item.beat = 1;
                    Console.WriteLine();
                }
            }

            /*  if (WxClient.beat == 0) WxClient.beat = 1;
              else WxClient.beat = 0;*/
        }
        private void WxHttpEvent(object sender, EventArgs e)
        {
            WxHTTP.WxHttpArg arg = e as WxHTTP.WxHttpArg;

            switch (arg.KeyString)
            {
                case "Access_Token":Results.Text = arg.InfoString;
                    tkExpires.Text = "Reamains: " + WxClient.AccessToken.Remains().ToString("hh\\:mm\\:ss");
                    break;
                case "querydevices":
                    ParseDevicesResults(JObject.Parse(arg.Results));
                    break;
                case "updatedevices":
                    {
                        ReturnData.Text = arg.Results;
                        string x = devs.SelectedItem.ToString();

                        if (x != null && x[0] == '*')
                            x.Remove(0);
                     }
                    break;
                default:
                    break;
            }
        }        
        private void ParseDevicesResults(JObject jobj)
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
        private void Button_Click(object sender, RoutedEventArgs e)    //query
        {
            WxClient.QueryDevices();
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)      //get
        {
            if (WxClient.GetAccessToken() != null)
                tkExpires.Text = "Reamains: " + WxClient.AccessToken.Remains().ToString("hh\\:mm\\:ss");
            else
                WxClient.QueryAccessToken();
        }
        private void RadioButton_Checked(object sender, RoutedEventArgs e)        //radio
        {
            RadioButton rd = sender as RadioButton;
            status_string = rd.Content.ToString();
        }
        private void Button_Click_2(object sender, RoutedEventArgs e)       //change
        {
            if (status_string == null || status_string.Length <= 1 || devs.SelectedItem == null)
                return;           

            string []infos = devs.SelectedItem.ToString().Split(':');

            if (infos == null)
                return;

            //if (sn[0] == '*')
            //    sn = sn.Remove(0);

            WxClient.UpdateDevStatus(infos[0], status_string);
        }

        private void Button_Click_3(object sender, RoutedEventArgs e)     //send UDP
        {
            if (devOnLan.LastEndPoint != null)
            {
                devOnLan.SendString(UdpSend.Text,devOnLan.LastEndPoint);
            }
        }

        private void Button_Click_4(object sender, RoutedEventArgs e)       //clear
        {
            UdpRecvs.Text = "";
        }
        private void Button_Click_5(object sender, RoutedEventArgs e)       //clear
        {
             Window1 window1 = new Window1();
            window1.WindowStartupLocation = WindowStartupLocation.Manual;   //使新窗口位置在原来的位置上
            window1.Left = this.Left;  //使新窗口位置在原来的位置上
            window1.Top = this.Top;  //使新窗口位置在原来的位置上
            this.Hide();
             window1.Show();
        }
    }
}
