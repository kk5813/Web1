
// DevDemoDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "DevDemo.h"
#include "DevDemoDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDevDemoDlg �Ի���




CDevDemoDlg::CDevDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDevDemoDlg::IDD, pParent),
	ICarDev("CAR_DT_2022N001","0.0.0.0",15001)
	, m_sUserID(_T(""))
	, m_bFilerQueryBack(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	qrBmp = NULL;
}

void CDevDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_sUserID);
	DDX_Control(pDX, IDC_LIST1, m_cList);
	DDX_Check(pDX, IDC_CHECK1, m_bFilerQueryBack);
	DDX_Control(pDX, IDC_IPADDRESS1, m_cIP);	
}

BEGIN_MESSAGE_MAP(CDevDemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDCANCEL, &CDevDemoDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &CDevDemoDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CDevDemoDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CDevDemoDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CDevDemoDlg::OnBnClickedButton3)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON4, &CDevDemoDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CDevDemoDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CDevDemoDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_CHECK1, &CDevDemoDlg::OnBnClickedCheck1)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS1, &CDevDemoDlg::OnIpnFieldchangedIpaddress1)
	ON_CBN_SELCHANGE(IDC_COMBO6, &CDevDemoDlg::OnCbnSelchangeCombo6)
	ON_BN_CLICKED(IDC_BUTTON7, &CDevDemoDlg::OnBnClickedButton7)
END_MESSAGE_MAP()


// CDevDemoDlg ��Ϣ�������

BOOL CDevDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	m_cIP.SetAddress(127, 0, 0, 1);

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	SetTimer(0,200,NULL);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CDevDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CDevDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else 
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������
		if(qrBmp != NULL && IsShowQRCode())
		{			
			//CDialogEx::OnPaint();
			CRect rc1;

			GetDlgItem(IDC_STATIC_RECT)->GetWindowRect(rc1);
			ScreenToClient(rc1);

			Graphics gr(dc);
			gr.DrawImage(qrBmp,Rect(rc1.left,rc1.top,rc1.Width(),rc1.Height()),(INT)0,(INT)0L,(INT)qrBmp->GetWidth(),(INT)qrBmp->GetHeight(),UnitPixel,NULL,NULL,NULL);
	
			//if(memDC.GetSafeHdc() != NULL)
			//dc.BitBlt(0,0,rc1.Width(),rc1.Height(),&memDC,0,0,SRCCOPY);
		}
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CDevDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CDevDemoDlg::OnBnClickedCancel()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CDialogEx::OnCancel();
}


void CDevDemoDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//CDialogEx::OnOK();
}



void CDevDemoDlg::OnBnClickedButton2()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//ѵ����¼�ύ
	//char buf[1024] = {0};
	//string a("123");
	//string b("432");
	//string c("");

	////�����ύ��¼�Ļ�����Ϣ
	//sprintf("%s,%s<commit.sn = />\
	//	<commit.user.id = />",
	//	a.c_str(),
	//	b.c_str());


	UpdateData();

	if(!IsRunning())
	{	
		int nID = 0;
		CString type;
		CString name;
		GetDlgItemText(IDC_COMBO1,type);
		GetDlgItemText(IDC_COMBO2,name);

		nID = _wtoi(name);
		name = name.Right(name.GetLength() - 4);

		if(type.IsEmpty() || name.IsEmpty())
		{
			AfxMessageBox(L"��ָ���γ̺Ϳγ����ͣ�");
			return;
		}

		SetDlgItemText(IDC_EDIT5,CString(data.userID.c_str()));
		SetDlgItemText(IDC_EDIT6,CString(data.userName.c_str()));
		SetDlgItemText(IDC_EDIT7,CTime::GetCurrentTime().Format(L"%Y-%m-%d %H:%M:%S"));
		SetDlgItemText(IDC_EDIT8,L"00:00:00");



		if(StartCourse(101,FromWchar(type),FromWchar(name)))
			SetDlgItemText(IDC_BUTTON2,L"������ϰ");
	}
	else
	{
		CString scores;
		CString assess;
		CString remark;

		GetDlgItemText(IDC_COMBO3,scores);
		
		GetDlgItemText(IDC_COMBO5,assess);
		GetDlgItemText(IDC_COMBO5,remark);

		if(scores.IsEmpty() || assess.IsEmpty())
		{
			AfxMessageBox(L"��ָ���÷ֺ�������");
			return;
		}

		EndCourse(_wtoi(scores),FromWchar(assess),FromWchar(remark));
		SetDlgItemText(IDC_BUTTON2,L"��ʼ��ϰ");
	}
}


