// HandleProtectProcessDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ISISandBoxUI.h"
#include "HandleProtectProcessDlg.h"


// CHandleProtectProcessDlg �Ի���

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


// CHandleProtectProcessDlg ��Ϣ�������
BOOL CHandleProtectProcessDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	UpdateData(FALSE);

	return TRUE;
}
void CHandleProtectProcessDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	OnOK();
}
