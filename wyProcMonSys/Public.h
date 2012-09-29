
#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

// 定义全局变量
typedef struct _GLOBAL_DATA
{
	// 驱动对象
	PDRIVER_OBJECT iDriverObject;

	// 进程名偏移量
	ULONG iProcessNameOffsetInPEPROCESS;

	BOOLEAN iStartFilter;

	// 过滤器
	PFLT_FILTER iFilter;

	// 驱动通信端口
	PFLT_PORT iDriverPort;

	// 用户进程
	PEPROCESS iUserProcess;

	// 用户通信端口
	PFLT_PORT iUserPort;

} GLOBAL_DATA, *PGLOBAL_DATA;

extern GLOBAL_DATA globalData;

#ifdef __cplusplus
}
#endif

#endif