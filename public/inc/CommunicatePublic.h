#ifndef _SIMPLE_COMMUNICATE_PUBLIC_H_
#define _SIMPLE_COMMUNICATE_PUBLIC_H_

/**
 * ���ļ���Ϊ��MiniFilterͨ�ŵ��û�̬����ʹ�õĹ���ͷ�ļ������������п��Բ�������fltuser.h
 * ���ߣ�����
 * ���ڣ�2012-12-26
 */

#ifdef __cplusplus
extern "C"
{
#endif	// __cplusplus

#include <windows.h>

// ���������û�̬����Ϣͷ��
typedef struct _FILTER_MESSAGE_HEADER 
{
	// ���ص���Ϣ���ȣ�������ͷ�����ݽṹ��
	ULONG ReplyLength;

	// ��ϢID����Ψһ��
	ULONGLONG MessageId;

} FILTER_MESSAGE_HEADER, *PFILTER_MESSAGE_HEADER;

// �û�̬���͵���������Ϣͷ��
typedef struct _FILTER_REPLY_HEADER
{
	// ��ǰ������״̬�������룩
	NTSTATUS Status;

	// ��ϢID����Ψһ��
	ULONGLONG MessageId;

} FILTER_REPLY_HEADER, *PFILTER_REPLY_HEADER;

// �û�̬���ں�̬ͨ����غ�������
HRESULT
WINAPI
FilterConnectCommunicationPort(
							   __in LPCWSTR lpPortName,
							   __in DWORD dwOptions,
							   __in_bcount_opt(wSizeOfContext) LPCVOID lpContext,
							   __in WORD wSizeOfContext,
							   __in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes ,
							   __deref_out HANDLE *hPort
							   );

HRESULT
WINAPI
FilterSendMessage (
				   __in HANDLE hPort,
				   __in_bcount_opt(dwInBufferSize) LPVOID lpInBuffer,
				   __in DWORD dwInBufferSize,
				   __out_bcount_part_opt(dwOutBufferSize,*lpBytesReturned) LPVOID lpOutBuffer,
				   __in DWORD dwOutBufferSize,
				   __out LPDWORD lpBytesReturned
				   );

HRESULT
WINAPI
FilterGetMessage (
				  __in HANDLE hPort,
				  __out_bcount(dwMessageBufferSize) PFILTER_MESSAGE_HEADER lpMessageBuffer,
				  __in DWORD dwMessageBufferSize,
				  __inout LPOVERLAPPED lpOverlapped
				  );

HRESULT
WINAPI
FilterReplyMessage (
					__in HANDLE hPort,
					__in_bcount(dwReplyBufferSize) PFILTER_REPLY_HEADER lpReplyBuffer,
					__in DWORD dwReplyBufferSize
					);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif	// _SIMPLE_COMMUNICATE_PUBLIC_H_