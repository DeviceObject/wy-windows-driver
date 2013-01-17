#include "SSDTHook.h"

ULONG oldSysServiceAddr[MAX_SYSTEM_SERVICE_NUMBER] = {0};

/**
 *
 */
VOID DisableWriteProtect(ULONG oldAttr)
{
	_asm
	{
		mov eax, oldAttr;
		mov cr0, eax;
		sti;
	}
}

VOID EnableWriteProtect(PULONG pOldAttr)
{
	ULONG uAttr = 0;

	_asm
	{
		cli;
		mov eax, cr0;
		mov uAttr, eax;
		and eax, 0FFFEFFFFh; // CR0 16 Bit = 0;
		mov cr0, eax;
	}

	// 保留原有CR0属性
	*pOldAttr = uAttr;
}

VOID BackupSysServicesTable()
{
	ULONG i = 0;

	for (i = 0; (i < KeServiceDescriptorTable->ntoskrnl.NumberOfService) && (i < MAX_SYSTEM_SERVICE_NUMBER); ++i)
	{
		oldSysServiceAddr[i] = KeServiceDescriptorTable->ntoskrnl.ServiceCounterTableBase[i];

		KdPrint(("\\Function Information { Number: 0x%04X , Address: %08X}", i, oldSysServiceAddr[i]));
	}
}

NTSTATUS InstallSysServiceHook(ULONG oldService, ULONG newService)
{
	ULONG uOldAttr = 0;

	EnableWriteProtect(&uOldAttr);

	SYSCALL_FUNCTION(oldService) = newService;

	DisableWriteProtect(uOldAttr);

	return STATUS_SUCCESS;
}

NTSTATUS UnInstallSysServiceHook(ULONG oldService)
{
	ULONG uOldAttr = 0;

	EnableWriteProtect(&uOldAttr);

	SYSCALL_FUNCTION(oldService) = oldSysServiceAddr[SYSCALL_INDEX(oldService)];

	DisableWriteProtect(uOldAttr);

	return STATUS_SUCCESS;
}