// HandleExpressionDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ISISandBoxUI.h"
#include "HandleExpressionDlg.h"


// CHandleExpressionDlg �Ի���

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


// CHandleExpressionDlg ��Ϣ�������

void CHandleExpressionDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	if (!m_strExpression.IsEmpty())
		m_bIsEmpty = FALSE;
	
	OnOK();
}
