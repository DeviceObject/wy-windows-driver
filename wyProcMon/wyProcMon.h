// wyProcMon.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CwyProcMonApp:
// �йش����ʵ�֣������ wyProcMon.cpp
//

class CwyProcMonApp : public CWinApp
{
public:
	CwyProcMonApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CwyProcMonApp theApp;