
#include "SimpleCommunicate.h"

// ����ȫ�ֱ���
GLOBAL_DATA globalData;

#ifdef __cplusplus
extern "C"
{
#endif
// ǰ������һЩ�ڲ�����
/**
 * ���Ӷ˿�
 * ������
 *	ClientPort
 *	ServerPortCookie 
 *	ConnectionContext
 *	SizeOfContext
 *	ConnectionCookie
 */
NTSTATUS ConnectPort(__in PFLT_PORT ClientPort, __in_opt PVOID ServerPortCookie, 
					 __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
					 __in ULONG SizeOfContext, __deref_out_opt PVOID *ConnectionCookie);

/**
 * �Ͽ��˿�����
 * ������
 *	ConnectionCookie
 */
void DisconnectPort(__in_opt PVOID ConnectionCookie);

/**
 * ��Ϣ����ص�����
 * ������
 *	PortCookie
 *	InputBuffer
 *	InputBufferLength
 *	OutputBuffer
 *	OutputBufferLength
 *	ReturnOutputBufferLength
 */
NTSTATUS MessageNofity(IN PVOID PortCookie, IN PVOID InputBuffer OPTIONAL, 
					   IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL,
					   IN ULONG OutputBufferLength, OUT PULONG ReturnOutputBufferLength);



// �ڴ����
//#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, ConnectPort)
#pragma alloc_text(PAGE, DisconnectPort)
#pragma alloc_text(PAGE, MessageNofity)
//#endif

// IRP��������
const FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_OPERATION_END}
};

// ������ע��ṹ
const FLT_REGISTRATION FilterRegistration = {
	sizeof( FLT_REGISTRATION ),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags
	NULL,								//  Context Registration.
	Callbacks,                          //  Operation callbacks
	DriverUnload,						//  FilterUnload
	NULL,								//  InstanceSetup
	NULL,								//  InstanceQueryTeardown
	NULL,                               //  InstanceTeardownStart
	NULL,                               //  InstanceTeardownComplete
	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent
};

// ����ʵ��
NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(RegistryPath);

	status = FltRegisterFilter(DriverObject, &FilterRegistration, &globalData.iFilter);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	// ��ʼ���˿�����
	UNICODE_STRING uniString;
	RtlInitUnicodeString(&uniString, DriverPortName);

	// ������ȫ������
	PSECURITY_DESCRIPTOR sd = NULL;
	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (NT_SUCCESS(status))
	{
		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, &uniString, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, sd);

		status = FltCreateCommunicationPort(globalData.iFilter, &globalData.iServerPort, &oa,
			NULL, ConnectPort, DisconnectPort, MessageNofity, 1);

		FltFreeSecurityDescriptor(sd);

		if (NT_SUCCESS(status))
		{
			status = FltStartFiltering(globalData.iFilter);

			if (NT_SUCCESS(status))
			{
				return status;
			}

			FltCloseCommunicationPort(globalData.iServerPort);
		}
	}

	FltUnregisterFilter(globalData.iFilter);

	return status;
}

NTSTATUS DriverUnload(__in FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);

	FltCloseCommunicationPort(globalData.iServerPort);

	FltUnregisterFilter(globalData.iFilter);

	return STATUS_SUCCESS;
}

NTSTATUS ConnectPort(__in PFLT_PORT ClientPort, __in_opt PVOID ServerPortCookie, 
					 __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
					 __in ULONG SizeOfContext, __deref_out_opt PVOID *ConnectionCookie)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ClientPort);
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);

	ASSERT(globalData.iClientPort == NULL);
	ASSERT(globalData.iUserProcess == NULL);

	globalData.iUserProcess = PsGetCurrentProcess();
	globalData.iClientPort = ClientPort;

	KdPrint(("[SimpleConmmunicate] Someone has connected to this driver ! processId : %x | clientPort : %x", 
		globalData.iUserProcess, globalData.iClientPort));

	// �������������пͻ������ӵ������������Է���һ�������ӵ�״̬��Ϣ
	CHAR msg[512] = {0};
	RtlCopyString(msg, 512, "SimpleConmmunicate has recved your message .\n");

	NTSTATUS status = FltSendMessage(globalData.iFilter, &globalData.iClientPort, (PVOID)msg, 512, NULL, NULL, NULL);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("[SimpleConmmunicate] Reply message failed .\n"));
	}

	return STATUS_SUCCESS;
}

void DisconnectPort(__in_opt PVOID ConnectionCookie)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ConnectionCookie);

	FltCloseClientPort(globalData.iFilter, &globalData.iClientPort);

	KdPrint(("[SimpleConmmunicate] Someone has disconnected from this driver !"));

	globalData.iClientPort = NULL;
}

NTSTATUS MessageNofity(IN PVOID PortCookie, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength,
		OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PULONG ReturnOutputBufferLength)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

	NTSTATUS status = STATUS_SUCCESS;

	CHAR *in_data = (CHAR *)ExAllocatePool(PagedPool, InputBufferLength + 2);
	RtlZeroMemory(in_data, InputBufferLength + 2);

	ANSI_STRING dataStr;
	RtlInitAnsiString(&dataStr, in_data);
	KdPrint(("%z\n", &dataStr));

	// ���û�̬������Ϣ
	ANSI_STRING sendData;
	RtlInitAnsiString(&sendData, "Driver has recv message !");

	status = FltSendMessage(globalData.iFilter, &globalData.iClientPort, 
		sendData.Buffer, sendData.Length, NULL, NULL, NULL);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("Driver send message failed ...\n"));
	}

	ExFreePool(in_data);
	in_data = NULL;

	return status;
}


#ifdef __cplusplus
}
#endif	// __cplusplus