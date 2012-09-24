
#include "public.h"
#include "wyFileSysFlt.h"
#include "context.h"

// ��һ���Ҳ�֪���Ǹ����õģ�scanner�����������У�������ֻ���հ����
#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

// 
// ȫ�ֱ���
GLOBAL_DATA globalData;

// 
// һЩ��������
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)	// ����ʱ������page_fault_in_nonpaged_area�������������Ϊ��ҳ�ڴ�����
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
		FSFPreCreate,
		FSFPostCreate
	},

	//{
	//	IRP_MJ_READ,
	//	FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
	//	FSFPreRead,
	//	FSFPostRead
	//},

	//{
	//	IRP_MJ_WRITE,
	//	FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
	//	FSFPreWrite,
	//	FSFPostWrite
	//},

	{
		IRP_MJ_CLEANUP,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
		FSFPreCleanup,
		NULL//FSFPostCleanup
	},

	{ IRP_MJ_OPERATION_END }
};

// 
// ������ע�ᶨ��
const FLT_CONTEXT_REGISTRATION g_ContextRegistration[] = {
	{
		FLT_INSTANCE_CONTEXT,
		0,
		FSFContextCleanup,
		WY_INSTANCE_CONTEXT_SIZE,
		WY_INSTANCE_CONTEXT_TAG
	},

	{
		FLT_FILE_CONTEXT,
		0,
		FSFContextCleanup,
		WY_FILE_CONTEXT_SIZE,
		WY_FILE_CONTEXT_TAG
	},

	{
		FLT_STREAM_CONTEXT,
		0,
		FSFContextCleanup,
		WY_STREAM_CONTEXT_SIZE,
		WY_STREAM_CONTEXT_TAG,
	},

	{
		FLT_STREAMHANDLE_CONTEXT,
		0,
		FSFContextCleanup,
		WY_STREAM_HANDLE_CONTEXT_SIZE,
		WY_STREAM_HANDLE_CONTEXT_TAG
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

NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��ֹδ���þ���
	UNREFERENCED_PARAMETER(RegistryPath);

	// ��ʼ��ȫ�ֱ���
	RtlZeroMemory(&globalData, sizeof(globalData));

	// �������һ��ע���·��
	DbgPrint("wyFileSysFlt.sys : DriverEntry -> RegistryPath = %wZ .\n", RegistryPath);

	// ע��΢������
	status = FltRegisterFilter(DriverObject, &g_FltRegistration, &globalData.iFilter);
	if (!NT_SUCCESS(status))
	{
		// ע�������ʧ��
		DbgPrint("wyFileSysFlt.sys : DriverEntry -> FltRegisterFilter() Failed 0x%08X.\n", status);
		return status;
	}

	// ��ʼ����IO
	status = FltStartFiltering(globalData.iFilter);
	if (!NT_SUCCESS(status))
	{
		// ����IO����ʧ��
		DbgPrint("wyFileSysFlt.sys : DriverEntry -> FltStartFiltering() Failed 0x%08X.\n", status);

		// ��ע�������
		FltUnregisterFilter(globalData.iFilter);
	}

	return status;
}

NTSTATUS DriverUnload(__in FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);

	// ȷ�������߳�������һ�������ҳ���㹻��IRQL����
	PAGED_CODE();

	FltUnregisterFilter(globalData.iFilter);
	globalData.iFilter = NULL;

	return STATUS_SUCCESS;
}

