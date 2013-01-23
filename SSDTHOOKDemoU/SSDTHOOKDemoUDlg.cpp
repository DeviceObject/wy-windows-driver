// SSDTHOOKDemoUDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "SSDTHOOKDemoU.h"
#include "SSDTHOOKDemoUDlg.h"
#include <vector>
#include <Psapi.h>

#pragma comment(lib, "psapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CSSDTHOOKDemoUDlg �Ի���
BOOL DebugPrivilege(BOOL bEnable) 
{ 
	BOOL bResult = TRUE; 
	HANDLE hToken; 
	TOKEN_PRIVILEGES TokenPrivileges; 

	if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES, &hToken) == 0) 
	{ 
		printf("OpenProcessToken Error: %d\n ",GetLastError()); 
		bResult = FALSE; 
	} 

	TokenPrivileges.PrivilegeCount = 1; 
	TokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0; 
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &TokenPrivileges.Privileges[0].Luid); 
	AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL); 
	if(GetLastError() != ERROR_SUCCESS) 
	{ 
		bResult = FALSE; 
	} 
	CloseHandle(hToken); 

	return bResult; 
}



CSSDTHOOKDemoUDlg::CSSDTHOOKDemoUDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSSDTHOOKDemoUDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSSDTHOOKDemoUDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CSSDTHOOKDemoUDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_INTSALL_DRIVER, &CSSDTHOOKDemoUDlg::OnBnClickedBtnIntsallDriver)
	ON_BN_CLICKED(IDC_BTN_UNINSTALL_DRIVER, &CSSDTHOOKDemoUDlg::OnBnClickedBtnUninstallDriver)
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CSSDTHOOKDemoUDlg::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CSSDTHOOKDemoUDlg::OnBnClickedBtnDisconnect)
	ON_BN_CLICKED(IDC_BTN_START_WORKING, &CSSDTHOOKDemoUDlg::OnBnClickedBtnStartWorking)
	ON_BN_CLICKED(IDC_BTN_STOP_WORKING, &CSSDTHOOKDemoUDlg::OnBnClickedBtnStopWorking)
	ON_BN_CLICKED(IDC_BTN_HIDE_PROCESS, &CSSDTHOOKDemoUDlg::OnBnClickedBtnHideProcess)
	ON_BN_CLICKED(IDC_BTN_PROTECT_PROCESS, &CSSDTHOOKDemoUDlg::OnBnClickedBtnProtectProcess)
	ON_NOTIFY(HDN_ITEMCLICK, 0, &CSSDTHOOKDemoUDlg::OnHdnItemclickList1)
	ON_NOTIFY(NM_CLICK, IDC_LIST1, &CSSDTHOOKDemoUDlg::OnNMClickList1)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST1, &CSSDTHOOKDemoUDlg::OnLvnKeydownList1)
END_MESSAGE_MAP()


// CSSDTHOOKDemoUDlg ��Ϣ�������

BOOL CSSDTHOOKDemoUDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	m_list.InsertColumn(0, L"����ID", LVCFMT_LEFT, 50);
	m_list.InsertColumn(1, L"������", LVCFMT_LEFT, 120);
	m_list.InsertColumn(2, L"ӳ��·��", LVCFMT_LEFT, 550);
	m_list.InsertColumn(3, L"״̬", LVCFMT_LEFT, 70);

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT );

	DebugPrivilege(TRUE);
	RefreshProcessList();

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CSSDTHOOKDemoUDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CSSDTHOOKDemoUDlg::OnPaint()
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
HCURSOR CSSDTHOOKDemoUDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LPWSTR GetProcessUserName(DWORD dwPID)
{
	HANDLE hProcessToken;
	HANDLE hProcess;

	//Open the Remote Process

	hProcess=::OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,0,dwPID);
	if(hProcess)
	{
		//Open The Process's Token which can be used 
		//to achieve more security information

		if(::OpenProcessToken(hProcess,TOKEN_READ,&hProcessToken))
		{
			DWORD TokenInformationLength = 0;
			DWORD ReturnLength = 0;
			PTOKEN_USER tuUser = new TOKEN_USER;
			LPWSTR UserName = new WCHAR[255];   // Username declared in the Heap
			wmemset(UserName, 0, 255);

			DWORD cchName = 0;
			LPWSTR lpReferencedDomainName = new WCHAR[255];
			wmemset(lpReferencedDomainName, 0, 255);
			DWORD  cchReferencedDomainName = 0;
			SID_NAME_USE peUse;
			TokenInformationLength = sizeof(tuUser);
			//Get the User Information from the Token

			if(::GetTokenInformation(hProcessToken,TokenUser,tuUser,
				TokenInformationLength,&ReturnLength)) 
			{
				::LookupAccountSid(NULL,tuUser->User.Sid,UserName,&cchName, 
					lpReferencedDomainName,&cchReferencedDomainName,&peUse);

			}
			else // The structure is not large enough

			{
				delete tuUser;
				tuUser=(PTOKEN_USER)(new BYTE[ReturnLength]);
				TokenInformationLength=ReturnLength;
				if(::GetTokenInformation(hProcessToken,TokenUser,tuUser,
					TokenInformationLength,&ReturnLength))
				{
					::LookupAccountSid(NULL,tuUser->User.Sid,UserName,&cchName,
						lpReferencedDomainName,&cchReferencedDomainName,&peUse);
				}
				delete tuUser;
			}
			delete [] lpReferencedDomainName;
			lpReferencedDomainName = NULL;
			return UserName;    
		}
	}
	return NULL;
}

void CSSDTHOOKDemoUDlg::RefreshProcessList()
{
	// ����ö�ٽ���
	DWORD processIds[1024] = {0};
	DWORD needed = 0;
	BOOL b = EnumProcesses(processIds, 1024, &needed);
	if (!b)
		return ;

	DWORD currentProcessCount = needed / sizeof(DWORD);

	int itemCount = 0;
	for (DWORD i = 0; i < currentProcessCount; ++i)
	{
		// ������
		wchar_t processName[MAX_PATH] = {0};

		// ȫ·��
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, processIds[i]);
		if (hProcess == INVALID_HANDLE_VALUE)
			continue;

		DWORD errCode = GetModuleBaseName(hProcess, NULL, processName, MAX_PATH);
		if (errCode == 0)
		{
			CloseHandle(hProcess);
			continue;
		}

		wchar_t processPath[MAX_PATH] = {0};
		HMODULE hModules[1024] = {0};
		DWORD cb = 1024, needed2 = 0;
		EnumProcessModules(hProcess, hModules, cb, &needed2);
		errCode = GetModuleFileNameEx(hProcess, hModules[0], processPath, MAX_PATH);
		if (errCode == 0)
		{
			CloseHandle(hProcess);
			continue;
		}

		// ����״̬
		wchar_t state[512] = {0};
		bool isFindSpecialProcess = false;
		vector<PROCESS_STATE_EX>::iterator iter;
		for (iter = iProcessState.begin(); iter != iProcessState.end(); ++iter)
		{
			if (iter->processId == processIds[i])
			{
				// �ҵ�������̣���������
				if ((iter->processState & TerminateProcessProtect) == TerminateProcessProtect)
					wcscat_s(state, 512, L"���� ");
				
				if ((iter->processState & QueryProcessInfoProtect) == QueryProcessInfoProtect)
					wcscat_s(state, 512, L"���� ");

				isFindSpecialProcess = true;
				break;
			}
		}

		if (isFindSpecialProcess)
		{
			// Do nothing ...
		}
		else
		{
			wcscpy_s(state, 512, L"��ͨ");
		}

		wchar_t processId[32] = {0};
		_itow_s(processIds[i], processId, 10);
		m_list.InsertItem(itemCount, processId);
		m_list.SetItemText(itemCount, 1, processName);
		m_list.SetItemText(itemCount, 2, processPath);
		m_list.SetItemText(itemCount, 3, state);

		++itemCount;
	}
}

