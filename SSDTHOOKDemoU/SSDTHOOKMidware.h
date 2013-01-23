#ifndef _SSDT_HOOK_MIDWARE_H_
#define _SSDT_HOOK_MIDWARE_H_

#include <tchar.h>
#include <WINDOWS.H>
#include <winioctl.h>

/**
 * 文件说明：
 *	此文件定义了SSDT HOOK中间件对象
 *	此对象提供了基础的用户态与驱动通信事件
 * 作者：
 *	wangy
 * 日期：
 *	2013-01-17
 */

#define SYMBO_LINK_NAME_T	_T("\\\\.\\SSDTHOOKDemo")

// SSDT驱动类型
#define	SSDT_DEVICE_TYPE				FILE_DEVICE_UNKNOWN

//定义用于应用程序和驱动程序通信的宏，这里使用的是缓冲区读写方式
#define IO_START_WORKING				(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_STOP_WORKING					(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x702, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_INSERT_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_REMOVE_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_INSERT_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_REMOVE_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)


// 这里定义系统保护类型，可进行位运算组合添加
#define TerminateProcessProtect		0x00000001		// 进程保护
#define QueryProcessInfoProtect		0x00000010		// 进程隐藏

// 进程状态
typedef struct _PROCRSS_STATE
{
	// 进程ID
	DWORD processId;

	// 进程状态标记，位标记
	unsigned int processState;

} PROCESS_STATE_EX, *PPROCESS_STATE_EX;

class SSDTHOOKMidware
{
public:
	SSDTHOOKMidware();
	~SSDTHOOKMidware();

public:
	/**
	 * 启动SSDT HOOK驱动
	 * 如果驱动未安装，则安装驱动，否则直接启动它
	 * 参数：
	 *	@aServiceName	驱动服务名称
	 * 返回值：
	 *	0 - 操作成功，驱动已经成功启动
	 */
	int RunSSDTHOOKDriver(TCHAR *aServiceName);

	/**
	 * 停止SSDT HOOK驱动
	 * 参数：
	 *	@aServiceName	驱动服务名称
	 * 返回值：
	 *	0 - 操作成功，驱动已经成功停止
	 */
	int StopSSDTHOOKDriver(TCHAR *aServiceName);

	/**
	 * 连接到驱动设备对象
	 * 参数：
	 *	@aDeviceName	设备对象符号链接
	 * 返回值：
	 *	0 - 连接成功
	 */
	int ConnectToDriver(TCHAR *aDeviceName);

	/**
	 * 断开驱动设备对象通信
	 * 返回值：
	 *	0 - 断开成功
	 */
	int DisconnectToDriver();

	/**
	 * 启动工作状态
	 */
	int StartSSDTHOOK();

	/**
	 * 停止工作状态
	 */
	int StopSSDTHOOK();

	/**
	 * 增加挂钩进程
	 * 参数：
	 *	@aHookType		挂钩类型
	 *	@aProcessId		进程ID
	 * 返回值：
	 *	成功返回相应的位
	 */
	int AddSSDTHOOKProcess(int aHookType, DWORD aProcessId);

	/**
	 * 移除挂钩进程
	 * 参数：
	 *	@aHookType		挂钩类型
	 *	@aProcessId		进程ID
	 * 返回值：
	 *	成功返回相应的位
	 */
	int RemoveSSDTHOOKProcess(int aHookType, DWORD aProcessId);

private:
	/**
	 * 驱动通信上下文
	 */
	void *iDriverContext;
};

#endif