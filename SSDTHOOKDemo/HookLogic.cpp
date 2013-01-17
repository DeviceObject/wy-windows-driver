#include "HookLogic.h"


ULONG global_ProtectedProcessIds[MAX_SSDT_HOOK_PROCESS_ID_NUMBER] = {0};
ULONG global_ProtectedProcessIdsLength = 0;

ULONG global_HidedProcessIds[MAX_SSDT_HOOK_PROCESS_ID_NUMBER] = {0};
ULONG global_HidedProcessIdsLength = 0;

NTQUERYSYSTEMINFORMATION pOldNtQuerySystemInformation = NULL;
NTTERMINATEPROCESS pOldNtTerminateProcess = NULL;

ULONG CheckProcessIsInProtectList(ULONG aProcessId)
{
	if (aProcessId == 0)
		return -1;

	for (ULONG i = 0; (i < global_ProtectedProcessIdsLength && i < MAX_SSDT_HOOK_PROCESS_ID_NUMBER); ++i)
	{
		if (global_ProtectedProcessIds[i] == aProcessId)
			return i;
	}

	return -1;
}

ULONG CheckProcessIsInHideList(ULONG aProcessId)
{
	if (aProcessId == 0)
		return -1;

	for (ULONG i = 0; (i < global_HidedProcessIdsLength && i < MAX_SSDT_HOOK_PROCESS_ID_NUMBER); ++i)
	{
		if (global_HidedProcessIds[i] == aProcessId)
			return i;
	}

	return -1;
}

BOOLEAN InsertProcessInProtectList(ULONG aProcessId)
{
	if (CheckProcessIsInProtectList(aProcessId) == -1 && global_ProtectedProcessIdsLength < MAX_SSDT_HOOK_PROCESS_ID_NUMBER)
	{
		global_ProtectedProcessIds[global_ProtectedProcessIdsLength++] = aProcessId;

		return TRUE;
	}

	return FALSE;
}

BOOLEAN RemoveProcessFromProtectList(ULONG aProcessId)
{
	ULONG index = CheckProcessIsInProtectList(aProcessId);

	if (index != -1)
	{
		global_ProtectedProcessIds[index] = global_ProtectedProcessIds[global_ProtectedProcessIdsLength--];

		return TRUE;
	}

	return FALSE;
}

BOOLEAN InsertProcessInHideList(ULONG aProcessId)
{
	if (CheckProcessIsInHideList(aProcessId) == -1 && global_HidedProcessIdsLength < MAX_SSDT_HOOK_PROCESS_ID_NUMBER)
	{
		global_HidedProcessIds[global_HidedProcessIdsLength++] = aProcessId;

		return TRUE;
	}

	return FALSE;
}

BOOLEAN RemoveProcessFromHideList(ULONG aProcessId)
{
	ULONG index = CheckProcessIsInHideList(aProcessId);

	if (index != -1)
	{
		global_HidedProcessIds[index] = global_HidedProcessIds[global_HidedProcessIdsLength--];

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

	pOldNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)oldSysServiceAddr[SYSCALL_INDEX(ZwQuerySystemInformation)];

	status = pOldNtQuerySystemInformation(SystemInformationClass, 
		SystemInformation, SystemInformationLength, ReturnLength);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (SystemProcessInformation != SystemInformationClass)
	{
		return status;
	}

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
					// ��ǰ���ص���������ǽ������������һ��
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

	// ͨ�����̾������øý�������Ӧ��FileObject�������������ǽ��̶��󣬻�õľ���EPROCESS����
	NTSTATUS status = ObReferenceObjectByHandle(ProcessHandle, FILE_READ_DATA, NULL, KernelMode, (PVOID *)&pEProcess, NULL);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	// ����SSDT��ԭ����NtTerminateProcess��ַ
	pOldNtTerminateProcess = (NTTERMINATEPROCESS)oldSysServiceAddr[SYSCALL_INDEX(ZwTerminateProcess)];

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

	// �Ǳ������̣������������NtTerminalProcess��������
	status = pOldNtTerminateProcess(ProcessHandle, ExitStatus);

	return status;
}