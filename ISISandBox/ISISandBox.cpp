
#include "ISISandBox.h"
#include "DataStruct.h"
#include "RedirectHelper.h"

// 声明全局变量
GLOBAL_DATA globalData;

#define DriverPortName	L"\\ISISandBoxPort"
#define DosDeviceHeader	L"\\DosDevice\\"
#define NtDeviceHeader	L"\\??\\"

#ifdef __cplusplus
extern "C"
{
#endif

// 前置声明一些内部函数
/**
 * 连接端口
 * 参数：
 *	ClientPort
 *	ServerPortCookie 
 *	ConnectionContext
 *	SizeOfContext
 *	ConnectionCookie
 */
NTSTATUS ISISandBoxConnectPort
(
	__in PFLT_PORT ClientPort,
	__in_opt PVOID ServerPortCookie, 
	__in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
	__in ULONG SizeOfContext, 
	__deref_out_opt PVOID *ConnectionCookie
);

/**
 * 断开端口连接
 * 参数：
 *	ConnectionCookie
 */
void ISISandBoxDisconnectPort
(
	__in_opt PVOID ConnectionCookie
);

/**
 * 消息处理回调函数
 * 参数：
 *	PortCookie
 *	InputBuffer
 *	InputBufferLength
 *	OutputBuffer
 *	OutputBufferLength
 *	ReturnOutputBufferLength
 */
NTSTATUS ISISandBoxMessageNofity
(
	IN PVOID PortCookie, 
	IN PVOID InputBuffer OPTIONAL, 
	IN ULONG InputBufferLength, 
	OUT PVOID OutputBuffer OPTIONAL,
	IN ULONG OutputBufferLength, 
	OUT PULONG ReturnOutputBufferLength
);

NTSTATUS ISISandBoxInstanceSetup
(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_SETUP_FLAGS Flags,
	__in DEVICE_TYPE VolumeDeviceType,
	__in FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

NTSTATUS ISISandBoxInstanceQueryTeardown
(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

NTSTATUS ISISandBoxReplaceFileObjectName
(
	__in PFILE_OBJECT FileObject,
	__in_bcount(FileNameLength) PWSTR NewFileName,
	__in USHORT FileNameLength
);

VOID CleanupVolumeContext
(
	__in PFLT_CONTEXT Context,
	__in FLT_CONTEXT_TYPE ContextType
);

// 内存分配
//#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, ISISandBoxConnectPort)
#pragma alloc_text(PAGE, ISISandBoxDisconnectPort)
#pragma alloc_text(PAGE, ISISandBoxMessageNofity)
#pragma alloc_text(PAGE, ISISandBoxInstanceSetup)

// IRP过滤设置
const FLT_OPERATION_REGISTRATION Callbacks[] = {
	{
		IRP_MJ_CREATE,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,	// 这个字段干嘛的，需要查《寒江独钓》
		ISISafeBoxPreCreate,
		NULL
	},

	{ IRP_MJ_OPERATION_END}
};

CONST FLT_CONTEXT_REGISTRATION ContextNotifications[] = {

	{ FLT_VOLUME_CONTEXT,
	0,
	CleanupVolumeContext,
	sizeof(VOLUME_CONTEXT),
	CONTEXT_TAG },

	{ FLT_CONTEXT_END }
};

// 过滤器注册结构
const FLT_REGISTRATION FilterRegistration = {
	sizeof( FLT_REGISTRATION ),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags
	ContextNotifications,				//  Context Registration.
	Callbacks,                          //  Operation callbacks
	DriverUnload,						//  FilterUnload
	ISISandBoxInstanceSetup,			//  InstanceSetup
	ISISandBoxInstanceQueryTeardown,	//  InstanceQueryTeardown
	NULL,                               //  InstanceTeardownStart
	NULL,                               //  InstanceTeardownComplete
	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent
};

// 函数实现
NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(RegistryPath);

	UNICODE_STRING replaceRoutineName;
	RtlInitUnicodeString(&replaceRoutineName, REPLACE_ROUTINE_NAME_STRING);

	globalData.ReplaceFileNameFunction = (PReplaceFileObjectName)MmGetSystemRoutineAddress(&replaceRoutineName);
	if (globalData.ReplaceFileNameFunction == NULL)
	{
		globalData.ReplaceFileNameFunction = ISISandBoxReplaceFileObjectName;
	}

	status = FltRegisterFilter(DriverObject, &FilterRegistration, &globalData.Filter);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	// 初始化端口名称
	UNICODE_STRING uniString;
	RtlInitUnicodeString(&uniString, DriverPortName);

	// 创建安全描述符
	PSECURITY_DESCRIPTOR sd = NULL;
	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (NT_SUCCESS(status))
	{
		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, &uniString, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, sd);

		status = FltCreateCommunicationPort(globalData.Filter,
			&globalData.ServerPort,
			&oa,
			NULL,
			ISISandBoxConnectPort, 
			ISISandBoxDisconnectPort,
			ISISandBoxMessageNofity, 
			1);

		FltFreeSecurityDescriptor(sd);

		if (NT_SUCCESS(status))
		{
			status = FltStartFiltering(globalData.Filter);

			if (NT_SUCCESS(status))
			{
				// 
				// 这里处理一些其他的初始化操作
				KeInitializeSpinLock(&globalData.WokingLock);

				// 初始化保护进程链表
				InitializeProctectProcessList();

				// 初始化重定向匹配链表
				InitializeMatchExpressionList();

				return status;
			}

			FltCloseCommunicationPort(globalData.ServerPort);
		}
	}

	FltUnregisterFilter(globalData.Filter);

	return status;
}

NTSTATUS DriverUnload(__in FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);

	KIRQL irql;
	KeAcquireSpinLock(&globalData.WokingLock, &irql);
	globalData.isWorking = FALSE;
	KeReleaseSpinLock(&globalData.WokingLock, irql);

	DbgPrint("[ISISanedBox.sys] DriverUnload step 1\n");
	FltCloseCommunicationPort(globalData.ServerPort);

	if (globalData.ClientPort)
		FltCloseCommunicationPort(globalData.ClientPort);

	DbgPrint("[ISISanedBox.sys] DriverUnload step 2\n"); 
	// 清理保护进程链表
	DestroyProctectProcessList();

	DbgPrint("[ISISanedBox.sys] DriverUnload step 3\n");
	// 清理重定向匹配链表
	DestroyMatchExpressionList();

	DbgPrint("[ISISanedBox.sys] DriverUnload step 4\n");
	FltUnregisterFilter(globalData.Filter);

	DbgPrint("[ISISanedBox.sys] DriverUnload step 5\n");
	return STATUS_SUCCESS;
}

