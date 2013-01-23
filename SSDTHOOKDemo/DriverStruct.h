#ifndef _DRVICE_STRUCT_H_
#define _DRVICE_STRUCT_H_

//=====================================================================================//
// ���ļ��Ǵ�������ʵ���߼�����
// ������ͬ��SSDT HOOK����ʱֻ��Ҫ�Ķ��˲��ֵĺ�������
//=====================================================================================//

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "HookLogic.h"
#include <stdlib.h>
#include <WINDEF.H>

extern BOOL global_WorkState;		// ȫ�ֱ�������������״̬
extern KMUTEX global_Synchronism;	// ȫ�ֱ���������״̬ͬ������

// ������ں���
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING  pRegistryPath);

// ����ж�غ���
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject);

// ��ǲ����
NTSTATUS CreateDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS CloseDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS ReadDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS WriteDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DeviceIoControlDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

// ������ǲ������ֱ�ӽ�IRP������һ��
NTSTATUS GeneralDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, CreateDispatch)
#pragma alloc_text(PAGE, CloseDispatch)
#pragma alloc_text(PAGE, ReadDispatch)
#pragma alloc_text(PAGE, WriteDispatch)
#pragma alloc_text(PAGE, DeviceIoControlDispatch)
#pragma alloc_text(PAGE, GeneralDispatch)

#ifdef __cplusplus
};
#endif // __cplusplus

#endif	// _DRVICE_STRUCT_H_