NTSTATUS DriverInstanceSetup(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_SETUP_FLAGS Flags, __in DEVICE_TYPE VolumeDeviceType, __in FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
	// 
	// ����˵����
	// ��һ��������һ���µ�ʵ��������ʱ����������ᱻ���á�
	// ��������ṩ�˻���ȥattach�����
	// 

	PWY_INSTANCE_CONTEXT instanceContext = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG volumeNameLength = 0;

	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	PAGED_CODE();

	DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Instance setup started (Volume = %p, Instance = %p)\n",
		FltObjects->Volume, FltObjects->Instance);

	// 
	// ���������ص������Ĳ���ʼ��

	// ����ʵ�������ģ����õ����������ڴ�ռ䷵�ص�PWY_INSTANCE_CONTEXT��
	DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Allocate instance context (Volume = %p, Instance = %p)\n",
		FltObjects->Volume, FltObjects->Instance);
	status = FltAllocateContext(globalData.iFilter, FLT_INSTANCE_CONTEXT, WY_INSTANCE_CONTEXT_SIZE, NonPagedPool, (PFLT_CONTEXT *)&instanceContext);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Failed allocate instance context (Volume = %p, Instance = %p, Status = 0x%x)\n", 
			FltObjects->Volume, FltObjects->Instance, status);
		goto DriverInstanceSetupCleanup;
	}

	// ��ȡNT�����ĳ���
	status = FltGetVolumeName(FltObjects->Volume, NULL, &volumeNameLength);
	if (!NT_SUCCESS(status) &&  (status != STATUS_BUFFER_TOO_SMALL))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Unexpected failure in FltGetVolumeName. (Volume = %p, Instance = %p, Status = 0x%x)\n",
			FltObjects->Volume, FltObjects->Instance, status);
		goto DriverInstanceSetupCleanup;
	}

	// ����һ���㹻����ַ�������������ž���
	instanceContext->iVolumeName.MaximumLength = (USHORT)volumeNameLength;
	status = CtxAllocateUnicodeString(&instanceContext->iVolumeName);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Failed to allocate volume name string. (Volume = %p, Instance = %p, Status = 0x%x)\n",
			FltObjects->Volume, FltObjects->Instance, status);
		goto DriverInstanceSetupCleanup;
	}

	// ��ȡNT����
	status = FltGetVolumeName(FltObjects->Volume, &instanceContext->iVolumeName, &volumeNameLength);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Unexpected failure in FltGetVolumeName. (Volume = %p, Instance = %p, Status = 0x%x)\n",
			FltObjects->Volume, FltObjects->Instance, status);
		goto DriverInstanceSetupCleanup;
	}

	// ��������Լ�������ʵ�����
	instanceContext->iVolume = FltObjects->Volume;
	instanceContext->iInstance = FltObjects->Instance;

	// ����ʵ��������
	DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Setting instance context %p for volume \"%wZ\" (Volume = %p, Instance = %p)\n",
		instanceContext, &instanceContext->iVolumeName, FltObjects->Volume, FltObjects->Instance);

	status = FltSetInstanceContext(FltObjects->Instance, FLT_SET_CONTEXT_KEEP_IF_EXISTS, instanceContext, NULL);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Setting instance context %p for volume %wZ (Volume = %p, Instance = %p)\n",
			&instanceContext->iVolumeName, FltObjects->Volume, FltObjects->Instance, status);
		goto DriverInstanceSetupCleanup;
	}

DriverInstanceSetupCleanup:
	// ����FltSetInstanceContext�ɹ�������FltAllocateContext�ɹ��ˣ����Ǳ����ͷŵ���������ġ�
	// FltAllocateContext������һ�����ü�����
	// һ���ɹ���FltSetInstanceContext����һ�����ü������������ļ�ϵͳ����������������
	//
	// FltReleaseContext����һ�����ü���
	//
	// ��FltSetInstanceContext�ɹ�֮�󣬵���FltReleaseContext��ί��һ���ļ�ϵͳ�ṹ���ü���Ϊ1���ڲ�����������
	//����仰����ĺܱ��磬����Ȥ����鿴ԭ����⣩
	//
	// ��FltSetInstanceContextʧ���ˣ�����FltReleaseContext��ί��һ���ļ�ϵͳ�ṹ���ü���Ϊ0���ڲ�����������
	// (���ͬ�ϣ�����һЩ���ӱ��Һ����ˣ����ҪŪ�����׵Ļ���鿴ԭ��)
	if (instanceContext != NULL)
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Releasing instance context %p (Volume = %p, Instance = %p)\n",
			instanceContext, FltObjects->Volume, FltObjects->Instance);
		FltReleaseContext(instanceContext);
	}

	if (NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Instance setup complete (Volume = %p, Instance = %p). Filter will attach to the volume.\n",
			FltObjects->Volume,	FltObjects->Instance);
	}
	else
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Instance setup complete (Volume = %p, Instance = %p). Filter will not attach to the volume.\n",
			FltObjects->Volume,	FltObjects->Instance);
	}

	return status;
}