NTSTATUS ISISandBoxInstanceSetup(__in PCFLT_RELATED_OBJECTS FltObjects,
					   __in FLT_INSTANCE_SETUP_FLAGS Flags,
					   __in DEVICE_TYPE VolumeDeviceType,
					   __in FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	NTSTATUS status = STATUS_SUCCESS;

	PVOLUME_CONTEXT volumeContext = NULL;
	PDEVICE_OBJECT deviceObject = NULL;
	__try
	{
		status = FltAllocateContext(FltObjects->Filter, FLT_VOLUME_CONTEXT, 
			sizeof(VOLUME_CONTEXT), NonPagedPool, (PFLT_CONTEXT*)&volumeContext);

		if (!NT_SUCCESS(status))
		{
			KdPrint(("[ISISandBox] InstanceSetup -> Allocate volume context failed. %u.\n", status));
			__leave;
		}

		UCHAR volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES) + 512];
		RtlZeroMemory(volPropBuffer, sizeof(FLT_VOLUME_PROPERTIES) + 512);
		PFLT_VOLUME_PROPERTIES volProp = (PFLT_VOLUME_PROPERTIES)volPropBuffer;

		ULONG retLen = 0;
		status = FltGetVolumeProperties(FltObjects->Volume, volProp, sizeof(volPropBuffer), &retLen);

		if (!NT_SUCCESS(status))
		{
			KdPrint(("[ISISandBox] InstanceSetup -> Get volume properties failed. %u\n", status));
			__leave;
		}

		ASSERT((volProp->SectorSize == 0) || (volProp->SectorSize >= MIN_SECTOR_SIZE));
		volumeContext->SectorSize = max(volProp->SectorSize, MIN_SECTOR_SIZE);
		volumeContext->Name.Buffer = NULL;

		status = FltGetDiskDeviceObject(FltObjects->Volume, &deviceObject);

		if (NT_SUCCESS(status))
		{
			KdPrint(("[ISISandBox] InstanceSetup -> FltGetDiskDeviceObject OK.\n"));

			// 成功
			#pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "Used to maintain compatability with Win 2k")

			//UNICODE_STRING dosName;
			status = IoVolumeDeviceToDosName(deviceObject, &volumeContext->Name);

			PUNICODE_STRING workingName = NULL;
			if (!NT_SUCCESS(status))
			{
				KdPrint(("[ISISandBox] InstanceSetup -> Get volume's dos-name failed. %u\n", status));

				ASSERT(volumeContext->Name.Buffer == NULL);

				if (volProp->RealDeviceName.Length > 0)
				{
					workingName = &volProp->RealDeviceName;
				} 
				else if (volProp->FileSystemDeviceName.Length > 0) 
				{
					workingName = &volProp->FileSystemDeviceName;
				} 
				else 
				{
					// 获取不到卷名称
					status = STATUS_FLT_DO_NOT_ATTACH;
					__leave;
				}

				USHORT size = workingName->Length + sizeof(WCHAR);

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "volumeContext->Name.Buffer will not be leaked because it is freed in CleanupVolumeContext")

				volumeContext->Name.Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, size, 'mnBS');

				if (volumeContext->Name.Buffer == NULL)
				{
					status = STATUS_INSUFFICIENT_RESOURCES;
					__leave;
				}

				volumeContext->Name.Length = 0;
				volumeContext->Name.MaximumLength = size;

				RtlCopyUnicodeString(&volumeContext->Name, workingName);
				RtlAppendUnicodeToString(&volumeContext->Name, L":");
			}

			// 这里顺便保存该卷的卷名
			//UNICODE_STRING volDevObjName;
			//volDevObjName.MaximumLength = 260;
			//volDevObjName.Buffer = (PWSTR)ExAllocatePoolWithTag(NonPagedPool, volDevObjName.MaximumLength, 'vdon');
			//if (volDevObjName.Buffer != NULL)
			//{
			//	ULONG size = 0;
			//	status = FltGetVolumeName(FltObjects->Volume, &volDevObjName, &size);
			//	if (!NT_SUCCESS(status) && status == STATUS_BUFFER_TOO_SMALL)
			//	{
			//		ExFreePool(volDevObjName.Buffer);
			//		volDevObjName.MaximumLength = (USHORT)size;
			//		volDevObjName.Buffer = (PWSTR)ExAllocatePoolWithTag(NonPagedPool, volDevObjName.MaximumLength, 'vdon');
			//		if (volDevObjName.Buffer != NULL)
			//		{
			//			status = FltGetVolumeName(FltObjects->Volume, &volDevObjName, &size);
			//			volumeContext->VolumeDeviceName.Buffer = (PWSTR)ExAllocatePoolWithTag(NonPagedPool, size, 'vdn0');
			//			RtlCopyUnicodeString(&volumeContext->VolumeDeviceName, &volDevObjName);
			//			volumeContext->VolumeDeviceName.Length = (USHORT)size;
			//			volumeContext->VolumeDeviceName.MaximumLength = (USHORT)size;
			//		}
			//	}
			//}

			KdPrint(("[ISISandBox] InstanceSetup -> volume name : %wZ\n", &volumeContext->Name));
			status = FltSetVolumeContext(FltObjects->Volume, FLT_SET_CONTEXT_REPLACE_IF_EXISTS, volumeContext, NULL);

			if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED)
			{
				KdPrint(("[ISISandBox] InstanceSetup -> Set new volume context failed. %u\n", status));
				status = STATUS_SUCCESS;
			}
		}
	}
	__finally
	{
		if (volumeContext)
		{
			FltReleaseContext(volumeContext);
		}

		if (deviceObject) 
		{
			ObDereferenceObject(deviceObject);
		}
	}

	return status;
}

