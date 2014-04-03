// ISISandBoxUIDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "ISISandBox.h"


// CISISandBoxUIDlg �Ի���
class CISISandBoxUIDlg : public CDialog
{
// ����
public:
	CISISandBoxUIDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_ISISANDBOXUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListBox m_listRedirectRoot;
	CListBox m_listExpression;
	CListBox m_listProtectProcess;

public:
	

public:
	afx_msg void OnBnClickedBtnAddRedirectRoot();
	afx_msg void OnBnClickedBtnRemoveRedirectRoot();
	afx_msg void OnBnClickedBtnActiveRedirectRoot();
	afx_msg void OnBnClickedBtnAddExpression();
	afx_msg void OnBnClickedBtnRemoveExpression();
	afx_msg void OnBnClickedBtnAddProtectProcess();
	afx_msg void OnBnClickedBtnRemoveProtectProcess();
	afx_msg void OnBnClickedBtnStartDriverWorking();
	afx_msg void OnBnClickedBtnStopDriverWorking();
	afx_msg void OnBnClickedBtnLoadDriver();
	afx_msg void OnBnClickedBtnConnectDriver();
	afx_msg void OnBnClickedBtnDisconnectDriver();
};