NTSTATUS DriverInstanceQueryTeardown(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
	// 
	// ����˵����
	// ������FltDetachVolume �� FilterDetach�ֶ�ɾ��һ��ʵ����ʱ��˺��������á�
	// ������ǻ���ȥ������ɷ�����ʧ��
	// 
	// ����������ֱ�ӷ���

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	// Do Nothing ...

	return STATUS_SUCCESS;
}

VOID DriverInstanceTeardownStart(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
	//
	// ����˵����
	// ��ʵ����ж��ʼ��ʱ���������������
	// 
	// ����������ֱ�ӷ���

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	// Do Nothing ...

	return ;
}

VOID DriverInstanceTeardownComplete(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
	// 
	// ����˵���� 
	// ��ʵ����ж���֮�����ô˺���

	PWY_INSTANCE_CONTEXT instanceContext = NULL;
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	DbgPrint("wyFileSysFlt.sys : DriverInstanceTeardownComplete -> Instance teardown complete started (Instance = %p)\n",
		FltObjects->Instance);

	DbgPrint("wyFileSysFlt.sys : DriverInstanceTeardownComplete -> Getting instance context (Volume = %p, Instance = %p)\n",
		FltObjects->Volume, FltObjects->Instance);

	status = FltGetInstanceContext(FltObjects->Instance, (PFLT_CONTEXT *)&instanceContext);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceTeardownComplete -> Instance teardown for volume %wZ (Volume = %p, Instance = %p, InstanceContext = %p)\n",
			&instanceContext->iVolumeName, FltObjects->Volume, FltObjects->Instance, instanceContext);

		// 
		// ���������������ִ���κι��������Լ��Ľṹ�ϵ�ʵ���Ĳ�ж��
		// 
		// �����������ͷ��ڴ��ʵ�������Ĺ����ķ����ȥ��ͬ��������������ʵ���������е�cleanup�ص��б�ִ�С�
		DbgPrint("wyFileSysFlt.sys : DriverInstanceTeardownComplete -> Releasing instance context %p for volume %wZ (Volume = %p, Instance = %p)\n",
			instanceContext, &instanceContext->iVolumeName, FltObjects->Volume, FltObjects->Instance);
	}
	else
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceTeardownComplete -> Failed to get instance context (Volume = %p, Instance = %p Status = 0x%x)\n",
			FltObjects->Volume, FltObjects->Instance, status);
	}

	DbgPrint("wyFileSysFlt.sys : DriverInstanceTeardownComplete -> Instance teardown complete ended (Instance = %p)\n", FltObjects->Instance);

	return ;
}

