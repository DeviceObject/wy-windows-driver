// ISISandBoxUIDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ISISandBoxUI.h"
#include "ISISandBoxUIDlg.h"
#include "HandleExpressionDlg.h"
#include "HandleProtectProcessDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CISISandBoxUIDlg �Ի���
#pragma comment(lib, "ISISandBox.lib")


#define PROGRAM_NAME	L"ISIɳ��"
#define STATE_WORKING	L"������"
#define STATE_PENDING	L"δ����"

CISISandBoxUIDlg::CISISandBoxUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CISISandBoxUIDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CISISandBoxUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_REDIRECT_ROOT, m_listRedirectRoot);
	DDX_Control(pDX, IDC_LIST_EXPRESSION, m_listExpression);
	DDX_Control(pDX, IDC_LIST_PROTECT_PROCESS, m_listProtectProcess);
}

BEGIN_MESSAGE_MAP(CISISandBoxUIDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_ADD_REDIRECT_ROOT, &CISISandBoxUIDlg::OnBnClickedBtnAddRedirectRoot)
	ON_BN_CLICKED(IDC_BTN_REMOVE_REDIRECT_ROOT, &CISISandBoxUIDlg::OnBnClickedBtnRemoveRedirectRoot)
	ON_BN_CLICKED(IDC_BTN_ACTIVE_REDIRECT_ROOT, &CISISandBoxUIDlg::OnBnClickedBtnActiveRedirectRoot)
	ON_BN_CLICKED(IDC_BTN_ADD_EXPRESSION, &CISISandBoxUIDlg::OnBnClickedBtnAddExpression)
	ON_BN_CLICKED(IDC_BTN_REMOVE_EXPRESSION, &CISISandBoxUIDlg::OnBnClickedBtnRemoveExpression)
	ON_BN_CLICKED(IDC_BTN_ADD_PROTECT_PROCESS, &CISISandBoxUIDlg::OnBnClickedBtnAddProtectProcess)
	ON_BN_CLICKED(IDC_BTN_REMOVE_PROTECT_PROCESS, &CISISandBoxUIDlg::OnBnClickedBtnRemoveProtectProcess)
	ON_BN_CLICKED(IDC_BTN_START_DRIVER_WORKING, &CISISandBoxUIDlg::OnBnClickedBtnStartDriverWorking)
	ON_BN_CLICKED(IDC_BTN_STOP_DRIVER_WORKING, &CISISandBoxUIDlg::OnBnClickedBtnStopDriverWorking)
	ON_BN_CLICKED(IDC_BTN_LOAD_DRIVER, &CISISandBoxUIDlg::OnBnClickedBtnLoadDriver)
	ON_BN_CLICKED(IDC_BTN_CONNECT_DRIVER, &CISISandBoxUIDlg::OnBnClickedBtnConnectDriver)
	ON_BN_CLICKED(IDC_BTN_DISCONNECT_DRIVER, &CISISandBoxUIDlg::OnBnClickedBtnDisconnectDriver)
END_MESSAGE_MAP()


// CISISandBoxUIDlg ��Ϣ�������

BOOL CISISandBoxUIDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	WCHAR titleText[512] = {0};
	wsprintf(titleText, L"%s - %s", PROGRAM_NAME, STATE_PENDING);
	SetWindowText(titleText);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CISISandBoxUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
