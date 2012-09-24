
#include "wycomflt.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <ntddk.h>
#include <ntstrsafe.h>
#ifdef __cplusplus
}
#endif

#define NTSTRSAFE_LIB


// �궨�壬��Ҫ����һЩ��־λ
#ifndef SetFlag
#define SetFlag(_F,_SF)       ((_F) |= (_SF))
#endif
#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif
#define CFT_MAX_COM_ID 32

// �����豸����ʵ�豸
static PDEVICE_OBJECT global_FileDevice[CFT_MAX_COM_ID] = {0};
static PDEVICE_OBJECT global_RealDevice[CFT_MAX_COM_ID] = {0};

// ��һ���˿��豸
PDEVICE_OBJECT CFTOpenCom(ULONG aId, NTSTATUS *aStatus)
{
	UNICODE_STRING nameString;
	static WCHAR name[32] = {0};
	PFILE_OBJECT fileObject = NULL;
	PDEVICE_OBJECT deviceObject = NULL;

	// �����ַ���
	RtlZeroMemory(name, sizeof(WCHAR)*32);
	RtlStringCchPrintfW(name, 32, L"\\Device\\Serial%d", aId);
	RtlInitUnicodeString(&nameString, name);

	// ���豸����
	*aStatus = IoGetDeviceObjectPointer(&nameString, FILE_ALL_ACCESS, &fileObject, &deviceObject);
	if (*aStatus == STATUS_SUCCESS)
		ObDereferenceObject(fileObject);

	return deviceObject;
}

NTSTATUS CFTAttachDevice(PDRIVER_OBJECT aDriverObject, PDEVICE_OBJECT aOldDeviceObject, PDEVICE_OBJECT *aFilterDeviceObject, PDEVICE_OBJECT *aNextDeviceObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT topDeviceObject = NULL;

	// ���ɹ��˲��豸���󲢰�
	status = IoCreateDevice(aDriverObject, 0, NULL, aOldDeviceObject->DeviceType, 0, FALSE, aFilterDeviceObject);

	if (!NT_SUCCESS(status))
		return status;

	// ������Ҫ��־λ
	if (aOldDeviceObject->Flags & DO_BUFFERED_IO)
		(*aFilterDeviceObject)->Flags |= DO_BUFFERED_IO;

	if (aOldDeviceObject->Flags & DO_DIRECT_IO)
		(*aFilterDeviceObject)->Flags |= DO_DIRECT_IO;

	if (aOldDeviceObject->Characteristics & FILE_DEVICE_SECURE_OPEN)
		(*aFilterDeviceObject)->Characteristics |= FILE_DEVICE_SECURE_OPEN;

	(*aFilterDeviceObject)->Flags |= DO_POWER_PAGABLE;

	// �󶨵���һ���豸
	topDeviceObject = IoAttachDeviceToDeviceStack(*aFilterDeviceObject, aOldDeviceObject);
	if (topDeviceObject == NULL)
	{
		// �����ʧ�ܣ�����֮
		IoDeleteDevice(*aFilterDeviceObject);
		*aFilterDeviceObject = NULL;
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	*aNextDeviceObject = topDeviceObject;

	// ���ô��豸������
	(*aFilterDeviceObject)->Flags = (*aFilterDeviceObject)->Flags & ~DO_DEVICE_INITIALIZING;
	return STATUS_SUCCESS;
}

VOID CFPAttachAllComs(PDRIVER_OBJECT aDriverObject)
{
	ULONG i = 0;
	PDEVICE_OBJECT comDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;

	for (i = 0; i < CFT_MAX_COM_ID; ++i)
	{
		// ����豸����������
		comDeviceObject = CFTOpenCom(i, &status);
		if (comDeviceObject == NULL)
			continue;

		// ����󶨣������Ƿ�󶨳ɹ�
		CFTAttachDevice(aDriverObject, comDeviceObject, &global_FileDevice[i], &global_RealDevice[i]);

		// ȡ��Object����
	}
}

VOID CFPUnloadDriver(PDRIVER_OBJECT aDriverObject)
{
	ULONG i = 0;
	LARGE_INTEGER interval;
	//interval.LowPart = 0;
	//interval.HighPart = 0;

	// ���Ƚ����
	for (i = 0; i < CFT_MAX_COM_ID; ++i)
	{
		if (global_RealDevice[i] != NULL)
			IoDetachDevice(global_RealDevice[i]);
	}

	// ˯��5�룬�ȴ�����IRP�������
	interval.QuadPart = (5 * 1000 * DELAY_ONE_MILLISECOND);
	KeDelayExecutionThread(KernelMode, FALSE, &interval);

	// ɾ����Щ�豸
	for (i = 0; i < CFT_MAX_COM_ID; ++i)
	{
		if (global_FileDevice[i] != NULL)
			IoDeleteDevice(global_FileDevice[i]);	// �����������
	}

	return ;
}

NTSTATUS CFTDefaultDispatch(PDEVICE_OBJECT aDeviceObject, PIRP aIrp)
{
	PIO_STACK_LOCATION ioStackLocation = IoGetCurrentIrpStackLocation(aIrp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG i = 0, j = 0;

	// ������Ҫ֪�����͸������ĸ��豸��
	for (i = 0; i < CFT_MAX_COM_ID; ++i)
	{
		if (global_FileDevice[i] == aDeviceObject)
		{
			// ���е�Դ����һ�ɲ�����
			if (ioStackLocation->MajorFunction == IRP_MJ_POWER)
			{
				// ֱ�ӷ���
				PoStartNextPowerIrp(aIrp);
				IoSkipCurrentIrpStackLocation(aIrp);
				return PoCallDriver(global_RealDevice[i], aIrp);
			}

			// ����ֻ����д���󣬻�û������Լ����ȣ���ӡ������
			if (ioStackLocation->MajorFunction == IRP_MJ_WRITE)
			{
				// ��ó���
				ULONG len = ioStackLocation->Parameters.Write.Length;

				// ��û�����
				PUCHAR buf = NULL;
				if (aIrp->MdlAddress != NULL)
					buf = (PUCHAR)MmGetSystemAddressForMdlSafe(aIrp->MdlAddress, NormalPagePriority);
				else
					buf = (PUCHAR)aIrp->UserBuffer;

				if (buf == NULL)
					buf = (PUCHAR)aIrp->AssociatedIrp.SystemBuffer;

				// ��ӡ����
				for (j = 0; j < len; ++j)
					DbgPrint("COM capture : Send-Data : %2x\r\n", buf[j]);
			}
		}

		// ֱ���·�
		IoSkipCurrentIrpStackLocation(aIrp);
		return IoCallDriver(global_RealDevice[i], aIrp);
	}

	// ��������Ͳ��ڱ��󶨵��豸�У�����������ģ�ֱ�ӷ��ز���
	aIrp->IoStatus.Information = 0;
	aIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
	IoCompleteRequest(aIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT aDriverObject, PUNICODE_STRING aRegisterPath)
{
	size_t i = 0;

	// ���зַ�����������ΪĬ�ϵ�
	// ��CFTDefaultDispatch()�У��������ɷ���ͬ��IRP����ʵ���������ݿɷָ���Ӧ�ĺ�������
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		aDriverObject->MajorFunction[i] = CFTDefaultDispatch;

	// ����ж�غ�����֧�ֶ�̬ж��
	aDriverObject->DriverUnload = CFPUnloadDriver;

	// �����д���
	CFPAttachAllComs(aDriverObject);

	// ֱ�ӷ��سɹ�
	return STATUS_SUCCESS;
}