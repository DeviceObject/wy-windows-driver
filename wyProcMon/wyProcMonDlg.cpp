// wyProcMonDlg.cpp : 实现文件
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

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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


// CwyProcMonDlg 对话框




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


// CwyProcMonDlg 消息处理程序

BOOL CwyProcMonDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	// 初始化控件
	m_cListCtrl.InsertColumn(0, L"时间", LVCFMT_LEFT, 50);
	m_cListCtrl.InsertColumn(1, L"进程名", LVCFMT_LEFT, 100);
	m_cListCtrl.InsertColumn(2, L"进程ID", LVCFMT_LEFT, 50);
	m_cListCtrl.InsertColumn(3, L"操作", LVCFMT_LEFT, 100);
	m_cListCtrl.InsertColumn(4, L"路径", LVCFMT_LEFT, 500);
	m_cListCtrl.InsertColumn(5, L"结果", LVCFMT_LEFT, 100);
	m_cListCtrl.InsertColumn(6, L"详细", LVCFMT_LEFT, 500);

	// 开启通信端口
	
	HANDLE port = NULL;
	HRESULT hResult = FilterConnectCommunicationPort(wyProcMonSysPortName, 0, NULL, 0, NULL, &port);
	if (IS_ERROR(hResult))
	{
		TRACE("FilterConnectCommunicationPort failed .\n");
		return TRUE;
	}

	// 创建一个完成端口
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

	// 启动接收线程
	CreateThread(NULL, 0, MonThread, this, 0, NULL);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CwyProcMonDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
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

	// 启动指定数量的线程
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
			// 创建消息
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "msg will not be leaked because it is freed in ScannerWorker")

			dlg->notification = new PMESSAGE_NOTIFICATION;
			if (dlg->notification == NULL)
			{
				ret = ERROR_NOT_ENOUGH_MEMORY;
			}

			ZeroMemory(dlg->notification, sizeof(PMESSAGE_NOTIFICATION));

			// 从驱动获取消息
		}
	}

	return 0;
}