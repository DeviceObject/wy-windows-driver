// HandleExpressionDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ISISandBoxUI.h"
#include "HandleExpressionDlg.h"


// CHandleExpressionDlg 对话框

IMPLEMENT_DYNAMIC(CHandleExpressionDlg, CDialog)

CHandleExpressionDlg::CHandleExpressionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHandleExpressionDlg::IDD, pParent)
	, m_bIsEmpty(TRUE)
	, m_strExpression(_T(""))
{

}

CHandleExpressionDlg::~CHandleExpressionDlg()
{
}

BOOL CHandleExpressionDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_bIsEmpty = FALSE;
	m_strExpression.Empty();
	UpdateData(FALSE);

	return TRUE;
}

void CHandleExpressionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_EXPRESSION, m_editExpression);
	DDX_Text(pDX, IDC_EDIT_EXPRESSION, m_strExpression);
}


BEGIN_MESSAGE_MAP(CHandleExpressionDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CHandleExpressionDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CHandleExpressionDlg 消息处理程序

void CHandleExpressionDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (!m_strExpression.IsEmpty())
		m_bIsEmpty = FALSE;
	
	OnOK();
}
