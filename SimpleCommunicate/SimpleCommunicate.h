#ifndef _SIMPLE_COMMUNICATE_H_
#define _SIMPLE_COMMUNICATE_H_

#include <fltkernel.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ����ȫ�ֱ���
#define DriverPortName	L"\\SimpleCommunicatePort"

typedef struct _GLOBAL_DATA
{
	// ����������
	PFLT_FILTER iFilter;

	// �˿�
	PFLT_PORT iServerPort;

	// �˿�
	PFLT_PORT iClientPort;

	// �û�̬����
	PEPROCESS iUserProcess;


} GLOBAL_DATA, *PGLOBAL_DATA;
extern GLOBAL_DATA globalData;

/**
 * ������ں���
 * ������
 *	DriverObject	��������
 *	RegistryPath	��������ע���·��
 */
DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath);

/**
 * ����ж�غ���
 * ������
 *	Flags	ж�ر��
 */
NTSTATUS DriverUnload(__in FLT_FILTER_UNLOAD_FLAGS Flags);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// _SIMPLE_COMMUNICATE_H_