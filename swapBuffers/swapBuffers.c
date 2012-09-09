/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    SwapBuffers.c

Abstract:
	这是一个过滤器例子，它演示了如何适当的访问缓冲区的数据和关于交换缓冲区的通用指引。

	现在，只是交换缓冲区：

	IRP_MJ_READ
	IRP_MJ_WRITE
	IRP_MJ_DIRECTORY_CONTROL

	默认情况下这个过滤器附加到所有被通知的卷。它支持在一个给定的卷上拥有多个实例。

    This is a sample filter which demonstrates proper access of data buffer
    and a general guideline of how to swap buffers.
    For now it only swaps buffers for:

    IRP_MJ_READ
    IRP_MJ_WRITE
    IRP_MJ_DIRECTORY_CONTROL

    By default this filter attaches to all volumes it is notified about.  It
    does support having multiple instances on a given volume.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER gFilterHandle;

/*************************************************************************
    Pool Tags 内存标记
*************************************************************************/

#define BUFFER_SWAP_TAG     'bdBS'
#define CONTEXT_TAG         'xcBS'
#define NAME_TAG            'mnBS'
#define PRE_2_POST_TAG      'ppBS'

/*************************************************************************
    Local structures 本地结构
*************************************************************************/

//
//  这是一个卷上下文，其中一个被附加到我们监视的卷上。
//  这是为了获取一个DOS名称在调式过程中显示
//  
//  This is a volume context, one of these are attached to each volume
//  we monitor.  This is used to get a "DOS" name for debug display.
//

typedef struct _VOLUME_CONTEXT {

    //
	//  保村这个名称用于显示
    //  Holds the name to display
    //

    UNICODE_STRING Name;

    //
	//  保存这个卷的扇区大小
    //  Holds the sector size for this volume.
    //

    ULONG SectorSize;

} VOLUME_CONTEXT, *PVOLUME_CONTEXT;

#define MIN_SECTOR_SIZE 0x200


//
//  这是一个用来将前操作状态传递到后操作回调的上下文结构。
//
//  This is a context structure that is used to pass state from our
//  pre-operation callback to our post-operation callback.
//

typedef struct _PRE_2_POST_CONTEXT {

    //
	//  指向我们的卷上下文结构。
	//  我们总是在前操作获取上下文，因为你不能在DPC级安全的获取到它。
	//  我们接下来在后操作中释放它，在DPC级别中释放它是安全的。
	//  
    //  Pointer to our volume context structure.  We always get the context
    //  in the preOperation path because you can not safely get it at DPC
    //  level.  We then release it in the postOperation path.  It is safe
    //  to release contexts at DPC level.
    //

    PVOLUME_CONTEXT VolCtx;

    //
	//  [第一句不会翻译，这里直接阅读原文体会意思吧！]
	//  我们需要传我们新的目标缓冲区给我们的后操作函数，所以我们可以释放它。
	//  
    //  Since the post-operation parameters always receive the "original"
    //  parameters passed to the operation, we need to pass our new destination
    //  buffer to our post operation routine so we can free it.
    //

    PVOID SwappedBuffer;

} PRE_2_POST_CONTEXT, *PPRE_2_POST_CONTEXT;

//
//  这是一个Lookaside链表，用于分配我们的PRE_2_POST_CONTEXT上下文结构内存。
//  This is a lookAside list used to allocate our pre-2-post structure.
//

NPAGED_LOOKASIDE_LIST Pre2PostContextList;

/*************************************************************************
    Prototypes 函数原型
*************************************************************************/

NTSTATUS
InstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
CleanupVolumeContext(
    __in PFLT_CONTEXT Context,
    __in FLT_CONTEXT_TYPE ContextType
    );

NTSTATUS
InstanceQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

DRIVER_INITIALIZE DriverEn__try;
NTSTATUS
DriverEn__try (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING Regis__tryPath
    );

NTSTATUS
FilterUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreReadBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostReadBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostReadBuffersWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostDirCtrlBuffersWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreWriteBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostWriteBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

VOID
ReadDriverParameters (
    __in PUNICODE_STRING Regis__tryPath
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, InstanceSetup)
#pragma alloc_text(PAGE, CleanupVolumeContext)
#pragma alloc_text(PAGE, InstanceQueryTeardown)
#pragma alloc_text(INIT, DriverEn__try)
#pragma alloc_text(INIT, ReadDriverParameters)
#pragma alloc_text(PAGE, FilterUnload)
#endif

//
//  我们关心的操作
//  Operation we currently care about.
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_READ,
      0,
      SwapPreReadBuffers,
      SwapPostReadBuffers },

    { IRP_MJ_WRITE,
      0,
      SwapPreWriteBuffers,
      SwapPostWriteBuffers },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      SwapPreDirCtrlBuffers,
      SwapPostDirCtrlBuffers },

    { IRP_MJ_OPERATION_END }
};

//
//  定义我们当前关心的上下文。
//  注意：系统会创建一个Lookaside链表给卷上下文。因为上下文的大小被显示的指定了。
//  
//  Context definitions we currently care about.  Note that the system will
//  create a lookAside list for the volume context because an explicit size
//  of the context is specified.
//

