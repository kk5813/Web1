/*
	设备管理接口定义
*/

#ifndef INTERFACE_CAR_DEVICES
#define INTERFACE_CAR_DEVICES

#pragma once

#include <iostream>
#include <string>
#include <vector>



using namespace std;

////////////////////////////////////////
//网络处理接口
class UdpSocket
{
	SOCKET hSocket;

	string ips;
	unsigned short port;
public:
	UdpSocket(string ip,unsigned short port);
	~UdpSocket();	
	void Close();

	//接收到数据
	virtual int OnReceiveString(char *buf,int len,const string &ip,unsigned short port);
	virtual int OnTimeout(int nTimeouts);	//

	int SendString(const char *buf,int len,string ip,unsigned short port);

	string GetMyIP();
	unsigned short GetMyPort();
private:
	static DWORD WINAPI Run(LPVOID pData);
	void OnRun();
};

/////////////////////////////////////////////////
//设备管理
typedef int (CALLBACK *LPFCOMMAND)(const string&,const string&,const string&,unsigned short);				//接收处理回调函数形式 ReceiveProc(int iTask,void *pDatas)
class CmdCallback
{
protected:
	CmdCallback(){};
public:
	virtual bool OnCmd(const string &key,const string &params,const string &ip,unsigned short port)
	{
	}
};
class ICarDev : public UdpSocket
{
public:
	//设备状态定义
	enum Status
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

	//状态转换
	static int StatusFromString(const string sz);
	static const string StatusString(int s);

	struct DetailItem
	{
		int nCode;
		string name;
		string step;
		
		//位置
		double x;
		double y;
		double z;
		int offsec;	//从开始运行到该错误时间

		//操作状态
		union{
			struct{
				int engine;			//0
				int steer;			//
				int brake;
				int gas;
				int clutch;
				int gear;
				int headlight;		//
				int turnlight;		//
				int safebelt;		//
			};
			int infos[10];
		};
		void Reset();
		DetailItem()
		{
			Reset();
		}
	};
	class Records
	{
	public:
		Records();
		void Reset();
		bool IsChanged();

		bool Set(int nCourseNumber, int score, const char *assess);	//

		struct RcdItem
		{
			bool bChanged;
			int number;
			int score;
			string assess;

			int AssessValue()
			{
				return AssessValue(assess);
			}
			int AssessValue(string &s)
			{
				//if (s == "未完成") return 0;
				//if (s == "完成") return 1;
				//if (s == "优良") return 2;
				if (s == "D") return 0;
				if (s == "C") return 1;
				if (s == "B") return 2;
				if (s == "A") return 3;

				return -1;
			}
		};

		int GetCount();
		RcdItem *Get(int index);

		int ParseRecords(const string &szJsonRecords,bool bReset);	//根据字符串解析（下载记录）
		string PackRecords(bool bAll = false);				//根据当前内容打包数据，bAll = false，只打包更改的
	protected:
		bool ParseAndInsertItem(string &item);
		vector<RcdItem> rcds;
		bool bChanged;
	};
	struct RECORD
	{
		string ID;			//训练记录ID

		time_t timestart;	//开始运行时间
		int seconds;		//已经运行时间
		int courseID;		//课程编号
		string cs_type;
		string cs_name;		//课程类型和名称

		//评价信息
		int scores;
		int faults;	//

		string assess;
		string remark;

		vector<DetailItem> details;

		void Reset();
		RECORD()
		{
			Reset();
		}
	};
	struct DEVICE
	{
		//设备信息
		int status;		//设备当前状态
		
		string sn;		//设备序列号
		string name;	//设备名称
		string rem;		//设备描述
		
		string userID;
		string userName;	//当前用户信息

		Records UserRcds;	//当前用户的所有课程训练成绩

		bool bRunning;		//
		RECORD rcd;	
	};

protected:
	string hostIP;
	unsigned short hostPort;
	LPFCOMMAND pfnCallback;
	CmdCallback *pCmdOperator;

	string WaitingCmd;	//等待指令
	time_t tWaiting;	//开始等待时间

	time_t tQRCodeShow;	//
public:
	DEVICE data;

	ICarDev(string sn,string szIP,unsigned short port);	//设备初始化，指定设备编号
	~ICarDev();

	//以下四个Try指令需要等待
	//设备登录
	bool TryConnect(const string &szHostIP,unsigned short port);			//设备连接服务器（管理程序）
	void TryDisconnect();
	//用户登录设备
	bool TryLogin(const string &user);	//设备登录
	void TryLogoff();					//注销

	//网络在线状态管理
	void QueryState();	//查询一次状态

	void QueryRecords();//查询一次训练记录(发送一次请求)

	/*设备使用QR Code登录
		由于当前系统不支持云数据库主动反馈信息，因此需要设备定时查询状态；
	平时系统查询周期较长(1分钟），但是如果系统使用二维码登录，那么系统将会加快查询频率(5秒)
	直到登录成功（在30秒内）
	*/
	void UsingQRCode();	
	int IsShowQRCode();	//是否允许二维码显示，返回允许显示剩余秒数

	bool IsConnected();			//是否已经连接到服务器（设备管理程序）
	bool IsLogin();				//是否已经有用户登录
	bool IsRunning();			//有课程设置

	//开始课程
	bool StartCourse(int courseID,const string &type,const string &name);
	//运行过程中添加评测数据（也可以在课程结束之前一起添加）
	void AddDetail(DetailItem &item);	//添加一次评测数据数据,只有在IsRunning()状态下才能添加(EndCourse()函数之前)	

	//结束本次训练
	void EndCourse(int score,const string &assess,const string &remark = "null");

	int SendCmd(const string &key,const string &params);
	int SendCmds(const char *cmds);	//SendString()

	//工具函数
	string FromWchar(LPCWSTR sz);
	string FormatTime(time_t t,bool bID = false);

	string GetWaitingCmd();
	int GetWaiting();					//查询正在等待的指令和剩余时间(秒),默认等待10秒
	void SetWaiting(string cmd);	//设置等待反馈指令
	void ClearWaiting(string cmd);	//比较并取消取消指令
protected:
	void SetStatus(int s);
	virtual int OnReceiveString(char *buf,int len,const string &ip,unsigned short port);


	/*OnReceiveString()会调用OnCmd()函数
		ICarDev::OnCmd()
		{
			//Base Process

			//回调对象（如果不为空）
			CmdCallbac();

			//回调函数（如果不为空）
			CommandFnc();
		}
	*/
	//数据接收回调		
	CmdCallback *SetCallbackOperator(CmdCallback *cb);	//
	LPFCOMMAND   SetCallbackFunction(LPFCOMMAND pfn);	//两个函数互斥,调用其中一个会把另一个置为NULL
	virtual bool OnCmd(const string &key,const string &params,const string &ip,unsigned short port);		

	//默认定时器
	virtual int OnTimeout(int nTimeouts);	//
private:
	//数据解析
	int ParseString(const char *txt,int len,const string &ip,unsigned short port);
	static string TrimString(const string &str);	//删除前后空格
	bool DoCmd(const string &key,const string &params,const string &ip,unsigned short port);
};

#endif