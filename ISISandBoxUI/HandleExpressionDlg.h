#pragma once
#include "afxwin.h"


// CHandleExpressionDlg �Ի���

class CHandleExpressionDlg : public CDialog
{
	DECLARE_DYNAMIC(CHandleExpressionDlg)

public:
	CHandleExpressionDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CHandleExpressionDlg();
	
// �Ի�������
	enum { IDD = IDD_HANDLE_EXPRESSION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_editExpression;
	BOOLEAN m_bIsEmpty;
	afx_msg void OnBnClickedOk();
	CString m_strExpression;
};