void CDevDemoDlg::OnBnClickedButton1()
{
	if (!IsConnected())
	{
		AfxMessageBox(L"�豸��δ�Ǽǣ����ܵ�¼ʹ��");
		return;
	}

	if (GetWaiting() > 0)
	{
		CString str;
		str.Format(L"��һָ��[%s]��δ��ɣ����Ե�(%d��) ...", (LPCWSTR)CString(GetWaitingCmd().c_str()), GetWaiting());

		SetDlgItemText(IDC_STATIC_STATUS, str);
		return;
	}

	UpdateData();

	if (IsLogin())
	{
		TryLogoff();
	}
	else //if (IsConnected())
	{
		CStringA tmp(m_sUserID);

		TryLogin(tmp.GetBuffer(0));
		
		//GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	}

	//GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
}


void CDevDemoDlg::OnBnClickedButton3()
{
	if (GetWaiting() > 0)
	{
		CString str;
		str.Format(L"��һָ��[%s]��δ��ɣ����Ե�(%d��) ...",(LPCWSTR)CString(GetWaitingCmd().c_str()), GetWaiting());

		SetDlgItemText(IDC_STATIC_STATUS, str);
		return;
	}

	if(IsConnected())
	{
		TryDisconnect();

		//GetDlgItem(IDC_COMBO6)->EnableWindow(FALSE);
		//m_cIP.EnableWindow(FALSE);
	}
	else
	{
		BYTE b[4] = { 0 };
		m_cIP.GetAddress(b[0], b[1], b[2], b[3]);

		CStringA ip;
		ip.Format("%d.%d.%d.%d",b[0],b[1],b[2],b[3]);

		TryConnect(ip.GetString(),15005);		
		
		m_cIP.EnableWindow(FALSE);
	}

	//GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
}
	
int CDevDemoDlg::OnReceiveString(char *buf,int len,const string &ip,unsigned short port)
{
	__super::OnReceiveString(buf,len,ip,port);

	if (m_bFilerQueryBack)
	{
		if (strstr(buf, "<answer") != NULL &&
			strstr(buf, "ok;query") != NULL)
			return 0;
	}


	CString str;;

	str.Format(L"%s>> %s",
		(LPCWSTR)CTime::GetCurrentTime().Format(L"%H:%M:%S"),
		//(LPCWSTR)CString(ip.c_str()),port,
		(LPCWSTR)CString(buf));

	m_cList.InsertString(0,str);


	return 0;
}
//����Э��
bool CDevDemoDlg::OnCmd(const string& key, const string& params, const string &ip, unsigned short port)
{
	__super::OnCmd(key, params, ip, port);

	if (key == "statuschanged")
	{
		SetDlgItemText(IDC_EDIT2, CString(params.c_str()));

		GetDlgItem(IDC_BUTTON2)->EnableWindow(data.status == dev_busy);

		CString txt = L"��¼";
		if (data.status == dev_busy)
			txt = L"ע��";

		GetDlgItem(IDC_BUTTON1)->EnableWindow(data.status >= dev_ready);
		SetDlgItemText(IDC_BUTTON1, txt);

		SetDlgItemText(IDC_BUTTON3, data.status == dev_normal ? L"����" : L"�Ͽ�");

		ClearWaiting("register");
		ClearWaiting("unregister");

	}
	else if (key == "timeout")
	{
		SetDlgItemText(IDC_STATIC_STATUS, CString(params.c_str()));
	}
	else if (key == "join")
	{
		SetDlgItemText(IDC_EDIT1, CString(data.userID.c_str()));
		SetDlgItemText(IDC_EDIT3, CString(data.userName.c_str()));

		ClearWaiting("login");
		ClearWaiting("logoff");
	}
	else if (key == "user.name")
	{
		SetDlgItemText(IDC_EDIT3, CString(params.c_str()));
	}
	else if (key == "user.rcds")
	{
		m_cList.InsertString(0,L"Scores of Courses:");
		for (int i = 0; i < data.UserRcds.GetCount(); i++)
		{
			//m_cList.InsertString(0, str);
			Records::RcdItem *x = data.UserRcds.Get(i);

			if (x == NULL)
				break;

			CString t;
			t.Format(L"    >>%d - Courese:%d; Score: %d; Assess: %S",i,x->number,x->score,x->assess.c_str());
			m_cList.InsertString(0, t);
		}
	}

	return true;
}

void CDevDemoDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (IsRunning())
	{
		int seconds = time(NULL) - data.rcd.timestart;

		int h = seconds / 3600;
		int m = (seconds % 3600) / 60;
		int s = seconds % 60;

		data.rcd.seconds = seconds;

		CString str;
		str.Format(L"%02d:%02d:%02d", h, m, s);
		SetDlgItemText(IDC_EDIT8, str);
	}
	else
	{

		CString btn6;
		GetDlgItemText(IDC_BUTTON6, btn6);

		int sec = IsShowQRCode();
		CString str(L"��ά��");
		if (sec > 0)
			str.Format(L"��ά�� %d", sec);

		if (btn6 != str)
		{
			SetDlgItemText(IDC_BUTTON6, str);

			if (str == "��ά��" || btn6 == "��ά��")
			{
				CRect rc1;
				GetDlgItem(IDC_STATIC_RECT)->GetWindowRect(rc1);
				ScreenToClient(rc1);
				InvalidateRect(rc1, TRUE);
			}
		}

		sec = GetWaiting();				//ʣ��ʱ��
		if (sec > 0)
		{			
			waiting_cmd = GetWaitingCmd();

			if (waiting_cmd == "login" || waiting_cmd == "logoff")
			{

			}
			else if(waiting_cmd ==  "register" || waiting_cmd == "unrigister")
			{
				str.Format(L"%d",sec);				
				
				SetDlgItemText(IDC_BUTTON3, str);
			}
		}
		else if(waiting_cmd != "")
		{
			if (waiting_cmd == "login" || waiting_cmd == "logoff")
			{

			}
			else if (waiting_cmd == "register" || waiting_cmd == "unrigister")
			{				
				SetDlgItemText(IDC_BUTTON3, IsConnected() ? L"�Ͽ�" : L"����");
				m_cIP.EnableWindow(TRUE);
			}
			waiting_cmd = "";
		}
	}

	__super::OnTimer(nIDEvent);
}


