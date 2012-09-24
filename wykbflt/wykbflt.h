
/**
�ļ�˵����
	���������ڹ��˼��̲���
���ߣ�
	����
ʱ�䣺
	2012-09-09
**/

#pragma once

// ��C���򵼳�/����
#ifdef __cplusplus
extern "C"
{
#endif
#include <wdm.h>
#ifdef __cplusplus
}
#endif

// ������һЩ���ֵĶ���
#define INITDATA data_seg("INIT")
#define LOCKEDDATA data_seg()
#define PAGEDDATA data_seg("PAGE")

#define INITCODE code_seg("INIT")
#define LOCKEDCODE code_seg()
#define PAGEDCODE code_seg("PAGE")

// ����һЩ����
#define KBD_DRIVER_NAME L"\\Driver\\kbdclass"

#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)

// ����������
typedef struct _DEVICE_EXTENSION
{
	ULONG iSize;
	PDEVICE_OBJECT iFilterDeviceObject;
	KSPIN_LOCK iIoRequestsSpinLock;
	KEVENT iIoInProcessEvent;
	PDEVICE_OBJECT iTargetDeviceObject;
	PDEVICE_OBJECT iLowerDeviceObject;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// ������صı���
extern POBJECT_TYPE IoDriverObjectType;
ULONG gC2pKeyCount = 0;
PDRIVER_OBJECT gDriverObject = NULL;

// ������һЩ����Ķ���
#ifdef __cplusplus
extern "C"
{
#endif
NTSTATUS DriverEntry(IN PDRIVER_OBJECT aDriverObject, IN PUNICODE_STRING aRegistryPath);

NTSTATUS KBFAddDevice(IN PDRIVER_OBJECT aDriverObject, IN PDEVICE_OBJECT aPhysicalDeviceObject);

VOID KBFDriverUnload(IN PDRIVER_OBJECT aDriverObject);

NTSTATUS KBFInitDeviceExtension(IN PDEVICE_EXTENSION aDeviceExtension,
								IN PDEVICE_OBJECT aFilterDeviceObject,
								IN PDEVICE_OBJECT aTargetDeviceObject,
								IN PDEVICE_OBJECT aLowerDeviceObject);

// Ĭ��IRP��ǲ����
NTSTATUS KBFDefaultIRPDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp);

// IRP��ǲ����
NTSTATUS KBFPowerDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp);
NTSTATUS KBFPnpDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp);
NTSTATUS KBFReadDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp);

// READ��ɺ���
NTSTATUS KBFReadCompleteRoutine(IN PDEVICE_OBJECT aDeviceObjec, IN PIRP aIrp, IN PVOID aContext);

// ���ϵͳû����ȷ��ָ����ӿڣ���ʵ���Ǵ��ڵģ�ֻ��Ҫ�ڴ�����һ��
NTKERNELAPI 
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectName,
								 ULONG Attributes,
								 PACCESS_STATE AccessState,
								 ACCESS_MASK DesiredAccess,
								 POBJECT_TYPE ObjectType,
								 KPROCESSOR_MODE AccessMode,
								 PVOID ParseContext,
								 PVOID *Object);

POBJECT_TYPE IoDeviceObjectType;

// ��������Ҫ���豸
//NTSTATUS KBFAttachDevices(IN PDRIVER_OBJECT aDriverObject, IN PUNICODE_STRING aRegistryPath);

NTSTATUS KBFAttachDevicesEx(IN PDRIVER_OBJECT aDriverObject, IN PUNICODE_STRING aRegistryPath);

#ifdef __cplusplus
}
#endif