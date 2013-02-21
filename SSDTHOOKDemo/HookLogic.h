#ifndef _HOOK_LOGIC_H_
#define _HOOK_LOGIC_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "SSDTHook.h"
#include "ProcessInfo.h"
#include "ProjectPublic.h"
#include <WINDEF.H>

/**
 * 表操作
 */
#define DEVICE_NAME_W		L"\\Device\\SSDTHHOOKDemo"
#define SYMBO_LINK_NAME_W	L"\\??\\SSDTHOOKDemo"

#define MAX_SSDT_HOOK_PROCESS_ID_NUMBER	1024

// 控制进程列表
extern ULONG global_ProtectedProcessIds[MAX_SSDT_HOOK_PROCESS_ID_NUMBER];
extern ULONG global_ProtectedProcessIdsLength;
extern KMUTEX global_ProtectedProcessIdsSynchronism;

extern ULONG global_HidedProcessIds[MAX_SSDT_HOOK_PROCESS_ID_NUMBER];
extern ULONG global_HidedProcessIdsLength;
extern KMUTEX global_HidedProcessIdsSynchronism;

// 白名单列表
extern ULONG global_WhiteListProcessIds[MAX_SSDT_HOOK_PROCESS_ID_NUMBER];
extern ULONG global_WhiteListProcessIdsLength;
extern KMUTEX global_WhiteListProcessIdsSynchronism;

VOID InitializeSynchronObjects();

LONG CheckProcessIsInProtectList(ULONG aProcessId);

LONG CheckProcessIsInHideList(ULONG aProcessId);

BOOLEAN InsertProcessInProtectList(ULONG aProcessId);

BOOLEAN RemoveProcessFromProtectList(ULONG aProcessId);

BOOLEAN InsertProcessInHideList(ULONG aProcessId);

BOOLEAN RemoveProcessFromHideList(ULONG aProcessId);

// SSDT HOOK钩子
NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation (
	__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
	__out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
	__in ULONG SystemInformationLength,
	__out_opt PULONG ReturnLength
	);

typedef NTSTATUS (* NTQUERYSYSTEMINFORMATION)(
	__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
	__out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
	__in ULONG SystemInformationLength,
	__out_opt PULONG ReturnLength
	);

NTSTATUS HookNtQuerySystemInformation (
	__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
	__out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
	__in ULONG SystemInformationLength,
	__out_opt PULONG ReturnLength
	);

extern NTQUERYSYSTEMINFORMATION pOldNtQuerySystemInformation;


typedef NTSTATUS (* NTTERMINATEPROCESS)(
	__in_opt HANDLE ProcessHandle,
	__in NTSTATUS ExitStatus
	);

NTSTATUS HookNtTerminateProcess(
	__in_opt HANDLE ProcessHandle,
	__in NTSTATUS ExitStatus
	);

extern NTTERMINATEPROCESS pOldNtTerminateProcess;


PUCHAR PsGetProcessImageFileName(__in PEPROCESS Process);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif