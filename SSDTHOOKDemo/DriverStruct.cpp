
#include "DriverStruct.h"



NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNICODE_STRING deviceName;
	UNICODE_STRING symboLinkName;
	RtlInitUnicodeString(&deviceName, DEVICE_NAME_W);
	RtlInitUnicodeString(&symboLinkName, SYMBO_LINK_NAME_W);

	// 设置派遣函数
	for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = GeneralDispatch;
	}

	pDriverObject->MajorFunction[IRP_MJ_CREATE]			= CreateDispatch;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]			= CloseDispatch;
	pDriverObject->MajorFunction[IRP_MJ_READ]			= ReadDispatch;
	pDriverObject->MajorFunction[IRP_MJ_WRITE]			= WriteDispatch;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoControlDispatch;

	// 设置卸载函数
	pDriverObject->DriverUnload = DriverUnload;

	// 创建设备
	PDEVICE_OBJECT pDeviceObject = NULL;
	status = IoCreateDevice(pDriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObject);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (!pDeviceObject)
	{
		return STATUS_UNEXPECTED_IO_ERROR;
	}

	// 设置其他
	pDeviceObject->Flags |= DO_DIRECT_IO;
	pDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;
	status = IoCreateSymbolicLink(&symboLinkName, &deviceName);

	pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	// 这里是重点！
	// 要开始进行SSDT HOOK了
	BackupSysServicesTable();

	// 安装HOOK
	InstallSysServiceHook((ULONG)ZwQuerySystemInformation, (ULONG)HookNtQuerySystemInformation);
	InstallSysServiceHook((ULONG)ZwTerminateProcess, (ULONG)HookNtTerminateProcess);

	return status;
}

VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING symboLinkName;
	
	RtlInitUnicodeString(&symboLinkName, SYMBO_LINK_NAME_W);
	IoDeleteSymbolicLink(&symboLinkName);
	IoDeleteDevice(pDriverObject->DeviceObject);

	// 这里是重点
	// 解除SSDT HOOK
	UnInstallSysServiceHook((ULONG)ZwQuerySystemInformation);
	UnInstallSysServiceHook((ULONG)ZwTerminateProcess);
}

NTSTATUS GeneralDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return pIrp->IoStatus.Status;
}

NTSTATUS CreateDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS CloseDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS ReadDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	return status;
}

NTSTATUS WriteDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	return status;
}

NTSTATUS DeviceIoControlDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;

	PIO_STACK_LOCATION pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);

	ULONG inputBufferLength = pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outputBufferLength = pIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG controlCode = pIoStackLocation->Parameters.DeviceIoControl.IoControlCode;

	// 使用缓冲区方式与应用程序进行通信
	PCHAR pInputBuffer = (PCHAR)pIrp->AssociatedIrp.SystemBuffer;

	if (inputBufferLength < 4)
	{
		status = STATUS_INVALID_PARAMETER;
		goto FINISH_DEVICE_IO_CONTROL_DISPATCH;
	}

	ULONG processId = atol(pInputBuffer);

	switch (controlCode)
	{
	case IO_INSERT_PROTECT_PROCESS:
		{
			if (InsertProcessInProtectList(processId) == 0)
				status = STATUS_PROCESS_IS_TERMINATING;
		}
		break;
	case IO_REMOVE_PROTECT_PROCESS:
		{
			if (RemoveProcessFromProtectList(processId) == 0)
				status = STATUS_PROCESS_IS_TERMINATING;
		}
		break;
	case IO_INSERT_HIDE_PROCESS:
		{
			if (InsertProcessInHideList(processId) == 0)
				status = STATUS_PROCESS_IS_TERMINATING;
		}
		break;
	case IO_REMOVE_HIDE_PROCESS:
		{
			if (RemoveProcessFromHideList(processId) == 0)
				status = STATUS_PROCESS_IS_TERMINATING;
		}
		break;
	default:
		{
			status = STATUS_INVALID_VARIANT;
		}
		break;
	}

FINISH_DEVICE_IO_CONTROL_DISPATCH:
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}
