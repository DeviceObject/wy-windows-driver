#ifndef _SIMPLE_COMMUNICATE_U_H_
#define _SIMPLE_COMMUNICATE_U_H_

#ifdef __cplusplus
extern "C"
{
#endif	// __cplusplus

#include "CommunicatePublic.h"
#include <windows.h>

#define DriverPortName	L"\\SimpleCommunicatePort"

typedef struct _SimpleCommunicateMessageData
{
	UCHAR iData[2048];	// 2K数据区域

	ULONG iDataLength;

} SIMPLE_COMMUNICATE_DATA, *PSIMPLE_COMMUNICATE_DATA;

typedef struct _SimpleCommunicateMessageToUser
{
	// 过滤器消息头部
	FILTER_MESSAGE_HEADER iMessageHeader;

	// 消息实际数据
	SIMPLE_COMMUNICATE_DATA *iMessageData;

} SIMPLE_COMMUNICATE_MESSAGE_TO_USER, *PSIMPLE_COMMUNICATE_MESSAGE_TO_USER;

typedef struct _SimpleCommunicateMessageToDriver
{
	// 过滤器消息头部
	FILTER_REPLY_HEADER iReplyHeader;

	// 消息实际数据
	SIMPLE_COMMUNICATE_DATA *iMessageData;

} SIMPLE_COMMUNICATE_MESSAGE_TO_DRIVER, *PSIMPLE_COMMUNICATE_MESSAGE_TO_DRIVER;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _SIMPLE_COMMUNICATE_U_H_