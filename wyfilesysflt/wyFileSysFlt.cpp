
#include "public.h"
#include "wyFileSysFlt.h"
#include "context.h"

// 这一句我不知道是干嘛用的，scanner的例子里面有，我这里只是照搬过来
#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

// 
// 全局变量
GLOBAL_DATA globalData;

// 
// 一些文字声明
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)	// 启动时引起了page_fault_in_nonpaged_area蓝屏，这里调整为分页内存试试
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, DriverInstanceSetup)
#pragma alloc_text(PAGE, DriverInstanceQueryTeardown)
#pragma alloc_text(PAGE, DriverInstanceTeardownStart)
#pragma alloc_text(PAGE, DriverInstanceTeardownComplete)
#endif


// 
// 过滤回调函数定义
FLT_OPERATION_REGISTRATION g_CallbackRoutines[] = {
	{
		IRP_MJ_CREATE,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,	// 这个字段干嘛的，需要查《寒江独钓》
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
// 上下文注册定义
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
// 微过滤器注册结构
FLT_REGISTRATION g_FltRegistration = {
	sizeof(FLT_REGISTRATION),		// 本结构大小
	FLT_REGISTRATION_VERSION,		// 过滤器版本
	0,								// 标志位
	g_ContextRegistration,			// 上下文注册结构
	g_CallbackRoutines,				// 回调函数集合
	DriverUnload,					// 卸载例程
	DriverInstanceSetup,			// 实例加载例程
	DriverInstanceQueryTeardown,	// 不知道干嘛用的，看字面意思是查询实例拆卸
	DriverInstanceTeardownStart,	// 实例拆卸开始
	DriverInstanceTeardownComplete,	// 实例拆卸完成
	NULL,							// 这3个NULL也不知道是干嘛的
	NULL,
	NULL
};

NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	// 防止未引用警告
	UNREFERENCED_PARAMETER(RegistryPath);

	// 初始化全局变量
	RtlZeroMemory(&globalData, sizeof(globalData));

	// 这里输出一下注册表路径
	DbgPrint("wyFileSysFlt.sys : DriverEntry -> RegistryPath = %wZ .\n", RegistryPath);

	// 注册微过滤器
	status = FltRegisterFilter(DriverObject, &g_FltRegistration, &globalData.iFilter);
	if (!NT_SUCCESS(status))
	{
		// 注册过滤器失败
		DbgPrint("wyFileSysFlt.sys : DriverEntry -> FltRegisterFilter() Failed 0x%08X.\n", status);
		return status;
	}

	// 开始过滤IO
	status = FltStartFiltering(globalData.iFilter);
	if (!NT_SUCCESS(status))
	{
		// 启动IO过滤失败
		DbgPrint("wyFileSysFlt.sys : DriverEntry -> FltStartFiltering() Failed 0x%08X.\n", status);

		// 反注册过滤器
		FltUnregisterFilter(globalData.iFilter);
	}

	return status;
}

NTSTATUS DriverUnload(__in FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);

	// 确保调用线程运行在一个允许分页的足够低IRQL级别。
	PAGED_CODE();

	FltUnregisterFilter(globalData.iFilter);
	globalData.iFilter = NULL;

	return STATUS_SUCCESS;
}

NTSTATUS DriverInstanceSetup(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_SETUP_FLAGS Flags, __in DEVICE_TYPE VolumeDeviceType, __in FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
	// 
	// 函数说明：
	// 当一个卷上有一个新的实例被创建时，这个函数会被调用。
	// 这给我们提供了机会去attach这个卷。
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
	// 申请与卷相关的上下文并初始化

	// 分配实例上下文，将得到的上下文内存空间返回到PWY_INSTANCE_CONTEXT中
	DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Allocate instance context (Volume = %p, Instance = %p)\n",
		FltObjects->Volume, FltObjects->Instance);
	status = FltAllocateContext(globalData.iFilter, FLT_INSTANCE_CONTEXT, WY_INSTANCE_CONTEXT_SIZE, NonPagedPool, (PFLT_CONTEXT *)&instanceContext);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Failed allocate instance context (Volume = %p, Instance = %p, Status = 0x%x)\n", 
			FltObjects->Volume, FltObjects->Instance, status);
		goto DriverInstanceSetupCleanup;
	}

	// 获取NT卷名的长度
	status = FltGetVolumeName(FltObjects->Volume, NULL, &volumeNameLength);
	if (!NT_SUCCESS(status) &&  (status != STATUS_BUFFER_TOO_SMALL))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Unexpected failure in FltGetVolumeName. (Volume = %p, Instance = %p, Status = 0x%x)\n",
			FltObjects->Volume, FltObjects->Instance, status);
		goto DriverInstanceSetupCleanup;
	}

	// 分配一个足够大的字符串缓冲区来存放卷名
	instanceContext->iVolumeName.MaximumLength = (USHORT)volumeNameLength;
	status = CtxAllocateUnicodeString(&instanceContext->iVolumeName);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Failed to allocate volume name string. (Volume = %p, Instance = %p, Status = 0x%x)\n",
			FltObjects->Volume, FltObjects->Instance, status);
		goto DriverInstanceSetupCleanup;
	}

	// 获取NT卷名
	status = FltGetVolumeName(FltObjects->Volume, &instanceContext->iVolumeName, &volumeNameLength);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("wyFileSysFlt.sys : DriverInstanceSetup -> Unexpected failure in FltGetVolumeName. (Volume = %p, Instance = %p, Status = 0x%x)\n",
			FltObjects->Volume, FltObjects->Instance, status);
		goto DriverInstanceSetupCleanup;
	}

	// 填充我们自己创建的实例句柄
	instanceContext->iVolume = FltObjects->Volume;
	instanceContext->iInstance = FltObjects->Instance;

	// 设置实例上下文
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
	// 不管FltSetInstanceContext成功与否，如果FltAllocateContext成功了，我们必须释放掉这个上下文。
	// FltAllocateContext增加了一个引用计数。
	// 一个成功的FltSetInstanceContext增加一个引用计数，并且与文件系统对象的上下文相关联
	//
	// FltReleaseContext减少一个引用计数
	//
	// 当FltSetInstanceContext成功之后，调用FltReleaseContext将委托一个文件系统结构引用计数为1的内部引用上下文
	//（这句话翻译的很悲剧，有兴趣的请查看原文理解）
	//
	// 当FltSetInstanceContext失败了，调用FltReleaseContext将委托一个文件系统结构引用计数为0的内部引用上下文
	// (这句同上，还有一些句子被我忽略了，如果要弄个明白的话请查看原文)
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
	// 函数说明：
	// 当调用FltDetachVolume 或 FilterDetach手动删除一个实例的时候此函数被调用。
	// 这给我们机会去让这个派发请求失败
	// 
	// 我们在这里直接放行

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	// Do Nothing ...

	return STATUS_SUCCESS;
}

