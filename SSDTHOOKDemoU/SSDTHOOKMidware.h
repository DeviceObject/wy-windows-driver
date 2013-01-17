#ifndef _SSDT_HOOK_MIDWARE_H_
#define _SSDT_HOOK_MIDWARE_H_

#include <tchar.h>
#include <WINDOWS.H>

/**
 * 文件说明：
 *	此文件定义了SSDT HOOK中间件对象
 *	此对象提供了基础的用户态与驱动通信事件
 * 作者：
 *	wangy
 * 日期：
 *	2013-01-17
 */

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
	 * 增加挂钩进程
	 * 参数：
	 *	@aHookType		挂钩类型
	 *	@aProcessId		进程ID
	 * 返回值：
	 *	0 - 添加成功
	 */
	int AddSSDTHOOKProcess(int aHookType, DWORD aProcessId);

	/**
	 * 移除挂钩进程
	 * 参数：
	 *	@aHookType		挂钩类型
	 *	@aProcessId		进程ID
	 * 返回值：
	 *	0 - 移除成功
	 */
	int RemoveSSDTHOOKProcess(int aHookType, DWORD aProcessId);

private:
	/**
	 * 驱动通信上下文
	 */
	void *iDriverContext;
};

#endif