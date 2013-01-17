#include "SimpleCommunicateU.h"
#include <stdlib.h>
//#include <stdio.h>
#include <winioctl.h>
#include <string.h>
#include <crtdbg.h>
#include <assert.h>

typedef struct _PORT_STATE
{
	HANDLE port;
	HANDLE complete;
} PORT_STATE, *PPORT_STATE;

DWORD WINAPI WorkingThread(LPVOID lpParam)
{
	// 实际的工作线程，用于发送与接收信息

	PPORT_STATE pPortState = (PPORT_STATE)lpParam;
	if (pPortState == NULL)
	{
		exit(0);
	}

	HANDLE hPort = pPortState->port;

	//// 获取完成端口状态
	//DWORD outSize = 0;
	//ULONG_PTR key = 0;
	//LPOVERLAPPED overLap = NULL;
	//BOOL result = GetQueuedCompletionStatus(pPortState->complete, &outSize, &key, &overLap, INFINITE);

	//PSIMPLE_COMMUNICATE_DATA communicateData = NULL;

	//communicateData = CONTAINING_RECORD(overLap, SIMPLE_COMMUNICATE_DATA, overLap);

	SIMPLE_COMMUNICATE_MESSAGE_TO_USER communicateMsgToUser;
	SIMPLE_COMMUNICATE_MESSAGE_TO_DRIVER communicateMsgToDriver;

	while (TRUE)
	{
		// 清空缓冲区
		memset(&communicateMsgToUser.iMessageData, 0, sizeof(SIMPLE_COMMUNICATE_DATA));
		memset(&communicateMsgToDriver.iReplyHeader, 0, sizeof(SIMPLE_COMMUNICATE_DATA));

		// 获取驱动发来的消息，确定连接成功
		HRESULT hr = FilterGetMessage(hPort, (PFILTER_MESSAGE_HEADER)&communicateMsgToUser, 
			sizeof(SIMPLE_COMMUNICATE_MESSAGE_TO_USER), NULL);

		if (hr != S_OK)
		{
			OutputDebugString(L"get msg from kernel failed .\n");
			continue;
		}

		// 在用户态处理一些逻辑
		communicateMsgToDriver.iReplyHeader.Status = 0;
		communicateMsgToDriver.iReplyHeader.MessageId = communicateMsgToUser.iMessageHeader.MessageId;

		communicateMsgToDriver.iMessageData->iDataLength = (ULONG)strlen("this is user-mode message .\n") + 1;
		memcpy_s(communicateMsgToDriver.iMessageData->iData, 2048, "this is user-mode message .\n",
			communicateMsgToDriver.iMessageData->iDataLength);

		// 返回结果给驱动
		hr = FilterReplyMessage(hPort, (PFILTER_REPLY_HEADER)&communicateMsgToDriver, 
			sizeof(communicateMsgToDriver.iReplyHeader) + sizeof(communicateMsgToDriver.iMessageData));

		// 无论发送信息的结果如何，都等待下一次驱动的消息
		continue;
	}

	return 0;
}

void main()
{
	// 打开一个与过滤驱动通信的端口
	HANDLE port = NULL;
	HRESULT hr = FilterConnectCommunicationPort(DriverPortName, 0, NULL, 0, NULL, &port);
	if (IS_ERROR(hr))
	{
		return ;
	}

	//// 创建一个完成端口
	//HANDLE complete = CreateIoCompletionPort(port, NULL, 0, 1);

	PORT_STATE portState;
	portState.port = port;
	portState.complete = NULL;

	HANDLE hThread = CreateThread(NULL, 0, WorkingThread, &portState, 0, NULL);
	if (hThread == NULL)
	{
		return ;
	}

	while(1)
	{
		// 主循环，每隔1秒执行一次
		Sleep(1000);
	}
}