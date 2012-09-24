
#include "public.h"
#include "context.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FSFContextCleanup)
#endif

VOID FSFContextCleanup(__in PFLT_CONTEXT Context, __in FLT_CONTEXT_TYPE ContextType)
{
	// 定义各上下文结构指针
	PWY_INSTANCE_CONTEXT instanceContext = NULL;
	PWY_FILE_CONTEXT fileContext = NULL;
	PWY_STREAM_CONTEXT streamContext = NULL;
	PWY_STREAM_HANDLE_CONTEXT streamHandleContext = NULL;

	PAGED_CODE();

	switch(ContextType)
	{
	case FLT_INSTANCE_CONTEXT:
		{
			// 释放实例上下文
			instanceContext = (PWY_INSTANCE_CONTEXT)Context;

			// 在这里过滤器需要释放内存或上下文内申请的同步对象。
			// 实例句柄自己不会释放。当实例句柄的引用计数为0时，过滤管理器会释放它

			// 释放掉上下文中的字符串
			CtxFreeUnicodeString(&instanceContext->iVolumeName);

			break;
		}
	case FLT_FILE_CONTEXT:
		{
			// 释放文件上下文
			fileContext = (PWY_FILE_CONTEXT)Context;

			// 释放文件名
			CtxFreeUnicodeString(&fileContext->iFileName);

			break;
		}
	case FLT_STREAM_CONTEXT:
		{
			// 释放流上下文
			streamContext = (PWY_STREAM_CONTEXT)Context;

			// 释放资源和给资源分配的内存
			if (streamContext->iLockResource != NULL)
			{
				// 从系统资源列表中删除指定的资源
				ExDeleteResourceLite(streamContext->iLockResource);

				// 释放资源所占用的内存
				CtxFreeResource(streamContext->iLockResource);
			}

			// 释放流上下文关联的文件名
			if (streamContext->iFileName.Buffer != NULL)
				CtxFreeUnicodeString(&streamContext->iFileName);

			break;
		}
	case FLT_STREAMHANDLE_CONTEXT:
		{
			// 释放流句柄上下文
			streamHandleContext = (PWY_STREAM_HANDLE_CONTEXT)Context;

			// 释放资源和给资源分配的内存
			if (streamHandleContext->iLockResource != NULL)
			{
				ExDeleteResourceLite(streamHandleContext->iLockResource);
				CtxFreeResource(streamHandleContext->iLockResource);
			}

			if (streamHandleContext->iFileName.Buffer != NULL)
				CtxFreeUnicodeString(&streamHandleContext->iFileName);

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

	// 第一步，尝试获取文件上下文
	status = FltGetFileContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, (PFLT_CONTEXT *)&fileContext);
	if (!NT_SUCCESS(status) && (status == STATUS_NOT_FOUND) && CreateIfNotFound)
	{
		// 获取失败、没找到、需要新建，则新建之

		// 创建文件上下文
		status = FSFCreateFileContext(FileName, &fileContext);
		if (!NT_SUCCESS(status))
		{
			return status;
		}

		// 将新的文件上下文设置到文件对象上
		status = FltSetFileContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, FLT_SET_CONTEXT_KEEP_IF_EXISTS, fileContext, (PFLT_CONTEXT *)&oldFileContext);
		if (!NT_SUCCESS(status))
		{
			// FltSetFileContext调用失败，我们需要释放上下文
			// 如果FltSetFileContext调用成功，则上下文会返回给调用者，上下文使用完毕后由调用者释放此上下文
			FltReleaseContext(fileContext);

			if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
			{
				// 如果FltSetFileContext失败的错误不是上下文已经被定义，那么当前对象没有任何上下文，我们给调用者返回失败
				return status;
			}

			// 保留旧的文件上下文
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

	status = FltAllocateContext(globalData.iFilter, FLT_FILE_CONTEXT, WY_FILE_CONTEXT_SIZE, PagedPool, (PFLT_CONTEXT *)&fileContext);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	// 初始化新分配的上下文
	fileContext->iFileName.MaximumLength = FileName->Length;
	status = CtxAllocateUnicodeString(&fileContext->iFileName);
	if (NT_SUCCESS(status))
		RtlCopyUnicodeString(&fileContext->iFileName, FileName);

	// 这个地方引起蓝屏，原来的实现这里不是返回的文件上下文，真尼玛弱智了
	*FileContext = fileContext;

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

	status = FltGetStreamContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, (PFLT_CONTEXT *)&streamContext);
	if (!NT_SUCCESS(status) && (status == STATUS_NOT_FOUND) && CreateIfNotFound)
	{
		// 创建流上下文
		status = FSFCreateStreamContext(&streamContext);

		if (!NT_SUCCESS(status))
		{
			return status;
		}

		status = FltSetStreamContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, FLT_SET_CONTEXT_KEEP_IF_EXISTS, streamContext, (PFLT_CONTEXT *)&oldStreamContext);
		if (!NT_SUCCESS(status))
		{
			FltReleaseContext(streamContext);

			if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
			{
				return status;
			}

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

	// 分配一个流上下文
	status = FltAllocateContext(globalData.iFilter, FLT_STREAM_CONTEXT, WY_STREAM_CONTEXT_SIZE, PagedPool, (PFLT_CONTEXT *)&streamContext);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	// 初始化新创建的上下文
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

	// 释放掉任何已经存在的名称
	if (StreamContext->iFileName.Buffer != NULL)
		CtxFreeUnicodeString(&StreamContext->iFileName);

	// 申请和复制目录名
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

	// 创建一个流句柄上下文
	status = FSFCreateStreamHandleContext(&streamHandleContext);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	// 设置上下文
	status = FltSetStreamHandleContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, 
			ReplaceIfExists ? FLT_SET_CONTEXT_REPLACE_IF_EXISTS : FLT_SET_CONTEXT_KEEP_IF_EXISTS, 
			streamHandleContext, (PFLT_CONTEXT *)&oldStreamHandleContext);
	if (!NT_SUCCESS(status))
	{
		FltReleaseContext(streamHandleContext);

		if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
		{
			return status;
		}

		ASSERT(ReplaceIfExists == FALSE);

		streamHandleContext = oldStreamHandleContext;
		status = STATUS_SUCCESS;
	}
	else
	{
		if (ReplaceIfExists && oldStreamHandleContext != NULL)
		{
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

	// 给流句柄上下文分配内存空间
	status = FltAllocateContext(globalData.iFilter, FLT_STREAMHANDLE_CONTEXT, WY_STREAM_HANDLE_CONTEXT_SIZE, PagedPool, (PFLT_CONTEXT *)&streamHandleContext);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	// 初始化流句柄上下文
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