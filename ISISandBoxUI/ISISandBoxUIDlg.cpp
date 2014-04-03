// ISISandBoxUIDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ISISandBoxUI.h"
#include "ISISandBoxUIDlg.h"
#include "HandleExpressionDlg.h"
#include "HandleProtectProcessDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CISISandBoxUIDlg 对话框
#pragma comment(lib, "ISISandBox.lib")


#define PROGRAM_NAME	L"ISI沙盒"
#define STATE_WORKING	L"工作中"
#define STATE_PENDING	L"未工作"

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


// CISISandBoxUIDlg 消息处理程序

BOOL CISISandBoxUIDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	WCHAR titleText[512] = {0};
	wsprintf(titleText, L"%s - %s", PROGRAM_NAME, STATE_PENDING);
	SetWindowText(titleText);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CISISandBoxUIDlg::OnPaint()
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
HCURSOR CISISandBoxUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#define   BIF_NEWDIALOGSTYLE    0x0040   
#define   BIF_USENEWUI (BIF_NEWDIALOGSTYLE | BIF_EDITBOX)     
void CISISandBoxUIDlg::OnBnClickedBtnAddRedirectRoot()
{
	// 获取文件夹路径，然后处理尾部
	WCHAR directoryName[MAX_PATH] = {0};

	BROWSEINFO browserInfo;
	ZeroMemory(&browserInfo, sizeof(BROWSEINFO));
	browserInfo.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();
	browserInfo.lpszTitle = L"请选择沙盒根路径";
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

	// 这里后面要加入保存根目录信息的操作
	
	m_listRedirectRoot.AddString(directoryName);

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnRemoveRedirectRoot()
{
	// 得到选中的项目
	int selectIndex = m_listRedirectRoot.GetCurSel();

	WCHAR itemText[MAX_PATH] = {0};
	m_listRedirectRoot.GetText(selectIndex, itemText);

	// 在持久数据中移除此项

	m_listRedirectRoot.DeleteString(selectIndex);

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnActiveRedirectRoot()
{
	// 得到选中的项目
	int selectIndex = m_listRedirectRoot.GetCurSel();

	if (selectIndex < 0)
	{
		MessageBox(L"请选择沙盒路径！", L"提示", MB_OK|MB_ICONINFORMATION);
		return ;
	}

	WCHAR itemText[MAX_PATH] = {0};
	m_listRedirectRoot.GetText(selectIndex, itemText);

	int errCode = ISISandBoxSetRedirectPath(itemText);
	if (errCode != 0)
	{
		MessageBox(L"激活沙盒失败！", L"错误", MB_OK|MB_ICONERROR);
		return ;
	}

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnAddExpression()
{
	// TODO: 在此添加控件通知处理程序代码
	CHandleExpressionDlg dlg;
	INT_PTR ret = dlg.DoModal();
	if (ret == IDOK)
	{
		if (dlg.m_bIsEmpty)
			return ;

		// 添加表达式
		WCHAR expression[MAX_PATH] = {0};
		wcscpy_s(expression, MAX_PATH, dlg.m_strExpression.GetBuffer(0));
		_wcsupr_s(expression);

		// 向驱动发送
		int errCode = ISISandBoxSetProtectPath(expression);
		if (errCode != 0)
		{
			MessageBox(L"添加表达式失败！", L"错误", MB_OK|MB_ICONERROR);
			return ;
		}

		m_listExpression.AddString(expression);
	}

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnRemoveExpression()
{
	// 得到选中的项目
	int selectIndex = m_listExpression.GetCurSel();

	WCHAR itemText[MAX_PATH] = {0};
	m_listExpression.GetText(selectIndex, itemText);

	int errCode = ISISandBoxRemoveProtectPath(itemText);
	if (errCode != 0)
	{
		MessageBox(L"移除表达式失败！", L"错误", MB_OK|MB_ICONERROR);
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
			MessageBox(L"添加保护程序失败！", L"错误", MB_OK|MB_ICONERROR);
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
	// 得到选中的项目
	int selectIndex = m_listProtectProcess.GetCurSel();

	WCHAR itemText[MAX_PATH] = {0};
	m_listProtectProcess.GetText(selectIndex, itemText);

	DWORD processId = _wtoi(itemText);
	int errCode = ISISandBoxRemoveProtectProcess(processId);
	if (errCode != 0)
	{
		MessageBox(L"移除保护程序失败！", L"错误", MB_OK|MB_ICONERROR);
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
		MessageBox(L"开启工作失败！", L"错误", MB_OK|MB_ICONERROR);
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
		MessageBox(L"停止工作失败！", L"错误", MB_OK|MB_ICONERROR);
		return ;
	}

	WCHAR titleText[512] = {0};
	wsprintf(titleText, L"%s - %s", PROGRAM_NAME, STATE_PENDING);
	SetWindowText(titleText);

	return ;
}

void CISISandBoxUIDlg::OnBnClickedBtnLoadDriver()
{
	// 先安装，再启动，这里简单的使用fltmc启动

	WinExec("fltmc load isisandbox", SW_HIDE);
	Sleep(1500);
}

void CISISandBoxUIDlg::OnBnClickedBtnConnectDriver()
{
	//int errCode = sandbox.ConnectToISISafeBox();
	//if (errCode != 0)
	//{
	//	MessageBox(L"连接驱动程序失败！", L"错误", MB_OK|MB_ICONERROR);
	//	return ;
	//}

	//MessageBox(L"连接驱动程序成功！", L"错误", MB_OK|MB_ICONERROR);
}

void CISISandBoxUIDlg::OnBnClickedBtnDisconnectDriver()
{
	//sandbox.DisconnectFromISISafeBox();
	//MessageBox(L"断开驱动程序连接成功！", L"信息", MB_OK|MB_ICONINFORMATION);
}
