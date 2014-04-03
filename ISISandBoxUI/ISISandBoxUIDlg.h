// ISISandBoxUIDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "ISISandBox.h"


// CISISandBoxUIDlg 对话框
class CISISandBoxUIDlg : public CDialog
{
// 构造
public:
	CISISandBoxUIDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_ISISANDBOXUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListBox m_listRedirectRoot;
	CListBox m_listExpression;
	CListBox m_listProtectProcess;

public:
	

public:
	afx_msg void OnBnClickedBtnAddRedirectRoot();
	afx_msg void OnBnClickedBtnRemoveRedirectRoot();
	afx_msg void OnBnClickedBtnActiveRedirectRoot();
	afx_msg void OnBnClickedBtnAddExpression();
	afx_msg void OnBnClickedBtnRemoveExpression();
	afx_msg void OnBnClickedBtnAddProtectProcess();
	afx_msg void OnBnClickedBtnRemoveProtectProcess();
	afx_msg void OnBnClickedBtnStartDriverWorking();
	afx_msg void OnBnClickedBtnStopDriverWorking();
	afx_msg void OnBnClickedBtnLoadDriver();
	afx_msg void OnBnClickedBtnConnectDriver();
	afx_msg void OnBnClickedBtnDisconnectDriver();
};
