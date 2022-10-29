#include "stdafx.h"
#include "ICarDev.h"

void  Clear_WSAECONNRESET(SOCKET h)
{
	DWORD dwBytesReturned = 0;
	BOOL  bNewBehavior = FALSE;
	DWORD status;

	status = WSAIoctl(h, SIO_UDP_CONNRESET,
						&bNewBehavior, sizeof(bNewBehavior),
						NULL, 0, &dwBytesReturned,
						NULL, NULL);

	if (SOCKET_ERROR == status)
	{
		DWORD dwErr = WSAGetLastError();
		if (WSAEWOULDBLOCK == dwErr)
		{
		}
		else
		{
		}
	}
}
int ICarDev::StatusFromString(const string st)
{	
	const char *sz = st.c_str();

	//常用
	if(stricmp(sz,"normal") == 0)	return dev_normal;
	if(stricmp(sz,"ready") == 0)	return dev_ready;
	if(stricmp(sz,"busy") == 0)		return dev_busy;

	//	
	if(stricmp(sz,"producing") == 0)return dev_producing;
	if(stricmp(sz,"installing") == 0)return dev_installing;
	if(stricmp(sz,"close") == 0)	return dev_close;
	if(stricmp(sz,"maintain") == 0)	return dev_maintain;
	if(stricmp(sz,"fault") == 0)	return dev_fault;
	
	return dev_unknown;
}
const string ICarDev::StatusString(int s)
{
	string sz("");
	switch(s)
	{
	//case dev_unknown:
	case dev_producing:	sz = "producing";	break;
	case dev_installing:sz = "installing";	break;
	case dev_close:		sz = "close";		break;
	case dev_maintain:	sz = "maintain";	break;
	case dev_fault:		sz = "fault";		break;
	case dev_normal:	sz = "normal";		break;
	case dev_ready:		sz = "ready";		break;
	case dev_busy:		sz = "busy";		break;
	default:			sz = "unknown";	break;
		break;
	}
	return sz;
}
UdpSocket::UdpSocket(string ip,unsigned short pt)
{
	hSocket = WSASocket(AF_INET,SOCK_DGRAM,IPPROTO_UDP,NULL,0,0);

	ips = ip;
	port = pt;

	if(hSocket != SOCKET_ERROR)
	{
		sockaddr_in adr;

		memset(&adr,sizeof(sockaddr_in),0);	//置0

		adr.sin_port = htons(port);
		adr.sin_family = AF_INET;
		adr.sin_addr.S_un.S_addr = inet_addr(ips.c_str());

		if(bind(hSocket,(sockaddr*)&adr,sizeof(SOCKADDR)) ==SOCKET_ERROR)
		{
			closesocket(hSocket);

			hSocket = SOCKET_ERROR;
		}
		else
		{
			//Clear SOCKET
			Clear_WSAECONNRESET(hSocket);

			CreateThread(NULL,0,Run,(LPVOID)this,0,0);

			
		}
			
	}
}

