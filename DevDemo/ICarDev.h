/*
	�豸����ӿڶ���
*/

#ifndef INTERFACE_CAR_DEVICES
#define INTERFACE_CAR_DEVICES

#pragma once

#include <iostream>
#include <string>
#include <vector>



using namespace std;

////////////////////////////////////////
//���紦��ӿ�
class UdpSocket
{
	SOCKET hSocket;

	string ips;
	unsigned short port;
public:
	UdpSocket(string ip,unsigned short port);
	~UdpSocket();	
	void Close();

	//���յ�����
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
//�豸����
typedef int (CALLBACK *LPFCOMMAND)(const string&,const string&,const string&,unsigned short);				//���մ���ص�������ʽ ReceiveProc(int iTask,void *pDatas)
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
	//�豸״̬����
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

	//״̬ת��
	static int StatusFromString(const string sz);
	static const string StatusString(int s);

	struct DetailItem
	{
		int nCode;
		string name;
		string step;
		
		//λ��
		double x;
		double y;
		double z;
		int offsec;	//�ӿ�ʼ���е��ô���ʱ��

		//����״̬
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
				//if (s == "δ���") return 0;
				//if (s == "���") return 1;
				//if (s == "����") return 2;
				if (s == "D") return 0;
				if (s == "C") return 1;
				if (s == "B") return 2;
				if (s == "A") return 3;

				return -1;
			}
		};

		int GetCount();
		RcdItem *Get(int index);

		int ParseRecords(const string &szJsonRecords,bool bReset);	//�����ַ������������ؼ�¼��
		string PackRecords(bool bAll = false);				//���ݵ�ǰ���ݴ�����ݣ�bAll = false��ֻ������ĵ�
	protected:
		bool ParseAndInsertItem(string &item);
		vector<RcdItem> rcds;
		bool bChanged;
	};
	struct RECORD
	{
		string ID;			//ѵ����¼ID

		time_t timestart;	//��ʼ����ʱ��
		int seconds;		//�Ѿ�����ʱ��
		int courseID;		//�γ̱��
		string cs_type;
		string cs_name;		//�γ����ͺ�����

		//������Ϣ
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
		//�豸��Ϣ
		int status;		//�豸��ǰ״̬
		
		string sn;		//�豸���к�
		string name;	//�豸����
		string rem;		//�豸����
		
		string userID;
		string userName;	//��ǰ�û���Ϣ

		Records UserRcds;	//��ǰ�û������пγ�ѵ���ɼ�

		bool bRunning;		//
		RECORD rcd;	
	};

protected:
	string hostIP;
	unsigned short hostPort;
	LPFCOMMAND pfnCallback;
	CmdCallback *pCmdOperator;

	string WaitingCmd;	//�ȴ�ָ��
	time_t tWaiting;	//��ʼ�ȴ�ʱ��

	time_t tQRCodeShow;	//
public:
	DEVICE data;

	ICarDev(string sn,string szIP,unsigned short port);	//�豸��ʼ����ָ���豸���
	~ICarDev();

	//�����ĸ�Tryָ����Ҫ�ȴ�
	//�豸��¼
	bool TryConnect(const string &szHostIP,unsigned short port);			//�豸���ӷ��������������
	void TryDisconnect();
	//�û���¼�豸
	bool TryLogin(const string &user);	//�豸��¼
	void TryLogoff();					//ע��

	//��������״̬����
	void QueryState();	//��ѯһ��״̬

	void QueryRecords();//��ѯһ��ѵ����¼(����һ������)

	/*�豸ʹ��QR Code��¼
		���ڵ�ǰϵͳ��֧�������ݿ�����������Ϣ�������Ҫ�豸��ʱ��ѯ״̬��
	ƽʱϵͳ��ѯ���ڽϳ�(1���ӣ����������ϵͳʹ�ö�ά���¼����ôϵͳ����ӿ��ѯƵ��(5��)
	ֱ����¼�ɹ�����30���ڣ�
	*/
	void UsingQRCode();	
	int IsShowQRCode();	//�Ƿ������ά����ʾ������������ʾʣ������

	bool IsConnected();			//�Ƿ��Ѿ����ӵ����������豸�������
	bool IsLogin();				//�Ƿ��Ѿ����û���¼
	bool IsRunning();			//�пγ�����

	//��ʼ�γ�
	bool StartCourse(int courseID,const string &type,const string &name);
	//���й���������������ݣ�Ҳ�����ڿγ̽���֮ǰһ����ӣ�
	void AddDetail(DetailItem &item);	//���һ��������������,ֻ����IsRunning()״̬�²������(EndCourse()����֮ǰ)	

	//��������ѵ��
	void EndCourse(int score,const string &assess,const string &remark = "null");

	int SendCmd(const string &key,const string &params);
	int SendCmds(const char *cmds);	//SendString()

	//���ߺ���
	string FromWchar(LPCWSTR sz);
	string FormatTime(time_t t,bool bID = false);

	string GetWaitingCmd();
	int GetWaiting();					//��ѯ���ڵȴ���ָ���ʣ��ʱ��(��),Ĭ�ϵȴ�10��
	void SetWaiting(string cmd);	//���õȴ�����ָ��
	void ClearWaiting(string cmd);	//�Ƚϲ�ȡ��ȡ��ָ��
protected:
	void SetStatus(int s);
	virtual int OnReceiveString(char *buf,int len,const string &ip,unsigned short port);


	/*OnReceiveString()�����OnCmd()����
		ICarDev::OnCmd()
		{
			//Base Process

			//�ص����������Ϊ�գ�
			CmdCallbac();

			//�ص������������Ϊ�գ�
			CommandFnc();
		}
	*/
	//���ݽ��ջص�		
	CmdCallback *SetCallbackOperator(CmdCallback *cb);	//
	LPFCOMMAND   SetCallbackFunction(LPFCOMMAND pfn);	//������������,��������һ�������һ����ΪNULL
	virtual bool OnCmd(const string &key,const string &params,const string &ip,unsigned short port);		

	//Ĭ�϶�ʱ��
	virtual int OnTimeout(int nTimeouts);	//
private:
	//���ݽ���
	int ParseString(const char *txt,int len,const string &ip,unsigned short port);
	static string TrimString(const string &str);	//ɾ��ǰ��ո�
	bool DoCmd(const string &key,const string &params,const string &ip,unsigned short port);
};

#endif