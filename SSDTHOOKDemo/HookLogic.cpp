#include "HookLogic.h"
#include "DriverStruct.h"


ULONG global_ProtectedProcessIds[MAX_SSDT_HOOK_PROCESS_ID_NUMBER] = {0};
ULONG global_ProtectedProcessIdsLength = 0;
KMUTEX global_ProtectedProcessIdsSynchronism;

ULONG global_HidedProcessIds[MAX_SSDT_HOOK_PROCESS_ID_NUMBER] = {0};
ULONG global_HidedProcessIdsLength = 0;
KMUTEX global_HidedProcessIdsSynchronism;

NTQUERYSYSTEMINFORMATION pOldNtQuerySystemInformation = NULL;
NTTERMINATEPROCESS pOldNtTerminateProcess = NULL;


VOID InitializeSynchronObjects()
{
	KeInitializeMutex(&global_ProtectedProcessIdsSynchronism, 0);
	KeInitializeMutex(&global_HidedProcessIdsSynchronism, 0);
}

LONG CheckProcessIsInProtectList(ULONG aProcessId)
{
	if (aProcessId == 0)
		return -1;

	LARGE_INTEGER li;
	li.QuadPart = 100*1000*1000*1000*1;
	KeWaitForSingleObject(&global_ProtectedProcessIdsSynchronism, Executive, KernelMode, FALSE, &li);

	for (ULONG i = 0; (i < global_ProtectedProcessIdsLength && i < MAX_SSDT_HOOK_PROCESS_ID_NUMBER); ++i)
	{
		if (global_ProtectedProcessIds[i] == aProcessId)
		{
			KeReleaseMutex(&global_ProtectedProcessIdsSynchronism, FALSE);
			return i;
		}
	}

	KeReleaseMutex(&global_ProtectedProcessIdsSynchronism, FALSE);

	return -1;
}

LONG CheckProcessIsInHideList(ULONG aProcessId)
{
	if (aProcessId == 0)
		return -1;

	LARGE_INTEGER li;
	li.QuadPart = 100*1000*1000*1000*1;
	KeWaitForSingleObject(&global_HidedProcessIdsSynchronism, Executive, KernelMode, FALSE, &li);

	for (ULONG i = 0; (i < global_HidedProcessIdsLength && i < MAX_SSDT_HOOK_PROCESS_ID_NUMBER); ++i)
	{
		if (global_HidedProcessIds[i] == aProcessId)
		{
			KeReleaseMutex(&global_HidedProcessIdsSynchronism, FALSE);
			return i;
		}
	}

	KeReleaseMutex(&global_HidedProcessIdsSynchronism, FALSE);

	return -1;
}

BOOLEAN InsertProcessInProtectList(ULONG aProcessId)
{
	// 链表操作，添加相应的项
	PMARKED_PROCESS pMarkedProcess = AllocElement();
	if (pMarkedProcess)
	{
		pMarkedProcess->ProcessId = aProcessId;
		pMarkedProcess->ProcessProtectFlag1 |= TerminateProcessProtect;
	}
	if (CheckProcessIsInProtectList(aProcessId) == -1 && global_ProtectedProcessIdsLength < MAX_SSDT_HOOK_PROCESS_ID_NUMBER)
	{
		LARGE_INTEGER li;
		li.QuadPart = 100*1000*1000*1000*1;
		KeWaitForSingleObject(&global_ProtectedProcessIdsSynchronism, Executive, KernelMode, FALSE, &li);
		global_ProtectedProcessIds[global_ProtectedProcessIdsLength++] = aProcessId;
		KeReleaseMutex(&global_HidedProcessIdsSynchronism, FALSE);

		return TRUE;
	}

	return FALSE;
}

BOOLEAN RemoveProcessFromProtectList(ULONG aProcessId)
{
	ULONG index = CheckProcessIsInProtectList(aProcessId);

	if (index != -1)
	{
		LARGE_INTEGER li;
		li.QuadPart = 100*1000*1000*1000*1;
		KeWaitForSingleObject(&global_ProtectedProcessIdsSynchronism, Executive, KernelMode, FALSE, &li);
		global_ProtectedProcessIds[index] = global_ProtectedProcessIds[global_ProtectedProcessIdsLength--];
		KeReleaseMutex(&global_HidedProcessIdsSynchronism, FALSE);

		return TRUE;
	}

	return FALSE;
}

BOOLEAN InsertProcessInHideList(ULONG aProcessId)
{
	if (CheckProcessIsInHideList(aProcessId) == -1 && global_HidedProcessIdsLength < MAX_SSDT_HOOK_PROCESS_ID_NUMBER)
	{
		LARGE_INTEGER li;
		li.QuadPart = 100*1000*1000*1000*1;
		KeWaitForSingleObject(&global_HidedProcessIdsSynchronism, Executive, KernelMode, FALSE, &li);
		global_HidedProcessIds[global_HidedProcessIdsLength++] = aProcessId;
		KeReleaseMutex(&global_HidedProcessIdsSynchronism, FALSE);

		return TRUE;
	}

	return FALSE;
}

BOOLEAN RemoveProcessFromHideList(ULONG aProcessId)
{
	ULONG index = CheckProcessIsInHideList(aProcessId);

	if (index != -1)
	{
		LARGE_INTEGER li;
		li.QuadPart = 100*1000*1000*1000*1;
		KeWaitForSingleObject(&global_HidedProcessIdsSynchronism, Executive, KernelMode, FALSE, &li);
		global_HidedProcessIds[index] = global_HidedProcessIds[global_HidedProcessIdsLength--];
		KeReleaseMutex(&global_HidedProcessIdsSynchronism, FALSE);

		return TRUE;
	}

	return FALSE;
}