CONST FLT_CONTEXT_REGISTRATION ContextNotifications[] = {

     { FLT_VOLUME_CONTEXT,
       0,
       CleanupVolumeContext,
       sizeof(VOLUME_CONTEXT),
       CONTEXT_TAG },

     { FLT_CONTEXT_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    ContextNotifications,               //  Context
    Callbacks,                          //  Operation callbacks

    FilterUnload,                       //  MiniFilterUnload

    InstanceSetup,                      //  InstanceSetup
    InstanceQueryTeardown,              //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};

/*************************************************************************
    Debug tracing information
*************************************************************************/

//
//  Definitions to display log messages.  The regis__try DWORD en__try:
//  "hklm\system\CurrentControlSet\Services\Swapbuffers\DebugFlags" defines
//  the default state of these logging flags
//

#define LOGFL_ERRORS    0x00000001  // if set, display error messages
#define LOGFL_READ      0x00000002  // if set, display READ operation info
#define LOGFL_WRITE     0x00000004  // if set, display WRITE operation info
#define LOGFL_DIRCTRL   0x00000008  // if set, display DIRCTRL operation info
#define LOGFL_VOLCTX    0x00000010  // if set, display VOLCTX operation info

ULONG LoggingFlags = 0;             // all disabled by default

#define LOG_PRINT( _logFlag, _string )                              \
    (FlagOn(LoggingFlags,(_logFlag)) ?                              \
        DbgPrint _string  :                                         \
        ((int)0))

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//                      Routines
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


/*++
Routine Description:
	
	当一个卷上创建一个新的实例的时候这个函数会被调用。

	默认情况下我们希望附加到所有卷上。这个函数将尝试获取指定卷的DOS名。如果获取不到，我们将
	尝试获取这个卷的NT名(可能在网络卷上出现)。如果名称被检索到，一个卷上下文将以这个名称创建。

    This routine is called whenever a new instance is created on a volume.

    By default we want to attach to all volumes.  This routine will try and
    get a "DOS" name for the given volume.  If it can't, it will try and
    get the "NT" name for the volume (which is what happens on network
    volumes).  If a name is retrieved a volume context will be created with
    that name.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
NTSTATUS
InstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
    PDEVICE_OBJECT devObj = NULL;
    PVOLUME_CONTEXT ctx = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG retLen;
    PUNICODE_STRING workingName;
    USHORT size;
    UCHAR volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES)+512];
    PFLT_VOLUME_PROPERTIES volProp = (PFLT_VOLUME_PROPERTIES)volPropBuffer;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    __try
	{
		//  初始化卷上下文结构
        //  Allocate a volume context structure.
        status = FltAllocateContext(FltObjects->Filter, FLT_VOLUME_CONTEXT, sizeof(VOLUME_CONTEXT), NonPagedPool, &ctx);
        if (!NT_SUCCESS(status))
            __leave;

        //  总是获取卷属性，然后我们能获取扇区大小
        //  Always get the volume properties, so I can get a sector size
        status = FltGetVolumeProperties(FltObjects->Volume, volProp, sizeof(volPropBuffer), &retLen);
        if (!NT_SUCCESS(status))
            __leave;

        //  将扇区大小保存到卷上下文，备以后使用。
		//  注意：如果一个扇区大小没有被指定，我们将选择最小的扇区大小。
        //  Save the sector size in the context for later use.  Note that
        //  we will pick a minimum sector size if a sector size is not
        //  specified.
        ASSERT((volProp->SectorSize == 0) || (volProp->SectorSize >= MIN_SECTOR_SIZE));
        ctx->SectorSize = max(volProp->SectorSize,MIN_SECTOR_SIZE); // 哪个大选哪个

        //  初始化缓冲区字段（稍后将要分配内存的）
        //  Init the buffer field (which may be allocated later).
		ctx->Name.Buffer = NULL;

        //  获取我们想要获取名称的存储设备对象
        //  Get the storage device object we want a name for.
        status = FltGetDiskDeviceObject( FltObjects->Volume, &devObj );
        if (NT_SUCCESS(status)) 
		{
            //  尝试获取DOS名称。如果成功，则分配内存给名字缓冲区。如果不成功，则设置为NULL。
            //  try and get the DOS name.  If it succeeds we will have
            //  an allocated name buffer.  If not, it will be NULL
#pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "Used to maintain compatability with Win 2k")
            status = RtlVolumeDeviceToDosName( devObj, &ctx->Name );
        }

        //  如果我们不能获取DOS名，获取NT名
        //  If we could not get a DOS name, get the NT name.
        if (!NT_SUCCESS(status))
		{
            ASSERT(ctx->Name.Buffer == NULL);

            //  从属性中找出使用的名字
            //  Figure out which name to use from the properties
            if (volProp->RealDeviceName.Length > 0)
                workingName = &volProp->RealDeviceName;
            else if (volProp->FileSystemDeviceName.Length > 0)
                workingName = &volProp->FileSystemDeviceName;
            else 
			{
                //  没有名字，不保存上下文
                //  No name, don't save the context
                status = STATUS_FLT_DO_NOT_ATTACH;
                __leave;
            }

            //  用缓冲区大小申请内存。这个长度包含了字符串长度加上结尾的冒号。
            //  Get size of buffer to allocate.  This is the length of the
            //  string plus room for a trailing colon.
            size = workingName->Length + sizeof(WCHAR);

            //  现在申请内存区保存名称
            //  Now allocate a buffer to hold this name
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "ctx->Name.Buffer will not be leaked because it is freed in CleanupVolumeContext")
            ctx->Name.Buffer = ExAllocatePoolWithTag( NonPagedPool, size, NAME_TAG );
            if (ctx->Name.Buffer == NULL) 
			{
                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            //  初始化其他字段
            //  Init the rest of the fields
            ctx->Name.Length = 0;
            ctx->Name.MaximumLength = size;

            //  复制名字
            //  Copy the name in
            RtlCopyUnicodeString( &ctx->Name,
                                  workingName );

            //  加一个冒号，看上去比较好
            //  Put a trailing colon to make the display look good
            RtlAppendUnicodeToString( &ctx->Name, L":" );
        }

        //  设置上下文
        //  Set the context
        status = FltSetVolumeContext( FltObjects->Volume, FLT_SET_CONTEXT_KEEP_IF_EXISTS, ctx, NULL );

        //  记录调试日志
        //  Log debug info
        LOG_PRINT( LOGFL_VOLCTX,
                   ("SwapBuffers!InstanceSetup:                  Real SectSize=0x%04x, Used SectSize=0x%04x, Name=\"%wZ\"\n",
                    volProp->SectorSize,
                    ctx->SectorSize,
                    &ctx->Name) );

        //  如果已经设置了上下文，则设置错误码为STATUS_SUCCESS
        //  It is OK for the context to already be defined.
        //

        if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED)
            status = STATUS_SUCCESS;

    } 
	__finally 
	{

        //  总是释放这个上下文。如果设置失败，将释放上下文。反之，将移除这个set添加的引用。
		//  注意：ctx中的名字缓冲区将被上下文中的清理函数释放。
        //  Always release the context.  If the set failed, it will free the
        //  context.  If not, it will remove the reference added by the set.
        //  Note that the name buffer in the ctx will get freed by the context
        //  cleanup routine.
        if (ctx)
            FltReleaseContext( ctx );

        //  移除FltGetDiskDeviceObject添加到设备对象的引用
        //  Remove the reference added to the device object by
        //  FltGetDiskDeviceObject.
        if (devObj)
            ObDereferenceObject( devObj );
    }

    return status;
}

/*++

Routine Description:

    The given context is being freed.
    Free the allocated name buffer if there one.

Arguments:

    Context - The context being freed

    ContextType - The type of context this is

Return Value:

    None

--*/
VOID
CleanupVolumeContext(
    __in PFLT_CONTEXT Context,
    __in FLT_CONTEXT_TYPE ContextType
    )
{
    PVOLUME_CONTEXT ctx = Context;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( ContextType );

    ASSERT(ContextType == FLT_VOLUME_CONTEXT);

    if (ctx->Name.Buffer != NULL) {

        ExFreePool(ctx->Name.Buffer);
        ctx->Name.Buffer = NULL;
    }
}

/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach.  We always return it is OK to
    detach.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Always succeed.

--*/
NTSTATUS
InstanceQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    return STATUS_SUCCESS;
}


/*************************************************************************
    Initialization and unload routines.
*************************************************************************/

/*++

Routine Description:

    This is the initialization routine.  This registers with FltMgr and
    initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    Regis__tryPath - Unicode string identifying where the parameters for this
        driver are located in the regis__try.

Return Value:

    Status of the operation

--*/
NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;

    //  获取调试跟踪标志
    //  Get debug trace flags
    ReadDriverParameters( RegistryPath );

    //  申请Lookaside结构
    //  Init lookaside list used to allocate our context structure used to
    //  pass information from out preOperation callback to our postOperation
    //  callback.
    ExInitializeNPagedLookasideList( &Pre2PostContextList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(PRE_2_POST_CONTEXT),
                                     PRE_2_POST_TAG,
                                     0 );

    //  注册过滤器
    //  Register with FltMgr
    status = FltRegisterFilter( DriverObject, &FilterRegistration, &gFilterHandle );
    if (! NT_SUCCESS( status ))
        goto SwapDriverEntryExit;

    //  开始过滤IO
    //  Start filtering i/o
    status = FltStartFiltering( gFilterHandle );
    if (! NT_SUCCESS( status ))
	{
        FltUnregisterFilter( gFilterHandle );
        goto SwapDriverEntryExit;
    }

SwapDriverEntryExit:

    if(! NT_SUCCESS( status ))
        ExDeleteNPagedLookasideList( &Pre2PostContextList );

    return status;
}


