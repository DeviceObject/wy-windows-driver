// SSDTHOOKDemoUDlg.h : 头文件
//

#pragma once
#include <WinIoCtl.h>
#include <Psapi.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include "afxcmn.h"
#include "SSDTHOOKMidware.h"
using namespace std;

#define MAX_PROCESS_COUNT				1024
#define SSDT_PROCESS_TIMER_ID			10012
#define SSDT_PROCESS_TIMER_INTERVAL		1500

#define SSDT01_SYS_FILE_PATH			L"\\SSDTHOOKDemo.sys"
#define SSDT01_SERVICE_NAME				L"SSDTHOOKDemo"
#define SSDT01_DEVICE_NAME				L"\\\\.\\SSDTHHOOKDemo"

#define	SSDT01_DEVICE_TYPE				FILE_DEVICE_UNKNOWN

//定义用于应用程序和驱动程序通信的宏，这里使用的是缓冲区读写方式
//#define	IO_INSERT_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT01_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define	IO_REMOVE_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT01_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define	IO_INSERT_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT01_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define	IO_REMOVE_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT01_DEVICE_TYPE, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

//自定义的进程状态枚举类型
//typedef enum _PROCESS_STATE
//{
//	ProcessStateGeneral,
//	ProcessStateHide,
//	ProcessStateProtect,
//	ProcessStateHideAndProtect,
//	ProcessStateUnknown
//
//}PROCESS_STATE;

//自定义的进程 ID 和进程状态绑定后的数据结构
//typedef struct _PROCESS_BIND 
//{
//	DWORD dwPID;
//	PROCESS_STATE state;
//
//}PROCESS_BIND, * PPROCESS_BIND;


// CSSDTHOOKDemoUDlg 对话框
class CSSDTHOOKDemoUDlg : public CDialog
{
// 构造
public:
	CSSDTHOOKDemoUDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SSDTHOOKDEMOU_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	CListCtrl m_list;

	vector<PROCESS_STATE_EX> iProcessState;

	SSDTHOOKMidware iSSDTHOOKMidware;

public:
	void RefreshProcessList();
	afx_msg void OnBnClickedBtnIntsallDriver();
	afx_msg void OnBnClickedBtnUninstallDriver();
	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnBnClickedBtnDisconnect();
	afx_msg void OnBnClickedBtnStartWorking();
	afx_msg void OnBnClickedBtnStopWorking();
	afx_msg void OnBnClickedBtnHideProcess();
	afx_msg void OnBnClickedBtnProtectProcess();
	afx_msg void OnHdnItemclickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownList1(NMHDR *pNMHDR, LRESULT *pResult);
};