FLT_PREOP_CALLBACK_STATUS FSFPreCreate(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID *CompletionContext)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PAGED_CODE();

	// Do nothing ...

	// ǰ�����ɹ�����Ҫ������ص�
	// ������������ĵ�������ڴ򿪺�������洦��
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS FSFPostCreate(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags)
{
	PWY_FILE_CONTEXT fileContext = NULL;
	PWY_STREAM_CONTEXT streamContext = NULL;
	PWY_STREAM_HANDLE_CONTEXT streamHandleContext = NULL;
	PFLT_FILE_NAME_INFORMATION fileNameInfo = NULL;
	UNICODE_STRING fileName;

	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN fileContextCreated, streamContextCreated, streamHandleReplaced;

	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	// ��ȡ�ļ���
	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED |	FLT_FILE_NAME_QUERY_DEFAULT, &fileNameInfo);
	if (!NT_SUCCESS(status))
	{
		goto FsfPostCreateCleanup;
	}

	// �����ǰ��ִ����ʵ�Ĵ򿪲���ʱʧ������ֱ�ӷ���
	if (!NT_SUCCESS(Data->IoStatus.Status))
	{
		DbgPrint("wyFileSysFlt.sys : FSFPostCreate -> �� \"%wZ\" ʧ�ܣ������룺0x%x\n", &fileNameInfo->Name, Data->IoStatus.Status);
		goto FsfPostCreateCleanup;
	}

	DbgPrint("wyFileSysFlt.sys : FSFPostCreate -> �� \"%wZ\" �ɹ���\n", &fileNameInfo->Name);

	// ���һ��ߴ�����������
	status = FSFCreateOrFindStreamContext(Data, TRUE, &streamContext, &streamContextCreated);
	if (!NT_SUCCESS(status))
	{
		// ���ʧ�ܺ��п�������Ϊ�������֧���������Ķ����Ƿ�����һ�������ĸ�����������������Ѿ���ɾ����
		goto FsfPostCreateCleanup;
	}

	// ��ȡ�����ĵ�дȨ��(��ȡ��������Դ��)
	CtxAcquireResourceExclusive(streamContext->iLockResource);

	// ���Ӵ򿪼���
	++streamContext->iCreateCount;

	// �����������ڵ��ļ���
	status = FSFUpdateNameInStreamContext(&fileNameInfo->Name, streamContext);

	// �ͷ������ĵ�дȨ��(�ͷ���������Դ��)
	CtxReleaseResource(streamContext->iLockResource);

	if (!NT_SUCCESS(status))
	{
		goto FsfPostCreateCleanup;
	}

	// �������滻һ�������������
	status = FSFCreateOrReplaceStreamHandleContext(Data, TRUE, &streamHandleContext, &streamHandleReplaced);
	if (!NT_SUCCESS(status))
	{
		goto FsfPostCreateCleanup;
	}

	// 
	CtxAcquireResourceExclusive(streamHandleContext->iLockResource);

	// �����������е�����
	status = FSFUpdateNameInStreamHandleContxt(&fileNameInfo->Name, streamHandleContext);

	CtxReleaseResource(streamHandleContext->iLockResource);

	if (!NT_SUCCESS(status))
	{
		goto FsfPostCreateCleanup;
	}

	// ��FltParseFileNameInformation֮��fileNameInfo->Name���������������ơ�
	// ����ֻϣ���ļ��������а����ļ�����������������
	fileName.Buffer = fileNameInfo->Name.Buffer;
	fileName.Length = fileNameInfo->Name.Length - fileNameInfo->Stream.Length;
	fileName.MaximumLength = fileName.Length;

	// Vista�����ϰ汾�Ĳ���ϵͳִ�����´���
	status = FSFFindOrCreateFileContext(Data, TRUE, &fileName, &fileContext, &fileContextCreated);
	if (!NT_SUCCESS(status))
	{
		goto FsfPostCreateCleanup;
	}

FsfPostCreateCleanup:

	// �ͷ������Ѿ���õ�����
	if (fileNameInfo != NULL)
		FltReleaseFileNameInformation(fileNameInfo);

	if (fileContext != NULL)
		FltReleaseContext(fileContext);

	if (streamContext != NULL)
		FltReleaseContext(streamContext);

	if (streamHandleContext != NULL)
		FltReleaseContext(streamHandleContext);

	return FLT_POSTOP_FINISHED_PROCESSING;
}

//FLT_PREOP_CALLBACK_STATUS FSFPreRead(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID *CompletionContext)
//{
//}
//
//FLT_POSTOP_CALLBACK_STATUS FSFPostRead(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags)
//{
//}
//
//FLT_PREOP_CALLBACK_STATUS FSFPreWrite(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID *CompletionContext)
//{
//}
//
//FLT_POSTOP_CALLBACK_STATUS FSFPostWrite(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags)
//{
//}

FLT_PREOP_CALLBACK_STATUS FSFPreCleanup(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID *CompletionContext)
{
	PWY_STREAM_CONTEXT streamContext = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN streamContextCreated = FALSE;

	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PAGED_CODE();

	// ��ȡ�������ģ�ע�⣬���ﲻ������
	status = FSFCreateOrFindStreamContext(Data, FALSE, &streamContext, &streamContextCreated);
	if (!NT_SUCCESS(status))
	{
		goto FsfPreCleanup;
	}

	CtxAcquireResourceExclusive(streamContext->iLockResource);

	++streamContext->iCleanupCount;

	CtxReleaseResource(streamContext->iLockResource);

FsfPreCleanup:

	if (streamContext != NULL)
		FltReleaseContext(streamContext);

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

//FLT_POSTOP_CALLBACK_STATUS FSFPostCleanup(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags)
//{
//}