/*++

Routine Description:

    Called when this mini-filter is about to be unloaded.  We unregister
    from the FltMgr and then return it is OK to unload

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns the final status of this operation.

--*/
NTSTATUS
FilterUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( Flags );

    //  从过滤管理器反注册
    //  Unregister from FLT mgr
    FltUnregisterFilter( gFilterHandle );

    //  删除Lookaside链表
    //  Delete lookaside list
    ExDeleteNPagedLookasideList( &Pre2PostContextList );

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/

/*++

Routine Description:

	这个函数展示了再读取操作是如何交换缓冲区的。
	注意：注意,它处理所有的错误,只要不做缓冲互换。
    This routine demonstrates how to swap buffers for the READ operation.

    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
FLT_PREOP_CALLBACK_STATUS
SwapPreReadBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	// IO参数块
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOID newBuf = NULL;	// 用于交换的内存缓冲区
    PMDL newMdl = NULL;
    PVOLUME_CONTEXT volCtx = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;	// 用于交换的上下文，其中包含了PVOLUME_CONTEXT卷上下文结构
    NTSTATUS status;
    ULONG readLen = iopb->Parameters.Read.Length;

    __try 
	{
        //  如果尝试读0字节，我们不做任何事情，我们不需要后操作回调。
        //  If they are trying to read ZERO bytes, then don't do anything and
        //  we don't need a post-operation callback.
        if (readLen == 0)
            __leave;

        //  获取我们卷上下文，我们能在调试输出信息显示卷名称
        //  Get our volume context so we can display our volume name in the
        //  debug output.
        status = FltGetVolumeContext( FltObjects->Filter, FltObjects->Volume, &volCtx );
        if (!NT_SUCCESS(status)) 
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreReadBuffers:             Error getting volume context, status=%x\n",
                        status) );
            __leave;
        }

        //  如果这是一个非缓冲IO，我们需要完成设备的扇区长度。
		//  我们必须做这一步，因为文件系统做这些并且我们必须确保我们的缓冲区能满足它期望的大小。
        //  If this is a non-cached I/O we need to round the length up to the
        //  sector size for this device.  We must do this because the file
        //  systems do this and we need to make sure our buffer is as big
        //  as they are expecting.
		
		//  检测 iopb->IrpFlags 中是否存在 IRP_NOCACHE 标志位
        if (FlagOn(IRP_NOCACHE,iopb->IrpFlags))
			// 对齐长度
            readLen = (ULONG)ROUND_TO_SIZE(readLen,volCtx->SectorSize);

        //  给我们要交换的缓冲区分配非分页内存。
		//  如果我们获取内存失败，那么在这个操作我们不交换缓冲区
        //  Allocate nonPaged memory for the buffer we are swapping to.
        //  If we fail to get the memory, just don't swap buffers on this
        //  operation.
        newBuf = ExAllocatePoolWithTag( NonPagedPool, readLen, BUFFER_SWAP_TAG );
        if (newBuf == NULL) 
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreReadBuffers:             %wZ Failed to allocate %d bytes of memory\n",
                        &volCtx->Name,
                        readLen) );
            __leave;
        }

        //  我们仅仅需要给IRP操作建立一个MDL（内存描述符表）。
		//  我们在FASTIO(快速IO)操作中不需要做这些因为FASTIO接口没有参数用来将MDL传入文件系统。
        //  We only need to build a MDL for IRP operations.  We don't need to
        //  do this for a FASTIO operation since the FASTIO interface has no
        //  parameter for passing the MDL to the file system.

		// FLTFL_CALLBACK_DATA_IRP_OPERATION 这个回调数据描述一个IRP操作
        if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_IRP_OPERATION))
		{
            //  为一个MDL分配一个新的已申请的内存。
			//  如果分配失败了则这个操作不进行缓冲区交换。
            //  Allocate a MDL for the new allocated memory.  If we fail
            //  the MDL allocation then we won't swap buffer for this operation

			// 在IRP中为一个内存创建一个MDL，返回MDL的地址
            newMdl = IoAllocateMdl( newBuf, readLen, FALSE, FALSE, NULL );
            if (newMdl == NULL) 
			{
                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPreReadBuffers:             %wZ Failed to allocate MDL\n",
                            &volCtx->Name) );
                __leave;
            }

            //  在刚才分配的非分页内存中安装MDL
            //  setup the MDL for the non-paged pool we just allocated

			// 传入MDL，更新MDL来描述指定虚拟内存的实际物理地址
            MmBuildMdlForNonPagedPool( newMdl );
        }

        //  我们已经准备好交换缓冲区，获取前-后操作传递上下文结构。
		//  我们需要它传递卷上下文和申请的内存缓冲区到后操作回调函数。
        //  We are ready to swap buffers, get a pre2Post context structure.
        //  We need it to pass the volume context and the allocate memory
        //  buffer to the post operation callback.

		// 从Pre2PostContextList中申请非分页内存。
		// 返回一个指针，Pre2PostContextList已经在DriverEntry中申请了。
        p2pCtx = ExAllocateFromNPagedLookasideList( &Pre2PostContextList );
        if (p2pCtx == NULL) 
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreReadBuffers:             %wZ Failed to allocate pre2Post context structure\n",
                        &volCtx->Name) );
            __leave;
        }

        //  
        //  Log that we are swapping
        LOG_PRINT( LOGFL_READ,
                   ("SwapBuffers!SwapPreReadBuffers:             %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d\n",
                    &volCtx->Name,
                    newBuf,
                    newMdl,
                    iopb->Parameters.Read.ReadBuffer,
                    iopb->Parameters.Read.MdlAddress,
                    readLen) );

        //  更新缓冲区指针和MDL地址，记录我们改变了一些东西
        //  Update the buffer pointers and MDL address, mark we have changed
        //  something.
        iopb->Parameters.Read.ReadBuffer = newBuf;
        iopb->Parameters.Read.MdlAddress = newMdl;

		//操作中改变数据 需要调用此函数设置数据为dirty提醒系统进行数据更新
        FltSetCallbackDataDirty( Data );

        //  传递状态到后操作回调
        //  Pass state to our post-operation callback.
        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = volCtx;

		// =====================================================
		// 注意这里，将用于传递的上下文指针赋给CompletionContext
        *CompletionContext = p2pCtx;
		// =====================================================

        //  返回我们希望进入后操作回调
        //  Return we want a post-operation callback
        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    } __finally {

        //  如果我们不希望进入后操作回调，则清理状态。
        //  If we don't want a post-operation callback, then cleanup state.
        if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {

            if (newBuf != NULL) {

                ExFreePool( newBuf );
            }

            if (newMdl != NULL) {

                IoFreeMdl( newMdl );
            }

            if (volCtx != NULL) {

                FltReleaseContext( volCtx );
            }
        }
    }

    return retValue;
}


/*++

Routine Description:

	这个函数处理读后缓冲区交换
    This routine does postRead buffer swap handling

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING
    FLT_POSTOP_MORE_PROCESSING_REQUIRED

--*/
FLT_POSTOP_CALLBACK_STATUS
SwapPostReadBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    PVOID origBuf;
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    BOOLEAN cleanupAllocatedBuffer = TRUE;

    //  不会翻译，求高手。。。
    //  This system won't draining an operation with swapped buffers, verify
    //  the draining flag is not set.
	//  当一个实例被卸除的时候，过滤管理器可能调用候后操作回调，但是此时操作还未真的完成。这时，标志FLTFL_POST_OPERATION_DRAINING会设置。
    ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));

    __try 
	{
        //  如果操作失败或者长度为0，没有数据被复制，直接返回。
        //  If the operation failed or the count is zero, there is no data to
        //  copy so just return now.
        if (!NT_SUCCESS(Data->IoStatus.Status) || (Data->IoStatus.Information == 0)) 
		{
            LOG_PRINT( LOGFL_READ,
                       ("SwapBuffers!SwapPostReadBuffers:            %wZ newB=%p No data read, status=%x, info=%x\n",
                        &p2pCtx->VolCtx->Name,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Status,
                        Data->IoStatus.Information) );
            __leave;
        }

        //  我们需要复制如出来的数据到用户缓冲区。
		//  注意：传入的参数应该是用户原始缓冲，而不是我们的交换缓冲。
        //  We need to copy the read data back into the users buffer.  Note
        //  that the parameters passed in are for the users original buffers
        //  not our swapped buffers.
        if (iopb->Parameters.Read.MdlAddress != NULL)
		{
            //  这里有一个MDL为原始缓冲定义，获取它的系统地址，我们就可以复制数据到里面了。
			//  我们必须做这个，因为我们不知道我们进入的是哪个线程上下文。
            //  There is a MDL defined for the original buffer, get a
            //  system address for it so we can copy the data back to it.
            //  We must do this because we don't know what thread context
            //  we are in.

			// 根据MDL描述返回一个非分页系统缓冲区
            origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Read.MdlAddress, NormalPagePriority );
            if (origBuf == NULL) 
			{
                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPostReadBuffers:            %wZ Failed to get system address for MDL: %p\n",
                            &p2pCtx->VolCtx->Name,
                            iopb->Parameters.Read.MdlAddress) );

                //  如果我们获取SYSTEM地址失败，记录读取失败并返回。
                //  If we failed to get a SYSTEM address, mark that the read
                //  failed and return.
                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                __leave;
            }
        } 
		else if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||
                   FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_FAST_IO_OPERATION)) 
		{
            //  如果这是一个系统缓冲，那么直接用它，因为他在所有的线程上下文都有效。
			//  如果这是一个快速IO操作，我们只能在__try/__except块中使用因为我们知道我们在正确的线程上下文
			//  我们不能挂起快速IO
            //  If this is a system buffer, just use the given address because
            //      it is valid in all thread contexts.
            //  If this is a FASTIO operation, we can just use the
            //      buffer (inside a __try/__except) since we know we are in
            //      the correct thread context (you can't pend FASTIO's).
            origBuf = iopb->Parameters.Read.ReadBuffer;

        } 
		else
		{
            //  他们没有MDL并且这不是一个系统缓冲或者快速IO，因此这可能是一些专用的用户缓冲。
			//  我们不能再DPC级别做这些操作，因此尝试获取一个安全的IRQL让我们去做这些操作。
            //  They don't have a MDL and this is not a system buffer
            //  or a fastio so this is probably some arbitrary user
            //  buffer.  We can not do the processing at DPC level so
            //  try and get to a safe IRQL so we can do the processing.

			// 如果能安全运行，则在后操作完成后调用SwapPostReadBuffersWhenSafe()这个回调函数
            if (FltDoCompletionProcessingWhenSafe( Data,
                                                   FltObjects,
                                                   CompletionContext,
                                                   Flags,
                                                   SwapPostReadBuffersWhenSafe,
                                                   &retValue )) 
			{
                //  这个操作已经被转移到一个安全的IRQL
				//  被调用的函数将执行或已经执行完毕，不用在我们的函数中进行。(这里翻译的肯定不对，求大神更正翻译)
                //  This operation has been moved to a safe IRQL, the called
                //  routine will do (or has done) the freeing so don't do it
                //  in our routine.
                cleanupAllocatedBuffer = FALSE;
            } 
			else
			{
                //  我们在不能获取安全的IRQL和没有MDL的环境中。
				//  我们不能在这里啊暖的复制数据到用户缓冲区，让操作失败并返回。
				//  （最后一句懒得翻译，总之就是进到这里不好）
                //  We are in a state where we can not get to a safe IRQL and
                //  we do not have a MDL.  There is nothing we can do to safely
                //  copy the data back to the users buffer, fail the operation
                //  and return.  This shouldn't ever happen because in those
                //  situations where it is not safe to post, we should have
                //  a MDL.
                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPostReadBuffers:            %wZ Unable to post to a safe IRQL\n",
                            &p2pCtx->VolCtx->Name) );

                Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
                Data->IoStatus.Information = 0;
            }

            __leave;
        }

        //  我们在适当的上下文中要么有一个系统缓冲区，要么是一个快速IO操作。
		//  拷贝数据并处理异常。
        //  We either have a system buffer or this is a fastio operation
        //  so we are in the proper context.  Copy the data handling an
        //  exception.
        __try 
		{
			// 将传递上下文中的交换缓冲区内存拷贝到origBuf
            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           Data->IoStatus.Information );  // 这个是系统调用成功后保存的读出字节

        } 
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
            //  拷贝失败，返回一个错误。
            //  The copy failed, return an error, failing the operation.
            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPostReadBuffers:            %wZ Invalid user buffer, oldB=%p, status=%x\n",
                        &p2pCtx->VolCtx->Name,
                        origBuf,
                        Data->IoStatus.Status) );
        }

    } 
	__finally 
	{
        //  我们应该清理申请的内存，释放卷上下文。
		//  MDL的清理交由Fltmgr过滤管理器。
        //  If we are supposed to, cleanup the allocated memory and release
        //  the volume context.  The freeing of the MDL (if there is one) is
        //  handled by FltMgr.
        if (cleanupAllocatedBuffer)
		{

            LOG_PRINT( LOGFL_READ,
                       ("SwapBuffers!SwapPostReadBuffers:            %wZ newB=%p info=%d Freeing\n",
                        &p2pCtx->VolCtx->Name,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Information) );

            ExFreePool( p2pCtx->SwappedBuffer );
            FltReleaseContext( p2pCtx->VolCtx );

            ExFreeToNPagedLookasideList( &Pre2PostContextList,
                                         p2pCtx );
        }
    }

    return retValue;
}


