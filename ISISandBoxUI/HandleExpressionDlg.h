#pragma once
#include "afxwin.h"


// CHandleExpressionDlg 对话框

class CHandleExpressionDlg : public CDialog
{
	DECLARE_DYNAMIC(CHandleExpressionDlg)

public:
	CHandleExpressionDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CHandleExpressionDlg();
	
// 对话框数据
	enum { IDD = IDD_HANDLE_EXPRESSION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_editExpression;
	BOOLEAN m_bIsEmpty;
	afx_msg void OnBnClickedOk();
	CString m_strExpression;
};
