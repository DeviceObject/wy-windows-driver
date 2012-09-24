
/**
文件说明：
	此驱动用于过滤键盘操作
作者：
	王煜
时间：
	2012-09-09
**/

#pragma once

// 以C程序导出/导入
#ifdef __cplusplus
extern "C"
{
#endif
#include <wdm.h>
#ifdef __cplusplus
}
#endif

// 这里是一些文字的定义
#define INITDATA data_seg("INIT")
#define LOCKEDDATA data_seg()
#define PAGEDDATA data_seg("PAGE")

#define INITCODE code_seg("INIT")
#define LOCKEDCODE code_seg()
#define PAGEDCODE code_seg("PAGE")

// 定义一些常量
#define KBD_DRIVER_NAME L"\\Driver\\kbdclass"

#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)

// 定义上下文
typedef struct _DEVICE_EXTENSION
{
	ULONG iSize;
	PDEVICE_OBJECT iFilterDeviceObject;
	KSPIN_LOCK iIoRequestsSpinLock;
	KEVENT iIoInProcessEvent;
	PDEVICE_OBJECT iTargetDeviceObject;
	PDEVICE_OBJECT iLowerDeviceObject;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// 定义相关的变量
extern POBJECT_TYPE IoDriverObjectType;
ULONG gC2pKeyCount = 0;
PDRIVER_OBJECT gDriverObject = NULL;

// 这里是一些程序的定义
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

// 默认IRP派遣函数
NTSTATUS KBFDefaultIRPDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp);

// IRP派遣函数
NTSTATUS KBFPowerDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp);
NTSTATUS KBFPnpDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp);
NTSTATUS KBFReadDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp);

// READ完成函数
NTSTATUS KBFReadCompleteRoutine(IN PDEVICE_OBJECT aDeviceObjec, IN PIRP aIrp, IN PVOID aContext);

// 这个系统没有明确的指出其接口，但实际是存在的，只需要在此声明一下
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

// 绑定我们需要的设备
//NTSTATUS KBFAttachDevices(IN PDRIVER_OBJECT aDriverObject, IN PUNICODE_STRING aRegistryPath);

NTSTATUS KBFAttachDevicesEx(IN PDRIVER_OBJECT aDriverObject, IN PUNICODE_STRING aRegistryPath);

#ifdef __cplusplus
}
#endif