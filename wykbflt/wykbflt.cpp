
#include "wykbflt.h"

#pragma INITCODE
#ifdef __cplusplus
extern "C"
#endif
NTSTATUS DriverEntry(IN PDRIVER_OBJECT aDriverObject, IN PUNICODE_STRING aRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("wykbflt.sys : enter DriverEntry\n"));

	// 初始化所有派遣函数
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		aDriverObject->MajorFunction[i] = KBFDefaultIRPDispatchRoutine;
	
	// 更改Read派遣函数，通过过滤此IRP获取键盘数据
	aDriverObject->MajorFunction[IRP_MJ_READ] = KBFReadDispatchRoutine;

	aDriverObject->MajorFunction[IRP_MJ_POWER] = KBFPowerDispatchRoutine;
	aDriverObject->MajorFunction[IRP_MJ_PNP] = KBFPnpDispatchRoutine;

	// 卸载函数
	aDriverObject->DriverUnload = KBFDriverUnload;
	gDriverObject = aDriverObject;

	// 注意：这里是关键
	// 绑定所有键盘设备
	status = KBFAttachDevices(aDriverObject, aRegistryPath);

	return status;
}

NTSTATUS KBFAttachDevices(IN PDRIVER_OBJECT aDriverObject, IN PUNICODE_STRING aRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint(("wykbflt.sys : KBFAttachDevices\n"));

	UNICODE_STRING deviceClassName;
	RtlInitUnicodeString(&deviceClassName, KBD_DRIVER_NAME);

	PDRIVER_OBJECT KbdDriverObject = NULL; 
	status = ObReferenceObjectByName(&deviceClassName, OBJ_CASE_INSENSITIVE, 
		NULL, 0, IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("wykbflt.sys : KBFAttachDevices Couldn't get the Device Object\n"));
		return status;
	}
	else
	{
		// 
		ObDereferenceObject(aDriverObject);
	}

	PDEVICE_OBJECT pFilterDeviceObject = NULL; 
	PDEVICE_OBJECT pTargetDeviceObject = NULL; 
	PDEVICE_OBJECT pLowerDeviceObject = NULL;

	pTargetDeviceObject = KbdDriverObject->DeviceObject;

	while (pTargetDeviceObject)
	{
		// 生成一个过滤设备
		status = IoCreateDevice(aDriverObject, sizeof(DEVICE_EXTENSION), NULL,
			pTargetDeviceObject->DeviceType, pTargetDeviceObject->Characteristics,
			FALSE, OUT &pFilterDeviceObject);

		if (!NT_SUCCESS(status))
		{
			KdPrint(("wykbflt.sys : KBFAttachDevices Couldn't create the Filter Device Object\n"));
			return status;
		}

		// 绑定。pLowerDeviceObject是底层物理设备
		pLowerDeviceObject = IoAttachDeviceToDeviceStack(pFilterDeviceObject, pTargetDeviceObject);

		if (!pLowerDeviceObject)
		{
			KdPrint((""));
		}
	}
}