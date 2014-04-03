
#include "RedirectHelper.h"
#include "ISISandBox.h"
#include <fltkernel.h>


NTSTATUS IoCreateDirectoryInUserMode(__in PUNICODE_STRING RedirectPath)
{
	if (!RedirectPath)
	{
		KdPrint(("[BaseModule] IoCreateDirectoryInUserMode STATUS_INVALID_PARAMETER.\n"));
		return STATUS_INVALID_PARAMETER;
	}

	MESSAGE_REDIRECT_PATH message;
	message.MessageType = MESSAGE_CREATE_DIRECTORY_BY_USERMODE;
	RtlCopyMemory(message.RedirectPath, RedirectPath->Buffer, sizeof(WCHAR) * RedirectPath->Length);
	message.Length = RedirectPath->Length;

	MESSAGE_REPLY reply;
	ULONG replyLength = 0;
	LARGE_INTEGER li;
	li.QuadPart = 10 * 500; // 500ms

	NTSTATUS status = FltSendMessage(globalData.Filter, &globalData.ClientPort, &message,
		sizeof(MESSAGE_REDIRECT_PATH), &reply, &replyLength, &li);

	KdPrint(("[BaseModule] replyLength : %u\n", replyLength));

	if (!NT_SUCCESS(status))
	{
		KdPrint(("[BaseModule] IoCreateDirectoryInUserMode send message failed. %u\n", status));
		return status;
	}

	if (reply.MessageType == MESSAGE_CREATE_DIRECTORY_BY_USERMODE)
	{
		return reply.status;
	}
	else
		status = STATUS_NOT_FOUND;

	return status;
}

NTSTATUS IoCreateDirectory(__in PUNICODE_STRING Path, __in PFLT_FILTER Filter, __in PFLT_INSTANCE Instance)
{
	NTSTATUS status = STATUS_SUCCESS;

	WCHAR tmpPath[260];
	RtlZeroBytes(tmpPath, sizeof(WCHAR) * 260);

	WCHAR path2[260];
	RtlZeroBytes(path2, sizeof(WCHAR) * 260);
	RtlCopyMemory(path2, Path->Buffer, sizeof(WCHAR) * 260);
	KdPrint(("[BaseModule] IoCreateDirectory originString %S\n", path2));

	WCHAR createDir[260];
	RtlZeroBytes(createDir, sizeof(WCHAR) * 260);
	RtlCopyMemory(createDir, L"\\??\\", sizeof(WCHAR) * wcslen(L"\\??\\"));
	RtlCopyMemory(Add2Ptr(createDir, sizeof(WCHAR) * wcslen(L"\\??\\")), path2, sizeof(WCHAR) * wcslen(path2));
	KdPrint(("[BaseModule] IoCreateDirectory TotalString 1 %S\n", createDir));

	//size_t pathLength = wcslen(createDir);
	//for (size_t index = 7; index < pathLength; ++index)
	//{
	//	tmpPath[index] = createDir[index];
	//	if (tmpPath[index] == L'\\')
	//	{
	//		OBJECT_ATTRIBUTES objectAttributes;
	//		UNICODE_STRING objectName;
	//		RtlInitUnicodeString(&objectName, tmpPath);
	//		KdPrint(("[BaseModule] IoCreateDirectory %wZ\n", &objectName));
	//		InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_KERNEL_HANDLE, NULL, NULL);

	//		HANDLE hDirectory = NULL;
	//		IO_STATUS_BLOCK IoStatus;

	//		if (PASSIVE_LEVEL < KeGetCurrentIrql())
	//		{
	//			KdPrint(("[BaseModule] IRQL_ERROR.\n"));
	//			return STATUS_INVALID_DEVICE_REQUEST;
	//		}

	//		status = FltCreateFile(Filter, Instance, &hDirectory, GENERIC_ALL, &objectAttributes, &IoStatus, NULL,
	//			FILE_ATTRIBUTE_DIRECTORY, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_CREATE,
	//			FILE_DIRECTORY_FILE, NULL, 0, 0);

	//		if (!NT_SUCCESS(status))
	//		{
	//			KdPrint(("[BaseModule] IoCreateDirectory %wZ failed, %u\n", &objectName, status));
	//		}
	//	}
	//}

	//RtlZeroMemory(Path->Buffer, sizeof(WCHAR) * Path->Length);
	//RtlInitUnicodeString(Path, createDir);

	return status;
}

BOOLEAN FsIsRedirectPath(__in PWCH RedirectBase, __in PFLT_FILE_NAME_INFORMATION FileName, __in PFLT_VOLUME Volume)
{
	NTSTATUS status = STATUS_SUCCESS;
	PVOLUME_CONTEXT volumeContext = NULL;

	status = FltGetVolumeContext(globalData.Filter, Volume, (PFLT_CONTEXT*)&volumeContext);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("[BaseModule] FsIsRedirectPath -> FltGetVolumeContext failed. %u\n", status));
		return TRUE;
	}

	// Æ´½ÓÂ·¾¶
	WCHAR newFileName[260];
	RtlZeroMemory(newFileName, 260 * sizeof(WCHAR));

	RtlCopyMemory(newFileName, volumeContext->Name.Buffer, volumeContext->Name.Length);
	RtlCopyMemory(Add2Ptr(newFileName, volumeContext->Name.Length),
				  Add2Ptr(FileName->Name.Buffer, FileName->Volume.Length),
				  FileName->Name.Length - FileName->Volume.Length);

	UNICODE_STRING newFileNameUnicode;
	RtlInitUnicodeString(&newFileNameUnicode, newFileName);

	WCHAR exp[260];
	RtlZeroMemory(exp, 260 * sizeof(WCHAR));
	const PWCHAR dot = L"*";
	ULONG dotLen = (ULONG)(wcslen(dot) * sizeof(WCHAR));
	RtlCopyMemory(exp, dot, dotLen);
	RtlCopyMemory(Add2Ptr(exp, dotLen), RedirectBase, wcslen(RedirectBase) * sizeof(WCHAR));
	RtlCopyMemory(Add2Ptr(exp, dotLen + wcslen(RedirectBase) * sizeof(WCHAR)), dot, dotLen);

	UNICODE_STRING expression;
	RtlInitUnicodeString(&expression, exp);

	KdPrint(("[BaseModule] FsIsRedirectPath -> expression : %wZ\n", &expression));
	KdPrint(("[BaseModule] FsIsRedirectPath -> newFileNameUnicode : %wZ\n", &newFileNameUnicode));

	if (FsRtlIsNameInExpression(&expression, &newFileNameUnicode, TRUE, NULL))
		return TRUE;
	else
		return FALSE;
}