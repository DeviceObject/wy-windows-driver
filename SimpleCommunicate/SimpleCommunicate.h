#ifndef _SIMPLE_COMMUNICATE_H_
#define _SIMPLE_COMMUNICATE_H_

#include <fltkernel.h>

#ifdef __cplusplus
extern "C"
{
#endif

// 定义全局变量
#define DriverPortName	L"\\SimpleCommunicatePort"

typedef struct _GLOBAL_DATA
{
	// 过滤器对象
	PFLT_FILTER iFilter;

	// 端口
	PFLT_PORT iServerPort;

	// 端口
	PFLT_PORT iClientPort;

	// 用户态进程
	PEPROCESS iUserProcess;


} GLOBAL_DATA, *PGLOBAL_DATA;
extern GLOBAL_DATA globalData;

/**
 * 驱动入口函数
 * 参数：
 *	DriverObject	驱动对象
 *	RegistryPath	驱动服务注册表路径
 */
DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath);

/**
 * 驱动卸载函数
 * 参数：
 *	Flags	卸载标记
 */
NTSTATUS DriverUnload(__in FLT_FILTER_UNLOAD_FLAGS Flags);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// _SIMPLE_COMMUNICATE_H_