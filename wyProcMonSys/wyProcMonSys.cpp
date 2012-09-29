
#include "wyProcMonSys.h"
#include "Communication.h"
#include "Context.h"

// 这一句我不知道是干嘛用的，scanner的例子里面有，我这里只是照搬过来
#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

// 
// 全局变量
GLOBAL_DATA globalData;

// 
// 一些文字声明
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
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
		NULL,
		PMSPostCreate
	},

	{
		IRP_MJ_READ,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,	// 这个字段干嘛的，需要查《寒江独钓》
		NULL,
		PMSPostRead
	},

	{
		IRP_MJ_WRITE,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,	// 这个字段干嘛的，需要查《寒江独钓》
		NULL,
		PMSPostWrite
	},

	{ IRP_MJ_OPERATION_END }
};

// 
// 上下文注册定义
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

// 通信端口名
const PWSTR wyProcMonSysPortName = L"\\wyProcMonSysPort";

// 
// 具体的代码逻辑实现
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

	// 初始化全局变量
	RtlZeroMemory(&globalData, sizeof(GLOBAL_DATA));

	// 注册微过滤器
	NTSTATUS status = FltRegisterFilter(DriverObject, &g_FltRegistration, &globalData.iFilter);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("[wyprocmonsys] 初始化过滤器失败！\n"));
		return status;
	}

	// 创建一个通信端口
	UNICODE_STRING portName;
	RtlInitUnicodeString(&portName, wyProcMonSysPortName);

	PSECURITY_DESCRIPTOR sd = NULL;
	OBJECT_ATTRIBUTES objectAttributs;
	// 创建一个安全描述符
	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (NT_SUCCESS(status))
	{
		InitializeObjectAttributes(&objectAttributs, &portName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, sd);

		status = FltCreateCommunicationPort(globalData.iFilter, &globalData.iDriverPort, &objectAttributs, NULL, PortConnect, PortDisconnect, NULL, 1);

		// 释放安全描述符
		FltFreeSecurityDescriptor(sd);

		if (NT_SUCCESS(status))
		{
			// 计算进程名偏移量
			globalData.iProcessNameOffsetInPEPROCESS = GetProcessNameOffset();

			// 没有开始过滤，不向用户态发送消息
			globalData.iStartFilter = FALSE;

			// 开始过滤IO
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

	// 关闭服务端口
	FltCloseCommunicationPort(globalData.iDriverPort);

	// 反注册过滤器
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