/*++

Routine Description:

	我们有一个专用的没有MDL的用户缓冲区，因此我么需要获取一个安全的IRQL。
	于是我们可以锁定它然后复制数据。
    We had an arbitrary users buffer without a MDL so we needed to get
    to a safe IRQL so we could lock it and then copy the data.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Contains state from our PreOperation callback

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - This is always returned.

--*/
FLT_POSTOP_CALLBACK_STATUS
SwapPostReadBuffersWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    PVOID origBuf;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    ASSERT(Data->IoStatus.Information != 0);

    //  这是某种没有MDL的用户缓冲区，锁定这个缓冲区，我们就能访问它了。
	//  这里将为他创建一个MDL。
    //  This is some sort of user buffer without a MDL, lock the user buffer
    //  so we can access it.  This will create a MDL for it.
    status = FltLockUserBuffer( Data );
    if (!NT_SUCCESS(status)) 
	{
        LOG_PRINT( LOGFL_ERRORS,
                   ("SwapBuffers!SwapPostReadBuffersWhenSafe:    %wZ Could not lock user buffer, oldB=%p, status=%x\n",
                    &p2pCtx->VolCtx->Name,
                    iopb->Parameters.Read.ReadBuffer,
                    status) );

        //  如果不能锁定缓冲区，本操作失败。
        //  If we can't lock the buffer, fail the operation
        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;
    } 
	else
	{
        //  给缓冲区获取一个系统地址（为什么要这样翻译？）
        //  Get a system address for this buffer.
        origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Read.MdlAddress, NormalPagePriority );
        if (origBuf == NULL)
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPostReadBuffersWhenSafe:    %wZ Failed to get system address for MDL: %p\n",
                        &p2pCtx->VolCtx->Name,
                        iopb->Parameters.Read.MdlAddress) );

            //  如果我们不能获取系统缓冲区地址，本操作失败
            //  If we couldn't get a SYSTEM buffer address, fail the operation
            Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Data->IoStatus.Information = 0;

        } 
		else
		{
            //  将数据拷贝回原始缓冲区。
			//  注意：我们不需要try/except模块，因为我们总是有一个系统缓冲区地址
            //  Copy the data back to the original buffer.  Note that we
            //  don't need a try/except because we will always have a system
            //  buffer address.
            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           Data->IoStatus.Information );
        }
    }

    //  释放申请的内存，释放卷上下文
    //  Free allocated memory and release the volume context
    LOG_PRINT( LOGFL_READ,
               ("SwapBuffers!SwapPostReadBuffersWhenSafe:    %wZ newB=%p info=%d Freeing\n",
                &p2pCtx->VolCtx->Name,
                p2pCtx->SwappedBuffer,
                Data->IoStatus.Information) );

    ExFreePool( p2pCtx->SwappedBuffer );
    FltReleaseContext( p2pCtx->VolCtx );

    ExFreeToNPagedLookasideList( &Pre2PostContextList,
                                 p2pCtx );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


