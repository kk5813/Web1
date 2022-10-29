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
using System.Net;

namespace WpfApp1
{
    /// <summary>
    /// Window1.xaml 的交互逻辑
    /// </summary>
    public partial class Window1 : Window
    {
        public DateTime time_get_token;
        public string device_sn;               //radioButton选中的设备
        public WxHTTP WxClient;
        public ICarDev devOnLan;
        public int rcv_count = 0;

        private DispatcherTimer mDataTimer = null; //定时器
        private long timerExeCount = 0; //定时器执行次数
        string path = System.Environment.CurrentDirectory + "\\";
        public Window1()
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
                int num = 1;
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
            if (timerExeCount == 1)      //第一次对每台设备进行初始化
            {
                foreach (var item in devOnLan.devs)
                {
                    await item.InitSetStatus("normal");
                    item.isconnected = false;
                }
                for (int i = 1; i <= devOnLan.devs.Count; i++)
                {
                    await Download("CAR_DT_2022N00" + i + ".png", ConfigurationManager.ConnectionStrings["qrcode" + i].ToString(),
                       path);
                    await Download("device" + i + ".png", ConfigurationManager.ConnectionStrings["device" + i].ToString(),
                       path);
                    ImageBrush brush = new ImageBrush();
                    string device = @path + "device" + i + ".png";
                    BitmapImage img = new BitmapImage(new Uri(device, UriKind.Absolute));
                    Button sc = (Button)MainGrid.FindName("device" + i);
                    brush.ImageSource = img;
                    sc.Background = brush;
                }
            }
            else
                timerExeCount = 2;
            /*上色在定时器中循环被调用*/
            if (WxClient.GetAccessToken() == null)
                WxClient.QueryAccessToken();
            foreach (var item in devOnLan.devs)
            {
                /*状态上色*/
                string fields = "{\'_id\':false,\'sn\':true,\'status\':true}";
                string where = "{\'sn\':\'" + item.dev.sn + "\'}";
                WxClient.HttpQuery("status_color", "devices", where, fields);
            }
            /*beat信号*/
            foreach (var item in devOnLan.devs)            //遍历每台设备
            {
                if (item.isconnected == true)               //如果设备已经连接过,没连接的设备不考虑
                {
                    Console.WriteLine("beat");
                    if (item.oldbeat == item.beat)               //没有收到响应
                        item.beatcount++;
                    else
                    { item.beatcount = 0; Console.WriteLine("响应" + item.dev.sn); }  //有响应的提示

                    if (item.beatcount == 10)                    //10s没有响应(由于计时器精度问题，实际上大于10秒)
                    {
                        Console.WriteLine("死机" + item.dev.sn);
                        await item.InitSetStatus("normal");
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
        public async Task Download(string fileName, string uri, string localPath)
        {
            WebClient wc = new WebClient();
            if (File.Exists(localPath + fileName))
            {
                return;
            }
            if (Directory.Exists(localPath) == false)
            {
                Directory.CreateDirectory(localPath);
            }
            wc.DownloadFile(uri, localPath + fileName);
            Console.WriteLine("successful down");
        }
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
        private void WxUdpEvent(object sender, UdpArg e)        //WXUDP事件
        {
            UdpArg arg = e as UdpArg;
            rcv_count++;                                  //收信计数
            string x = DateTime.Now.ToLocalTime() + ">> " + arg.addr.Address.ToString() + " - " + arg.data + "\n";
            Dispatcher.Invoke(new Action(() =>
            {
                if (FilterQuery.IsChecked == true && arg.data.Contains("<query"))
                    return;
                if (UdpRecvs.Text != null)
                    UdpRecvs.Text = x + UdpRecvs.Text;
                else
                    UdpRecvs.Text = x;
                if (rcv_count > 1500)               /*积攒过多信息，一次清空*/
                {
                    rcv_count = 0;
                    UdpRecvs.Text = "";
                }
            }));
            foreach (var item in devOnLan.devs)
            {
                if (item.addr.Address.ToString() == arg.addr.Address.ToString())
                {
                    if (item.beat == 1) item.beat = 0;
                    else item.beat = 1;
                    Console.WriteLine();
                }
            }
        }
        private void WxHttpEvent(object sender, EventArgs e)
        {
            WxHTTP.WxHttpArg arg = e as WxHTTP.WxHttpArg;
            if (arg.Results != null)
            {
                switch (arg.KeyString)
                {
                    /*显示二维码*/
                    case "keyInformation":
                        {
                            string[] info = arg.Results.Split('\\');
                            string name = info[3].Substring(1);
                            string sn = info[7].Substring(1);
                            string[] infos_w = ConfigurationManager.AppSettings["Device" + sn.Substring(sn.Length - 1, 1)].Split(';');
                            string status = info[11].Substring(1);
                            string userName = info[15].Substring(1);
                            string png = path + sn + ".png";
                            dev_name.Text = name;
                            dev_sn.Text = sn;
                            dev_userName.Text = userName;
                            qrcode.Source = new BitmapImage(new Uri(png));
                            dev_ip.Text = infos_w[2] + ";" + infos_w[3];
                            /*打印当前的课程信息*/
                            for (int i = 0; i < devOnLan.devs.Count; i++)
                            {
                                if (sn == devOnLan.devs[i].rcd.sn && status == "busy")
                                {
                                    course_name.Text = devOnLan.devs[i].rcd.name;
                                    course_type.Text = devOnLan.devs[i].rcd.type;
                                    course_time.Text = devOnLan.devs[i].rcd.seconds.ToString();
                                    rcd_faluts.Text = devOnLan.devs[i].rcd.details.Count().ToString();
                                    rcd_remark.Text = devOnLan.devs[i].rcd.remark;
                                    rcd_score.Text = devOnLan.devs[i].rcd.scores.ToString();
                                    if (devOnLan.devs[i].rcd.IsRuning)
                                        rcd_Update.Text = "未提交";
                                    else
                                        rcd_Update.Text = "已提交";
                                    break;
                                }
                                else
                                {
                                    course_name.Text = "";
                                    course_type.Text = "";
                                    course_name.Text = "";
                                    course_type.Text = "";
                                    course_time.Text = "";
                                    rcd_faluts.Text = "";
                                    rcd_remark.Text = "";
                                    rcd_score.Text = "";
                                    rcd_Update.Text = "";
                                }
                            }
                            if (arg.Results.Contains("\"errmsg\":\"ok\""))
                                UdpRecvs.Text = DateTime.Now.ToLocalTime() + ">> " + infos_w[2] + " - " + "<" + "查询设备信息成功" + "," + sn + "/>" + "\n" + UdpRecvs.Text;
                            else
                                UdpRecvs.Text = DateTime.Now.ToLocalTime() + ">> " + infos_w[2] + " - " + "<" + "查询设备信息失败" + "," + sn + "/>" + "\n" + UdpRecvs.Text;
                        }
                        break;
                    /*status的颜色*/
                    case "status_color":
                        {
                            string[] info = arg.Results.Split('\\');
                            string sn = info[3].Substring(1);
                            string[] infos_w = ConfigurationManager.AppSettings["Device" + sn.Substring(sn.Length - 1, 1)].Split(';');
                            string status = info[7].Substring(1);
                            Border sc = (Border)MainGrid.FindName(sn);
                            switch (status)
                            {
                                /*显示二维码*/
                                case "busy":
                                    {
                                        if (sc.Background != Brushes.Red)
                                        {
                                            sc.Background = Brushes.Red;
                                            UdpRecvs.Text = DateTime.Now.ToLocalTime() + ">> " + infos_w[2] + " - " + "<" + "检测状态变化" + "," + sn + "/>" + "\n" + UdpRecvs.Text;
                                        }
                                    }
                                    break;
                                case "ready":
                                    {
                                        if (sc.Background != Brushes.Green)
                                        {
                                            sc.Background = Brushes.Green;
                                            UdpRecvs.Text = DateTime.Now.ToLocalTime() + ">> " + infos_w[2] + " - " + "<" + "检测状态变化" + "," + sn + "/>" + "\n" + UdpRecvs.Text;
                                        }
                                    }
                                    break;
                                case "normal":
                                    {
                                        if (sc.Background != Brushes.Gray)
                                        {
                                            sc.Background = Brushes.Gray;
                                            UdpRecvs.Text = DateTime.Now.ToLocalTime() + ">> " + infos_w[2] + " - " + "<" + "检测状态变化" + "," + sn + "/>" + "\n" + UdpRecvs.Text;
                                        }
                                    }
                                    break;
                                default:
                                    UdpRecvs.Text = DateTime.Now.ToLocalTime() + ">> " + infos_w[2] + " - " + "<" + "状态异常！" + "," + sn + "/>" + "\n" + UdpRecvs.Text;
                                    break;
                            }
                        }
                        break;
                    case "Access_Token":
                        {
                            if (WxClient.GetAccessToken() != null)
                                UdpRecvs.Text = DateTime.Now.ToLocalTime() + ">> " + " - " + "<" + "获取Token成功" + "/>" + "\n" + UdpRecvs.Text;
                            else
                                UdpRecvs.Text = DateTime.Now.ToLocalTime() + ">> " + " - " + "<" + "获取Token失败，网络超时！" + "/>" + "\n" + UdpRecvs.Text;
                        }
                        break;
                    case "add":
                        {
                                UdpRecvs.Text = DateTime.Now.ToLocalTime() + ">> " + " - " + "<" + arg.Results + "/>" + "\n" + UdpRecvs.Text;
                        }
                        break;
                    default:
                        break;
                }
            }
            if (arg.Results != null && arg.Results.Contains("ok"))
                web_status.Text = "已连接";
            else
                web_status.Text = "未连接";
        }
        private void Click_Button(object sender, RoutedEventArgs e)
        {
            Button rd = sender as Button;
            device_sn = rd.Tag.ToString();

            string where = "{\'sn\':\'" + device_sn + "\'}";
            string fields = "{\'_id\':false,\'sn\':true,\'status\':true,\'name\':true,\'userName\':true}";
            WxClient.HttpQuery("keyInformation", "devices", where, fields);
        }

        private void Button_Click_Clear(object sender, RoutedEventArgs e)        //clear
        {
            UdpRecvs.Text = "";
        }

        /*private void Button_Click(object sender, RoutedEventArgs e)
        {
            MainWindow mainwindow = new MainWindow();
            mainwindow.WindowStartupLocation = WindowStartupLocation.Manual;   //使新窗口位置在原来的位置上
            mainwindow.Left = this.Left;  //使新窗口位置在原来的位置上
            mainwindow.Top = this.Top;  //使新窗口位置在原来的位置上
            this.Hide();
            mainwindow.Show();

        }*/
    }
}
