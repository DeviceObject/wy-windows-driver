// wyProcMonDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include <vector>

typedef struct _DRIVER_COMMUNICATE
{
	HANDLE port;
	HANDLE completion;
} DriverCommunicate, *PDriverCommunicate;

// CwyProcMonDlg �Ի���
class CwyProcMonDlg : public CDialog
{
// ����
public:
	CwyProcMonDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_WYPROCMON_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


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
	CListCtrl m_cListCtrl;

public:
	static DWORD WINAPI MonThread(PVOID lpParam);

	static DWORD WINAPI DriverCommunicateThread(PVOID lpParam);

public:
	DWORD threadCount;

	DriverCommunicate iDriverCom;

	HANDLE iThreads[64];

	PMESSAGE_NOTIFICATION notification;
};
