#ifndef _ISI_SANDBOX_U_H_
#define _ISI_SANDBOX_U_H_

/**
 * ���ļ�������ɳ��ͨ�Ŷ���
 */

#include <windows.h>

#define EXPORT_API_C    extern "C"__declspec(dllexport)
#ifdef EXPORT_MODULE
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API __declspec(dllimport)
#endif

/**
 * ��װ����
 */
int EXPORT_API InstallISISandBox(const wchar_t *aServiceName);

/**
 * ж������
 */
int EXPORT_API UninstallISISandBox(const wchar_t *aServiceName);

/**
 * ������Ҫ�ض����Ŀ¼
 */
int EXPORT_API ISISandBoxSetProtectPath(const wchar_t *aProtectPath);

/**
 * �Ƴ���Ҫ�ض����Ŀ¼
 */
int EXPORT_API ISISandBoxRemoveProtectPath(const wchar_t *aProtectPath);

/**
 * �Ƴ������ض����Ŀ¼
 */
int EXPORT_API ISISandBoxRemoveAllProtectPath();

/**
 * �����ض����ĸ�Ŀ¼
 */
int EXPORT_API ISISandBoxSetRedirectPath(const wchar_t *aRedirectPath);

/**
 * ������Ҫ���ض���Ľ���
 */
int EXPORT_API ISISandBoxSetProtectProcess(DWORD aProcessId);

/**
 * ������Ҫ���ض���Ľ���
 */
int EXPORT_API ISISandBoxRemoveProtectProcess(DWORD aProcessId);

/**
 * ������Ҫ���ض���Ľ���
 */
int EXPORT_API ISISandBoxRemoveAllProtectProcess();

/**
 * ��ʼ�ض���
 */
int EXPORT_API ISISandBoxStart();

/**
 * ֹͣ�ض���
 */
int EXPORT_API ISISandBoxStop();

#endif