// wyProcMonDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include <vector>

typedef struct _DRIVER_COMMUNICATE
{
	HANDLE port;
	HANDLE completion;
} DriverCommunicate, *PDriverCommunicate;

// CwyProcMonDlg 对话框
class CwyProcMonDlg : public CDialog
{
// 构造
public:
	CwyProcMonDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_WYPROCMON_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
