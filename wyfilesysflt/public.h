
/**
 * 公用头文件
 *
 */

#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// 定义全局变量
typedef struct _WY_GLOAB_DATA
{
	// 驱动对象
	PDRIVER_OBJECT iDriverObject;

	// 过滤器对象
	PFLT_FILTER iFilter;

} GLOBAL_DATA, *PGLOBAL_DATA;

extern GLOBAL_DATA globalData;

//////////////////////////////////////////////////////////////////////////////////////////////
// 定义上下文结构

// 
// 卷实例上下文
typedef struct _WY_INSTANCE_CONTEXT
{
	// 当前上下文的实例
	PFLT_INSTANCE iInstance;

	// 当前实例相关联的卷
	PFLT_VOLUME iVolume;

	// 当前实例相关联卷的名称
	UNICODE_STRING iVolumeName;

} WY_INSTANCE_CONTEXT, *PWY_INSTANCE_CONTEXT;

#define WY_INSTANCE_CONTEXT_SIZE sizeof(WY_INSTANCE_CONTEXT)

// 
// 文件上下文
typedef struct _WY_FILE_CONTEXT
{
	// 文件名（是否包含全路径，出于简单的学习，这里就是全路径了）
	UNICODE_STRING iFileName;

	// 文件状态，位标志域
	ULONG iFileState;

} WY_FILE_CONTEXT, *PWY_FILE_CONTEXT;

#define WY_FILE_CONTEXT_SIZE sizeof(WY_FILE_CONTEXT)

// 
// 流上下文
typedef struct _WY_STREAM_CONTEXT
{
	// 此上下文关联的文件名称
	UNICODE_STRING iFileName;

	// 在此流上打开的次数
	ULONG iCreateCount;

	// 在此流上清理的次数
	ULONG iCleanupCount;

	// 在此流上关闭的次数
	ULONG iCloseCount;

	// 用于锁定此上下文的资源
	PERESOURCE iLockResource;

} WY_STREAM_CONTEXT, *PWY_STREAM_CONTEXT;

#define WY_STREAM_CONTEXT_SIZE sizeof(WY_STREAM_CONTEXT)

// 
// 流句柄上下文（这个是干嘛用的？现在确实不知道）
typedef struct _WY_STREAM_HANDLE_CONTEXT
{
	// 此上下文相关联的文件名
	UNICODE_STRING iFileName;

	// 用于锁定此上下文的资源
	PERESOURCE iLockResource;

} WY_STREAM_HANDLE_CONTEXT, *PWY_STREAM_HANDLE_CONTEXT;

#define WY_STREAM_HANDLE_CONTEXT_SIZE sizeof(WY_STREAM_HANDLE_CONTEXT)

// 
// 上下文内存池文字标记
#define WY_INSTANCE_CONTEXT_TAG			'cIxC'
#define WY_FILE_CONTEXT_TAG				'cFxC'
#define WY_STREAM_CONTEXT_TAG			'cSxC'
#define WY_STREAM_HANDLE_CONTEXT_TAG	'cHxC'
#define WY_STRING_TAG					'sSxC'

#endif