NTSTATUS ISISandBoxInstanceQueryTeardown(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	__try
	{
		KdPrint(("[ISISandBox] InstanceQueryTeardown"));

		// 这里释放所有卷上下文
		PVOLUME_CONTEXT volumeContext = NULL;
		NTSTATUS status = FltGetVolumeContext(globalData.Filter, FltObjects->Volume, (PFLT_CONTEXT*)&volumeContext);
		if (!NT_SUCCESS(status))
			__leave;

		KdPrint(("[ISISandBox] InstanceQueryTeardown -> %wZ\n", &volumeContext->Name));
		FltReleaseContext((PFLT_CONTEXT)volumeContext);
	}
	__finally
	{
		// Do nothing ...
	}

	return STATUS_SUCCESS;
}

NTSTATUS ISISandBoxConnectPort(__in PFLT_PORT ClientPort, __in_opt PVOID ServerPortCookie, 
					 __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
					 __in ULONG SizeOfContext, __deref_out_opt PVOID *ConnectionCookie)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ClientPort);
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);

	ASSERT(globalData.ClientPort == NULL);

	globalData.ClientPort = ClientPort;

	KdPrint(("[ISISandBox] Someone has connected to this driver ! clientPort : %x.", globalData.ClientPort));

	return STATUS_SUCCESS;
}