/*++

Routine Description:

	这个函数演示了再目录控制操作中如何去交换缓冲区。
	这个函数在这里的原因是目录更改通知是长寿命并且允许我看到过滤管理器处理长寿命IRP才做..（妈的，不翻译这里了，自己看英文）
    This routine demonstrates how to swap buffers for the Directory Control
    operations.  The reason this routine is here is because directory change
    notifications are long lived and this allows you to see how FltMgr
    handles long lived IRP operations that have swapped buffers when the
    mini-filter is unloaded.  It does this by canceling the IRP.

	注意：它简单的处理了所有没有做缓冲区交换的错误
    Note that it handles all errors by simply not doing the
    buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
FLT_PREOP_CALLBACK_STATUS
SwapPreDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOID newBuf = NULL;
    PMDL newMdl = NULL;
    PVOLUME_CONTEXT volCtx = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;
    NTSTATUS status;

    __try
	{
        //  如果他们尝试获取0字节，那么不做任何事情，并且我们不需要进入后操作回调
        //  If they are trying to get ZERO bytes, then don't do anything and
        //  we don't need a post-operation callback.
        if (iopb->Parameters.DirectoryControl.QueryDirectory.Length == 0)
            __leave;

        //  获取我们的卷上下文。如果获取不到，直接返回
        //  Get our volume context.  If we can't get it, just return.
        status = FltGetVolumeContext( FltObjects->Filter, FltObjects->Volume, &volCtx );

        if (!NT_SUCCESS(status))
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreDirCtrlBuffers:          Error getting volume context, status=%x\n",
                        status) );
            __leave;
        }

        //  给我们将要交换的缓冲区分配非分页内存。
		//  如果获取内存失败，此次操作不交换缓冲区
        //  Allocate nonPaged memory for the buffer we are swapping to.
        //  If we fail to get the memory, just don't swap buffers on this
        //  operation.
        newBuf = ExAllocatePoolWithTag( NonPagedPool,
                                        iopb->Parameters.DirectoryControl.QueryDirectory.Length,
                                        BUFFER_SWAP_TAG );

        if (newBuf == NULL)
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreDirCtrlBuffers:          %wZ Failed to allocate %d bytes of memory.\n",
                        &volCtx->Name,
                        iopb->Parameters.DirectoryControl.QueryDirectory.Length) );
            __leave;
        }

        //  我们需要建立一个MDL，因为目录控制操作总是操作IRP。
        //  We need to build a MDL because Directory Control Operations are always IRP operations.  


        //  申请新的内存给MDL。
		//  如果申请失败，此次操作不交换缓冲区。
		//  （其实这里翻译一直有问题，应该翻成“本次”还是“这个”呢？）
        //  Allocate a MDL for the new allocated memory.  If we fail
        //  the MDL allocation then we won't swap buffer for this operation
        newMdl = IoAllocateMdl( newBuf,
                                iopb->Parameters.DirectoryControl.QueryDirectory.Length,
                                FALSE,
                                FALSE,
                                NULL );

        if (newMdl == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreDirCtrlBuffers:          %wZ Failed to allocate MDL.\n",
                        &volCtx->Name) );

           __leave;
        }

        //  将MDL装载到我们刚刚分配的非分页内存池
        //  setup the MDL for the non-paged pool we just allocated

		// 更新这个MDL描述的虚拟内存对应的物理内存地址
        MmBuildMdlForNonPagedPool( newMdl );

        //  我们已经准备好交换缓冲区，获取前后操作传递上下文结构。
		//  我们需要他将卷上下文和申请的内存缓冲区传递到后操作。
        //  We are ready to swap buffers, get a pre2Post context structure.
        //  We need it to pass the volume context and the allocate memory
        //  buffer to the post operation callback.
        p2pCtx = ExAllocateFromNPagedLookasideList( &Pre2PostContextList );
        if (p2pCtx == NULL)
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreDirCtrlBuffers:          %wZ Failed to allocate pre2Post context structure\n",
                        &volCtx->Name) );
            __leave;
        }

        //  记录我们正在交换
        //  Log that we are swapping
        LOG_PRINT( LOGFL_DIRCTRL,
                   ("SwapBuffers!SwapPreDirCtrlBuffers:          %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d\n",
                    &volCtx->Name,
                    newBuf,
                    newMdl,
                    iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer,
                    iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                    iopb->Parameters.DirectoryControl.QueryDirectory.Length) );

        //  更新缓冲区指针和MDL地址
        //  Update the buffer pointers and MDL address
        iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer = newBuf;
        iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress = newMdl;
        FltSetCallbackDataDirty( Data );

        //  将状态传递给后操作
        //  Pass state to our post-operation callback.
        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = volCtx;

        *CompletionContext = p2pCtx;

        //  返回我们需要进入后操作标志
        //  Return we want a post-operation callback
        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    }
	__finally 
	{
        //  如果我们不需要后操作，清理状态
        //  If we don't want a post-operation callback, then cleanup state.
        if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {

            if (newBuf != NULL) {

                ExFreePool( newBuf );
            }

            if (newMdl != NULL) {

                IoFreeMdl( newMdl );
            }

            if (volCtx != NULL) {

                FltReleaseContext( volCtx );
            }
        }
    }

    return retValue;
}


/*++

Routine Description:

	这个函数在目录控制后操作处理缓冲区交换
    This routine does the post Directory Control buffer swap handling.

Arguments:

    This routine does postRead buffer swap handling
    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING
    FLT_POSTOP_MORE_PROCESSING_REQUIRED

--*/
FLT_POSTOP_CALLBACK_STATUS
SwapPostDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    PVOID origBuf;
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    BOOLEAN cleanupAllocatedBuffer = TRUE;

    //  
    //  Verify we are not draining an operation with swapped buffers
    ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));

    __try 
	{
        //  如果这个操作失败了或者计数为0，没有数据需要复制，直接返回
        //  If the operation failed or the count is zero, there is no data to
        //  copy so just return now.
        if (!NT_SUCCESS(Data->IoStatus.Status) || (Data->IoStatus.Information == 0)) 
		{
            LOG_PRINT( LOGFL_DIRCTRL,
                       ("SwapBuffers!SwapPostDirCtrlBuffers:         %wZ newB=%p No data read, status=%x, info=%x\n",
                        &p2pCtx->VolCtx->Name,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Status,
                        Data->IoStatus.Information) );
            __leave;
        }

        //  我们需要将读出来的数据拷贝会用户缓冲区。
		//  注意：传入的参数应该是用户的原始缓冲区，而不是我们的交换缓冲区。
        //  We need to copy the read data back into the users buffer.  Note
        //  that the parameters passed in are for the users original buffers
        //  not our swapped buffers
        if (iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress != NULL) 
		{
            //  用户缓冲区有一个MDL，给它获取一个系统地址，我们就可以往里面复制数据了。
			//  我们必须这么做，因为我们不知道我们现在在哪个线程上下文中。
            //  There is a MDL defined for the original buffer, get a
            //  system address for it so we can copy the data back to it.
            //  We must do this because we don't know what thread context
            //  we are in.
            origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                                                    NormalPagePriority );
            if (origBuf == NULL) 
			{
                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPostDirCtrlBuffers:         %wZ Failed to get system address for MDL: %p\n",
                            &p2pCtx->VolCtx->Name,
                            iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress) );

                //  如果我们获取系统地址失败，标记操作失败并返回。
                //  If we failed to get a SYSTEM address, mark that the
                //  operation failed and return.
                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                __leave;
            }

        }
		else if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) || FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
		{
            //  如果这是一个系统缓冲，直接用给出的地址，因为他在的所有线程上下文都是有效的。
			//  如果这是一个快速IO操作，我们仅仅在try/except模块中使用缓冲区，因为我们在正确的线程上下文
            //  If this is a system buffer, just use the given address because
            //      it is valid in all thread contexts.
            //  If this is a FASTIO operation, we can just use the
            //      buffer (inside a try/except) since we know we are in
            //      the correct thread context.
            origBuf = iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;

        } 
		else
		{
            //  没有MDL，不是系统缓冲，不是快速IO操作。因此可能是任意的某个用户缓冲。
			//  我们不能再DPC级别处理，因此尝试获取安全的IRQL去处理它们
            //  They don't have a MDL and this is not a system buffer
            //  or a fastio so this is probably some arbitrary user
            //  buffer.  We can not do the processing at DPC level so
            //  try and get to a safe IRQL so we can do the processing.
            if (FltDoCompletionProcessingWhenSafe( Data,
                                                   FltObjects,
                                                   CompletionContext,
                                                   Flags,
                                                   SwapPostDirCtrlBuffersWhenSafe,
                                                   &retValue ))
			{
                //  这个操作已经移到了安全的IRQL，被调用的函数将（或者已经）释放
				//	因此我们不用再自己的函数中处理。
                //  This operation has been moved to a safe IRQL, the called
                //  routine will do (or has done) the freeing so don't do it
                //  in our routine.
                cleanupAllocatedBuffer = FALSE;
            }
			else 
			{
                //  我们现在的状态是不能获取安全的IRQL并且没有一个MDL。
				//  我们没有办法安全的复制数据
                //  We are in a state where we can not get to a safe IRQL and
                //  we do not have a MDL.  There is nothing we can do to safely
                //  copy the data back to the users buffer, fail the operation
                //  and return.  This shouldn't ever happen because in those
                //  situations where it is not safe to post, we should have
                //  a MDL.
                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPostDirCtrlBuffers:         %wZ Unable to post to a safe IRQL\n",
                            &p2pCtx->VolCtx->Name) );

                Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
                Data->IoStatus.Information = 0;
            }
            __leave;
        }

        //  我们在适当的上下文中要么有一个系统缓冲，要么是一个快速IO操作。
		//  复制数据并处理异常。
        //  We either have a system buffer or this is a fastio operation
        //  so we are in the proper context.  Copy the data handling an
        //  exception.
        //
		//  注意：由于在FASTFAT里面存在一个BUG（在information字段返回一个错的长度）....（妈的，不翻了，自己看英文理解。不懂英文的屌丝玩蛋去吧！）
		//        我们总是要去复制原始缓冲区的长度。
        //  NOTE:  Due to a bug in FASTFAT where it is returning the wrong
        //         length in the information field (it is sort) we are always
        //         going to copy the original buffer length.
        __try 
		{
            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           /*Data->IoStatus.Information*/
                           iopb->Parameters.DirectoryControl.QueryDirectory.Length );

        }
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPostDirCtrlBuffers:         %wZ Invalid user buffer, oldB=%p, status=%x, info=%x\n",
                        &p2pCtx->VolCtx->Name,
                        origBuf,
                        Data->IoStatus.Status,
                        Data->IoStatus.Information) );
        }

    } 
	__finally
	{
        //  我们应该去清理分配的内存，释放卷上下文。
		//  MDL的释放处理交给Fltmgr
        //  If we are supposed to, cleanup the allocate memory and release
        //  the volume context.  The freeing of the MDL (if there is one) is
        //  handled by FltMgr.
        if (cleanupAllocatedBuffer) 
		{
            LOG_PRINT( LOGFL_DIRCTRL,
                       ("SwapBuffers!SwapPostDirCtrlBuffers:         %wZ newB=%p info=%d Freeing\n",
                        &p2pCtx->VolCtx->Name,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Information) );

            ExFreePool( p2pCtx->SwappedBuffer );
            FltReleaseContext( p2pCtx->VolCtx );

            ExFreeToNPagedLookasideList( &Pre2PostContextList, p2pCtx );
        }
    }

    return retValue;
}