void CDevDemoDlg::OnBnClickedButton4()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if(!IsRunning())
		return;

	CString fault;
	GetDlgItemText(IDC_COMBO4,fault);

	string x = ICarDev::FromWchar(fault);
	int n = x.find('@');

	ICarDev::DetailItem item;
	item.nCode = _wtoi(fault);
	item.name = x.substr(4,n-4);
	item.step  = x.substr(n+1,x.length() - n - 1);

	item.x = 1001.123;
	item.y = 100.123;
	item.z = 1.123;

	for(int i=0;i<10;i++)
		item.infos[i] = rand()%100;

	this->AddDetail(item);
}


void CDevDemoDlg::OnBnClickedButton5()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_cList.ResetContent();
}


void CDevDemoDlg::OnBnClickedButton6()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if(IsLogin())
	{		
		SetDlgItemText(IDC_STATIC_STATUS,L"�Ѿ���¼");
		return;
	}
	else if(!IsConnected())
	{
		SetDlgItemText(IDC_STATIC_STATUS,L"���ȵ�¼");
		return;
	}

	if (qrBmp == NULL)
	{
		wchar_t buf[MAX_PATH] = { 0 };
		::GetModuleFileName(NULL, buf, MAX_PATH);

		wchar_t *pch = wcsrchr(buf, '\\');

		if (pch != NULL)
		{
			CString erCode(this->data.sn.c_str());
			erCode += L".png";

			wcscpy(pch + 1, erCode);

			CFileFind ff;
			if (!ff.FindFile(buf))
			{
				SetDlgItemText(IDC_STATIC_STATUS, CString(L"û�ж�ά����Ϣ: ") + buf);
				return;
			}

			qrBmp = Bitmap::FromFile((LPCWSTR)buf, FALSE);

			InvalidateRect(NULL);
		}
	}

	UsingQRCode();

}


void CDevDemoDlg::OnBnClickedCheck1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
}


void CDevDemoDlg::OnIpnFieldchangedIpaddress1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
}


void CDevDemoDlg::OnCbnSelchangeCombo6()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}


void CDevDemoDlg::OnBnClickedButton7()
{
	if(IsLogin())
		QueryRecords();
	else
	{
		AfxMessageBox(L"���ȵ�¼�ٲ�ѯ!");
	}
}
