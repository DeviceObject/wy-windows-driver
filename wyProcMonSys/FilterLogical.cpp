
#include "Public.h"
#include "wyProcMonSys.h"
#include "Communication.h"

#define MAX_PROC_NAME_LEN 256
PCHAR GetCurrentProcessName()
{
	PEPROCESS curproc;
	char *nameptr = NULL;
	ULONG i = 0;;
	static CHAR  szName[MAX_PROC_NAME_LEN];

	// We only try and get the name if we located the name offset
	if( globalData.iProcessNameOffsetInPEPROCESS ) 
	{
		// Get a pointer to the current process block
		curproc = PsGetCurrentProcess();

		// Dig into it to extract the name. Make sure to leave enough room
		// in the buffer for the appended process ID.
		nameptr   = (PCHAR) curproc + globalData.iProcessNameOffsetInPEPROCESS;
		//strncpy( szName, nameptr, MAX_PROC_NAME_LEN-1 );
		RtlCopyMemory(szName, nameptr, MAX_PROC_NAME_LEN-1);
		szName[MAX_PROC_NAME_LEN-1] = 0;
	} 
	else
	{
		//strcpy( szName, "???");
		RtlCopyMemory(szName, "???", 3);
		szName[3] = '\0';
	}
	return szName;
}

VOID BuildAndSendMsgToUerMode(__in PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in ULONG aOperationType)
{
	PPMS_MESSAGE message = (PPMS_MESSAGE)ExAllocatePool(PagedPool, sizeof(PMS_MESSAGE));
	message->iSize = sizeof(PMS_MESSAGE);

	// �õ���ǰ����
	PCHAR pName = GetCurrentProcessName();
	RtlCopyMemory(message->iProcessName, pName, strlen(pName));

	// �õ���ǰ����ID
	message->iProcessId = PsGetCurrentProcessId();

	// �õ�����˵��
	message->iOperationType = aOperationType;

	// �õ��ļ�·��
	ANSI_STRING tmpFilePath;
	RtlUnicodeStringToAnsiString(&tmpFilePath, &FltObjects->FileObject->FileName, TRUE);
	RtlCopyMemory(message->iFilePath, tmpFilePath.Buffer, tmpFilePath.Length);
	RtlFreeAnsiString(&tmpFilePath);

	// �õ����
	message->iResult = Data->IoStatus.Status;

	// ���û�̬������Ϣ
	if (globalData.iStartFilter)
		SendMessageToUserMode(message);

	// ���Ҫ�ͷ��Ǹ�����
	ExFreePool(message);
	message = NULL;

	return ;
}

FLT_POSTOP_CALLBACK_STATUS PMSPostCreate(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	KdPrint(("PMSPostCreate\n"));

	BuildAndSendMsgToUerMode(Data, FltObjects, 0);

	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_POSTOP_CALLBACK_STATUS PMSPostRead(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	KdPrint(("PMSPostRead\n"));

	BuildAndSendMsgToUerMode(Data, FltObjects, 1);

	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_POSTOP_CALLBACK_STATUS PMSPostWrite(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __in_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	KdPrint(("PMSPostWrite\n"));

	BuildAndSendMsgToUerMode(Data, FltObjects, 2);

	return FLT_POSTOP_FINISHED_PROCESSING;
}