void ISISandBoxDisconnectPort(__in_opt PVOID ConnectionCookie)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ConnectionCookie);

	FltCloseClientPort(globalData.Filter, &globalData.ClientPort);

	KdPrint(("[ISISandBox] Someone has disconnected from this driver !"));

	globalData.ClientPort = NULL;
}

NTSTATUS ISISandBoxMessageNofity(IN PVOID PortCookie, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength,
					   OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PULONG ReturnOutputBufferLength)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

	NTSTATUS status = STATUS_SUCCESS;

	KdPrint(("[ISISandBox] Someone send some data here. datalength : %u.\n", InputBufferLength));

	if (InputBuffer == NULL || InputBufferLength <= 0)
	{
		return STATUS_INVALID_PARAMETER;
	}

	// 解析发来的数据
	PMESSAGE_TYPE_SEND message = (PMESSAGE_TYPE_SEND)InputBuffer;

	switch (message->Message.MessageType)
	{
	case MESSAGE_DRIVER_START_WORKING:
		{
			KIRQL irql;
			KeAcquireSpinLock(&globalData.WokingLock, &irql);
			if (!globalData.isWorking)
			{
				// 启动工作
				globalData.isWorking = TRUE;

				KdPrint(("[ISISandBox] MESSAGE_DRIVER_START_WORKING Start working.\n"));
			}
			else
			{
				KdPrint(("[ISISandBox] MESSAGE_DRIVER_START_WORKING Already working.\n"));
			}
			KeReleaseSpinLock(&globalData.WokingLock, irql);

			// 这里判断一下输出缓冲区是否可用，可以的话就返回操作数据
			__try
			{
				ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

				PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
				reply->MessageType = MESSAGE_DRIVER_START_WORKING;
				reply->status = 0;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			}
		}
		break;
	case MESSAGE_DRIVER_STOP_WORKING:
		{
			KIRQL irql;
			KeAcquireSpinLock(&globalData.WokingLock, &irql);
			if (globalData.isWorking)
			{
				// 停止工作
				globalData.isWorking = FALSE;
				KdPrint(("[ISISandBox] MESSAGE_DRIVER_STOP_WORKING Stop ...\n"));
			}
			else
			{
				KdPrint(("[ISISandBox] MESSAGE_DRIVER_STOP_WORKING Already stop.\n"));
			}
			KeReleaseSpinLock(&globalData.WokingLock, irql);

			__try
			{
				ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

				PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
				reply->MessageType = MESSAGE_DRIVER_STOP_WORKING;
				reply->status = 0;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			}
		}
		break;
	case MESSAGE_INSERT_PROTECT_PATH:
		{
			// 插入重定向基址（可为模式匹配的）
			PMESSAGE_PROTECT_PATH_SEND msg = (PMESSAGE_PROTECT_PATH_SEND)InputBuffer;

			status = InsertMatchExpression(msg->Message.ProtectPath, msg->Message.Length);

			__try
			{
				ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

				PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
				reply->MessageType = MESSAGE_INSERT_PROTECT_PATH;
				reply->status = status;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			}

			UNICODE_STRING BaseAddr;
			RtlInitUnicodeString(&BaseAddr, msg->Message.ProtectPath);
			KdPrint(("[ISISandBox] MESSAGE_INSERT_PROTECT_PATH Protect path : %wZ.\n", &BaseAddr));
		}
		break;
	case MESSAGE_REMOVE_PROTECT_PATH:
		{
			// 移除重定向基址
			PMESSAGE_PROTECT_PATH_SEND msg = (PMESSAGE_PROTECT_PATH_SEND)InputBuffer;

			KdPrint(("[ISISandBox] msg size : %u\n", InputBufferLength));

			status = RemoveMatchExpression(msg->Message.ProtectPath, msg->Message.Length);

			__try
			{
				ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

				PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
				reply->MessageType = MESSAGE_REMOVE_PROTECT_PATH;
				reply->status = status;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			}

			UNICODE_STRING BaseAddr;
			RtlInitUnicodeString(&BaseAddr, msg->Message.ProtectPath);
			KdPrint(("[ISISandBox] MESSAGE_REMOVE_PROTECT_PATH Protect path : %wZ.\n", &BaseAddr));
		}
		break;
	case MESSAGE_REMOVE_ALL_PROTECT_PATH:
		{
			// 移除所有重定向基址
			PMESSAGE_PROTECT_PATH_SEND msg = (PMESSAGE_PROTECT_PATH_SEND)InputBuffer;

			status = CleanupMatchExpression();

			__try
			{
				ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

				PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
				reply->MessageType = MESSAGE_REMOVE_ALL_PROTECT_PATH;
				reply->status = status;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			}

			UNICODE_STRING BaseAddr;
			RtlInitUnicodeString(&BaseAddr, msg->Message.ProtectPath);
			KdPrint(("[ISISandBox] MESSAGE_REMOVE_ALL_PROTECT_PATH Protect path : %wZ.\n", &BaseAddr));
		}
		break;
	case MESSAGE_INSERT_REDIRECT_PATH:
		{
			// 插入重定向根路径
			PMESSAGE_REDIRECT_PATH_SEND msg = (PMESSAGE_REDIRECT_PATH_SEND)InputBuffer;

			status = SetRedirectBaseDirectory(msg->Message.RedirectPath, msg->Message.Length);

			__try
			{
				ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

				PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
				reply->MessageType = MESSAGE_REMOVE_REDIRECT_PATH;
				reply->status = status;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			}

			UNICODE_STRING RedirectPath;
			RtlInitUnicodeString(&RedirectPath, msg->Message.RedirectPath);
			KdPrint(("[ISISandBox] MESSAGE_INSERT_REDIRECT_PATH Redirect path : %wZ.\n", &RedirectPath));
		}
		break;
	case MESSAGE_REMOVE_REDIRECT_PATH:
		{
			// 移除重定向根路径
			PMESSAGE_REDIRECT_PATH_SEND msg = (PMESSAGE_REDIRECT_PATH_SEND)InputBuffer;

			status = CleanupRedirectBaseDirectory();

			__try
			{
				ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

				PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
				reply->MessageType = MESSAGE_REMOVE_REDIRECT_PATH;
				reply->status = status;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			}

			UNICODE_STRING RedirectPath;
			RtlInitUnicodeString(&RedirectPath, msg->Message.RedirectPath);
			KdPrint(("[ISISandBox] MESSAGE_REMOVE_REDIRECT_PATH Redirect path : %wZ.\n", &RedirectPath));
		}
		break;
	case MESSAGE_REMOVE_ALL_REDIRECT_PATH:
		{
			// 移除所有重定向根路径
			PMESSAGE_REDIRECT_PATH_SEND msg = (PMESSAGE_REDIRECT_PATH_SEND)InputBuffer;

			UNICODE_STRING RedirectPath;
			RtlInitUnicodeString(&RedirectPath, msg->Message.RedirectPath);
			KdPrint(("[ISISandBox] MESSAGE_REMOVE_ALL_REDIRECT_PATH Redirect path : %wZ.\n", &RedirectPath));
		}
		break;
	case MESSAGE_INSERT_PROTECT_PROCESS:
		{
			// 插入重定向进程
			PMESSAGE_PROTECT_PROCESS_SEND msg = (PMESSAGE_PROTECT_PROCESS_SEND)InputBuffer;

			status = InsertProtectProcess(msg->Message.ProcessId);

			__try
			{
				ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

				PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
				reply->MessageType = MESSAGE_INSERT_PROTECT_PROCESS;
				reply->status = status;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			}

			KdPrint(("[ISISandBox] MESSAGE_INSERT_PROTECT_PROCESS Protect ProcessId : %u.\n", msg->Message.ProcessId));
		}
		break;
	case MESSAGE_REMOVE_PROTECT_PROCESS:
		{
			// 移除重定向进程
			PMESSAGE_PROTECT_PROCESS_SEND msg = (PMESSAGE_PROTECT_PROCESS_SEND)InputBuffer;

			status = RemoveProtectProcess(msg->Message.ProcessId);

			__try
			{
				ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

				PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
				reply->MessageType = MESSAGE_INSERT_PROTECT_PROCESS;
				reply->status = status;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			}

			KdPrint(("[ISISandBox] MESSAGE_INSERT_PROTECT_PROCESS Protect ProcessId : %u.\n", msg->Message.ProcessId));
		}
		break;
	case MESSAGE_REMOVE_ALL_PROTECT_PROCESS:
		{
			// 移除所有重定向进程
			PMESSAGE_PROTECT_PROCESS_SEND msg = (PMESSAGE_PROTECT_PROCESS_SEND)InputBuffer;

			status = CleanupProtectProcess();

			__try
			{
				ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

				PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
				reply->MessageType = MESSAGE_INSERT_PROTECT_PROCESS;
				reply->status = status;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			}

			KdPrint(("[ISISandBox] MESSAGE_INSERT_PROTECT_PROCESS Protect ProcessId : %u.\n", msg->Message.ProcessId));
		}
		break;
	case MESSAGE_DEBUG_DEVICENAME_TO_DOSNAME:
		{
			//UNICODE_STRING deviceName;
			//RtlInitUnicodeString(&deviceName, L"\\Device\\HarddiskVolume1");

			//UNICODE_STRING symboName;
			//status = FsDeviceNameToSymboName(&deviceName, &symboName);

			//__try
			//{
			//	ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

			//	PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
			//	reply->MessageType = MESSAGE_DEBUG_DEVICENAME_TO_DOSNAME;
			//	reply->status = status;
			//}
			//__except(EXCEPTION_EXECUTE_HANDLER)
			//{
			//	KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
			//}

			//KdPrint(("[ISISandBox] MESSAGE_DEBUG_DEVICENAME_TO_DOSNAME SymboName : %wZ.\n", &symboName));
		}
		break;
	default:
		KdPrint(("[ISISandBox] Someone send some data here, but i cannot recognize.\n"));

		__try
		{
			ProbeForWrite(OutputBuffer, sizeof(MESSAGE_REPLY), 4);

			PMESSAGE_REPLY reply = (PMESSAGE_REPLY)OutputBuffer;
			reply->MessageType = 0xFFFFFFFF;
			reply->status = 0xFFFFFFFF;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint(("[ISISandBox] OutputBuffer is invalid.\n"));
		}

		return STATUS_INVALID_PARAMETER;
	}

	return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS ISISafeBoxPreCreate(__inout PFLT_CALLBACK_DATA Data,
											  __in PCFLT_RELATED_OBJECTS FltObjects,
											  __deref_out_opt PVOID *CompletionContext
											  )
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	NTSTATUS status = STATUS_SUCCESS;

	if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE)) 
		goto EXIT_PRE_CREATE_IRP;

	if (FlagOn(Data->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN)) 
		goto EXIT_PRE_CREATE_IRP;


	KIRQL irql;
	KeAcquireSpinLock(&globalData.WokingLock, &irql);
	BOOLEAN workeStatus = globalData.isWorking;
	KeReleaseSpinLock(&globalData.WokingLock, irql);

	// 这里是打开前操作过滤
	if (!workeStatus)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	PFLT_FILE_NAME_INFORMATION fileNameInformation = NULL;
	__try
	{
		// 获取当前进程PID
		HANDLE currentProcessId = PsGetCurrentProcessId();
		status = CheckProtectProcess(currentProcessId);
		if (!NT_SUCCESS(status))
		{
			// 不是保护进程，不重定向
			return FLT_PREOP_SUCCESS_NO_CALLBACK;
		}

		// 获取当前文件路径
		status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, &fileNameInformation);

		if (!NT_SUCCESS(status))
		{
			status = FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED|FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, &fileNameInformation);

			if (!NT_SUCCESS(status))
			{
				KdPrint(("[ISISandBox] PreCreate -> Get file name info failed. %u\n", status));
				return FLT_PREOP_SUCCESS_NO_CALLBACK;
			}
		}

		status = FltParseFileNameInformation(fileNameInformation);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("[ISISandBox] PreCreate -> Parse file name info failed. %u\n", status));
			FltReleaseFileNameInformation(fileNameInformation);
			__leave;
		}

		// 检查重定向根目录是否设置成功
		status = ChechRedirectBaseDirectory();
		if (!NT_SUCCESS(status))
		{
			FltReleaseFileNameInformation(fileNameInformation);
			__leave;
		}

		// 检查打开的是否是重定向根目录中的文件
		WCHAR baseDir[260];
		ULONG baseDirLength = 0;
		RtlZeroMemory(baseDir, sizeof(WCHAR) * 260);
		status = GetRedirectBaseDirectory(baseDir, &baseDirLength);
		if (FsIsRedirectPath(baseDir, fileNameInformation, FltObjects->Volume))
		{
			FltReleaseFileNameInformation(fileNameInformation);
			__leave;
		}

		// 判断当前路径是否匹配重定向目录
		status = DoMatchExpression(&fileNameInformation->Name);
		if (!NT_SUCCESS(status))
		{
			FltReleaseFileNameInformation(fileNameInformation);
			__leave;
		}

		// 一切条件都符合，应该开始准备重定向了
		// 首先，应该创建重定向目标路径

		// 根据盘符获取路径
		WCHAR volSName[260];
		RtlZeroMemory(volSName, sizeof(WCHAR) * 260);
		RtlCopyMemory(volSName, baseDir, 4);

		UNICODE_STRING volSymboName;
		RtlInitUnicodeString(&volSymboName, volSName);

		PFLT_VOLUME volume = NULL;
		status = FltGetVolumeFromName(globalData.Filter, &volSymboName, &volume);
		if (!NT_SUCCESS(status) || !volume)
		{
			KdPrint(("[ISISandBox] PreCreate -> FltGetVolumeFromName failed : %u VolumeName : %wZ\n", status, &volSymboName));
			FltReleaseFileNameInformation(fileNameInformation);
			__leave;
		}

		UNICODE_STRING volName;
		RtlZeroMemory(&volName, sizeof(UNICODE_STRING));
		volName.MaximumLength = 512;
		volName.Length = 0;
		volName.Buffer = (PWSTR)ExAllocatePool(NonPagedPool, volName.MaximumLength);
		if (!volName.Buffer)
		{
			FltReleaseFileNameInformation(fileNameInformation);
			__leave;
		}

		RtlZeroMemory(volName.Buffer, volName.MaximumLength);
		status = FltGetVolumeName(volume/*FltObjects->Volume*/, &volName, NULL);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("[ISISandBox] PreCreate -> FltGetVolumeName failed : %u\n", status));
			ExFreePool(volName.Buffer);
			FltReleaseFileNameInformation(fileNameInformation);
			__leave;
		}

		WCHAR tmpBuffer[260];
		RtlZeroMemory(tmpBuffer, sizeof(WCHAR) * 260);

		RtlCopyMemory(tmpBuffer, volName.Buffer, volName.Length);
		KdPrint(("[ISISandBox] PreCreate -> RedirectPath 0 : %S\n", tmpBuffer));

		RtlCopyMemory(Add2Ptr(tmpBuffer, volName.Length),
			Add2Ptr(baseDir, 4),
			wcslen(baseDir) * sizeof(WCHAR) - 4);
		KdPrint(("[ISISandBox] PreCreate -> RedirectPath 1 : %S\n", tmpBuffer));

		ULONG fileNameLength = fileNameInformation->Name.Length - (fileNameInformation->Volume.Length + fileNameInformation->ParentDir.Length);
		RtlCopyMemory(Add2Ptr(tmpBuffer, volName.Length + wcslen(baseDir) * sizeof(WCHAR) - 4),
			Add2Ptr(fileNameInformation->Name.Buffer, (fileNameInformation->Volume.Length + fileNameInformation->ParentDir.Length)),
			fileNameLength);
		KdPrint(("[ISISandBox] PreCreate -> RedirectPath 2 : %S\n", tmpBuffer));

		UNICODE_STRING redirectPath;
		RtlInitUnicodeString(&redirectPath, tmpBuffer);

		KdPrint(("[ISISandBox] PreCreate -> RedirectPath : %wZ Length : %u\n", &redirectPath, redirectPath.Length));

		ExFreePool(volName.Buffer);

		// 检查条件，如果不包含"新建"的意思，就直接让它打开
		ULONG CreateDisposition = Data->Iopb->Parameters.Create.Options & 0xFF000000;
		ACCESS_MASK desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;

		if(!(CreateDisposition == FILE_SUPERSEDE || CreateDisposition == FILE_CREATE ||
			CreateDisposition == FILE_OVERWRITE_IF || CreateDisposition == FILE_OVERWRITE || 
			CreateDisposition == FILE_OPEN_IF)
			&& 
			!(desiredAccess & (FILE_WRITE_DATA | FILE_WRITE_EA | FILE_APPEND_DATA)))
		{
			// 这里需要注意的是：打开的时候需要检查重定向路径有没有该文档，如果有则打开重定向的路径
			// 检查重定向目录该文件是否存在，可以通过查询文件基本属性来计算
			// 如果存在则跳转
			OBJECT_ATTRIBUTES objectAttributes;
			InitializeObjectAttributes(&objectAttributes, &redirectPath, OBJ_KERNEL_HANDLE, NULL, NULL);

			HANDLE hFile = NULL;
			IO_STATUS_BLOCK ioStatus;
			FILE_BASIC_INFORMATION fileBaseInformation;
			status = NtQueryInformationFile(&hFile, &ioStatus, &fileBaseInformation, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation);
			if (!NT_SUCCESS(status) && status == STATUS_NO_SUCH_FILE)
			{
				FltReleaseFileNameInformation(fileNameInformation);
				__leave;
			}
		}

		// 创建成功，开始进行实际的重定向操作
		status = globalData.ReplaceFileNameFunction(Data->Iopb->TargetFileObject, redirectPath.Buffer, redirectPath.Length);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("[ISISandBox] PreCreate -> Redirect failed. %u\n", status));
			FltReleaseFileNameInformation(fileNameInformation);
			__leave;
		}

		KdPrint(("[ISISandBox] PreCreate -> Create %wZ\n", &Data->Iopb->TargetFileObject->FileName));

		status = STATUS_REPARSE;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// Do nothing ...
	}

	if (status == STATUS_REPARSE)
	{
		Data->IoStatus.Status = STATUS_REPARSE;
		Data->IoStatus.Information = IO_REPARSE;
		return FLT_PREOP_COMPLETE;
	}

