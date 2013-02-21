
#include "DriverStruct.h"

// ȫ�ֹ���״̬
BOOL global_WorkState = FALSE;

// ȫ��ͬ������ʹ�û�����
KMUTEX global_Synchronism;

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��ʼ���豸�������������
	UNICODE_STRING deviceName;
	UNICODE_STRING symboLinkName;
	RtlInitUnicodeString(&deviceName, DEVICE_NAME_W);
	RtlInitUnicodeString(&symboLinkName, SYMBO_LINK_NAME_W);

	// ������ǲ����
	for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		pDriverObject->MajorFunction[i] = GeneralDispatch;

	pDriverObject->MajorFunction[IRP_MJ_CREATE]			= CreateDispatch;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]			= CloseDispatch;
	pDriverObject->MajorFunction[IRP_MJ_READ]			= ReadDispatch;
	pDriverObject->MajorFunction[IRP_MJ_WRITE]			= WriteDispatch;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoControlDispatch;

	// ����ж�غ���
	pDriverObject->DriverUnload = DriverUnload;

	// �����豸
	PDEVICE_OBJECT pDeviceObject = NULL;
	status = IoCreateDevice(pDriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObject);
	if (!NT_SUCCESS(status))
		return status;

	if (!pDeviceObject)
		return STATUS_UNEXPECTED_IO_ERROR;

	// ��������
	pDeviceObject->Flags |= DO_DIRECT_IO;
	pDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;
	status = IoCreateSymbolicLink(&symboLinkName, &deviceName);
	pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	// ��ʼ������
	InitializeProcessList();

	// �������ص㣡
	// Ҫ��ʼ����SSDT HOOK��
	BackupSysServicesTable();

	// ��װHOOK
	InstallSysServiceHook((ULONG)ZwQuerySystemInformation, (ULONG)HookNtQuerySystemInformation);
	InstallSysServiceHook((ULONG)ZwTerminateProcess, (ULONG)HookNtTerminateProcess);

	// ��ʼ��ͬ����������ʹ�õ��ǻ�����
	KeInitializeMutex(&global_Synchronism, 0);
	InitializeSynchronObjects();

	return status;
}

VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	// ɾ�������������豸����
	UNICODE_STRING symboLinkName;
	RtlInitUnicodeString(&symboLinkName, SYMBO_LINK_NAME_W);

	IoDeleteSymbolicLink(&symboLinkName);
	IoDeleteDevice(pDriverObject->DeviceObject);

	// �������ص�
	// ���SSDT HOOK
	UnInstallSysServiceHook((ULONG)ZwQuerySystemInformation);
	UnInstallSysServiceHook((ULONG)ZwTerminateProcess);

	// ��������
	DistroyProcessList();
}

NTSTATUS GeneralDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PAGED_CODE();

	// �����ɷ�������ֱ�ӽ�IRP������һ��
	pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return pIrp->IoStatus.Status;
}

NTSTATUS CreateDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PAGED_CODE();

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS CloseDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PAGED_CODE();

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS ReadDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PAGED_CODE();

	NTSTATUS status = STATUS_NOT_SUPPORTED;

	return status;
}

NTSTATUS WriteDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PAGED_CODE();

	NTSTATUS status = STATUS_NOT_SUPPORTED;

	return status;
}

NTSTATUS DeviceIoControlDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PAGED_CODE();

	NTSTATUS status = STATUS_SUCCESS;

	PIO_STACK_LOCATION pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);

	ULONG inputBufferLength = pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outputBufferLength = pIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG controlCode = pIoStackLocation->Parameters.DeviceIoControl.IoControlCode;

	// ʹ�û�������ʽ��Ӧ�ó������ͨ��
	PCHAR pInputBuffer = (PCHAR)pIrp->AssociatedIrp.SystemBuffer;

	if (inputBufferLength < 4)
	{
		status = STATUS_INVALID_PARAMETER;
		goto FINISH_DEVICE_IO_CONTROL_DISPATCH;
	}

	// ��ȡ����Id
	ULONG processId = atol(pInputBuffer);

	switch (controlCode)
	{
	case IO_START_WORKING:
		{
			// ���¹���״̬��־��ע��ͬ������
			LARGE_INTEGER li;
			li.QuadPart = 100*1000*1000*1000*1;
			status = KeWaitForSingleObject(&global_Synchronism, Executive, KernelMode, FALSE, &li);
			global_WorkState = TRUE;
			KeReleaseMutex(&global_Synchronism, FALSE);
		}
		break;
	case IO_STOP_WORKING:
		{
			// ���¹���״̬��־��ע��ͬ������
			LARGE_INTEGER li;
			li.QuadPart = 100*1000*1000*1000*1;
			status = KeWaitForSingleObject(&global_Synchronism, Executive, KernelMode, FALSE, &li);
			global_WorkState = FALSE;
			KeReleaseMutex(&global_Synchronism, FALSE);
		}
		break;
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
