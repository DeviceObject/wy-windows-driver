#ifndef _ISI_SANDBOX_U_H_
#define _ISI_SANDBOX_U_H_

/**
 * 此文件定义了沙盒通信对象
 */

#include <windows.h>

#define EXPORT_API_C    extern "C"__declspec(dllexport)
#ifdef EXPORT_MODULE
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API __declspec(dllimport)
#endif

/**
 * 安装驱动
 */
int EXPORT_API InstallISISandBox(const wchar_t *aServiceName);

/**
 * 卸载驱动
 */
int EXPORT_API UninstallISISandBox(const wchar_t *aServiceName);

/**
 * 设置需要重定向的目录
 */
int EXPORT_API ISISandBoxSetProtectPath(const wchar_t *aProtectPath);

/**
 * 移除需要重定向的目录
 */
int EXPORT_API ISISandBoxRemoveProtectPath(const wchar_t *aProtectPath);

/**
 * 移除所有重定向的目录
 */
int EXPORT_API ISISandBoxRemoveAllProtectPath();

/**
 * 设置重定向后的根目录
 */
int EXPORT_API ISISandBoxSetRedirectPath(const wchar_t *aRedirectPath);

/**
 * 设置需要被重定向的进程
 */
int EXPORT_API ISISandBoxSetProtectProcess(DWORD aProcessId);

/**
 * 设置需要被重定向的进程
 */
int EXPORT_API ISISandBoxRemoveProtectProcess(DWORD aProcessId);

/**
 * 设置需要被重定向的进程
 */
int EXPORT_API ISISandBoxRemoveAllProtectProcess();

/**
 * 开始重定向
 */
int EXPORT_API ISISandBoxStart();

/**
 * 停止重定向
 */
int EXPORT_API ISISandBoxStop();

#endif