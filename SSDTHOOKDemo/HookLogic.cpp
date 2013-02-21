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
	// ��������������Ӧ����
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

	// ��ȡԭʼϵͳ���õ�ַ
	pOldNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)oldSysServiceAddr[SYSCALL_INDEX(ZwQuerySystemInformation)];

	status = pOldNtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);

	// ����ʧ��
	if (!NT_SUCCESS(status))
		return status;

	// ϵͳ��Ϣ���Ͳ���SystemProcessInformation
	if (SystemProcessInformation != SystemInformationClass)
		return status;

	// �����ǰϵͳ���ǹ���״̬��������ԭ������ֱ�ӷ���
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
		// ��ȡ��ǰ������ SYSTEM_PROCESS_INFORMATION �ڵ�Ľ������ƺͽ��� ID
		ULONG processId = (ULONG)pCurrProcessInfo->UniqueProcessId;
		UNICODE_STRING tmpProcessName = pCurrProcessInfo->ImageName;

		// �жϵ�ǰ�����Ƿ�Ϊ��Ҫ���صĽ���
		if (CheckProcessIsInHideList(processId) != -1)
		{
			if (pPrevProcessInfo)
			{
				if (pCurrProcessInfo->NextEntryOffset)
				{
					// ����ǰ���̴�SystemInformation���Ƴ�
					pPrevProcessInfo->NextEntryOffset += pCurrProcessInfo->NextEntryOffset;
				}
				else
				{
					// ��ǰ���ص���������ǽ�����������һ��
					pPrevProcessInfo->NextEntryOffset = 0;
				}
			}
			else
			{
				// ��һ���������Ľ��̾���Ҫ���صĽ���
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

		// ������һ��SYSTEM_PROCESS_INFORMATION�ڵ�
		pPrevProcessInfo = pCurrProcessInfo;

		// ��������
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

	// �����ǰϵͳ���ǹ���״̬��ֱ�ӵ���ԭ����
	LARGE_INTEGER li;
	li.QuadPart = 100 * 1000 * 1000* 1;
	KeWaitForSingleObject(&global_Synchronism, Executive, KernelMode, FALSE, &li);
	BOOLEAN workingState = global_WorkState;
	KeReleaseMutex(&global_Synchronism, FALSE);

	// ����SSDT��ԭ����NtTerminateProcess��ַ
	pOldNtTerminateProcess = (NTTERMINATEPROCESS)oldSysServiceAddr[SYSCALL_INDEX(ZwTerminateProcess)];

	if (!workingState)
		goto TERMINATE_PROCESS;

	// ͨ�����̾������øý�������Ӧ��FileObject�������������ǽ��̶��󣬻�õľ���EPROCESS����
	NTSTATUS status = ObReferenceObjectByHandle(ProcessHandle, FILE_READ_DATA, NULL, KernelMode, (PVOID *)&pEProcess, NULL);

	if (!NT_SUCCESS(status))
		return status;

	// ͨ���ú������Եõ��������ͽ���Id
	// �ú�����δ�ĵ����ĵ����������ɲο�WRK������Ҫ�Լ�����
	ULONG processId = (ULONG)PsGetProcessId(pEProcess);
	PCHAR processName = (PCHAR)PsGetProcessImageFileName(pEProcess);

	// ͨ������������ʼ��ANSI�ַ���
	ANSI_STRING ansiProcessName;
	RtlInitAnsiString(&ansiProcessName, processName);

	if (CheckProcessIsInProtectList(processId) != -1)
	{
		// ȷ�������߽����ܹ�������������Ҫָtaskmgr.exe��
		if (processId != (ULONG)PsGetProcessId(PsGetCurrentProcess()))
		{
			// ����Ǳ������Ľ��̣�������Ȩ�޷���
			return STATUS_ACCESS_DENIED;
		}
	}

TERMINATE_PROCESS:
	// �Ǳ������̣������������NtTerminalProcess��������
	status = pOldNtTerminateProcess(ProcessHandle, ExitStatus);

	return status;
}