EXIT_PRE_CREATE_IRP:
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS ISISandBoxReplaceFileObjectName(__in PFILE_OBJECT FileObject, __in_bcount(FileNameLength) PWSTR NewFileName, __in USHORT FileNameLength)
{
    PWSTR buffer;
    PUNICODE_STRING fileName;
    USHORT newMaxLength;

    PAGED_CODE();

    fileName = &FileObject->FileName;

    if (FileNameLength <= fileName->MaximumLength) 
        goto CopyAndReturn;

    newMaxLength = FileNameLength;

    buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, newMaxLength, SIMREP_STRING_TAG);

    if (!buffer) 
        return STATUS_INSUFFICIENT_RESOURCES;

    if (fileName->Buffer != NULL) 
	{
        ExFreePool(fileName->Buffer);
    }

    fileName->Buffer = buffer;
    fileName->MaximumLength = newMaxLength;

CopyAndReturn:

    fileName->Length = FileNameLength;
    RtlZeroMemory(fileName->Buffer, fileName->MaximumLength);
    RtlCopyMemory(fileName->Buffer, NewFileName, FileNameLength);

    return STATUS_SUCCESS;
}

VOID CleanupVolumeContext(__in PFLT_CONTEXT Context, __in FLT_CONTEXT_TYPE ContextType)
{
	PVOLUME_CONTEXT ctx = (PVOLUME_CONTEXT)Context;

	PAGED_CODE();

	UNREFERENCED_PARAMETER( ContextType );

	ASSERT(ContextType == FLT_VOLUME_CONTEXT);

	if (ctx->Name.Buffer != NULL) 
	{
		ExFreePool(ctx->Name.Buffer);
		ctx->Name.Buffer = NULL;
	}
}

#ifdef __cplusplus
};
#endif