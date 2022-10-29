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

	//����
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

		memset(&adr,sizeof(sockaddr_in),0);	//��0

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
	//Socket�����߳�
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

	while(hSocket != SOCKET_ERROR)							//���Ҫ�˳��̣߳�ֱ�ӽ�hCtrlThread�ó�NULL
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

				if(nTimeouts > 5)	//�������5����յ�һ����Ϣ����ô����Timeout(0)
					OnTimeout(0);

				nTimeouts = 0;	//ÿ�����յ�һ�����ݣ�������timeout
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
	if (score > 100)	score = 100;	//��Ч��Χ

	//����
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
	//������{}
	//number:102,score:100,assess:'ͨ��'
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
		
		if (x != t.assess.npos)	//assess:"ͨ��",number:104,...
		{
			t.assess = t.assess.substr(0, x);
		}

		t.assess = ICarDev::TrimString(t.assess);	//ȥ��"",'',�Լ��ո�
	}
	else
	{
		t.assess = "δͨ��";
	}

	//ֱ�����
	t.bChanged = false;
	rcds.push_back(t);

	return true;
}
int ICarDev::Records::ParseRecords(const string &szJson, bool bReset)	//�����ַ������������ؼ�¼��
{
	if (bReset)
	{
		Reset();
	}

	if (szJson.length() <= 4)//[{...}]���ٴ���4
		return 0;

	//JSON���ݽṹ�����飩
	//[{number:102,score:100,assess:'ͨ��'},{},{}]

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
string ICarDev::Records::PackRecords(bool bAll)				//���ݵ�ǰ���ݴ�����ݣ�bAll = false��ֻ������ĵ�
{
	return "";
}
////////////////////////////////////////////
void ICarDev::DetailItem::Reset()
{
	nCode = 0;
	name = "";
	step = "";
		
	//λ��
	x = 0;
	y = 0;
	z = 0;
	
	offsec  = 0;	//�ӿ�ʼ���е��ô���ʱ��

	//����״̬
	for(int i=0;i<10;i++)
		infos[i] = 0;
}
void ICarDev::RECORD::Reset()
{
	ID = "";			//ѵ����¼ID

	timestart = 0;		//��ʼ����ʱ��
	seconds = 0;		//�Ѿ�����ʱ��
	cs_type = "";
	cs_name = "";		//�γ����ͺ�����
	courseID = 0;

	//������Ϣ
	scores = 0;
	faults = 0;	//

	assess = "";
	remark = "";

	details.clear();
}
ICarDev::ICarDev(string sn,string szHostIP,unsigned short port) : 
	UdpSocket(szHostIP,port)	//�豸��ʼ����ָ���豸���
{
	hostIP = "192.168.0.104";
	hostPort = 15005;
			
	data.status = dev_normal;	//
	data.sn = sn;
	data.name = "�ݿ�ģ����";
	data.rem = "�ݿ�ģ����";
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
	tm *T = localtime(&t);	//��ȡʱ��ṹ��

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
int ICarDev::GetWaiting()				//��ѯ���ڵȴ���ָ��
{
	int dt = 10 - (time(NULL) - tWaiting);		//

	if (dt > 0 && dt <= 10)				//
		return dt;

	if (!WaitingCmd.empty())
		WaitingCmd = "";

	return 0;
}
void ICarDev::ClearWaiting(string cmd)	//ȡ��ָ��
{
	if (cmd == WaitingCmd)
	{
		WaitingCmd = "";
		tWaiting = 0;
	}
}
void ICarDev::SetWaiting(string cmd)	//���õȴ�����ָ��
{
	WaitingCmd = cmd;

	tWaiting = time(NULL);
}
//�豸��¼
bool ICarDev::TryConnect(const string &szHostIP,unsigned short pt)
{
	if (GetWaiting() > 0)
		return false;

	//����IP��Port
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

//�û���¼�豸
bool ICarDev::TryLogin(const string &user)	//�豸��¼
{
	if (GetWaiting() > 0)
		return false;
	
	string params = user + ";" + data.sn;

	SetWaiting("login");

	return SendCmd("login",params);
}
void ICarDev::TryLogoff()				//ע��
{
	if (GetWaiting() > 0)
		return;

	SetWaiting("logoff");
	SendCmd("logoff",data.sn);
}
void ICarDev::QueryState()	//��ѯһ��״̬
{
	if (IsConnected())
	{
		if(IsShowQRCode())
		{
			SendCmd("query",data.sn + ";qrcode");	//��ѯ�Ƿ����ƶ˵�¼
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
void ICarDev::QueryRecords()//��ѯһ��ѵ����¼(����һ������)
{
	if (IsLogin())
	{
		SendCmd("query.rcds", data.userID);	//��ѯָ���û���ѵ����¼
	}
}
void ICarDev::UsingQRCode()
{
	if (data.status == dev_ready)
		tQRCodeShow = time(NULL);
}
int ICarDev::IsShowQRCode()	//�Ƿ������ά����ʾ������������ʾʣ������
{
	int dt = 0;
	
	if (tQRCodeShow > 0)
	{
		if (data.status == dev_ready)	//ֻ��Ready״̬����������ʾ
			dt = 30 - (time(NULL) - tQRCodeShow);

		if (dt <= 0)
			tQRCodeShow = 0;	//������ʾ
	}

	return dt < 0 ? 0 : dt;
}
bool ICarDev::IsConnected()			//�Ƿ��Ѿ����ӵ����������豸�������
{
	return data.status >= dev_ready;
}
bool ICarDev::IsRunning()
{
	return data.bRunning;
}
bool ICarDev::IsLogin()				//�Ƿ��Ѿ����û���¼
{
	return data.status == dev_busy;
}

//��ʼ�γ�
bool ICarDev::StartCourse(int courseID, const string& type,const string& name)
{
	if(data.bRunning || !IsLogin())
		return false;	//�Ѿ�������

	data.rcd.Reset();	//����������Ϣ

	time_t t1 = ::time(NULL);	
	data.rcd.ID = data.sn + "_" +  FormatTime(t1,true);	//����һ����¼ID �豸SN+Date
	data.rcd.courseID = courseID;
	data.rcd.cs_type = type;
	data.rcd.cs_name = name;			//��¼��ǰ�γ���Ϣ
	data.rcd.timestart = t1;	//�γ̿�ʼʱ��

	data.rcd.details.clear();			//ȫ�����	

	data.bRunning = true;

	char tmp[256] = { 0 };
	sprintf_s(tmp,"%s;%s;%d;%s",
		data.rcd.ID.c_str(),
		type.c_str(),
		courseID,
		name.c_str());

	SendCmd("run", tmp);// data.rcd.ID + ";" + type + ";" + name);	//��ʼ�γ�

	return true;
}
//���й���������������ݣ�Ҳ�����ڿγ̽���֮ǰһ����ӣ�
void ICarDev::AddDetail(DetailItem &item)
{
	if(!IsRunning())
		return;

	if(item.offsec <= 0)
		item.offsec = time(NULL) - data.rcd.timestart;

	data.rcd.details.push_back(item);	

		//ѵ����¼�ύ
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

//��������ѵ��
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
	//ѵ����¼�ύ
	char buf[1200] = {0};

	//�����ύ��¼�Ļ�����Ϣ
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
		return;	//û�б仯

	data.status = s;

	OnCmd("statuschanged",StatusString(s),GetMyIP(),GetMyPort());	//״̬����
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
		SetCallbackFunction(NULL);	//���pfn

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
	if(key == "query")	//״̬��ѯ
	{
		SendCmd("status",StatusString(data.status));
	}
	else if(key == "setstatus")
	{
		SetStatus(StatusFromString(params));	//״̬����δReady
	}
	else if(key == "join")	//�û���¼/ע��
	{
		if(params == "")	//���û���û���Ϣ����ô�豸ע����¼
		{
			data.userID = "";
			data.userName = "";
			SetStatus(dev_ready);
		}
		else
		{
			SetStatus(dev_busy);		//���û�ʹ��

			int split = params.find_first_of(';');

			if(split < 0)	//ֻ���û�ID
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
string ICarDev::TrimString(const string& str)	//ɾ��ǰ��ո�
{
	if(str.length() <= 0)
		return str;

	int n0 = str.find_first_not_of(' ');
	int n1 = str.find_last_not_of(' ');

	string sz = "";
	if(n0 >= 0 && n1 >=n0)
		sz = str.substr(n0,n1 - n0 + 1);

	if(sz.length() > 2)	//ȥ��"",''
	{
		if((sz.front() == '\"' && sz.back() == '\"') ||	//˫����
			(sz.front() == '\'' && sz.back() == '\''))	//������
			sz = sz.substr(1,sz.length() - 2);
	}

	return sz;
}
int ICarDev::ParseString(const char *txt,int len,const string &ip,unsigned short port)
{
	const char *pch0 = strchr(txt,'<');
	const char *pch1 = pch0 != NULL ? strstr(pch0,"/>") : NULL;	//���ҳɶԵ� </>
	const char *buf = txt;
	
	while(pch0 != NULL && pch1 != NULL)
	{
		char tmp[1200] = {0};
		strncpy(tmp,pch0 + 1,pch1 - pch0 - 1);
		strlwr(tmp);	//�ؼ���תΪСд

		char *eq = strchr(tmp,'=');

		if(eq != NULL)
			*eq = 0;
		
		strlwr(tmp);	//���Ⱥ�ǰ��Ĺؼ���������ΪСд		

		string key = eq != NULL ? string(tmp,eq - tmp) : tmp;
		string params = eq != NULL ? eq + 1 : "";

	

		OnCmd(TrimString(key),TrimString(params),ip,port);

		//��һ��ָ��
		pch0 = strchr(pch1 + 1,'<');
		pch1 = pch1 = pch0 != NULL ? strstr(pch0,"/>") : NULL;	//���ҳɶԵ� </>		
	}


	return 0;
}
int ICarDev::OnTimeout(int nTimeouts)
{
	if(IsConnected())
	{
		QueryState();	//ÿ��һ��ʱ���ѯһ��״̬

		if(nTimeouts > 0)	//�������5������û�����ݷ�������ô�п��ܱ�ʾ���ض���
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