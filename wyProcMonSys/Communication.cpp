
#include "Public.h"
#include "Communication.h"

// 
// 一些文字声明
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

	// 设置用户进程和端口
	globalData.iUserPort = ClientPort;
	globalData.iUserProcess = PsGetCurrentProcess();

	KdPrint(("[wyprocmonsys] 端口已连接，端口号：0x%p\n", ClientPort));

	return STATUS_SUCCESS;
}

VOID PortDisconnect(__in_opt PVOID ConnectionCookie)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ConnectionCookie);

	KdPrint(("[wyprocmonsys] 端口已断开，端口号：0x%p\n", globalData.iUserPort));

	// 关闭客户连接端口
	FltCloseClientPort(globalData.iFilter, &globalData.iUserPort);

	// 清空用户进程信息
	globalData.iUserProcess = NULL;

	return ;
}

NTSTATUS SendMessageToUserMode(PPMS_MESSAGE aMessage)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(aMessage);

	// 由于只用发送不用等待，这里可以这样处理
	LARGE_INTEGER waitTime;
	waitTime.QuadPart = Int32x32To64(1 * 1000, -10000);// 等待1秒钟
	FltSendMessage(globalData.iFilter, &globalData.iUserPort, aMessage, aMessage->iSize, NULL, NULL, &waitTime);

	return STATUS_SUCCESS;
}