
#include "Public.h"
#include "Communication.h"

// 
// һЩ��������
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PortConnect)
#pragma alloc_text(PAGE, PortDisconnect)
#pragma alloc_text(PAGE, SendMessageToUserMode)
#endif

NTSTATUS PortConnect(__in PFLT_PORT ClientPort, __in_opt PVOID ServerPortCookie, __in_bcount_opt(SizeOfContext) PVOID ConnectionContext, __in ULONG SizeOfContext, __deref_out_opt PVOID *ConnectionCookie)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ClientPort);
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);

	ASSERT(globalData.iUserPort == NULL);
	ASSERT(globalData.iUserProcess == NULL);

	// �����û����̺Ͷ˿�
	globalData.iUserPort = ClientPort;
	globalData.iUserProcess = PsGetCurrentProcess();

	KdPrint(("[wyprocmonsys] �˿������ӣ��˿ںţ�0x%p\n", ClientPort));

	return STATUS_SUCCESS;
}

VOID PortDisconnect(__in_opt PVOID ConnectionCookie)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ConnectionCookie);

	KdPrint(("[wyprocmonsys] �˿��ѶϿ����˿ںţ�0x%p\n", globalData.iUserPort));

	// �رտͻ����Ӷ˿�
	FltCloseClientPort(globalData.iFilter, &globalData.iUserPort);

	// ����û�������Ϣ
	globalData.iUserProcess = NULL;

	return ;
}

NTSTATUS SendMessageToUserMode(PPMS_MESSAGE aMessage)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(aMessage);

	// ����ֻ�÷��Ͳ��õȴ������������������
	LARGE_INTEGER waitTime;
	waitTime.QuadPart = Int32x32To64(1 * 1000, -10000);// �ȴ�1����
	FltSendMessage(globalData.iFilter, &globalData.iUserPort, aMessage, aMessage->iSize, NULL, NULL, &waitTime);

	return STATUS_SUCCESS;
}