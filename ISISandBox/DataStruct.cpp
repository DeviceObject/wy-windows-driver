#include "DataStruct.h"
#include <Ntstrsafe.h>

PROTECT_PROCESS_LIST ProtectProcessList;

NTSTATUS InitializeProctectProcessList()
{
	// ��ʼ��������
	KeInitializeSpinLock(&ProtectProcessList.Lock);

	// ��ʼ������
	InitializeListHead(&ProtectProcessList.HeadList);

	// ��ʼ��LookAside�ṹ
	ExInitializePagedLookasideList(&ProtectProcessList.PageList, NULL, NULL, 0, sizeof(PROTECT_PROCESS), 'ppBx', 0);

	return STATUS_SUCCESS;
}

VOID DestroyProctectProcessList()
{
	// ��������
	CleanupProtectProcess();

	// ����Lookaside�ṹ
	ExDeletePagedLookasideList(&ProtectProcessList.PageList);

	// ����������
	// WDKû���ἰ�����������ʱ������������
}

NTSTATUS InsertProtectProcess(__in HANDLE ProcessId)
{
	NTSTATUS status = CheckProtectProcess(ProcessId);
	if (NT_SUCCESS(status))
	{
		KdPrint(("[ISISandBox] InsertProtectProcess STATUS_ALIAS_EXISTS ProcessId : %u.\n", ProcessId));
		return STATUS_ALIAS_EXISTS;
	}

	PPROTECT_PROCESS element = (PPROTECT_PROCESS)ExAllocateFromPagedLookasideList(&ProtectProcessList.PageList);
	if (element == NULL)
	{
		KdPrint(("[ISISandBox] InsertProtectProcess STATUS_MEMORY_NOT_ALLOCATED.\n"));
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	element->ProcessId = ProcessId;

	InsertTailList(&ProtectProcessList.HeadList, &element->ListEntry);
	KdPrint(("[ISISandBox] InsertProtectProcess ProcessId : %u.\n", element->ProcessId));

	return STATUS_SUCCESS;
}

NTSTATUS CheckProtectProcess(__in HANDLE ProcessId)
{
	// ����Ҫ���ж������Ƿ�Ϊ��
	if (IsListEmpty(&ProtectProcessList.HeadList))
		return STATUS_NOT_FOUND;

	KIRQL irql;
	KeAcquireSpinLock(&ProtectProcessList.Lock, &irql);

	// ����ͷ
	PLIST_ENTRY pList = ProtectProcessList.HeadList.Flink;

	BOOLEAN isFind = FALSE;
	while (pList != &ProtectProcessList.HeadList)
	{
		PPROTECT_PROCESS element = CONTAINING_RECORD(pList, PROTECT_PROCESS, ListEntry);

		if (element->ProcessId == ProcessId)
		{
			KdPrint(("[ISISandBox] CheckProtectProcess Find ProcessId : %u.\n", element->ProcessId));
			isFind = TRUE;
			break;
		}

		pList = pList->Flink;
	}

	KeReleaseSpinLock(&ProtectProcessList.Lock, irql);

	if (isFind)
		return STATUS_SUCCESS;
	else
	{
		KdPrint(("[ISISandBox] CheckProtectProcess Cannot find ProcessId : %u.\n", ProcessId));
		return STATUS_NOT_FOUND;
	}
}

NTSTATUS RemoveProtectProcess(__in HANDLE ProcessId)
{
	// ����Ҫ���ж������Ƿ�Ϊ��
	if (IsListEmpty(&ProtectProcessList.HeadList))
		return STATUS_SUCCESS;

	KIRQL irql;
	KeAcquireSpinLock(&ProtectProcessList.Lock, &irql);

	// ����ͷ
	PLIST_ENTRY pList = ProtectProcessList.HeadList.Flink;

	BOOLEAN isFind = FALSE;
	while (pList != &ProtectProcessList.HeadList)
	{
		PPROTECT_PROCESS element = CONTAINING_RECORD(pList, PROTECT_PROCESS, ListEntry);

		if (element->ProcessId == ProcessId)
		{
			pList->Blink->Flink = pList->Flink;
			pList->Flink->Blink = pList->Blink;
			pList = pList->Flink;

			// �����ָ�붼�Ѿ��Ͽ��ˣ��������в�����element�ˣ�����ֻ��Ҫ�����ڴ��ͷž�OK��
			ExFreeToPagedLookasideList(&ProtectProcessList.PageList, element);
			KdPrint(("[ISISandBox] RemoveProtectProcess Delete process | ProcessId : %u.\n", ProcessId));

			isFind = TRUE;
			break;
		}

		pList = pList->Flink;
	}

	KeReleaseSpinLock(&ProtectProcessList.Lock, irql);

	if (isFind)
		return STATUS_SUCCESS;
	else
	{
		KdPrint(("[ISISandBox] RemoveProtectProcess Cannot find ProcessId : %u.\n", ProcessId));
		return STATUS_NOT_FOUND;
	}
}

NTSTATUS CleanupProtectProcess()
{
	if (IsListEmpty(&ProtectProcessList.HeadList))
		return STATUS_SUCCESS;

	KIRQL irql;
	KeAcquireSpinLock(&ProtectProcessList.Lock, &irql);

	// ����ͷ
	PLIST_ENTRY pList = ProtectProcessList.HeadList.Flink;

	while (pList != &ProtectProcessList.HeadList)
	{
		PPROTECT_PROCESS element = CONTAINING_RECORD(pList, PROTECT_PROCESS, ListEntry);

		pList->Blink->Flink = pList->Flink;
		pList->Flink->Blink = pList->Blink;
		pList = pList->Flink;

		// �����ָ�붼�Ѿ��Ͽ��ˣ��������в�����element�ˣ�����ֻ��Ҫ�����ڴ��ͷž�OK��
		ExFreeToPagedLookasideList(&ProtectProcessList.PageList, element);
	}

	KeReleaseSpinLock(&ProtectProcessList.Lock, irql);

	KdPrint(("[ISISandBox] CleanupProtectProcess Cleanup all process.\n"));
	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

MATCH_EXPRESSION_LIST MatchExpressionList;

NTSTATUS InitializeMatchExpressionList()
{
	// ��ʼ��������
	KeInitializeSpinLock(&MatchExpressionList.Lock);

	// ��ʼ������
	InitializeListHead(&MatchExpressionList.HeadList);

	// ��ʼ��LookAside�ṹ
	ExInitializePagedLookasideList(&MatchExpressionList.PageList, NULL, NULL, 0, sizeof(MATCH_EXPRESSION), 'meBx', 0);

	return STATUS_SUCCESS;
}

VOID DestroyMatchExpressionList()
{
	// ��������
	CleanupMatchExpression();

	// ����Lookaside�ṹ
	ExDeletePagedLookasideList(&MatchExpressionList.PageList);

	// ����������
	// WDKû���ἰ�����������ʱ������������
}

NTSTATUS InsertMatchExpression(__in WCHAR Expression[], __in ULONG ExpressionLength)
{
	NTSTATUS status = CheckMatchExpression(Expression, ExpressionLength);
	if (NT_SUCCESS(status))
	{
		KdPrint(("[ISISandBox] InsertMatchExpression STATUS_ALIAS_EXISTS Expression : %S.\n", Expression));
		return STATUS_ALIAS_EXISTS;
	}

	PMATCH_EXPRESSION element = (PMATCH_EXPRESSION)ExAllocateFromPagedLookasideList(&MatchExpressionList.PageList);
	if (element == NULL)
	{
		KdPrint(("[ISISandBox] InsertProtectProcess STATUS_MEMORY_NOT_ALLOCATED.\n"));
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	RtlZeroMemory(element, sizeof(MATCH_EXPRESSION));
	RtlCopyMemory(element->Expression, Expression, sizeof(WCHAR) * ExpressionLength);
	element->Length = ExpressionLength * sizeof(WCHAR);

	InsertTailList(&MatchExpressionList.HeadList, &element->ListEntry);
	KdPrint(("[ISISandBox] InsertMatchExpression Expression : %S.\n", Expression));

	return STATUS_SUCCESS;
}

NTSTATUS CheckMatchExpression(__in WCHAR Expression[], __in ULONG ExpressionLength)
{
	// ����Ҫ���ж������Ƿ�Ϊ��
	if (IsListEmpty(&MatchExpressionList.HeadList))
		return STATUS_NOT_FOUND;

	KIRQL irql;
	KeAcquireSpinLock(&MatchExpressionList.Lock, &irql);

	// ����ͷ
	PLIST_ENTRY pList = MatchExpressionList.HeadList.Flink;

	BOOLEAN isFind = FALSE;
	while (pList != &MatchExpressionList.HeadList)
	{
		PMATCH_EXPRESSION element = CONTAINING_RECORD(pList, MATCH_EXPRESSION, ListEntry);

		if (RtlCompareMemory(element->Expression, Expression, ExpressionLength) == ExpressionLength)
		{
			KdPrint(("[ISISandBox] CheckMatchExpression Find Expression : %S.\n", Expression));
			isFind = TRUE;
			break;
		}

		pList = pList->Flink;
	}

	KeReleaseSpinLock(&MatchExpressionList.Lock, irql);

	if (isFind)
		return STATUS_SUCCESS;
	else
	{
		KdPrint(("[ISISandBox] CheckMatchExpression Cannot find Expression : %S.\n", Expression));
		return STATUS_NOT_FOUND;
	}
}

NTSTATUS RemoveMatchExpression(__in WCHAR Expression[], __in ULONG ExpressionLength)
{
	// ����Ҫ���ж������Ƿ�Ϊ��
	if (IsListEmpty(&MatchExpressionList.HeadList))
	{
		KdPrint(("[ISISandBox] RemoveMatchExpression -> List is empty.\n"));
		return STATUS_SUCCESS;
	}

	KdPrint(("[ISISandBox] RemoveMatchExpression -> Get spinlock.\n"));
	KIRQL irql;
	KeAcquireSpinLock(&MatchExpressionList.Lock, &irql);

	// ����ͷ
	PLIST_ENTRY pList = MatchExpressionList.HeadList.Flink;

	BOOLEAN isFind = FALSE;
	while (pList != &MatchExpressionList.HeadList)
	{
		PMATCH_EXPRESSION element = CONTAINING_RECORD(pList, MATCH_EXPRESSION, ListEntry);

		if (RtlCompareMemory(element->Expression, Expression, ExpressionLength) == ExpressionLength)
		{
			pList->Blink->Flink = pList->Flink;
			pList->Flink->Blink = pList->Blink;
			pList = pList->Flink;

			// �����ָ�붼�Ѿ��Ͽ��ˣ��������в�����element�ˣ�����ֻ��Ҫ�����ڴ��ͷž�OK��
			ExFreeToPagedLookasideList(&MatchExpressionList.PageList, element);
			KdPrint(("[ISISandBox] RemoveMatchExpression Delete expression | Expression : %S.\n", Expression));

			isFind = TRUE;
			break;
		}

		pList = pList->Flink;
	}

	KeReleaseSpinLock(&MatchExpressionList.Lock, irql);

	if (isFind)
		return STATUS_SUCCESS;
	else
	{
		KdPrint(("[ISISandBox] RemoveMatchExpression Cannot find expression : %S.\n", Expression));
		return STATUS_NOT_FOUND;
	}
}

NTSTATUS DoMatchExpression(PUNICODE_STRING Path)
{
	// �Ƿ���Ҫ�������ж��Ƿ����ض����Ŀ¼��

	// ����Ҫ���ж������Ƿ�Ϊ��
	if (IsListEmpty(&MatchExpressionList.HeadList))
		return STATUS_NOT_FOUND;

	UNICODE_STRING tmpPath;
	RtlZeroMemory(&tmpPath, sizeof(UNICODE_STRING));
	tmpPath.MaximumLength = sizeof(WCHAR) * 260;
	tmpPath.Buffer = (PWSTR)ExAllocatePool(NonPagedPool, tmpPath.MaximumLength);
	if (tmpPath.Buffer == NULL)
		return STATUS_NOT_FOUND;

	RtlCopyUnicodeString(&tmpPath, Path);

	KIRQL irql;
	KeAcquireSpinLock(&MatchExpressionList.Lock, &irql);

	// ����ͷ
	PLIST_ENTRY pList = MatchExpressionList.HeadList.Flink;

	BOOLEAN isMatched = FALSE;
	while (pList != &MatchExpressionList.HeadList)
	{
		PMATCH_EXPRESSION element = CONTAINING_RECORD(pList, MATCH_EXPRESSION, ListEntry);

		// ע�⣬�������������ڴ���ʵ��ͦƵ����
		// ������ﻹ�������⣬���ܾ�Ҫ��Looksaide�ṹ�������ڴ�������
		UNICODE_STRING Expression;
		RtlZeroMemory(&Expression, sizeof(UNICODE_STRING));
		Expression.MaximumLength = sizeof(WCHAR) * 260;
		Expression.Buffer = (PWSTR)ExAllocatePool(NonPagedPool, Expression.MaximumLength);
		if (Expression.Buffer == NULL)
		{
			KdPrint(("[ISISandBox] DoMatchExpression -> Alloc pool for Expression string buffer failed.\n"));
			pList = pList->Flink;
			continue;
		}

		RtlUnicodeStringCbCopyStringN(&Expression, element->Expression, sizeof(WCHAR) * 260);
		Expression.Length = (USHORT)element->Length;
		
		KdPrint(("[ISISandBox] Current expression : %wZ %u\n", &Expression, Expression.Length));
		KdPrint(("[ISISandBox] Current path : %wZ\n", &tmpPath));
		
		// ע�⣺���ﾭ���������������һ������Ϊ DRIVER_CORRUPTED_EXPOOL (C5)
		// ˵���˴����ڴ�Ķ�д��Ҫ��Ƚϸߣ�Ӧ��ʹ�����ȸ��ߵ��ڴ�ռ�
		// �����������ַ��������ڴ涼���ڴ˺����ж�̬����ķǷ�ҳ�ڴ棬Ӧ���ܽ������
		BOOLEAN ret = FsRtlIsNameInExpression(&Expression, &tmpPath, TRUE, NULL);

		ExFreePool(Expression.Buffer);

		if (ret)
		{
			KdPrint(("[ISISandBox] Match Expression OK : %wZ.\n", &tmpPath));
			isMatched = TRUE;
			break;
		}

		pList = pList->Flink;
	}

	KeReleaseSpinLock(&MatchExpressionList.Lock, irql);

	ExFreePool(tmpPath.Buffer);

	if (isMatched)
		return STATUS_SUCCESS;
	else
	{
		//KdPrint(("[ISISandBox] CheckMatchExpression Cannot find Expression : %wZ.\n", Expression));
		return STATUS_NOT_FOUND;
	}
}

NTSTATUS CleanupMatchExpression()
{
	if (IsListEmpty(&MatchExpressionList.HeadList))
		return STATUS_SUCCESS;

	KIRQL irql;
	KeAcquireSpinLock(&MatchExpressionList.Lock, &irql);

	// ����ͷ
	PLIST_ENTRY pList = MatchExpressionList.HeadList.Flink;

	while (pList != &MatchExpressionList.HeadList)
	{
		PMATCH_EXPRESSION element = CONTAINING_RECORD(pList, MATCH_EXPRESSION, ListEntry);

		pList->Blink->Flink = pList->Flink;
		pList->Flink->Blink = pList->Blink;
		pList = pList->Flink;

		// �����ָ�붼�Ѿ��Ͽ��ˣ��������в�����element�ˣ�����ֻ��Ҫ�����ڴ��ͷž�OK��
		ExFreeToPagedLookasideList(&MatchExpressionList.PageList, element);
	}

	KeReleaseSpinLock(&MatchExpressionList.Lock, irql);

	KdPrint(("[ISISandBox] CleanupMatchExpression Cleanup all expression.\n"));
	return STATUS_SUCCESS;
}


REDIRECT_DIR RedirectDirectory;

NTSTATUS InitializeRedirectDirectory()
{
	// ��ʼ��������
	KeInitializeSpinLock(&RedirectDirectory.Lock);

	return STATUS_SUCCESS;
}

NTSTATUS SetRedirectBaseDirectory(__in WCHAR Directory[], __in ULONG Length)
{
	KIRQL irql;
	KeAcquireSpinLock(&RedirectDirectory.Lock, &irql);

	RtlZeroMemory(RedirectDirectory.RedirectBaseDirectory, sizeof(WCHAR) * 260);
	RtlCopyMemory(RedirectDirectory.RedirectBaseDirectory, Directory, sizeof(WCHAR) * Length);
	RedirectDirectory.Length = Length * sizeof(WCHAR);

	KeReleaseSpinLock(&RedirectDirectory.Lock, irql);

	return STATUS_SUCCESS;
}

NTSTATUS CleanupRedirectBaseDirectory()
{
	KIRQL irql;
	KeAcquireSpinLock(&RedirectDirectory.Lock, &irql);

	RtlZeroMemory(RedirectDirectory.RedirectBaseDirectory, sizeof(WCHAR) * RedirectDirectory.Length);
	RedirectDirectory.Length = 0;

	KeReleaseSpinLock(&RedirectDirectory.Lock, irql);

	return STATUS_SUCCESS;
}

NTSTATUS GetRedirectBaseDirectory(__inout WCHAR Directory[260], __inout PULONG Length)
{
	KIRQL irql;
	KeAcquireSpinLock(&RedirectDirectory.Lock, &irql);

	RtlCopyMemory(Directory, RedirectDirectory.RedirectBaseDirectory, sizeof(WCHAR) * RedirectDirectory.Length);
	*Length = RedirectDirectory.Length;

	KeReleaseSpinLock(&RedirectDirectory.Lock, irql);

	return STATUS_SUCCESS;
}

NTSTATUS ChechRedirectBaseDirectory()
{
	NTSTATUS status = STATUS_NOT_FOUND;

	KIRQL irql;
	KeAcquireSpinLock(&RedirectDirectory.Lock, &irql);
	ULONG length = RedirectDirectory.Length;
	KeReleaseSpinLock(&RedirectDirectory.Lock, irql);

	if (length > 0)
		status = STATUS_SUCCESS;
	
	return status;
}