UdpSocket::~UdpSocket()
{
	Close();
}
void UdpSocket::Close()
{
	if(hSocket != SOCKET_ERROR)
		closesocket(hSocket);
}
int UdpSocket::OnReceiveString(char *buf,int len,const string &ip,unsigned short port)
{
	return 0;
}
int UdpSocket::OnTimeout(int nTimeouts)
{
	return 0;
}
string UdpSocket::GetMyIP()
{
	return ips;
}
unsigned short UdpSocket::GetMyPort()
{
	return port;
}
int UdpSocket::SendString(const char *data,int len,string ip,unsigned short port)
{
	if(hSocket == SOCKET_ERROR)
		return 0;

	WSABUF buf;
	buf.buf = (char*)data;
	buf.len = len;

	DWORD dwSend = 0;
	DWORD dwFlags = 0;

	sockaddr_in addr;
	memset(&addr,0,sizeof(sockaddr_in));

	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());

	WSASendTo(hSocket,&buf,1,&dwSend,dwFlags,(sockaddr*)&addr,sizeof(sockaddr_in),NULL,NULL);

	return 0;
}
DWORD WINAPI UdpSocket::Run(LPVOID pData)
{
	//Socket接收线程
	((UdpSocket*)pData)->OnRun();
	
	return 0;
}
void UdpSocket::OnRun()
{
	HANDLE hEvent = WSACreateEvent();

	WSAEventSelect(hSocket,hEvent,FD_READ|FD_WRITE|FD_CLOSE);

	char bytes[2048] = {0};
	WSABUF buf;
	buf.buf = bytes;
	buf.len = 2048;

	DWORD dwFlags = 0;
	DWORD dwRecv = 0;

	sockaddr_in addr;	
	int addr_len = sizeof(sockaddr);
	
	ULONG dwIP0 = 0;
	char ips[64] = {0};	//IP String
	UINT nTimeouts = 0;

	while(hSocket != SOCKET_ERROR)							//如果要退出线程，直接将hCtrlThread置成NULL
	{
		DWORD wait = ::WaitForSingleObject(hEvent,5000);
		switch(wait)
		{
		case WAIT_TIMEOUT:	
			OnTimeout(++nTimeouts);
			break;
		case WAIT_OBJECT_0:		
			{
				buf.len = 2048;
				
				
				do
				{
					dwRecv = 0;
					memset(bytes,0,2048);
					WSARecvFrom(hSocket,&buf,1,&dwRecv,&dwFlags,(sockaddr*)&addr,&addr_len,NULL,NULL);

					if(dwRecv > 0)
					{
						if(dwIP0 != addr.sin_addr.S_un.S_addr)
						{
							sprintf(ips,"%u.%u.%u.%u",
								addr.sin_addr.S_un.S_un_b.s_b1,
								addr.sin_addr.S_un.S_un_b.s_b2,
								addr.sin_addr.S_un.S_un_b.s_b3,
								addr.sin_addr.S_un.S_un_b.s_b4);
						}					
						OnReceiveString(buf.buf,dwRecv,string(ips),htons(addr.sin_port));
					}
					else
					{
						int err = WSAGetLastError();
					}
				}while(dwRecv > 0);

				ResetEvent(hEvent);

				if(nTimeouts > 5)	//如果超过5秒才收到一个消息，那么调用Timeout(0)
					OnTimeout(0);

				nTimeouts = 0;	//每当接收到一包数据，则重置timeout
			}
			break;
		default:break;
		}
		
	}

	CloseHandle(hEvent);
	return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
ICarDev::Records::Records()
{
	Reset();
}
void ICarDev::Records::Reset()
{
	rcds.clear();
	bChanged = false;
}
bool ICarDev::Records::IsChanged()
{
	return bChanged;
}
int ICarDev::Records::GetCount()
{
	return rcds.size();
}
ICarDev::Records::RcdItem *ICarDev::Records::Get(int index)
{
	if (index >= 0 && index < rcds.size())
		return &rcds[index];

	return NULL;
}
bool ICarDev::Records::Set(int nCourseNumber, int score, const char *assess)
{
	if (nCourseNumber <= 0)
		return false;

	if (score < -100)	score = -100;
	if (score > 100)	score = 100;	//有效范围

	//查找
	for (size_t i = 0; i < rcds.size(); i++)
	{
		RcdItem &t = rcds[i];

		if (t.number == nCourseNumber)
		{
			if (score > t.score)
			{
				t.bChanged = true;
				t.score = score;
			}
			if(t.AssessValue(string(assess)) > t.AssessValue())
			{
				t.bChanged = true;
				t.assess = assess;				
			}

			if (t.bChanged)
				bChanged = true;
			break;
		}
	}

	RcdItem item;
	item.bChanged = true;
	item.number = nCourseNumber;
	item.score = score;
	item.assess = assess;

	rcds.push_back(item);
	bChanged = true;

	return true;
}
bool ICarDev::Records::ParseAndInsertItem(string &item)
{
	//不包含{}
	//number:102,score:100,assess:'通过'
	const char *pch = strstr(item.c_str(),"number:");

	if (pch == NULL)	return false;

	RcdItem t;

	t.number = atoi(pch + 7);

	pch = strstr(item.c_str(), "score:");
	t.score = pch != NULL ? atoi(pch + 6) : 0;

	pch = strstr(item.c_str(), "assess:");
	if (pch != NULL)
	{
		t.assess = pch + 7;		
		size_t x = t.assess.find(',');
		
		if (x != t.assess.npos)	//assess:"通过",number:104,...
		{
			t.assess = t.assess.substr(0, x);
		}

		t.assess = ICarDev::TrimString(t.assess);	//去掉"",'',以及空格
	}
	else
	{
		t.assess = "未通过";
	}

	//直接添加
	t.bChanged = false;
	rcds.push_back(t);

	return true;
}
int ICarDev::Records::ParseRecords(const string &szJson, bool bReset)	//根据字符串解析（下载记录）
{
	if (bReset)
	{
		Reset();
	}

	if (szJson.length() <= 4)//[{...}]至少大于4
		return 0;

	//JSON数据结构（数组）
	//[{number:102,score:100,assess:'通过'},{},{}]

	int items = 0;
	int idx0 = -1;	//'{'
	//int idx1 = -1;	//['}']
	for (int i = 0; i < szJson.length(); i++)
	{
		if (idx0 < 0 && szJson[i] == '{')
		{
			idx0 = i;
		}
		else if (idx0 >= 0 && szJson[i] == '}')
		{
			if (ParseAndInsertItem(szJson.substr(idx0 + 1, i - idx0 - 1)))
				items++;

			idx0 = -1;
		}
	}

	return items;
	
}
string ICarDev::Records::PackRecords(bool bAll)				//根据当前内容打包数据，bAll = false，只打包更改的
{
	return "";
}
////////////////////////////////////////////
void ICarDev::DetailItem::Reset()
{
	nCode = 0;
	name = "";
	step = "";
		
	//位置
	x = 0;
	y = 0;
	z = 0;
	
	offsec  = 0;	//从开始运行到该错误时间

	//操作状态
	for(int i=0;i<10;i++)
		infos[i] = 0;
}
void ICarDev::RECORD::Reset()
{
	ID = "";			//训练记录ID

	timestart = 0;		//开始运行时间
	seconds = 0;		//已经运行时间
	cs_type = "";
	cs_name = "";		//课程类型和名称
	courseID = 0;

	//评价信息
	scores = 0;
	faults = 0;	//

	assess = "";
	remark = "";

	details.clear();
}
ICarDev::ICarDev(string sn,string szHostIP,unsigned short port) : 
	UdpSocket(szHostIP,port)	//设备初始化，指定设备编号
{
	hostIP = "192.168.0.104";
	hostPort = 15005;
			
	data.status = dev_normal;	//
	data.sn = sn;
	data.name = "驾考模拟器";
	data.rem = "驾考模拟器";
	data.userID = "";
	data.userName = "";
	data.bRunning = false;

	pfnCallback = NULL;
	pCmdOperator = NULL;
	
}
ICarDev::~ICarDev()
{
	if(IsLogin())
		SendCmd("logoff", data.sn);
	if (IsConnected())
		SendCmd("unregister", data.sn);
}
string ICarDev::FormatTime(time_t t,bool bID)
{
	tm *T = localtime(&t);	//获取时间结构体

	char sz[128] = {0};
	if(bID)
	{
		sprintf(sz,"%d%02d%02d_%02d%02d%02d",		
			T->tm_year + 1900,
			T->tm_mon+ 1,
			T->tm_mday,
			T->tm_hour,
			T->tm_min,
			T->tm_sec);
	}
	else
	{		
		sprintf(sz,"%d-%02d-%02d %02d:%02d:%02d",
			T->tm_year + 1900,
			T->tm_mon+ 1,
			T->tm_mday,
			T->tm_hour,
			T->tm_min,
			T->tm_sec);
	}
	return string(sz);
}
string ICarDev::FromWchar(LPCWSTR sz)
{
	if(sz == NULL || sz[0] == 0)
		return string("");

	static char buf[2048] = {0};

	WideCharToMultiByte(CP_ACP, 0,sz, -1,buf,2048, NULL, NULL );  

    return string(buf);  
}
int ICarDev::SendCmds(const char *cmds)
{
	return SendString(cmds,strlen(cmds),hostIP,hostPort);
}
int ICarDev::SendCmd(const string &key,const string &params)
{
	char buf[1024] = {0};
	sprintf(buf,"<%s = %s/>",key.c_str(),params.c_str());

	return SendString(buf,strlen(buf),hostIP,hostPort);
}
string ICarDev::GetWaitingCmd()
{
	return GetWaiting() > 0 ? WaitingCmd : "";
}
int ICarDev::GetWaiting()				//查询正在等待的指令
{
	int dt = 10 - (time(NULL) - tWaiting);		//

	if (dt > 0 && dt <= 10)				//
		return dt;

	if (!WaitingCmd.empty())
		WaitingCmd = "";

	return 0;
}
void ICarDev::ClearWaiting(string cmd)	//取消指令
{
	if (cmd == WaitingCmd)
	{
		WaitingCmd = "";
		tWaiting = 0;
	}
}
void ICarDev::SetWaiting(string cmd)	//设置等待反馈指令
{
	WaitingCmd = cmd;

	tWaiting = time(NULL);
}
//设备登录
bool ICarDev::TryConnect(const string &szHostIP,unsigned short pt)
{
	if (GetWaiting() > 0)
		return false;

	//保留IP和Port
	hostIP = szHostIP;
	hostPort = pt;

	SetWaiting("register");

	return SendCmd("register",data.sn);	//
}
void ICarDev::TryDisconnect()
{
	if (GetWaiting() > 0)
		return;

	SetWaiting("unregister");

	SendCmd("unregister",data.sn);
}

//用户登录设备
bool ICarDev::TryLogin(const string &user)	//设备登录
{
	if (GetWaiting() > 0)
		return false;
	
	string params = user + ";" + data.sn;

	SetWaiting("login");

	return SendCmd("login",params);
}
void ICarDev::TryLogoff()				//注销
{
	if (GetWaiting() > 0)
		return;

	SetWaiting("logoff");
	SendCmd("logoff",data.sn);
}
void ICarDev::QueryState()	//查询一次状态
{
	if (IsConnected())
	{
		if(IsShowQRCode())
		{
			SendCmd("query",data.sn + ";qrcode");	//查询是否有云端登录
		}
		else if(IsLogin())
		{
			SendCmd("query", data.sn + ";islogin");
		}
		else
		{
			SendCmd("query", data.sn);
		}
	}
}
void ICarDev::QueryRecords()//查询一次训练记录(发送一次请求)
{
	if (IsLogin())
	{
		SendCmd("query.rcds", data.userID);	//查询指定用户的训练记录
	}
}
void ICarDev::UsingQRCode()
{
	if (data.status == dev_ready)
		tQRCodeShow = time(NULL);
}
int ICarDev::IsShowQRCode()	//是否允许二维码显示，返回允许显示剩余秒数
{
	int dt = 0;
	
	if (tQRCodeShow > 0)
	{
		if (data.status == dev_ready)	//只有Ready状态才有需求显示
			dt = 30 - (time(NULL) - tQRCodeShow);

		if (dt <= 0)
			tQRCodeShow = 0;	//不再显示
	}

	return dt < 0 ? 0 : dt;
}
bool ICarDev::IsConnected()			//是否已经连接到服务器（设备管理程序）
{
	return data.status >= dev_ready;
}
bool ICarDev::IsRunning()
{
	return data.bRunning;
}
bool ICarDev::IsLogin()				//是否已经有用户登录
{
	return data.status == dev_busy;
}

//开始课程
bool ICarDev::StartCourse(int courseID, const string& type,const string& name)
{
	if(data.bRunning || !IsLogin())
		return false;	//已经在运行

	data.rcd.Reset();	//首先重置信息

	time_t t1 = ::time(NULL);	
	data.rcd.ID = data.sn + "_" +  FormatTime(t1,true);	//生成一个记录ID 设备SN+Date
	data.rcd.courseID = courseID;
	data.rcd.cs_type = type;
	data.rcd.cs_name = name;			//记录当前课程信息
	data.rcd.timestart = t1;	//课程开始时间

	data.rcd.details.clear();			//全部清空	

	data.bRunning = true;

	char tmp[256] = { 0 };
	sprintf_s(tmp,"%s;%s;%d;%s",
		data.rcd.ID.c_str(),
		type.c_str(),
		courseID,
		name.c_str());

	SendCmd("run", tmp);// data.rcd.ID + ";" + type + ";" + name);	//开始课程

	return true;
}
//运行过程中添加评测数据（也可以在课程结束之前一起添加）
void ICarDev::AddDetail(DetailItem &item)
{
	if(!IsRunning())
		return;

	if(item.offsec <= 0)
		item.offsec = time(NULL) - data.rcd.timestart;

	data.rcd.details.push_back(item);	

		//训练记录提交
	char buf[1200] = {0};
			  //code;name;step;pos(x,y,z);offset;(infos[10])
	sprintf(buf,"%d;%s;%s;(%.3f,%.3f,%.3f);%d;(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)",
					 item.nCode,
					 item.name.c_str(),
					 item.step.c_str(),
					 item.x,item.y,item.z,
					 item.offsec,
					 item.infos[0],item.infos[1],item.infos[2],item.infos[3],item.infos[4],
					 item.infos[5],item.infos[6],item.infos[7],item.infos[8],item.infos[9]);	

	SendCmd("fault",buf);
}

//结束本次训练
void ICarDev::EndCourse(int score,const string& assess,const string& remark)
{
	if(!IsRunning())
		return;

	data.rcd.seconds = time(NULL) - data.rcd.timestart;

	data.rcd.scores = score;
	data.rcd.assess = assess;
	data.rcd.remark = remark;
	data.rcd.faults = data.rcd.details.size();
	
	///////////////////////////////////////
	//训练记录提交
	char buf[1200] = {0};

	//首先提交记录的基本信息
	sprintf(buf,"%d;%s;%s",
		data.rcd.scores,
		data.rcd.assess.c_str(),
		data.rcd.remark.c_str());

	SendCmd("end",buf);	

	data.bRunning = false;
}

void ICarDev::SetStatus(int s)
{
	if(s == data.status)
		return;	//没有变化

	data.status = s;

	OnCmd("statuschanged",StatusString(s),GetMyIP(),GetMyPort());	//状态更改
}
int ICarDev::OnReceiveString(char *buf,int len,const string &ip,unsigned short port)
{
	return ParseString(buf,len,ip,port);
}

CmdCallback *ICarDev::SetCallbackOperator(CmdCallback *cb)
{
	if(cb == pCmdOperator)
		return pCmdOperator;

	CmdCallback *tmp = this->pCmdOperator;
	if(cb != NULL)
		SetCallbackFunction(NULL);	//清空pfn

	pCmdOperator = cb;

	return tmp;
}
LPFCOMMAND ICarDev::SetCallbackFunction(LPFCOMMAND pfn)
{
	if(pfn == this->pfnCallback)
		return pfnCallback;

	LPFCOMMAND tmp = pfnCallback;

	if(pfn != NULL)
		SetCallbackOperator(NULL);

	pfnCallback = pfn;

	return tmp;
}
bool ICarDev::OnCmd(const string& key,const string& params,const string &ip,unsigned short port)
{
	if(key == "query")	//状态查询
	{
		SendCmd("status",StatusString(data.status));
	}
	else if(key == "setstatus")
	{
		SetStatus(StatusFromString(params));	//状态设置未Ready
	}
	else if(key == "join")	//用户登录/注销
	{
		if(params == "")	//如果没有用户信息，那么设备注销登录
		{
			data.userID = "";
			data.userName = "";
			SetStatus(dev_ready);
		}
		else
		{
			SetStatus(dev_busy);		//有用户使用

			int split = params.find_first_of(';');

			if(split < 0)	//只有用户ID
			{
				data.userID = params;		//
				data.userName = "noname";	//
			}
			else
			{
				data.userID = params.substr(0,split);
				data.userName = params.substr(split + 1,params.length() - split);
			}

		}
	}
	else if(key == "user.name")
	{
		data.userName = params;
	}
	else if (key == "user.rcds")
	{
		data.UserRcds.ParseRecords(params, true);
	}

	if(pfnCallback != NULL)
		return pfnCallback(key,params,ip,port);
	
	if(pCmdOperator  != NULL)
		return pCmdOperator->OnCmd(key,params,ip,port);

	return true;
}
string ICarDev::TrimString(const string& str)	//删除前后空格
{
	if(str.length() <= 0)
		return str;

	int n0 = str.find_first_not_of(' ');
	int n1 = str.find_last_not_of(' ');

	string sz = "";
	if(n0 >= 0 && n1 >=n0)
		sz = str.substr(n0,n1 - n0 + 1);

	if(sz.length() > 2)	//去掉"",''
	{
		if((sz.front() == '\"' && sz.back() == '\"') ||	//双引号
			(sz.front() == '\'' && sz.back() == '\''))	//单引号
			sz = sz.substr(1,sz.length() - 2);
	}

	return sz;
}
int ICarDev::ParseString(const char *txt,int len,const string &ip,unsigned short port)
{
	const char *pch0 = strchr(txt,'<');
	const char *pch1 = pch0 != NULL ? strstr(pch0,"/>") : NULL;	//查找成对的 </>
	const char *buf = txt;
	
	while(pch0 != NULL && pch1 != NULL)
	{
		char tmp[1200] = {0};
		strncpy(tmp,pch0 + 1,pch1 - pch0 - 1);
		strlwr(tmp);	//关键字转为小写

		char *eq = strchr(tmp,'=');

		if(eq != NULL)
			*eq = 0;
		
		strlwr(tmp);	//将等号前面的关键部分设置为小写		

		string key = eq != NULL ? string(tmp,eq - tmp) : tmp;
		string params = eq != NULL ? eq + 1 : "";

	

		OnCmd(TrimString(key),TrimString(params),ip,port);

		//下一条指令
		pch0 = strchr(pch1 + 1,'<');
		pch1 = pch1 = pch0 != NULL ? strstr(pch0,"/>") : NULL;	//查找成对的 </>		
	}


	return 0;
}
int ICarDev::OnTimeout(int nTimeouts)
{
	if(IsConnected())
	{
		QueryState();	//每隔一定时间查询一次状态

		if(nTimeouts > 0)	//如果超过5个周期没有数据反馈，那么有可能表示主控断网
		{
			char buf[128] = {0};
			sprintf(buf,"Host No Response [%d] seconds",nTimeouts * 5);

			OnCmd("timeout",buf,this->GetMyIP(),this->GetMyPort());

		}
		else if(nTimeouts == 0)
		{
			OnCmd("timeout","",this->GetMyIP(),this->GetMyPort());
		}
	}
	else if(this->hostIP != "")
	{
		//TryConnect(hostIP,hostPort);
		string cmd = GetWaitingCmd();

	}

	return __super::OnTimeout(nTimeouts);
}