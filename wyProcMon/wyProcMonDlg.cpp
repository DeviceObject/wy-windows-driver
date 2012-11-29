// wyProcMonDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "wyProcMon.h"
#include "wyProcMonDlg.h"
#include "wyProcMonPublic.h"
#include <WINDOWS.H>
#include <winioctl.h>
#include <string.h>
#include <crtdbg.h>
#include <assert.h>
#include <dontuse.h>
#include <fltuser.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define SCANNER_DEFAULT_REQUEST_COUNT       5
#define SCANNER_DEFAULT_THREAD_COUNT        2

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


// CwyProcMonDlg �Ի���




CwyProcMonDlg::CwyProcMonDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CwyProcMonDlg::IDD, pParent)
	, port(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CwyProcMonDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_cListCtrl);
}

BEGIN_MESSAGE_MAP(CwyProcMonDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CwyProcMonDlg ��Ϣ�������

BOOL CwyProcMonDlg::OnInitDialog()
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
	// ��ʼ���ؼ�
	m_cListCtrl.InsertColumn(0, L"ʱ��", LVCFMT_LEFT, 50);
	m_cListCtrl.InsertColumn(1, L"������", LVCFMT_LEFT, 100);
	m_cListCtrl.InsertColumn(2, L"����ID", LVCFMT_LEFT, 50);
	m_cListCtrl.InsertColumn(3, L"����", LVCFMT_LEFT, 100);
	m_cListCtrl.InsertColumn(4, L"·��", LVCFMT_LEFT, 500);
	m_cListCtrl.InsertColumn(5, L"���", LVCFMT_LEFT, 100);
	m_cListCtrl.InsertColumn(6, L"��ϸ", LVCFMT_LEFT, 500);

	// ����ͨ�Ŷ˿�
	
	HANDLE port = NULL;
	HRESULT hResult = FilterConnectCommunicationPort(wyProcMonSysPortName, 0, NULL, 0, NULL, &port);
	if (IS_ERROR(hResult))
	{
		TRACE("FilterConnectCommunicationPort failed .\n");
		return TRUE;
	}

	// ����һ����ɶ˿�
	threadCount = SCANNER_DEFAULT_THREAD_COUNT;
	HANDLE completion = CreateIoCompletionPort(port, NULL, 0, threadCount);
	if (completion == NULL)
	{
		TRACE("CreateIoCompletionPort failed .\n");
		CloseHandle(port);
		return TRUE;
	}

	iDriverCom.port = port;
	iDriverCom.completion = completion;

	// ���������߳�
	CreateThread(NULL, 0, MonThread, this, 0, NULL);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CwyProcMonDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CwyProcMonDlg::OnPaint()
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
HCURSOR CwyProcMonDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

DWORD CwyProcMonDlg::MonThread(PVOID lpParam)
{
	CwyProcMonDlg *dlg = (CwyProcMonDlg*)lpParam;
	if (dlg == NULL)
		return -1;

	HRESULT ret = 0;

	// ����ָ���������߳�
	DWORD threadId = 0;
	for (int i = 0; i < dlg->threadCount; ++i)
	{
		dlg->iThreads[i] = CreateThread(NULL, 0, MonThread, &dlg->iDriverCom, 0, &threadId);

		if (dlg->iThreads[i] == NULL)
		{
			ret = GetLastError();
		}

		for (int j = 0; j < 1; ++j)
		{
			// ������Ϣ
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "msg will not be leaked because it is freed in ScannerWorker")

			dlg->notification = new PMESSAGE_NOTIFICATION;
			if (dlg->notification == NULL)
			{
				ret = ERROR_NOT_ENOUGH_MEMORY;
			}

			ZeroMemory(dlg->notification, sizeof(PMESSAGE_NOTIFICATION));

			// ��������ȡ��Ϣ
		}
	}

	return 0;
}