/*++

Routine Description:

	我们有一个随机的没有MDL的用户缓冲区，因此我们需要获取一个安全的IRQL，然后锁定它复制数据
    We had an arbitrary users buffer without a MDL so we needed to get
    to a safe IRQL so we could lock it and then copy the data.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The buffer we allocated and swapped to

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - This is always returned.

--*/
FLT_POSTOP_CALLBACK_STATUS
SwapPostDirCtrlBuffersWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    PVOID origBuf;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    ASSERT(Data->IoStatus.Information != 0);

    //  锁定用户缓冲
    //  This is some sort of user buffer without a MDL, lock the
    //  user buffer so we can access it
    status = FltLockUserBuffer( Data );

    if (!NT_SUCCESS(status)) 
	{
        LOG_PRINT( LOGFL_ERRORS,
                   ("SwapBuffers!SwapPostDirCtrlBuffersWhenSafe: %wZ Could not lock user buffer, oldB=%p, status=%x\n",
                    &p2pCtx->VolCtx->Name,
                    iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer,
                    status) );

        //  
        //  If we can't lock the buffer, fail the operation
        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;

    } else {

        //  为这个缓冲区获得一个系统地址
        //  Get a system address for this buffer.
        origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress, NormalPagePriority );
        if (origBuf == NULL) 
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPostDirCtrlBuffersWhenSafe: %wZ Failed to get System address for MDL: %p\n",
                        &p2pCtx->VolCtx->Name,
                        iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress) );

            //
            //  If we couldn't get a SYSTEM buffer address, fail the operation
            Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Data->IoStatus.Information = 0;

        }
		else
		{
            //
            //  Copy the data back to the original buffer
            //
            //  NOTE:  Due to a bug in FASTFAT where it is returning the wrong
            //         length in the information field (it is short) we are
            //         always going to copy the original buffer length.
            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           /*Data->IoStatus.Information*/
                           iopb->Parameters.DirectoryControl.QueryDirectory.Length );
        }
    }

    //
    //  Free the memory we allocated and return
    LOG_PRINT( LOGFL_DIRCTRL,
               ("SwapBuffers!SwapPostDirCtrlBuffersWhenSafe: %wZ newB=%p info=%d Freeing\n",
                &p2pCtx->VolCtx->Name,
                p2pCtx->SwappedBuffer,
                Data->IoStatus.Information) );

    ExFreePool( p2pCtx->SwappedBuffer );
    FltReleaseContext( p2pCtx->VolCtx );

    ExFreeToNPagedLookasideList( &Pre2PostContextList,
                                 p2pCtx );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


