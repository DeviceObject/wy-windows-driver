
#include "wykbflt.h"

#pragma INITCODE
#ifdef __cplusplus
extern "C"
#endif
NTSTATUS DriverEntry(IN PDRIVER_OBJECT aDriverObject, IN PUNICODE_STRING aRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("wykbflt.sys : enter DriverEntry\n"));

	// ��ʼ��������ǲ����
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		aDriverObject->MajorFunction[i] = KBFDefaultIRPDispatchRoutine;
	
	// ����Read��ǲ������ͨ�����˴�IRP��ȡ��������
	aDriverObject->MajorFunction[IRP_MJ_READ] = KBFReadDispatchRoutine;

	aDriverObject->MajorFunction[IRP_MJ_POWER] = KBFPowerDispatchRoutine;
	aDriverObject->MajorFunction[IRP_MJ_PNP] = KBFPnpDispatchRoutine;

	// ж�غ���
	aDriverObject->DriverUnload = KBFDriverUnload;
	gDriverObject = aDriverObject;

	// ע�⣺�����ǹؼ�
	// �����м����豸
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
		// ����һ�������豸
		status = IoCreateDevice(aDriverObject, sizeof(DEVICE_EXTENSION), NULL,
			pTargetDeviceObject->DeviceType, pTargetDeviceObject->Characteristics,
			FALSE, OUT &pFilterDeviceObject);

		if (!NT_SUCCESS(status))
		{
			KdPrint(("wykbflt.sys : KBFAttachDevices Couldn't create the Filter Device Object\n"));
			return status;
		}

		// �󶨡�pLowerDeviceObject�ǵײ������豸
		pLowerDeviceObject = IoAttachDeviceToDeviceStack(pFilterDeviceObject, pTargetDeviceObject);

		if (!pLowerDeviceObject)
		{
			KdPrint((""));
		}
	}
}