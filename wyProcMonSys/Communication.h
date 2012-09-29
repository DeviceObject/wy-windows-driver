
#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _PMS_MESSAGE
{
	ULONG iSize;
	CHAR iProcessName[512];
	HANDLE iProcessId;
	ULONG iOperationType;
	CHAR iFilePath[512];
	ULONG iResult;
	
} PMS_MESSAGE, *PPMS_MESSAGE;

NTSTATUS PortConnect(__in PFLT_PORT ClientPort, __in_opt PVOID ServerPortCookie,
					 __in_bcount_opt(SizeOfContext) PVOID ConnectionContext, 
					 __in ULONG SizeOfContext, __deref_out_opt PVOID *ConnectionCookie);

VOID PortDisconnect(__in_opt PVOID ConnectionCookie);

NTSTATUS SendMessageToUserMode(PPMS_MESSAGE aMessage);

#ifdef __cplusplus
}
#endif

#endif