/*++

Routine Description:

	这个函数展示了怎么在写操作交换缓冲区
	注意：（自己看去吧，懒得写了...）
    This routine demonstrates how to swap buffers for the WRITE operation.
    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback
    FLT_PREOP_COMPLETE -
--*/
FLT_PREOP_CALLBACK_STATUS
SwapPreWriteBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOID newBuf = NULL;
    PMDL newMdl = NULL;
    PVOLUME_CONTEXT volCtx = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;
    PVOID origBuf;
    NTSTATUS status;
    ULONG writeLen = iopb->Parameters.Write.Length;

    __try
	{
        //  写入长度为0，直接返回
        //  If they are trying to write ZERO bytes, then don't do anything and
        //  we don't need a post-operation callback.
        if (writeLen == 0)
            __leave;

        //  获取卷上下文
        //  Get our volume context so we can display our volume name in the
        //  debug output.
        status = FltGetVolumeContext( FltObjects->Filter, FltObjects->Volume, &volCtx );

        if (!NT_SUCCESS(status)) 
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreWriteBuffers:            Error getting volume context, status=%x\n",
                        status) );

            __leave;
        }

        //  如果这个IRP是无缓冲IO，我们需要....(幹！)
		//  我们必须做这一步，只是为了确保我们的缓冲有他们期望的那么大大
        //  If this is a non-cached I/O we need to round the length up to the
        //  sector size for this device.  We must do this because the file
        //  systems do this and we need to make sure our buffer is as big
        //  as they are expecting.
        if (FlagOn(IRP_NOCACHE,iopb->IrpFlags))
            writeLen = (ULONG)ROUND_TO_SIZE(writeLen,volCtx->SectorSize);

        //  
        //  Allocate nonPaged memory for the buffer we are swapping to.
        //  If we fail to get the memory, just don't swap buffers on this
        //  operation.
        newBuf = ExAllocatePoolWithTag( NonPagedPool, writeLen, BUFFER_SWAP_TAG );
        if (newBuf == NULL) 
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreWriteBuffers:            %wZ Failed to allocate %d bytes of memory.\n",
                        &volCtx->Name,
                        writeLen) );

            __leave;
        }

        //  我们需要为IRP操作创建一个MDL。在快速IO操作下不需要这么做，因为没有参数将MDL传递给文件系统
        //  We only need to build a MDL for IRP operations.  We don't need to
        //  do this for a FASTIO operation because it is a waste of time since
        //  the FASTIO interface has no parameter for passing the MDL to the
        //  file system.
        if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_IRP_OPERATION)) 
		{
            //
            //  Allocate a MDL for the new allocated memory.  If we fail
            //  the MDL allocation then we won't swap buffer for this operation
            newMdl = IoAllocateMdl( newBuf, writeLen, FALSE, FALSE, NULL );
            if (newMdl == NULL) 
			{
                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPreWriteBuffers:            %wZ Failed to allocate MDL.\n",
                            &volCtx->Name) );

                __leave;
            }

            //  为非分页内存创建MDL
            //  setup the MDL for the non-paged pool we just allocated
            MmBuildMdlForNonPagedPool( newMdl );
        }

        //
        //  If the users original buffer had a MDL, get a system address.
        if (iopb->Parameters.Write.MdlAddress != NULL) 
		{

            origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Write.MdlAddress, NormalPagePriority );
            if (origBuf == NULL) 
			{
                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPreWriteBuffers:            %wZ Failed to get system address for MDL: %p\n",
                            &volCtx->Name,
                            iopb->Parameters.Write.MdlAddress) );

                //
                //  If we could not get a system address for the users buffer,
                //  then we are going to fail this operation.
                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                retValue = FLT_PREOP_COMPLETE;
                __leave;
            }

        }
		else
		{
            //
            //  There was no MDL defined, use the given buffer address.
            origBuf = iopb->Parameters.Write.WriteBuffer;
        }

        //
        //  Copy the memory, we must do this inside the try/except because we
        //  may be using a users buffer address
        __try 
		{
            RtlCopyMemory( newBuf, origBuf, writeLen );
        } 
		__except (EXCEPTION_EXECUTE_HANDLER) 
		{
            //
            //  The copy failed, return an error, failing the operation.
            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;
            retValue = FLT_PREOP_COMPLETE;

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreWriteBuffers:            %wZ Invalid user buffer, oldB=%p, status=%x\n",
                        &volCtx->Name,
                        origBuf,
                        Data->IoStatus.Status) );

            __leave;
        }

        //
        //  We are ready to swap buffers, get a pre2Post context structure.
        //  We need it to pass the volume context and the allocate memory
        //  buffer to the post operation callback.
        p2pCtx = ExAllocateFromNPagedLookasideList( &Pre2PostContextList );
        if (p2pCtx == NULL)
		{
            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreWriteBuffers:            %wZ Failed to allocate pre2Post context structure\n",
                        &volCtx->Name) );

            __leave;
        }

        //
        //  Set new buffers
        LOG_PRINT( LOGFL_WRITE,
                   ("SwapBuffers!SwapPreWriteBuffers:            %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d\n",
                    &volCtx->Name,
                    newBuf,
                    newMdl,
                    iopb->Parameters.Write.WriteBuffer,
                    iopb->Parameters.Write.MdlAddress,
                    writeLen) );

        iopb->Parameters.Write.WriteBuffer = newBuf;
        iopb->Parameters.Write.MdlAddress = newMdl;
        FltSetCallbackDataDirty( Data );

        //
        //  Pass state to our post-operation callback.
        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = volCtx;

        *CompletionContext = p2pCtx;

        //
        //  Return we want a post-operation callback
        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    } __finally {

        //
        //  If we don't want a post-operation callback, then free the buffer
        //  or MDL if it was allocated.
        if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {

            if (newBuf != NULL) {

                ExFreePool( newBuf );
            }

            if (newMdl != NULL) {

                IoFreeMdl( newMdl );
            }

            if (volCtx != NULL) {

                FltReleaseContext( volCtx );
            }
        }
    }

    return retValue;
}