//
HCURSOR CISISandBoxUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#define   BIF_NEWDIALOGSTYLE    0x0040   
#define   BIF_USENEWUI (BIF_NEWDIALOGSTYLE | BIF_EDITBOX)     
void CISISandBoxUIDlg::OnBnClickedBtnAddRedirectRoot()
{
	// ��ȡ�ļ���·����Ȼ����β��
	WCHAR directoryName[MAX_PATH] = {0};

	BROWSEINFO browserInfo;
	ZeroMemory(&browserInfo, sizeof(BROWSEINFO));
	browserInfo.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();
	browserInfo.lpszTitle = L"��ѡ��ɳ�и�·��";
	browserInfo.ulFlags = BIF_RETURNFSANCESTORS|BIF_USENEWUI;

	LPITEMIDLIST idl = SHBrowseForFolder(&browserInfo);
	if (idl == NULL)
		return ;

	SHGetPathFromIDList(idl, directoryName);

	if (wcslen(directoryName) <= 0)
		return ;

	if (directoryName[wcslen(directoryName)] != L'\\')
		wcscat_s(directoryName, L"\\");

	_wcsupr_s(directoryName);

	// �������Ҫ���뱣���Ŀ¼��Ϣ�Ĳ���
	
	m_listRedirectRoot.AddString(directoryName);

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnRemoveRedirectRoot()
{
	// �õ�ѡ�е���Ŀ
	int selectIndex = m_listRedirectRoot.GetCurSel();

	WCHAR itemText[MAX_PATH] = {0};
	m_listRedirectRoot.GetText(selectIndex, itemText);

	// �ڳ־��������Ƴ�����

	m_listRedirectRoot.DeleteString(selectIndex);

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnActiveRedirectRoot()
{
	// �õ�ѡ�е���Ŀ
	int selectIndex = m_listRedirectRoot.GetCurSel();

	if (selectIndex < 0)
	{
		MessageBox(L"��ѡ��ɳ��·����", L"��ʾ", MB_OK|MB_ICONINFORMATION);
		return ;
	}

	WCHAR itemText[MAX_PATH] = {0};
	m_listRedirectRoot.GetText(selectIndex, itemText);

	int errCode = ISISandBoxSetRedirectPath(itemText);
	if (errCode != 0)
	{
		MessageBox(L"����ɳ��ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
		return ;
	}

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnAddExpression()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CHandleExpressionDlg dlg;
	INT_PTR ret = dlg.DoModal();
	if (ret == IDOK)
	{
		if (dlg.m_bIsEmpty)
			return ;

		// ��ӱ��ʽ
		WCHAR expression[MAX_PATH] = {0};
		wcscpy_s(expression, MAX_PATH, dlg.m_strExpression.GetBuffer(0));
		_wcsupr_s(expression);

		// ����������
		int errCode = ISISandBoxSetProtectPath(expression);
		if (errCode != 0)
		{
			MessageBox(L"��ӱ��ʽʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
			return ;
		}

		m_listExpression.AddString(expression);
	}

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnRemoveExpression()
{
	// �õ�ѡ�е���Ŀ
	int selectIndex = m_listExpression.GetCurSel();

	WCHAR itemText[MAX_PATH] = {0};
	m_listExpression.GetText(selectIndex, itemText);

	int errCode = ISISandBoxRemoveProtectPath(itemText);
	if (errCode != 0)
	{
		MessageBox(L"�Ƴ����ʽʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
		return ;
	}

	m_listExpression.DeleteString(selectIndex);

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnAddProtectProcess()
{
	CHandleProtectProcessDlg dlg;
	INT_PTR ret = dlg.DoModal();
	if (ret == IDOK)
	{
		if (dlg.m_dwProcessId == 0)
			return ;

		int errCode = ISISandBoxSetProtectProcess(dlg.m_dwProcessId);
		if (errCode != 0)
		{
			MessageBox(L"��ӱ�������ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
			return ;
		}

		WCHAR strProcessId[64] = {0};
		_itow_s(dlg.m_dwProcessId, strProcessId, 10);

		m_listProtectProcess.AddString(strProcessId);
	}

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnRemoveProtectProcess()
{
	// �õ�ѡ�е���Ŀ
	int selectIndex = m_listProtectProcess.GetCurSel();

	WCHAR itemText[MAX_PATH] = {0};
	m_listProtectProcess.GetText(selectIndex, itemText);

	DWORD processId = _wtoi(itemText);
	int errCode = ISISandBoxRemoveProtectProcess(processId);
	if (errCode != 0)
	{
		MessageBox(L"�Ƴ���������ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
		return ;
	}

	m_listProtectProcess.DeleteString(selectIndex);

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnStartDriverWorking()
{
	int errCode = ISISandBoxStart();
	if (errCode != 0)
	{
		MessageBox(L"��������ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
		return ;
	}

	WCHAR titleText[512] = {0};
	wsprintf(titleText, L"%s - %s", PROGRAM_NAME, STATE_WORKING);
	SetWindowText(titleText);

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnStopDriverWorking()
{
	int errCode = ISISandBoxStop();
	if (errCode != 0)
	{
		MessageBox(L"ֹͣ����ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
		return ;
	}

	WCHAR titleText[512] = {0};
	wsprintf(titleText, L"%s - %s", PROGRAM_NAME, STATE_PENDING);
	SetWindowText(titleText);

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnLoadDriver()
{
	// �Ȱ�װ��������������򵥵�ʹ��fltmc����

	WinExec("fltmc load isisandbox", SW_HIDE);
	Sleep(1500);
}

void CISISandBoxUIDlg::OnBnClickedBtnConnectDriver()
{
	//int errCode = sandbox.ConnectToISISafeBox();
	//if (errCode != 0)
	//{
	//	MessageBox(L"������������ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
	//	return ;
	//}

	//MessageBox(L"������������ɹ���", L"����", MB_OK|MB_ICONERROR);
}

void CISISandBoxUIDlg::OnBnClickedBtnDisconnectDriver()
{
	//sandbox.DisconnectFromISISafeBox();
	//MessageBox(L"�Ͽ������������ӳɹ���", L"��Ϣ", MB_OK|MB_ICONINFORMATION);
}
