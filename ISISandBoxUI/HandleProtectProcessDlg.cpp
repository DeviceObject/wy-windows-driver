// HandleProtectProcessDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ISISandBoxUI.h"
#include "HandleProtectProcessDlg.h"


// CHandleProtectProcessDlg 对话框

IMPLEMENT_DYNAMIC(CHandleProtectProcessDlg, CDialog)

CHandleProtectProcessDlg::CHandleProtectProcessDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHandleProtectProcessDlg::IDD, pParent)
	, m_dwProcessId(0)
{

}

CHandleProtectProcessDlg::~CHandleProtectProcessDlg()
{
}

void CHandleProtectProcessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_editProcessId);
	DDX_Text(pDX, IDC_EDIT1, m_dwProcessId);
}


BEGIN_MESSAGE_MAP(CHandleProtectProcessDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CHandleProtectProcessDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CHandleProtectProcessDlg 消息处理程序
BOOL CHandleProtectProcessDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	UpdateData(FALSE);

	return TRUE;
}
void CHandleProtectProcessDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	OnOK();
}