FLT_POSTOP_CALLBACK_STATUS
SwapPostWriteBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    LOG_PRINT( LOGFL_WRITE,
               ("SwapBuffers!SwapPostWriteBuffers:           %wZ newB=%p info=%d Freeing\n",
                &p2pCtx->VolCtx->Name,
                p2pCtx->SwappedBuffer,
                Data->IoStatus.Information) );

    //
    //  Free allocate POOL and volume context
    //

    ExFreePool( p2pCtx->SwappedBuffer );
    FltReleaseContext( p2pCtx->VolCtx );

    ExFreeToNPagedLookasideList( &Pre2PostContextList,
                                 p2pCtx );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


VOID
ReadDriverParameters (
    __in PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine tries to read the driver-specific parameters from
    the registry.  These values will be found in the registry location
    indicated by the RegistryPath passed in.

Arguments:

    RegistryPath - the path key passed to the driver during driver entry.

Return Value:

    None.

--*/
{
    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING valueName;
    UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( LONG )];

    //
    //  If this value is not zero then somebody has already explicitly set it
    //  so don't override those settings.
    //

    if (0 == LoggingFlags) {

        //
        //  Open the desired registry key
        //

        InitializeObjectAttributes( &attributes,
                                    RegistryPath,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        status = ZwOpenKey( &driverRegKey,
                            KEY_READ,
                            &attributes );

        if (!NT_SUCCESS( status )) {

            return;
        }

        //
        // Read the given value from the regis__try.
        //

        RtlInitUnicodeString( &valueName, L"DebugFlags" );

        status = ZwQueryValueKey( driverRegKey,
                                  &valueName,
                                  KeyValuePartialInformation,
                                  buffer,
                                  sizeof(buffer),
                                  &resultLength );

        if (NT_SUCCESS( status )) {

            LoggingFlags = *((PULONG) &(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data));
        }

        //
        //  Close the regis__try en__try
        //

        ZwClose(driverRegKey);
    }
}

