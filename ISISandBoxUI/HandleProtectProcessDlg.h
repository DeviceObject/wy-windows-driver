#pragma once
#include "afxwin.h"


// CHandleProtectProcessDlg 对话框

class CHandleProtectProcessDlg : public CDialog
{
	DECLARE_DYNAMIC(CHandleProtectProcessDlg)

public:
	CHandleProtectProcessDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CHandleProtectProcessDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_editProcessId;
	DWORD m_dwProcessId;
	afx_msg void OnBnClickedOk();
};
