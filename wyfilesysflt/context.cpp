
#include "public.h"
#include "context.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FSFContextCleanup)
#endif

VOID FSFContextCleanup(__in PFLT_CONTEXT Context, __in FLT_CONTEXT_TYPE ContextType)
{
	// ����������Ľṹָ��
	PWY_INSTANCE_CONTEXT instanceContext = NULL;
	PWY_FILE_CONTEXT fileContext = NULL;
	PWY_STREAM_CONTEXT streamContext = NULL;
	PWY_STREAM_HANDLE_CONTEXT streamHandleContext = NULL;

	PAGED_CODE();

	switch(ContextType)
	{
	case FLT_INSTANCE_CONTEXT:
		{
			// �ͷ�ʵ��������
			instanceContext = (PWY_INSTANCE_CONTEXT)Context;

			DbgPrint("wyFileSysFlt.sys : FSFContextCleanup -> Clean up instance context for valume %wZ (Context = %p) .\n", 
				&instanceContext->iVolumeName, Context);

			// �������������Ҫ�ͷ��ڴ���������������ͬ������
			// ʵ������Լ������ͷš���ʵ����������ü���Ϊ0ʱ�����˹��������ͷ���

			// �ͷŵ��������е��ַ���
			CtxFreeUnicodeString(&instanceContext->iVolumeName);

			DbgPrint("wyFileSysFlt.sys : FSFContextCleanup -> Clean up instance context complete .\n");

			break;
		}
	case FLT_FILE_CONTEXT:
		{
			// �ͷ��ļ�������
			fileContext = (PWY_FILE_CONTEXT)Context;

			DbgPrint("wyFileSysFlt.sys : FSFContextCleanup -> Clean up file context for file %wZ (FileContext = %p) .\n", &fileContext->iFileName, Context);

			// �ͷ��ļ���
			CtxFreeUnicodeString(&fileContext->iFileName);

			DbgPrint("wyFileSysFlt.sys : FSFContextCleanup -> Clean up file context complete .\n");

			break;
		}
	case FLT_STREAM_CONTEXT:
		{
			// �ͷ���������
			streamContext = (PWY_STREAM_CONTEXT)Context;

			DbgPrint("wyFileSysFlt.sys : FSFContextCleanup -> Clean up stream context for file %wZ (StreamContext = %p) .\niCreateCount=%x \niCleanupCount=%x \niCloseCount=%x \n",
				&streamContext->iFileName, Context, streamContext->iCreateCount, streamContext->iCleanupCount, streamContext->iCloseCount);

			// �ͷ���Դ�͸���Դ������ڴ�
			if (streamContext->iLockResource != NULL)
			{
				// ��ϵͳ��Դ�б���ɾ��ָ������Դ
				ExDeleteResourceLite(streamContext->iLockResource);

				// �ͷ���Դ��ռ�õ��ڴ�
				CtxFreeResource(streamContext->iLockResource);
			}

			// �ͷ��������Ĺ������ļ���
			if (streamContext->iFileName.Buffer != NULL)
				CtxFreeUnicodeString(&streamContext->iFileName);

			DbgPrint("wyFileSysFlt.sys : FSFContextCleanup -> Clean up stream context complete .\n");

			break;
		}
	case FLT_STREAMHANDLE_CONTEXT:
		{
			// �ͷ������������
			streamHandleContext = (PWY_STREAM_HANDLE_CONTEXT)Context;

			DbgPrint("wyFileSysFlt.sys : FSFContextCleanup -> Clean up stream handle context for file %wZ (StreamContext = %p) .\n", &streamContext->iFileName, Context);

			// �ͷ���Դ�͸���Դ������ڴ�
			if (streamHandleContext->iLockResource != NULL)
			{
				ExDeleteResourceLite(streamHandleContext->iLockResource);
				CtxFreeResource(streamHandleContext->iLockResource);
			}

			if (streamHandleContext->iFileName.Buffer != NULL)
				CtxFreeUnicodeString(&streamHandleContext->iFileName);

			DbgPrint("wyFileSysFlt.sys : FSFContextCleanup -> Clean up stream handle context complete .\n");

			break;
		}
	}
}

