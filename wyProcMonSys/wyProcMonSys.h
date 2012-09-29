/**
 说明：这是一个文件系统过滤驱动，用于记录文件操作相关的
	   在这个驱动里面我们会想用户态传递文件操作状态，并由用户态显示出来
 作者：wangy
 日期：2012-09-24
 **/

#ifndef _WYPROCMONSYS_H_
#define _WYPROCMONSYS_H_

#include "Public.h"

#ifdef __cplusplus
extern "C"
{
#endif

DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath);

NTSTATUS DriverUnload(__in FLT_FILTER_UNLOAD_FLAGS Flags);

NTSTATUS DriverInstanceSetup(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_SETUP_FLAGS Flags, __in DEVICE_TYPE VolumeDeviceType, __in FLT_FILESYSTEM_TYPE VolumeFilesystemType);

NTSTATUS DriverInstanceQueryTeardown(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);

VOID DriverInstanceTeardownStart(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_TEARDOWN_FLAGS Flags);

VOID DriverInstanceTeardownComplete(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_TEARDOWN_FLAGS Flags);

// 过滤函数
FLT_POSTOP_CALLBACK_STATUS PMSPostCreate(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags);

FLT_POSTOP_CALLBACK_STATUS PMSPostRead(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags);

FLT_POSTOP_CALLBACK_STATUS PMSPostWrite(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags);


#ifdef __cplusplus
}
#endif

#endif