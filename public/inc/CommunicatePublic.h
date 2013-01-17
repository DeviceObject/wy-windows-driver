#ifndef _SIMPLE_COMMUNICATE_PUBLIC_H_
#define _SIMPLE_COMMUNICATE_PUBLIC_H_

/**
 * 此文件作为与MiniFilter通信的用户态程序使用的公共头文件，这样工程中可以不必引用fltuser.h
 * 作者：王煜
 * 日期：2012-12-26
 */

#ifdef __cplusplus
extern "C"
{
#endif	// __cplusplus

#include <windows.h>

// 驱动发到用户态的消息头部
typedef struct _FILTER_MESSAGE_HEADER 
{
	// 返回的消息长度（包含了头部数据结构）
	ULONG ReplyLength;

	// 消息ID，是唯一的
	ULONGLONG MessageId;

} FILTER_MESSAGE_HEADER, *PFILTER_MESSAGE_HEADER;

// 用户态发送到驱动的消息头部
typedef struct _FILTER_REPLY_HEADER
{
	// 当前操作的状态（错误码）
	NTSTATUS Status;

	// 消息ID，是唯一的
	ULONGLONG MessageId;

} FILTER_REPLY_HEADER, *PFILTER_REPLY_HEADER;

// 用户态与内核态通信相关函数定义
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