void CSSDTHOOKDemoUDlg::OnBnClickedBtnIntsallDriver()
{
	int errCode = iSSDTHOOKMidware.RunSSDTHOOKDriver(_T("SSDTHOOKDemo"));

	if (errCode != 0)
		MessageBox(L"������װʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);

	return ;
}

void CSSDTHOOKDemoUDlg::OnBnClickedBtnUninstallDriver()
{
	int errCode = iSSDTHOOKMidware.StopSSDTHOOKDriver(_T("SSDTHOOKDemo"));

	if (errCode != 0)
		MessageBox(L"����ж��ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);

	return ;
}

void CSSDTHOOKDemoUDlg::OnBnClickedBtnConnect()
{
	int errCode = iSSDTHOOKMidware.ConnectToDriver(SYMBO_LINK_NAME_T);

	if (errCode != 0)
		MessageBox(L"���ӵ�����ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);

	return ;
}

void CSSDTHOOKDemoUDlg::OnBnClickedBtnDisconnect()
{
	int errCode = iSSDTHOOKMidware.DisconnectToDriver();

	if (errCode != 0)
		MessageBox(L"�Ͽ���������ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);

	return ;
}

void CSSDTHOOKDemoUDlg::OnBnClickedBtnStartWorking()
{
	int errCode = iSSDTHOOKMidware.StartSSDTHOOK();

	if (errCode != 0)
		MessageBox(L"��������ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);

	return ;
}

void CSSDTHOOKDemoUDlg::OnBnClickedBtnStopWorking()
{
	int errCode = iSSDTHOOKMidware.StopSSDTHOOK();

	if (errCode != 0)
		MessageBox(L"ֹͣ����ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);

	return ;
}

void CSSDTHOOKDemoUDlg::OnBnClickedBtnHideProcess()
{
	// ��ȡ��Ӧģ��Ľ���ID
	int selectedItemIndex = m_list.GetSelectionMark();
	if (selectedItemIndex < 0)
		return ;

	wchar_t processIdStr[32] = {0};
	m_list.GetItemText(selectedItemIndex, 0, processIdStr, 32);
	DWORD processId = _wtoi(processIdStr);

	if (processId <= 0)
		return ;

	// ��ȡ��ǰ����״̬
	wchar_t state[512] = {0};
	bool isSpecialState = false;
	vector<PROCESS_STATE_EX>::iterator iter;
	for (iter = iProcessState.begin(); iter != iProcessState.end(); ++iter)
	{
		if (iter->processId == processId)
		{
			if ((iter->processState & QueryProcessInfoProtect) == QueryProcessInfoProtect)
				isSpecialState = true;

			break;
		}
	}

	int errCode = ERROR_SUCCESS;
	if (isSpecialState)
	{
		// ������״̬���Ƴ�
		errCode = iSSDTHOOKMidware.RemoveSSDTHOOKProcess(QueryProcessInfoProtect, processId);
		if ((errCode & QueryProcessInfoProtect) != QueryProcessInfoProtect)
		{
			MessageBox(L"�ָ����ؽ���ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
			return ;
		}

		// �ɹ�������״̬
		iter->processState = ~(iter->processState) ^ (~QueryProcessInfoProtect);
		
		// �жϣ����״̬ȫ������˾�ȥ�����������
		if (iter->processState == 0)
			iter = iProcessState.erase(iter);
	}
	else
	{
		// ��ͨ״̬�����
		errCode = iSSDTHOOKMidware.AddSSDTHOOKProcess(QueryProcessInfoProtect, processId);
		if ((errCode & QueryProcessInfoProtect) != QueryProcessInfoProtect)
		{
			MessageBox(L"���ؽ���ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
			return ;
		}

		// �ɹ�������״̬
		PROCESS_STATE_EX processStateEx;
		processStateEx.processId = processId;
		processStateEx.processState |= QueryProcessInfoProtect;
		iProcessState.push_back(processStateEx);
	}
	
	// ˢ���б�
	m_list.DeleteAllItems();
	RefreshProcessList();

	return ;
}

void CSSDTHOOKDemoUDlg::OnBnClickedBtnProtectProcess()
{
	// ��ȡ��Ӧģ��Ľ���ID
	int selectedItemIndex = m_list.GetSelectionMark();
	if (selectedItemIndex < 0)
		return ;

	wchar_t processIdStr[32] = {0};
	m_list.GetItemText(selectedItemIndex, 0, processIdStr, 32);
	DWORD processId = _wtoi(processIdStr);

	if (processId <= 0)
		return ;

	// ��ȡ��ǰ����״̬
	wchar_t state[512] = {0};
	bool isSpecialState = false;
	vector<PROCESS_STATE_EX>::iterator iter;
	for (iter = iProcessState.begin(); iter != iProcessState.end(); ++iter)
	{
		if (iter->processId == processId)
		{
			if ((iter->processState & TerminateProcessProtect) == TerminateProcessProtect)
				isSpecialState = true;

			break;
		}
	}

	int errCode = ERROR_SUCCESS;
	if (isSpecialState)
	{
		// ������״̬���Ƴ�
		errCode = iSSDTHOOKMidware.RemoveSSDTHOOKProcess(TerminateProcessProtect, processId);
		if ((errCode & TerminateProcessProtect) != TerminateProcessProtect)
		{
			MessageBox(L"ȡ����������ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
			return ;
		}

		// �ɹ�������״̬
		iter->processState = ~(iter->processState) ^ (~TerminateProcessProtect);

		// �жϣ����״̬ȫ������˾�ȥ�����������
		if (iter->processState == 0)
			iter = iProcessState.erase(iter);
	}
	else
	{
		// ��ͨ״̬�����
		errCode = iSSDTHOOKMidware.AddSSDTHOOKProcess(TerminateProcessProtect, processId);
		if ((errCode & TerminateProcessProtect) != TerminateProcessProtect)
		{
			MessageBox(L"��������ʧ�ܣ�", L"����", MB_OK|MB_ICONERROR);
			return ;
		}

		// �ɹ�������״̬
		PROCESS_STATE_EX processStateEx;
		processStateEx.processId = processId;
		processStateEx.processState |= TerminateProcessProtect;
		iProcessState.push_back(processStateEx);
	}

	// ˢ���б�
	m_list.DeleteAllItems();
	RefreshProcessList();
}

void CSSDTHOOKDemoUDlg::OnHdnItemclickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	*pResult = 0;
}

void CSSDTHOOKDemoUDlg::OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;

	int selectedItemIndex = m_list.GetSelectionMark();
	if (selectedItemIndex < 0)
		return ;

	// ��ȡID�����״̬��������Ӧ�Ŀؼ���Ϣ
	wchar_t processIdStr[32] = {0};
	m_list.GetItemText(selectedItemIndex, 0, processIdStr, 32);
	DWORD processId = _wtoi(processIdStr);

	if (processId <= 0)
		return ;

	// ��ȡ��ǰ����״̬
	wchar_t state[512] = {0};
	bool isSpecialState = false;
	vector<PROCESS_STATE_EX>::iterator iter;
	for (iter = iProcessState.begin(); iter != iProcessState.end(); ++iter)
	{
		if (iter->processId == processId)
		{
			if ((iter->processState & TerminateProcessProtect) == TerminateProcessProtect)
			{
				CWnd *pCwnd = GetDlgItem(IDC_BTN_PROTECT_PROCESS);
				if (pCwnd)
				{
					pCwnd->SetWindowText(L"ȡ������");
				}
			}
			
			if ((iter->processState & QueryProcessInfoProtect) == QueryProcessInfoProtect)
			{
				CWnd *pCwnd = GetDlgItem(IDC_BTN_PROTECT_PROCESS);
				if (pCwnd)
				{
					pCwnd->SetWindowText(L"ȡ������");
				}
			}
			
			break;
		}
	}
}

void CSSDTHOOKDemoUDlg::OnLvnKeydownList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	
	// ��ӦF5��
	if (pLVKeyDow->wVKey == VK_F5)
	{
		// ��¼֮ǰѡ�е���
		bool isRecord = false;
		int selectedItemIndex = m_list.GetSelectionMark();
		if (selectedItemIndex >= 0)
			isRecord = true;

		// ˢ���б�
		m_list.DeleteAllItems();
		RefreshProcessList();

		if (isRecord)
			m_list.SetSelectionMark(selectedItemIndex);
	}

	*pResult = 0;
}
