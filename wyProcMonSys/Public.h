
#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

// ����ȫ�ֱ���
typedef struct _GLOBAL_DATA
{
	// ��������
	PDRIVER_OBJECT iDriverObject;

	// ������ƫ����
	ULONG iProcessNameOffsetInPEPROCESS;

	BOOLEAN iStartFilter;

	// ������
	PFLT_FILTER iFilter;

	// ����ͨ�Ŷ˿�
	PFLT_PORT iDriverPort;

	// �û�����
	PEPROCESS iUserProcess;

	// �û�ͨ�Ŷ˿�
	PFLT_PORT iUserPort;

} GLOBAL_DATA, *PGLOBAL_DATA;

extern GLOBAL_DATA globalData;

#ifdef __cplusplus
}
#endif

#endif