NTSTATUS HookNtQuerySystemInformation(
	__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
	__out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
	__in ULONG SystemInformationLength,
	__out_opt PULONG ReturnLength
	)
{
	NTSTATUS status = STATUS_SUCCESS;

	// 获取原始系统调用地址
	pOldNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)oldSysServiceAddr[SYSCALL_INDEX(ZwQuerySystemInformation)];

	status = pOldNtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);

	// 调用失败
	if (!NT_SUCCESS(status))
		return status;

	// 系统信息类型不是SystemProcessInformation
	if (SystemProcessInformation != SystemInformationClass)
		return status;

	// 如果当前系统不是工作状态，调用完原函数后直接返回
	LARGE_INTEGER li;
	li.QuadPart = 100 * 1000 * 1000* 1;
	KeWaitForSingleObject(&global_Synchronism, Executive, KernelMode, FALSE, &li);
	BOOLEAN workingState = global_WorkState;
	KeReleaseMutex(&global_Synchronism, FALSE);

	if (!workingState)
		return status;

	PSYSTEM_PROCESS_INFORMATION pPrevProcessInfo = NULL;
	PSYSTEM_PROCESS_INFORMATION pCurrProcessInfo = (PSYSTEM_PROCESS_INFORMATION)SystemInformation;

	while (pCurrProcessInfo != NULL)
	{
		// 获取当前遍历的 SYSTEM_PROCESS_INFORMATION 节点的进程名称和进程 ID
		ULONG processId = (ULONG)pCurrProcessInfo->UniqueProcessId;
		UNICODE_STRING tmpProcessName = pCurrProcessInfo->ImageName;

		// 判断当前进程是否为需要隐藏的进程
		if (CheckProcessIsInHideList(processId) != -1)
		{
			if (pPrevProcessInfo)
			{
				if (pCurrProcessInfo->NextEntryOffset)
				{
					// 将当前进程从SystemInformation中移除
					pPrevProcessInfo->NextEntryOffset += pCurrProcessInfo->NextEntryOffset;
				}
				else
				{
					// 当前隐藏的这个进程是进程链表的最后一个
					pPrevProcessInfo->NextEntryOffset = 0;
				}
			}
			else
			{
				// 第一个遍历到的进程就是要隐藏的进程
				if (pCurrProcessInfo->NextEntryOffset)
				{
					BYTE *tempSystemInformation = (BYTE *)SystemInformation + pCurrProcessInfo->NextEntryOffset;
					SystemInformation = tempSystemInformation;
				}
				else
				{
					SystemInformation = NULL;
				}
			}
		}

		// 遍历下一个SYSTEM_PROCESS_INFORMATION节点
		pPrevProcessInfo = pCurrProcessInfo;

		// 遍历结束
		if (pCurrProcessInfo->NextEntryOffset)
		{
			pCurrProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(((PCHAR)pCurrProcessInfo) + pCurrProcessInfo->NextEntryOffset);
		}
		else
		{
			pCurrProcessInfo = NULL;
		}
	}

	return status;
}

NTSTATUS HookNtTerminateProcess( __in_opt HANDLE ProcessHandle, __in NTSTATUS ExitStatus )
{
	PEPROCESS pEProcess = NULL;

	// 如果当前系统不是工作状态，直接调用原函数
	LARGE_INTEGER li;
	li.QuadPart = 100 * 1000 * 1000* 1;
	KeWaitForSingleObject(&global_Synchronism, Executive, KernelMode, FALSE, &li);
	BOOLEAN workingState = global_WorkState;
	KeReleaseMutex(&global_Synchronism, FALSE);

	// 保存SSDT中原来的NtTerminateProcess地址
	pOldNtTerminateProcess = (NTTERMINATEPROCESS)oldSysServiceAddr[SYSCALL_INDEX(ZwTerminateProcess)];

	if (!workingState)
		goto TERMINATE_PROCESS;

	// 通过进程句柄来获得该进程所对应的FileObject对象，由于这里是进程对象，获得的就是EPROCESS对象
	NTSTATUS status = ObReferenceObjectByHandle(ProcessHandle, FILE_READ_DATA, NULL, KernelMode, (PVOID *)&pEProcess, NULL);

	if (!NT_SUCCESS(status))
		return status;

	// 通过该函数可以得到进程名和进程Id
	// 该函数是未文档化的导出函数（可参考WRK），需要自己声明
	ULONG processId = (ULONG)PsGetProcessId(pEProcess);
	PCHAR processName = (PCHAR)PsGetProcessImageFileName(pEProcess);

	// 通过进程名来初始化ANSI字符串
	ANSI_STRING ansiProcessName;
	RtlInitAnsiString(&ansiProcessName, processName);

	if (CheckProcessIsInProtectList(processId) != -1)
	{
		// 确保调用者进程能够结束（这里主要指taskmgr.exe）
		if (processId != (ULONG)PsGetProcessId(PsGetCurrentProcess()))
		{
			// 如果是被保护的进程，返回无权限访问
			return STATUS_ACCESS_DENIED;
		}
	}

TERMINATE_PROCESS:
	// 非保护进程，则调用真正的NtTerminalProcess结束进程
	status = pOldNtTerminateProcess(ProcessHandle, ExitStatus);

	return status;
}