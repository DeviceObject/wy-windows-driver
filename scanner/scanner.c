/*++
Copyright (c) 1999-2002  Microsoft Corporation

模块名称:
    scanner.c
摘要:
	这是scanner过滤器的主模块。
	这个过滤器在文件被允许打开之前扫描文件中的数据。这是一个简单的病毒检测的例子。
环境:
    内核模式
--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "scanuk.h"
#include "scanner.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

//
//  包含了所有全局数据结构的结构体。
//

SCANNER_DATA ScannerData;

//
//	这是一个我们在扫描中感兴趣的文件后缀名静态列表。
//

const UNICODE_STRING ScannerExtensionsToScan[] =
    { RTL_CONSTANT_STRING( L"doc"),
      RTL_CONSTANT_STRING( L"txt"),
      RTL_CONSTANT_STRING( L"bat"),
      RTL_CONSTANT_STRING( L"cmd"),
      RTL_CONSTANT_STRING( L"inf"),
      /*RTL_CONSTANT_STRING( L"ini"),   Removed, to much usage*/
      {0, 0, NULL}
    };


//
//	函数原型
//

NTSTATUS
ScannerPortConnect (
    __in PFLT_PORT ClientPort,
    __in_opt PVOID ServerPortCookie,
    __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    );

VOID
ScannerPortDisconnect (
    __in_opt PVOID ConnectionCookie
    );

NTSTATUS
ScannerpScanFileInUserMode (
    __in PFLT_INSTANCE Instance,
    __in PFILE_OBJECT FileObject,
    __out PBOOLEAN SafeToOpen
    );

BOOLEAN
ScannerpCheckExtension (
    __in PUNICODE_STRING Extension
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, ScannerInstanceSetup)
    #pragma alloc_text(PAGE, ScannerPreCreate)
    #pragma alloc_text(PAGE, ScannerPortConnect)
    #pragma alloc_text(PAGE, ScannerPortDisconnect)
#endif


//
//  Constant FLT_REGISTRATION structure for our filter.  This
//  initializes the callback routines our filter wants to register
//  for.  This is only used to register with the filter manager
//

const FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      0,
      ScannerPreCreate,
      ScannerPostCreate},

    { IRP_MJ_CLEANUP,
      0,
      ScannerPreCleanup,
      NULL},

    { IRP_MJ_WRITE,
      0,
      ScannerPreWrite,
      NULL},

    { IRP_MJ_OPERATION_END}
};

//
// 这个是什么作用？目前还不清楚
// 
const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

    { FLT_STREAMHANDLE_CONTEXT,  // 流上下文
      0,
      NULL,		// 上下文清理回调函数
      sizeof(SCANNER_STREAM_HANDLE_CONTEXT),  // 上下文大小
      'chBS'  // 内存标记，便于资源清理
	},

    { FLT_CONTEXT_END }
};

// 
// 过滤器注册结构
const FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags
    ContextRegistration,                //  Context Registration.
    Callbacks,                          //  Operation callbacks
    ScannerUnload,                      //  FilterUnload
    ScannerInstanceSetup,               //  InstanceSetup
    ScannerQueryTeardown,               //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete
    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent
};

////////////////////////////////////////////////////////////////////////////
//
//    Filter initialization and unload routines.
//
////////////////////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the Filter driver.  This
    registers the Filter with the filter manager and initializes all
    its global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Returns STATUS_SUCCESS.
--*/
{
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING uniString;
    PSECURITY_DESCRIPTOR sd;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    //
    //  向过滤管理器注册过滤器
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &ScannerData.Filter );


    if (!NT_SUCCESS( status )) {

        return status;
    }

    //
	//  创建一个通信端口
    //

    RtlInitUnicodeString( &uniString, ScannerPortName );

    //
    //  We secure the port so only ADMINs & SYSTEM can acecss it.
	//	我们设置只有管理员权限用户与SYSTEM用户权限的程序可以接入这个端口
    //

    status = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS );

    if (NT_SUCCESS( status )) {

        InitializeObjectAttributes( &oa,
                                    &uniString,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    NULL,
                                    sd );

        status = FltCreateCommunicationPort( ScannerData.Filter,
                                             &ScannerData.ServerPort,
                                             &oa,
                                             NULL,
                                             ScannerPortConnect,
                                             ScannerPortDisconnect,
                                             NULL,
                                             1 );
        //
		//	当调用FltCreateCommunicationPort()后，安全描述符已经没有需要了，应当释放掉。
        //

        FltFreeSecurityDescriptor( sd );

        if (NT_SUCCESS( status )) {

            //
			//	开始过滤IO
            //

            status = FltStartFiltering( ScannerData.Filter );

            if (NT_SUCCESS( status )) {

                return STATUS_SUCCESS;
            }

            FltCloseCommunicationPort( ScannerData.ServerPort );
        }
    }

    FltUnregisterFilter( ScannerData.Filter );

    return status;
}


/*++
函数说明
	当用户态连接这个服务器端口时此函数会被调用，从而创建一个连接。
	● 在调用FltCreateCommunicationPort()创建通信端口的时候被指定
参数表
    ClientPort - 这是客户端连接端口,将被用来从过滤器中发送消息到用户态。
    ServerPortCookie - 微过滤器创建的端口相关的上下文。
    ConnectionContext - 端口实际连接过来的上下文。（很可能是你的用户态服务程序）Context from entity connecting to this port (most likely
        your user mode service)
    SizeofContext - ConnectionContext的字节大小。
    ConnectionCookie - 传递到端口断开程序的上下文。
Return Value
    STATUS_SUCCESS - to accept the connection
--*/
NTSTATUS
ScannerPortConnect (
    __in PFLT_PORT ClientPort,
    __in_opt PVOID ServerPortCookie,
    __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( ConnectionContext );
    UNREFERENCED_PARAMETER( SizeOfContext);
    UNREFERENCED_PARAMETER( ConnectionCookie );

    ASSERT( ScannerData.ClientPort == NULL );
    ASSERT( ScannerData.UserProcess == NULL );

    //
    //  Set the user process and port.
	//  设置用户进程和端口
    //

    ScannerData.UserProcess = PsGetCurrentProcess();
    ScannerData.ClientPort = ClientPort;

    DbgPrint( "!!! scanner.sys --- connected, port=0x%p\n", ClientPort );

    return STATUS_SUCCESS;
}

/*++
函数说明
	当连接被断开的时候调用此函数。
	我们用它来关闭连接句柄。
参数表
    ConnectionCookie - 端口连接程序创建的上下文。Context from the port connect routine 
返回值
    None
--*/
VOID
ScannerPortDisconnect(
     __in_opt PVOID ConnectionCookie
     )
{
	// 展开传递的参数或表达式。其目的是避免编译器关于未引用参数的警告。
    UNREFERENCED_PARAMETER( ConnectionCookie );

    PAGED_CODE();

    DbgPrint( "!!! scanner.sys --- disconnected, port=0x%p\n", ScannerData.ClientPort );

    //
    //  Close our handle to the connection: note, since we limited max connections to 1,
    //  another connect will not be allowed until we return from the disconnect routine.
	//  关闭我们的连接句柄。
    //

    FltCloseClientPort( ScannerData.Filter, &ScannerData.ClientPort );

    //
    //  重置用户进程字段。Reset the user-process field.
    //

    ScannerData.UserProcess = NULL;
}

/*++
函数说明:
	这是过滤器驱动卸载函数。
	它从过滤管理器中反注册此过滤器，并且释放所有已申请过的全局数据结构的资源。
    This is the unload routine for the Filter driver.  This unregisters the
    Filter with the filter manager and frees any allocated global data
    structures.
参数表:
    None.
返回值:
    Returns the final status of the deallocation routines.
--*/
NTSTATUS
ScannerUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Flags );

    //
	//  关闭服务端口
    //

    FltCloseCommunicationPort( ScannerData.ServerPort );

    //
	//  反注册过滤器
    //

    FltUnregisterFilter( ScannerData.Filter );

    return STATUS_SUCCESS;
}

