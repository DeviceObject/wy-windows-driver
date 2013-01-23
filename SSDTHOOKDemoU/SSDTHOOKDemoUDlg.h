// SSDTHOOKDemoUDlg.h : ͷ�ļ�
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

//��������Ӧ�ó������������ͨ�ŵĺ꣬����ʹ�õ��ǻ�������д��ʽ
//#define	IO_INSERT_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT01_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define	IO_REMOVE_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT01_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define	IO_INSERT_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT01_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define	IO_REMOVE_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT01_DEVICE_TYPE, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

//�Զ���Ľ���״̬ö������
//typedef enum _PROCESS_STATE
//{
//	ProcessStateGeneral,
//	ProcessStateHide,
//	ProcessStateProtect,
//	ProcessStateHideAndProtect,
//	ProcessStateUnknown
//
//}PROCESS_STATE;

//�Զ���Ľ��� ID �ͽ���״̬�󶨺�����ݽṹ
//typedef struct _PROCESS_BIND 
//{
//	DWORD dwPID;
//	PROCESS_STATE state;
//
//}PROCESS_BIND, * PPROCESS_BIND;


// CSSDTHOOKDemoUDlg �Ի���
class CSSDTHOOKDemoUDlg : public CDialog
{
// ����
public:
	CSSDTHOOKDemoUDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_SSDTHOOKDEMOU_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
