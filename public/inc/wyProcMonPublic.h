
#ifndef _WY_PROC_MON_PUBLIC_H_
#define _WY_PROC_MON_PUBLIC_H_

/**
 * ���ļ��������ļ�����
 *
 */

const PWSTR wyProcMonSysPortName = L"\\wyProcMonSysPort";

#define MessageBufferLength	4096

typedef struct _MESSAGE_NOTIFICATION
{
	// ���͵��ֽ���
	ULONG ByteToSend;

	// ����������չΪ��ʶλ
	ULONG Reserved;

	// ���ݻ�����
	UCHAR Buffer[MessageBufferLength];

} MESSAGE_NOTIFICATION, *PMESSAGE_NOTIFICATION;

#endif