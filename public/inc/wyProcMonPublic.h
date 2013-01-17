
#ifndef _WY_PROC_MON_PUBLIC_H_
#define _WY_PROC_MON_PUBLIC_H_

/**
 * 此文件定义了文件监视
 *
 */

const PWSTR wyProcMonSysPortName = L"\\wyProcMonSysPort";

#define MessageBufferLength	4096

typedef struct _MESSAGE_NOTIFICATION
{
	// 发送的字节数
	ULONG ByteToSend;

	// 保留，可扩展为标识位
	ULONG Reserved;

	// 数据缓冲区
	UCHAR Buffer[MessageBufferLength];

} MESSAGE_NOTIFICATION, *PMESSAGE_NOTIFICATION;

#endif