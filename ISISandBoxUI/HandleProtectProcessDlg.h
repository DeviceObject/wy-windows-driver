#pragma once
#include "afxwin.h"


// CHandleProtectProcessDlg �Ի���

class CHandleProtectProcessDlg : public CDialog
{
	DECLARE_DYNAMIC(CHandleProtectProcessDlg)

public:
	CHandleProtectProcessDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CHandleProtectProcessDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_editProcessId;
	DWORD m_dwProcessId;
	afx_msg void OnBnClickedOk();
};