/*++
函数说明:
	当一个新的实例被创建的时候，过滤管理器会调用此函数。
	我们在注册表中指定我们想要手动附加，这些是我们在这里要接受的信息。
    This routine is called by the filter manager when a new instance is created.
    We specified in the registry that we only want for manual attachments,
    so that is all we should receive here.
参数表:
    FltObjects - 描述我们需要安装的实例或卷。Describes the instance and volume which we are being asked to
        setup.
    Flags - Flags describing the type of attachment this is.
    VolumeDeviceType - 将要被附加的实例或卷的DEVICE_TYPE（设备类型）。The DEVICE_TYPE for the volume to which this instance
        will attach.
    VolumeFileSystemType - 卷的文件系统格式。The file system formatted on this volume.
返回值:
  FLT_NOTIFY_STATUS_ATTACH              - we wish to attach to the volume
  FLT_NOTIFY_STATUS_DO_NOT_ATTACH       - no, thank you
--*/
NTSTATUS
ScannerInstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    ASSERT( FltObjects->Filter == ScannerData.Filter );

    //
    //  Don't attach to network volumes.
	//  禁止附加网络卷
    //

    if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {

       return STATUS_FLT_DO_NOT_ATTACH;
    }

    return STATUS_SUCCESS;
}

/*++
函数说明:

	// 这是过滤器的实例派遣函数。
	// 当一个用户启动一个手动派遣实例的时候，过滤管理器会调用这个函数。
	// 这是一个查询函数：如果过滤器不支持手动派遣，它会返回一个失败状态。
    This is the instance detach routine for the filter. This
    routine is called by filter manager when a user initiates a manual instance
    detach. This is a 'query' routine: if the filter does not want to support
    manual detach, it can return a failure status
参数表:
    FltObjects - 描述了我们接收的“query teardown”请求相关的实例或卷。Describes the instance and volume for which we are receiving
        this query teardown request.
    Flags - 不可用。Unused
Return Value:
    STATUS_SUCCESS - we allow instance detach to happen
--*/
NTSTATUS
ScannerQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    return STATUS_SUCCESS;
}


/*++
函数说明:
	前置打开回调。
	我们需要记住的是：不管这个文件已经被写操作权限打开。如果有，我们我们将清除后重新扫描。
	这个额外扫描计划有至少两个方面：
	  1. 也许由于权限问题创建失败了
	  2. 这个文件已经使用写权限打开了，但事实上从未被写入。
	假设写入比创建更普通，在写路径校验或设置上下文将比在打开前做一个猜测的影响要小得多。
    Pre create callback.  We need to remember whether this file has been
    opened for write access.  If it has, we'll want to rescan it in cleanup.
    This scheme results in extra scans in at least two cases:
    -- if the create fails (perhaps for access denied)
    -- the file is opened for write access but never actually written to
    The assumption is that writes are more common than creates, and checking
    or setting the context in the write path would be less efficient than
    taking a good guess before the create.
参数表:
    Data - 描述了操作参数的数据结构。The structure which describes the operation parameters.
    FltObject - 收到此操作影响的对象结构。The structure which describes the objects affected by this
        operation.
    CompletionContext - 输出一个可以在前操作到后操作之间被传递的参数。Output parameter which can be used to pass a context
        from this pre-create callback to the post-create callback.
Return Value:
   FLT_PREOP_SUCCESS_WITH_CALLBACK - If this is not our user-mode process.
   FLT_PREOP_SUCCESS_NO_CALLBACK - All other threads.
--*/
FLT_PREOP_CALLBACK_STATUS
ScannerPreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )

{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    //
    //  See if this create is being done by our user process.
	//  查看是否已被我们的进程打开
    //

	// IoThreadToProcess 返回指定线程所属的进程。
    if (IoThreadToProcess( Data->Thread ) == ScannerData.UserProcess)
	{

		// 如果相等，返回，不进入后操作。
        DbgPrint( "!!! scanner.sys -- allowing create for trusted process \n" );

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

/*++
Routine Description:
    Checks if this file name extension is something we are interested in
Arguments
    Extension - Pointer to the file name extension
Return Value
    TRUE - Yes we are interested
    FALSE - No
--*/
BOOLEAN
ScannerpCheckExtension (
    __in PUNICODE_STRING Extension
    )
{
    const UNICODE_STRING *ext;

    if (Extension->Length == 0) {

        return FALSE;
    }

    //
    //  Check if it matches any one of our static extension list
    //

    ext = ScannerExtensionsToScan;

    while (ext->Buffer != NULL) {

        if (RtlCompareUnicodeString( Extension, ext, TRUE ) == 0) {

            //
            //  A match. We are interested in this file
            //

            return TRUE;
        }
        ext++;
    }

    return FALSE;
}

/*++
Routine Description:
	打开后操作。
	打开操作进入文件系统后我们不能扫描文件。否则文件系统不会准备好为我们读取该文件。
	注：这里可能是引起数据安全保护系统加密驱动打不开文件的问题之一。
    Post create callback.  We can't scan the file until after the create has
    gone to the filesystem, since otherwise the filesystem wouldn't be ready
    to read the file for us.
Arguments:
    Data - The structure which describes the operation parameters.
    FltObject - The structure which describes the objects affected by this
        operation.
    CompletionContext - The operation context passed fron the pre-create
        callback.
    Flags - 这个标记说明了为什么我们要过去这个后操作回调。Flags to say why we are getting this post-operation callback.
Return Value:
    FLT_POSTOP_FINISHED_PROCESSING - ok to open the file or we wish to deny
                                     access to this file, hence undo the open
--*/
FLT_POSTOP_CALLBACK_STATUS
ScannerPostCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    PSCANNER_STREAM_HANDLE_CONTEXT scannerContext;
    FLT_POSTOP_CALLBACK_STATUS returnStatus = FLT_POSTOP_FINISHED_PROCESSING;
    PFLT_FILE_NAME_INFORMATION nameInfo;
    NTSTATUS status;
    BOOLEAN safeToOpen, scanFile;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    //
    //  If this create was failing anyway, don't bother scanning now.
    //  如果打开失败，直接返回，不扫描了

    if (!NT_SUCCESS( Data->IoStatus.Status ) || (STATUS_REPARSE == Data->IoStatus.Status)) 
        return FLT_POSTOP_FINISHED_PROCESSING;

    //
    //  Check if we are interested in this file.
	//  检查是否是我们感兴趣的文件。
    //

	//  查询文件名
    status = FltGetFileNameInformation( Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo );

    if (!NT_SUCCESS( status )) 
        return FLT_POSTOP_FINISHED_PROCESSING;

	// 拆分文件路径
    FltParseFileNameInformation( nameInfo );

    //
    //  Check if the extension matches the list of extensions we are interested in
    //

    scanFile = ScannerpCheckExtension( &nameInfo->Extension );

    //
    //  Release file name info, we're done with it
	//  释放nameInfo，我们已经完成对他的引用。
    //

    FltReleaseFileNameInformation( nameInfo );

    if (!scanFile) {

        //
        //  Not an extension we are interested in
        //

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

	// 此函数作用参见定义部分
    (VOID) ScannerpScanFileInUserMode( FltObjects->Instance,
                                       FltObjects->FileObject,
                                       &safeToOpen );

    if (!safeToOpen) {

        //
        //  Ask the filter manager to undo the create.
        //

        DbgPrint( "!!! scanner.sys -- foul language detected in postcreate !!!\n" );

        DbgPrint( "!!! scanner.sys -- undoing create \n" );

		// close a newly opened or created file
        FltCancelFileOpen( FltObjects->Instance, FltObjects->FileObject );

        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;

        returnStatus = FLT_POSTOP_FINISHED_PROCESSING;

    } else if (FltObjects->FileObject->WriteAccess) {

        //
        //  The create has requested write access, mark to rescan the file.
        //  Allocate the context.
		//  打开时请求了写操作，标记为重新扫描文件，并申请上下文
        //

		// allocates a context structure
        status = FltAllocateContext( ScannerData.Filter,
                                     FLT_STREAMHANDLE_CONTEXT,
                                     sizeof(SCANNER_STREAM_HANDLE_CONTEXT),
                                     PagedPool,
                                     &scannerContext );

        if (NT_SUCCESS(status)) {

            //
            //  Set the handle context.
            //

            scannerContext->RescanRequired = TRUE;

			// sets a context for a stream handle
			// 设置流句柄的上下文
            (VOID) FltSetStreamHandleContext( FltObjects->Instance,
                                              FltObjects->FileObject,
                                              FLT_SET_CONTEXT_REPLACE_IF_EXISTS,
                                              scannerContext,
                                              NULL );

			// 一般来讲，我们需要检查FltSetStreamHandleContext()的各种错误码返回值。
			// 然而，唯一的错误状态可能已经返回了。在这个案例，告诉我们上下文是不支持的。
			// 即使我们获取到了这个错误，如果没有成功设置，我们也仅仅是释放上下文并且释放内存

            //
            //  Normally we would check the results of FltSetStreamHandleContext
            //  for a variety of error cases. However, The only error status 
            //  that could be returned, in this case, would tell us that
            //  contexts are not supported.  Even if we got this error,
            //  we just want to release the context now and that will free
            //  this memory if it was not successfully set.
            //

            //
			//  释放我们在这个上下文上的引用
            //  Release our reference on the context (the set adds a reference)
            //

            FltReleaseContext( scannerContext );
        }
    }

    return returnStatus;
}

/*++
函数说明:
	清理前操作回调。如果文件用写权限打开，我们希望现在重新扫描它。
    Pre cleanup callback.  If this file was opened for write access, we want
    to rescan it now.
参数表:
    Data - The structure which describes the operation parameters.
    FltObject - The structure which describes the objects affected by this
        operation.
    CompletionContext - Output parameter which can be used to pass a context
        from this pre-cleanup callback to the post-cleanup callback.
Return Value:
    Always FLT_PREOP_SUCCESS_NO_CALLBACK.
--*/
FLT_PREOP_CALLBACK_STATUS
ScannerPreCleanup (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
    NTSTATUS status;
    PSCANNER_STREAM_HANDLE_CONTEXT context;
    BOOLEAN safe;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( CompletionContext );

    status = FltGetStreamHandleContext( FltObjects->Instance,
                                        FltObjects->FileObject,
                                        &context );

    if (NT_SUCCESS( status )) {

        if (context->RescanRequired) {

            (VOID) ScannerpScanFileInUserMode( FltObjects->Instance,
                                               FltObjects->FileObject,
                                               &safe );

            if (!safe) {

                DbgPrint( "!!! scanner.sys -- foul language detected in precleanup !!!\n" );
            }
        }

        FltReleaseContext( context );
    }


    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

/*++
函数说明:
	写入前操作回调。我们想知道现在写入了什么。
    Pre write callback.  We want to scan what's being written now.
Arguments:
    Data - The structure which describes the operation parameters.
    FltObject - The structure which describes the objects affected by this
        operation.
    CompletionContext - Output parameter which can be used to pass a context
        from this pre-write callback to the post-write callback.
Return Value:
    Always FLT_PREOP_SUCCESS_NO_CALLBACK.
--*/
FLT_PREOP_CALLBACK_STATUS
ScannerPreWrite (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
    FLT_PREOP_CALLBACK_STATUS returnStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    NTSTATUS status;
    PSCANNER_NOTIFICATION notification = NULL;
    PSCANNER_STREAM_HANDLE_CONTEXT context = NULL;
    ULONG replyLength;
    BOOLEAN safe = TRUE;
    PUCHAR buffer;

    UNREFERENCED_PARAMETER( CompletionContext );

    //
    //  If not client port just ignore this write.
    //

    if (ScannerData.ClientPort == NULL) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    status = FltGetStreamHandleContext( FltObjects->Instance,
                                        FltObjects->FileObject,
                                        &context );

    if (!NT_SUCCESS( status )) {

        //
        //  We are not interested in this file
        //

        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    }

    //
    //  Use try-finally to cleanup
    //

    __try {

        //
        //  Pass the contents of the buffer to user mode.
		//  将缓冲区的内容传送到用户模式
        //

        if (Data->Iopb->Parameters.Write.Length != 0) {

            //
            //  Get the users buffer address.  If there is a MDL defined, use
            //  it.  If not use the given buffer address.
			//  获取用户缓冲区地址。如果MDL被定义，用它。如果没有则提供缓冲区地址。
            //

            if (Data->Iopb->Parameters.Write.MdlAddress != NULL) {

				// returns a nonpaged system-space virtual address for the buffer that the specified MDL describes
                buffer = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.Write.MdlAddress,
                                                       NormalPagePriority );

                //
                //  If we have a MDL but could not get and address, we ran out
                //  of memory, report the correct error
				//  如果我们有一个MDL，但是不能获取到他的地址，我们耗尽内存，报告一个恰当的错误。
                //

                if (buffer == NULL) {

                    Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                    Data->IoStatus.Information = 0;
                    returnStatus = FLT_PREOP_COMPLETE;
                    __leave;
                }

            } else {

                //
                //  Use the users buffer
                //

                buffer  = Data->Iopb->Parameters.Write.WriteBuffer;
            }

            //
            //  In a production-level filter, we would actually let user mode scan the file directly.
            //  Allocating & freeing huge amounts of non-paged pool like this is not very good for system perf.
            //  This is just a sample!
			//  在一个商业级的过滤器中，我们会直接在用户模式扫描文件。
			//  申请和释放大量非分页内存池对系统性能并不是非常好。
			//  这仅仅是一个例子。
            //

            notification = ExAllocatePoolWithTag( NonPagedPool,
                                                  sizeof( SCANNER_NOTIFICATION ),
                                                  'nacS' );
            if (notification == NULL) {

                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                returnStatus = FLT_PREOP_COMPLETE;
                __leave;
            }

            notification->BytesToScan = min( Data->Iopb->Parameters.Write.Length, SCANNER_READ_BUFFER_SIZE );

            //
            //  The buffer can be a raw user buffer. Protect access to it
			//  这个缓冲区是一个原始用户缓冲区，保护对它的访问。
            //

            __try  {

                RtlCopyMemory( &notification->Contents,
                               buffer,
                               notification->BytesToScan );
				DbgPrint("!!! scanner.sys --- notification->BytesToScan : %d\n", notification->BytesToScan);

            } __except( EXCEPTION_EXECUTE_HANDLER ) {

                //
                //  Error accessing buffer. Complete i/o with failure
				//  错误的缓冲区访问。IO操作失败。
                //

                Data->IoStatus.Status = GetExceptionCode() ;
                Data->IoStatus.Information = 0;
                returnStatus = FLT_PREOP_COMPLETE;
                __leave;
            }

            //
            //  Send message to user mode to indicate it should scan the buffer.
            //  We don't have to synchronize between the send and close of the handle
            //  as FltSendMessage takes care of that.
			//  发送消息到用户模式表明它应该扫描缓冲区。
			//  我们没有必要对发送和关闭句柄进行同步。FltSendMessage会关心这些东西。（不知道是不是这样翻译）
            //

            replyLength = sizeof( SCANNER_REPLY );

            status = FltSendMessage( ScannerData.Filter,
                                     &ScannerData.ClientPort,
                                     notification,
                                     sizeof( SCANNER_NOTIFICATION ),
                                     notification,
                                     &replyLength,
                                     NULL );

            if (STATUS_SUCCESS == status) {

               safe = ((PSCANNER_REPLY) notification)->SafeToOpen;

           } else {

               //
               //  Couldn't send message. This sample will let the i/o through.
               //

               DbgPrint( "!!! scanner.sys --- couldn't send message to user-mode to scan file, status 0x%X\n", status );
           }
        }

        if (!safe) {

            //
            //  Block this write if not paging i/o (as a result of course, this scanner will not prevent memory mapped writes of contaminated
            //  strings to the file, but only regular writes). The effect of getting ERROR_ACCESS_DENIED for many apps to delete the file they
            //  are trying to write usually.
            //  To handle memory mapped writes - we should be scanning at close time (which is when we can really establish that the file object
            //  is not going to be used for any more writes)
            //

            DbgPrint( "!!! scanner.sys -- foul language detected in write !!!\n" );

            if (!FlagOn( Data->Iopb->IrpFlags, IRP_PAGING_IO )) {

                DbgPrint( "!!! scanner.sys -- blocking the write !!!\n" );

                Data->IoStatus.Status = STATUS_ACCESS_DENIED;
                Data->IoStatus.Information = 0;
                returnStatus = FLT_PREOP_COMPLETE;
            }
        }

    } __finally {

        if (notification != NULL) {

            ExFreePoolWithTag( notification, 'nacS' );
        }

        if (context) {

            FltReleaseContext( context );
        }
    }

    return returnStatus;
}

//////////////////////////////////////////////////////////////////////////
//  Local support routines.
//
/////////////////////////////////////////////////////////////////////////
/*++
函数说明:
	这个程序为了发送一个请求而被调用。
	这个请求是关于告诉用户模式扫描这个指定的文件并告诉调用者，不论打开这个文档是否安全。

	注意：如果扫描失败，我们设置SafeToOpen为“真”。扫描失败可能是因为服务还没启动，
	或者可能是因为这个create/cleanup操作是针对一个目录的，并且没有数据去读取/扫描。

	如果我们的扫描在服务没启动的时候失败，会出现一个问题——我们如何将这个.exe以一个服务来加载。

    This routine is called to send a request up to user mode to scan a given
    file and tell our caller whether it's safe to open this file.

    Note that if the scan fails, we set SafeToOpen to TRUE.  The scan may fail
    because the service hasn't started, or perhaps because this create/cleanup
    is for a directory, and there's no data to read & scan.

    If we failed creates when the service isn't running, there'd be a
    bootstrapping problem -- how would we ever load the .exe for the service?
参数表:
    Instance - Handle to the filter instance for the scanner on this volume.
    FileObject - File to be scanned.
    SafeToOpen - Set to FALSE if the file is scanned successfully and it contains
                 foul language.
Return Value:
    The status of the operation, hopefully STATUS_SUCCESS.  The common failure
    status will probably be STATUS_INSUFFICIENT_RESOURCES.
--*/
NTSTATUS
ScannerpScanFileInUserMode (
    __in PFLT_INSTANCE Instance,
    __in PFILE_OBJECT FileObject,
    __out PBOOLEAN SafeToOpen
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID buffer = NULL;
    ULONG bytesRead;
    PSCANNER_NOTIFICATION notification = NULL;
    FLT_VOLUME_PROPERTIES volumeProps;
    LARGE_INTEGER offset;
    ULONG replyLength, length;
    PFLT_VOLUME volume = NULL;

    *SafeToOpen = TRUE;

    //
    //  If not client port just return.
    //

    if (ScannerData.ClientPort == NULL) {

        return STATUS_SUCCESS;
    }

    __try {

        //
        //  Obtain the volume object .
		//  获得卷对象
        //

        status = FltGetVolumeFromInstance( Instance, &volume );

        if (!NT_SUCCESS( status )) {

            __leave;
        }

        //
        //  Determine sector size. Noncached I/O can only be done at sector size offsets, and in lengths which are
        //  multiples of sector size. A more efficient way is to make this call once and remember the sector size in the
        //  instance setup routine and setup an instance context where we can cache it.
        //

        status = FltGetVolumeProperties( volume,
                                         &volumeProps,
                                         sizeof( volumeProps ),
                                         &length );
        //
        //  STATUS_BUFFER_OVERFLOW can be returned - however we only need the properties, not the names
        //  hence we only check for error status.
        //

        if (NT_ERROR( status )) {

            __leave;
        }

        length = max( SCANNER_READ_BUFFER_SIZE, volumeProps.SectorSize );

        //
        //  Use non-buffered i/o, so allocate aligned pool
        //

        buffer = FltAllocatePoolAlignedWithTag( Instance,
                                                NonPagedPool,
                                                length,
                                                'nacS' );

        if (NULL == buffer) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        notification = ExAllocatePoolWithTag( NonPagedPool,
                                              sizeof( SCANNER_NOTIFICATION ),
                                              'nacS' );

        if(NULL == notification) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        //
        //  Read the beginning of the file and pass the contents to user mode.
        //

        offset.QuadPart = bytesRead = 0;
        status = FltReadFile( Instance,
                              FileObject,
                              &offset,
                              length,
                              buffer,
                              FLTFL_IO_OPERATION_NON_CACHED |
                               FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
                              &bytesRead,
                              NULL,
                              NULL );

        if (NT_SUCCESS( status ) && (0 != bytesRead)) {

            notification->BytesToScan = (ULONG) bytesRead;

            //
            //  Copy only as much as the buffer can hold
            //

            RtlCopyMemory( &notification->Contents,
                           buffer,
                           min( notification->BytesToScan, SCANNER_READ_BUFFER_SIZE ) );

            replyLength = sizeof( SCANNER_REPLY );

            status = FltSendMessage( ScannerData.Filter,
                                     &ScannerData.ClientPort,
                                     notification,
                                     sizeof(SCANNER_NOTIFICATION),
                                     notification,
                                     &replyLength,
                                     NULL );

            if (STATUS_SUCCESS == status) {

                *SafeToOpen = ((PSCANNER_REPLY) notification)->SafeToOpen;

            } else {

                //
                //  Couldn't send message
                //

                DbgPrint( "!!! scanner.sys --- couldn't send message to user-mode to scan file, status 0x%X\n", status );
            }
        }

    } __finally {

        if (NULL != buffer) {

            FltFreePoolAlignedWithTag( Instance, buffer, 'nacS' );
        }

        if (NULL != notification) {

            ExFreePoolWithTag( notification, 'nacS' );
        }

        if (NULL != volume) {

            FltObjectDereference( volume );
        }
    }

    return status;
}

