
// DevDemoDlg.h : ͷ�ļ�
//

#pragma once

#include "ICarDev.h"
#include "afxwin.h"


// CDevDemoDlg �Ի���
class CDevDemoDlg : public CDialogEx, public ICarDev
{
// ����
public:
	CDevDemoDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_DEVDEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��
	
	//����Э��
	virtual bool OnCmd(const string &key,const string &params,const string &ip,unsigned short port);
	virtual int OnReceiveString(char *buf,int len,const string &ip,unsigned short port);

	Bitmap *qrBmp;

	string waiting_cmd;
// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	CString m_sUserID;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButton4();
	CListBox m_cList;
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
	BOOL m_bFilerQueryBack;
	afx_msg void OnBnClickedCheck1();
	CIPAddressCtrl m_cIP;
	afx_msg void OnIpnFieldchangedIpaddress1(NMHDR *pNMHDR, LRESULT *pResult);	
	afx_msg void OnCbnSelchangeCombo6();
	afx_msg void OnBnClickedButton7();
};
