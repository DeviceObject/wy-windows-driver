
#include "wyProcMonSys.h"
#include "Communication.h"
#include "Context.h"

// ��һ���Ҳ�֪���Ǹ����õģ�scanner�����������У�������ֻ���հ����
#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

// 
// ȫ�ֱ���
GLOBAL_DATA globalData;

// 
// һЩ��������
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, DriverInstanceSetup)
#pragma alloc_text(PAGE, DriverInstanceQueryTeardown)
#pragma alloc_text(PAGE, DriverInstanceTeardownStart)
#pragma alloc_text(PAGE, DriverInstanceTeardownComplete)
#endif

// 
// ���˻ص���������
FLT_OPERATION_REGISTRATION g_CallbackRoutines[] = {
	{
		IRP_MJ_CREATE,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,	// ����ֶθ���ģ���Ҫ�顶����������
		NULL,
		PMSPostCreate
	},

	{
		IRP_MJ_READ,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,	// ����ֶθ���ģ���Ҫ�顶����������
		NULL,
		PMSPostRead
	},

	{
		IRP_MJ_WRITE,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,	// ����ֶθ���ģ���Ҫ�顶����������
		NULL,
		PMSPostWrite
	},

	{ IRP_MJ_OPERATION_END }
};

// 
// ������ע�ᶨ��
const FLT_CONTEXT_REGISTRATION g_ContextRegistration[] = {

	{
		FLT_STREAM_CONTEXT,
		0,
		//FSFContextCleanup,
		NULL,
		CTX_STREAM_CONTEXT_SIZE,
		CTX_STREAM_CONTEXT_TAG
	},

	{ FLT_CONTEXT_END }
};

//
// ΢������ע��ṹ
FLT_REGISTRATION g_FltRegistration = {
	sizeof(FLT_REGISTRATION),		// ���ṹ��С
	FLT_REGISTRATION_VERSION,		// �������汾
	0,								// ��־λ
	g_ContextRegistration,			// ������ע��ṹ
	g_CallbackRoutines,				// �ص���������
	DriverUnload,					// ж������
	DriverInstanceSetup,			// ʵ����������
	DriverInstanceQueryTeardown,	// ��֪�������õģ���������˼�ǲ�ѯʵ����ж
	DriverInstanceTeardownStart,	// ʵ����ж��ʼ
	DriverInstanceTeardownComplete,	// ʵ����ж���
	NULL,							// ��3��NULLҲ��֪���Ǹ����
	NULL,
	NULL
};

// ͨ�Ŷ˿���
const PWSTR wyProcMonSysPortName = L"\\wyProcMonSysPort";

// 
// ����Ĵ����߼�ʵ��
ULONG GetProcessNameOffset(VOID)
{
	PEPROCESS curproc;
	int i = 0;

	curproc = PsGetCurrentProcess();

	// Scan for 12KB, hopping the KPEB never grows that big!
	for( i = 0; i < 3 * PAGE_SIZE; i++ )
	{
		if( !strncmp( "System", (PCHAR) curproc + i, strlen("System") ))
			return i;
	}

	// Name not found - oh, well
	return 0;
}

NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	// ��ʼ��ȫ�ֱ���
	RtlZeroMemory(&globalData, sizeof(GLOBAL_DATA));

	// ע��΢������
	NTSTATUS status = FltRegisterFilter(DriverObject, &g_FltRegistration, &globalData.iFilter);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("[wyprocmonsys] ��ʼ��������ʧ�ܣ�\n"));
		return status;
	}

	// ����һ��ͨ�Ŷ˿�
	UNICODE_STRING portName;
	RtlInitUnicodeString(&portName, wyProcMonSysPortName);

	PSECURITY_DESCRIPTOR sd = NULL;
	OBJECT_ATTRIBUTES objectAttributs;
	// ����һ����ȫ������
	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (NT_SUCCESS(status))
	{
		InitializeObjectAttributes(&objectAttributs, &portName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, sd);

		status = FltCreateCommunicationPort(globalData.iFilter, &globalData.iDriverPort, &objectAttributs, NULL, PortConnect, PortDisconnect, NULL, 1);

		// �ͷŰ�ȫ������
		FltFreeSecurityDescriptor(sd);

		if (NT_SUCCESS(status))
		{
			// ���������ƫ����
			globalData.iProcessNameOffsetInPEPROCESS = GetProcessNameOffset();

			// û�п�ʼ���ˣ������û�̬������Ϣ
			globalData.iStartFilter = FALSE;

			// ��ʼ����IO
			status = FltStartFiltering(globalData.iFilter);
			if (NT_SUCCESS(status))
				return STATUS_SUCCESS;

			FltCloseCommunicationPort(globalData.iDriverPort);
		}
	}

	FltUnregisterFilter(globalData.iFilter);

	return status;
}

NTSTATUS DriverUnload(__in FLT_FILTER_UNLOAD_FLAGS Flags)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(Flags);

	// �رշ���˿�
	FltCloseCommunicationPort(globalData.iDriverPort);

	// ��ע�������
	FltUnregisterFilter(globalData.iFilter);

	return STATUS_SUCCESS;
}

NTSTATUS DriverInstanceSetup(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_SETUP_FLAGS Flags, __in DEVICE_TYPE VolumeDeviceType, __in FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	PAGED_CODE();

	// Do nothing ...

	return STATUS_SUCCESS;
}

NTSTATUS DriverInstanceQueryTeardown(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	// Do nothing ...

	return STATUS_SUCCESS;
}

VOID DriverInstanceTeardownStart(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	// Do noting ...

	return ;
}

VOID DriverInstanceTeardownComplete(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	// Do noting ...

	return ;
}