NTSTATUS FSFFindOrCreateFileContext(__in PFLT_CALLBACK_DATA Cbd, __in BOOLEAN CreateIfNotFound, __in_opt PUNICODE_STRING FileName, __deref_out PWY_FILE_CONTEXT *FileContext, __out_opt PBOOLEAN ContextCreated)
{
	NTSTATUS status = STATUS_SUCCESS;
	PWY_FILE_CONTEXT fileContext = NULL;
	PWY_FILE_CONTEXT oldFileContext = NULL;

	PAGED_CODE();

	*FileContext = NULL;
	if (ContextCreated != NULL)
		*ContextCreated = FALSE;

	// ��һ�������Ի�ȡ�ļ�������
	DbgPrint("wyFileSysFlt.sys : FSFFindOrCreateFileContext -> Trying to get file context (FileObject = %p, Instance = %p)\n",
		Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

	status = FltGetFileContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, (PFLT_CONTEXT *)&fileContext);
	if (!NT_SUCCESS(status) && (status == STATUS_NOT_FOUND) && CreateIfNotFound)
	{
		// ��ȡʧ�ܡ�û�ҵ�����Ҫ�½������½�֮
		DbgPrint("wyFileSysFlt.sys : FSFFindOrCreateFileContext -> Creating file context (FileObject = %p, Instance = %p)\n",
			Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

		// �����ļ�������
		status = FSFCreateFileContext(FileName, &fileContext);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("wyFileSysFlt.sys : FSFFindOrCreateFileContext -> Failed to create file context with status 0x%x. (FileObject = %p, Instance = %p)\n",
				status, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);
			return status;
		}

		// ���µ��ļ����������õ��ļ�������
		DbgPrint("wyFileSysFlt.sys : FSFFindOrCreateFileContext -> Setting file context %p (FileObject = %p, Instance = %p)\n",
			fileContext, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

		status = FltSetFileContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, FLT_SET_CONTEXT_KEEP_IF_EXISTS, fileContext, (PFLT_CONTEXT *)&oldFileContext);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("wyFileSysFlt.sys : FSFFindOrCreateFileContext -> Failed to set file context with status 0x%x. (FileObject = %p, Instance = %p)\n",
				status, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

			// FltSetFileContext����ʧ�ܣ�������Ҫ�ͷ�������
			// ���FltSetFileContext���óɹ����������Ļ᷵�ظ������ߣ�������ʹ����Ϻ��ɵ������ͷŴ�������

			DbgPrint("wyFileSysFlt.sys : FSFFindOrCreateFileContext -> Releasing file context %p (FileObject = %p, Instance = %p)\n",
				fileContext, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

			FltReleaseContext(fileContext);

			if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
			{
				// ���FltSetFileContextʧ�ܵĴ������������Ѿ������壬��ô��ǰ����û���κ������ģ����Ǹ������߷���ʧ��
				DbgPrint("wyFileSysFlt.sys : FSFFindOrCreateFileContext -> Failed to set file context with status 0x%x != STATUS_FLT_CONTEXT_ALREADY_DEFINED. (FileObject = %p, Instance = %p)\n",
					status, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);
				return status;
			}

			// �������������ﲻ�Ǻ�����...
			DbgPrint("wyFileSysFlt.sys : FSFFindOrCreateFileContext -> File context already defined. Retaining old file context %p (FileObject = %p, Instance = %p)\n",
				oldFileContext, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

			fileContext = oldFileContext;
			status = STATUS_SUCCESS;
		}
		else
		{
			if (ContextCreated != NULL)
				*ContextCreated = TRUE;
		}
	}

	*FileContext = fileContext;
	return status;
}

NTSTATUS FSFCreateFileContext(__in PUNICODE_STRING FileName, __deref_out PWY_FILE_CONTEXT *FileContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	PWY_FILE_CONTEXT fileContext = NULL;

	PAGED_CODE();

	// ����һ���ļ������ģ�Vista֮��ϵͳ��Ч��
	DbgPrint("wyFileSysFlt.sys : FSFCreateFileContext ->  Allocating file context \n");

	status = FltAllocateContext(globalData.iFilter, FLT_FILE_CONTEXT, WY_FILE_CONTEXT_SIZE, PagedPool, (PFLT_CONTEXT *)&fileContext);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : FSFCreateFileContext -> Failed to allocate file context with status 0x%x \n", status);
		return status;
	}

	// ��ʼ���·����������
	fileContext->iFileName.MaximumLength = FileName->Length;
	status = CtxAllocateUnicodeString(&fileContext->iFileName);
	if (NT_SUCCESS(status))
		RtlCopyUnicodeString(&fileContext->iFileName, FileName);

	*FileName = fileContext->iFileName;

	return STATUS_SUCCESS;
}

NTSTATUS FSFCreateOrFindStreamContext(__in PFLT_CALLBACK_DATA Cbd, __in BOOLEAN CreateIfNotFound, __deref_out PWY_STREAM_CONTEXT *StreamContext, __out_opt PBOOLEAN ContextCreated)
{
	NTSTATUS status = STATUS_SUCCESS;
	PWY_STREAM_CONTEXT streamContext = NULL;
	PWY_STREAM_CONTEXT oldStreamContext = NULL;

	PAGED_CODE();

	*StreamContext = NULL;
	if (ContextCreated != NULL)
		*ContextCreated = FALSE;

	DbgPrint("wyFileSysFlt.sys : FSFCreateOrFindStreamContext -> Trying to get stream context (FileObject = %p, Instance = %p)\n",
		Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

	status = FltGetStreamContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, (PFLT_CONTEXT *)&streamContext);
	if (!NT_SUCCESS(status) && (status == STATUS_NOT_FOUND) && CreateIfNotFound)
	{
		DbgPrint("wyFileSysFlt.sys : FSFCreateOrFindStreamContext -> Creating stream context (FileObject = %p, Instance = %p)\n",
			Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

		// ������������
		status = FSFCreateStreamContext(&streamContext);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("wyFileSysFlt.sys : FSFCreateOrFindStreamContext -> Failed to create stream context with status 0x%x. (FileObject = %p, Instance = %p)\n",
				status, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

			return status;
		}

		DbgPrint("wyFileSysFlt.sys : FSFCreateOrFindStreamContext -> Setting stream context %p (FileObject = %p, Instance = %p)\n",
			streamContext, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

		status = FltSetStreamContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, FLT_SET_CONTEXT_KEEP_IF_EXISTS, streamContext, (PFLT_CONTEXT *)&oldStreamContext);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("wyFileSysFlt.sys : FSFCreateOrFindStreamContext -> Failed to set stream context with status 0x%x. (FileObject = %p, Instance = %p)\n",
				status, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

			DbgPrint("wyFileSysFlt.sys : FSFCreateOrFindStreamContext -> Releasing stream context %p (FileObject = %p, Instance = %p)\n",
				streamContext, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

			FltReleaseContext(streamContext);

			if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
			{
				DbgPrint("wyFileSysFlt.sys : FSFCreateOrFindStreamContext -> Failed to set stream context with status 0x%x != STATUS_FLT_CONTEXT_ALREADY_DEFINED. (FileObject = %p, Instance = %p)\n",
					status, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

				return status;
			}

			DbgPrint("wyFileSysFlt.sys : FSFCreateOrFindStreamContext -> Stream context already defined. Retaining old stream context %p (FileObject = %p, Instance = %p)\n",
				oldStreamContext, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

			streamContext = oldStreamContext;
			status = STATUS_SUCCESS;
		}
		else
		{
			if (ContextCreated != NULL)
				*ContextCreated = TRUE;
		}
	}

	*StreamContext = streamContext;

	return status;
}

NTSTATUS FSFCreateStreamContext(__deref_out PWY_STREAM_CONTEXT *StreamContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	PWY_STREAM_CONTEXT streamContext = NULL;

	PAGED_CODE();

	// ����һ����������
	DbgPrint("wyFileSysFlt.sys : FSFCreateStreamContext -> Allocating stream context \n");

	status = FltAllocateContext(globalData.iFilter, FLT_STREAM_CONTEXT, WY_STREAM_CONTEXT_SIZE, PagedPool, (PFLT_CONTEXT *)&streamContext);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : FSFCreateStreamContext -> Failed to allocate stream context with status 0x%x \n", status);
		return status;
	}

	// ��ʼ���´�����������
	RtlZeroMemory(streamContext, WY_STREAM_CONTEXT_SIZE);

	streamContext->iLockResource = CtxAllocateResource();
	if (streamContext->iLockResource == NULL)
	{
		FltReleaseContext(streamContext);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	ExInitializeResource(streamContext->iLockResource);

	*StreamContext = streamContext;

	return STATUS_SUCCESS;
}

NTSTATUS FSFUpdateNameInStreamContext(__in PUNICODE_STRING DirectoryName, __inout PWY_STREAM_CONTEXT StreamContext)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();

	// �ͷŵ��κ��Ѿ����ڵ�����
	if (StreamContext->iFileName.Buffer != NULL)
		CtxFreeUnicodeString(&StreamContext->iFileName);

	// ����͸���Ŀ¼��
	StreamContext->iFileName.MaximumLength = DirectoryName->Length;
	status = CtxAllocateUnicodeString(&StreamContext->iFileName);
	if (NT_SUCCESS(status))
		RtlCopyUnicodeString(&StreamContext->iFileName, DirectoryName);

	return status;
}

NTSTATUS FSFCreateOrReplaceStreamHandleContext(__in PFLT_CALLBACK_DATA Cbd, __in BOOLEAN ReplaceIfExists, __deref_out PWY_STREAM_HANDLE_CONTEXT *StreamHandleContext, __out_opt PBOOLEAN ContextReplaced)
{
	NTSTATUS status = STATUS_SUCCESS;
	PWY_STREAM_HANDLE_CONTEXT streamHandleContext = NULL;
	PWY_STREAM_HANDLE_CONTEXT oldStreamHandleContext = NULL;

	PAGED_CODE();

	*StreamHandleContext = NULL;
	if (ContextReplaced != NULL)
		*ContextReplaced = FALSE;

	// ����һ�������������
	DbgPrint("wyFileSysFlt.sys : FSFCreateOrReplaceStreamHandleContext -> Creating stream handle context (FileObject = %p, Instance = %p)\n",
		Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

	status = FSFCreateStreamHandleContext(&streamHandleContext);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : FSFCreateOrReplaceStreamHandleContext -> Failed to create stream context with status 0x%x. (FileObject = %p, Instance = %p)\n",
			status, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);
		return status;
	}

	// ����������
	DbgPrint("wyFileSysFlt.sys : FSFCreateOrReplaceStreamHandleContext -> Setting stream context %p (FileObject = %p, Instance = %p, ReplaceIfExists = %x)\n",
		streamHandleContext, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance, ReplaceIfExists);

	status = FltSetStreamHandleContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, 
			ReplaceIfExists ? FLT_SET_CONTEXT_REPLACE_IF_EXISTS : FLT_SET_CONTEXT_KEEP_IF_EXISTS, 
			streamHandleContext, (PFLT_CONTEXT *)&oldStreamHandleContext);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : FSFCreateOrReplaceStreamHandleContext -> Failed to set stream handle context with status 0x%x. (FileObject = %p, Instance = %p)\n",
			status, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

		DbgPrint("wyFileSysFlt.sys : FSFCreateOrReplaceStreamHandleContext -> Releasing stream handle context %p (FileObject = %p, Instance = %p)\n",
			streamHandleContext, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

		FltReleaseContext(streamHandleContext);

		if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
		{
			DbgPrint("wyFileSysFlt.sys : FSFCreateOrReplaceStreamHandleContext -> Failed to set stream context with status 0x%x != STATUS_FLT_CONTEXT_ALREADY_DEFINED. (FileObject = %p, Instance = %p)\n",
				status, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);
			return status;
		}

		ASSERT(ReplaceIfExists == FALSE);

		DbgPrint("wyFileSysFlt.sys : FSFCreateOrReplaceStreamHandleContext -> Stream context already defined. Retaining old stream context %p (FileObject = %p, Instance = %p)\n",
			oldStreamHandleContext, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

		streamHandleContext = oldStreamHandleContext;
		status = STATUS_SUCCESS;
	}
	else
	{
		if (ReplaceIfExists && oldStreamHandleContext != NULL)
		{
			DbgPrint("wyFileSysFlt.sys : FSFCreateOrReplaceStreamHandleContext -> Releasing old stream handle context %p (FileObject = %p, Instance = %p)\n",
				oldStreamHandleContext, Cbd->Iopb->TargetFileObject, Cbd->Iopb->TargetInstance);

			FltReleaseContext(oldStreamHandleContext);

			if (ContextReplaced != NULL)
				*ContextReplaced = TRUE;
		}
	}

	*StreamHandleContext = streamHandleContext;

	return status;
}

NTSTATUS FSFCreateStreamHandleContext(__deref_out PWY_STREAM_HANDLE_CONTEXT *StreamHandleContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	PWY_STREAM_HANDLE_CONTEXT streamHandleContext = NULL;

	PAGED_CODE();

	DbgPrint("wyFileSysFlt.sys : FSFCreateStreamHandleContext -> Allocating stream handle context \n");

	// ������������ķ����ڴ�ռ�
	status = FltAllocateContext(globalData.iFilter, FLT_STREAMHANDLE_CONTEXT, WY_STREAM_HANDLE_CONTEXT_SIZE, PagedPool, (PFLT_CONTEXT *)&streamHandleContext);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : FSFCreateStreamHandleContext -> Failed to allocate stream handle context with status 0x%x \n", status);
		return status;
	}

	// ��ʼ�������������
	RtlZeroMemory(streamHandleContext, WY_STREAM_HANDLE_CONTEXT_SIZE);

	streamHandleContext->iLockResource = CtxAllocateResource();
	if (streamHandleContext->iLockResource == NULL)
	{
		FltReleaseContext(streamHandleContext);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	ExInitializeResourceLite(streamHandleContext->iLockResource);

	*StreamHandleContext = streamHandleContext;

	return STATUS_SUCCESS;
}

NTSTATUS FSFUpdateNameInStreamHandleContxt(__in PUNICODE_STRING DirectoryName, __inout PWY_STREAM_HANDLE_CONTEXT StreamHandleContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	
	PAGED_CODE();

	if (StreamHandleContext->iFileName.Buffer != NULL)
		CtxFreeUnicodeString(&StreamHandleContext->iFileName);

	StreamHandleContext->iFileName.MaximumLength = DirectoryName->Length;
	status = CtxAllocateUnicodeString(&StreamHandleContext->iFileName);
	if (NT_SUCCESS(status))
		RtlCopyUnicodeString(&StreamHandleContext->iFileName, DirectoryName);

	return status;
}

NTSTATUS CtxAllocateUnicodeString(__inout PUNICODE_STRING String)
{
	PAGED_CODE();

	String->Buffer = (PWCH)ExAllocatePoolWithTag( PagedPool, String->MaximumLength, WY_STRING_TAG);

	if (String->Buffer == NULL) 
	{
		DbgPrint("wyFileSysFlt.sys : CtxAllocateUnicodeString -> Failed to allocate unicode string of size 0x%x\n",
			String->MaximumLength);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	String->Length = 0;

	return STATUS_SUCCESS;
}

NTSTATUS CtxFreeUnicodeString(__inout PUNICODE_STRING String)
{
	PAGED_CODE();

	ExFreePoolWithTag( String->Buffer, WY_STRING_TAG );

	String->Length = String->MaximumLength = 0;
	String->Buffer = NULL;

	return STATUS_SUCCESS;
}