VOID DriverInstanceTeardownStart(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
	//
	// 函数说明：
	// 当实例拆卸开始的时候这个函数被调用
	// 
	// 我们在这里直接放行

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	// Do Nothing ...

	return ;
}

VOID DriverInstanceTeardownComplete(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
	// 
	// 函数说明： 
	// 当实例拆卸完成之后会调用此函数

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
		// 在这里过滤器可能执行任何关联到他自己的结构上的实例的拆卸。
		// 
		// 过滤器不会释放内存和实例上下文关联的分配出去的同步对象。它必须在实例上下文中的cleanup回调中被执行。
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

	// 前操作成功，需要后操作回调
	// 关于相关上下文的事情放在打开后操作里面处理
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

	// 获取文件名
	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED |	FLT_FILE_NAME_QUERY_DEFAULT, &fileNameInfo);
	if (!NT_SUCCESS(status))
	{
		goto FsfPostCreateCleanup;
	}

	// 如果在前面执行真实的打开操作时失败了则直接返回
	if (!NT_SUCCESS(Data->IoStatus.Status))
	{
		DbgPrint("wyFileSysFlt.sys : FSFPostCreate -> 打开 \"%wZ\" 失败！错误码：0x%x\n", &fileNameInfo->Name, Data->IoStatus.Status);
		goto FsfPostCreateCleanup;
	}

	DbgPrint("wyFileSysFlt.sys : FSFPostCreate -> 打开 \"%wZ\" 成功！\n", &fileNameInfo->Name);

	// 查找或者创建流上下文
	status = FSFCreateOrFindStreamContext(Data, TRUE, &streamContext, &streamContextCreated);
	if (!NT_SUCCESS(status))
	{
		// 这个失败很有可能是因为这个对象不支持流上下文而我们分派想一个上下文给他，或者这个对象已经被删除了
		goto FsfPostCreateCleanup;
	}

	// 获取上下文的写权限(获取上下文资源锁)
	CtxAcquireResourceExclusive(streamContext->iLockResource);

	// 增加打开计数
	++streamContext->iCreateCount;

	// 更新上下文内的文件名
	status = FSFUpdateNameInStreamContext(&fileNameInfo->Name, streamContext);

	// 释放上下文的写权限(释放上下文资源锁)
	CtxReleaseResource(streamContext->iLockResource);

	if (!NT_SUCCESS(status))
	{
		goto FsfPostCreateCleanup;
	}

	// 创建或替换一个流句柄上下文
	status = FSFCreateOrReplaceStreamHandleContext(Data, TRUE, &streamHandleContext, &streamHandleReplaced);
	if (!NT_SUCCESS(status))
	{
		goto FsfPostCreateCleanup;
	}

	// 
	CtxAcquireResourceExclusive(streamHandleContext->iLockResource);

	// 更新上下文中的数据
	status = FSFUpdateNameInStreamHandleContxt(&fileNameInfo->Name, streamHandleContext);

	CtxReleaseResource(streamHandleContext->iLockResource);

	if (!NT_SUCCESS(status))
	{
		goto FsfPostCreateCleanup;
	}

	// 在FltParseFileNameInformation之后，fileNameInfo->Name还包含了流的名称。
	// 我们只希望文件上下文中包含文件名而不包含流名称
	fileName.Buffer = fileNameInfo->Name.Buffer;
	fileName.Length = fileNameInfo->Name.Length - fileNameInfo->Stream.Length;
	fileName.MaximumLength = fileName.Length;

	// Vista或以上版本的操作系统执行以下代码
	status = FSFFindOrCreateFileContext(Data, TRUE, &fileName, &fileContext, &fileContextCreated);
	if (!NT_SUCCESS(status))
	{
		goto FsfPostCreateCleanup;
	}

FsfPostCreateCleanup:

	// 释放我们已经获得的引用
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

	// 获取流上下文（注意，这里不创建）
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