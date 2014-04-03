#ifndef _ISI_SANDBOX_H_
#define _ISI_SANDBOX_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#define SIMREP_STRING_TAG	'tSpR'
#define REPLACE_ROUTINE_NAME_STRING	L"IoReplaceFileObjectName"

typedef NTSTATUS (* PReplaceFileObjectName)
(
	__in PFILE_OBJECT FileObject,
	__in_bcount(FileNameLength) PWSTR NewFileName,
	__in USHORT FileNameLength
);

typedef struct _GLOBAL_DATA
{
	// �����������ʶ����������
	//PDRIVER_OBJECT DriverObject;

	PFLT_FILTER Filter;

	//  ���������յ�������
	PFLT_PORT ServerPort;

	//	�ͻ��˿ڣ����������û�̬
	PFLT_PORT ClientPort;

	// ����״̬
	BOOLEAN isWorking;

	KSPIN_LOCK WokingLock;

	PReplaceFileObjectName ReplaceFileNameFunction;

} GLOBAL_DATA, *PGLOBAL_DATA;

extern GLOBAL_DATA globalData;

typedef struct _VOLUME_CONTEXT 
{
	UNICODE_STRING Name;

	//UNICODE_STRING VolumeDeviceName;

	ULONG SectorSize;

} VOLUME_CONTEXT, *PVOLUME_CONTEXT;

#define MIN_SECTOR_SIZE	0x200
#define CONTEXT_TAG	'xcBS'

/**
 * ������ں���
 * ������
 *	DriverObject	��������
 *	RegistryPath	��������ע���·��
 */
DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry
(
	__in PDRIVER_OBJECT DriverObject, 
	__in PUNICODE_STRING RegistryPath
);

/**
 * ����ж�غ���
 * ������
 *	Flags	ж�ر��
 */
NTSTATUS DriverUnload
(
	__in FLT_FILTER_UNLOAD_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS ISISafeBoxPreCreate
(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects, 
	__deref_out_opt PVOID *CompletionContext
);

// 
// ����ͨ�ű��
#define MESSAGE_DRIVER_START_WORKING		0x00000001
#define MESSAGE_DRIVER_STOP_WORKING			0x00000002
#define MESSAGE_INSERT_PROTECT_PATH			0x10000001
#define MESSAGE_REMOVE_PROTECT_PATH			0x10000002
#define MESSAGE_REMOVE_ALL_PROTECT_PATH		0x10000004
#define MESSAGE_INSERT_REDIRECT_PATH		0x20000001
#define MESSAGE_REMOVE_REDIRECT_PATH		0x20000002
#define MESSAGE_REMOVE_ALL_REDIRECT_PATH	0x20000004
#define MESSAGE_INSERT_PROTECT_PROCESS		0x40000001
#define MESSAGE_REMOVE_PROTECT_PROCESS		0x40000002
#define MESSAGE_REMOVE_ALL_PROTECT_PROCESS	0x40000004

#define MESSAGE_CREATE_DIRECTORY_BY_USERMODE	0x70000001

#define MESSAGE_DEBUG_DEVICENAME_TO_DOSNAME	0x80000001

// ����ͨ�����ݶ���
typedef struct _MESSAGE_TYPE
{
	ULONG MessageType;

} MESSAGE_TYPE, *PMESSAGE_TYPE;

typedef struct _MESSAGE_REPLY
{
	ULONG MessageType;

	NTSTATUS status;

} MESSAGE_REPLY, *PMESSAGE_REPLY;

typedef struct _MESSAGE_PROTECT_PATH
{
	ULONG MessageType;

	WCHAR ProtectPath[260];

	ULONG Length;

} MESSAGE_PROTECT_PATH, *PMESSAGE_PROTECT_PATH;

typedef struct _MESSAGE_REDIRECT_PATH
{
	ULONG MessageType;

	WCHAR RedirectPath[260];

	ULONG Length;

} MESSAGE_REDIRECT_PATH, *PMESSAGE_REDIRECT_PATH;

typedef struct _MESSAGE_PROTECT_PROCESS
{
	ULONG MessageType;

	HANDLE ProcessId;

} MESSAGE_PROTECT_PROCESS, *PMESSAGE_PROTECT_PROCESS;

typedef struct _MESSAGE_TYPE_SEND
{
	FILTER_MESSAGE_HEADER Header;

	MESSAGE_TYPE Message;

} MESSAGE_TYPE_SEND, *PMESSAGE_TYPE_SEND;

typedef struct _MESSAGE_REPLY_SEND
{
	FILTER_MESSAGE_HEADER Header;

	MESSAGE_REPLY Message;

} MESSAGE_REPLY_SEND, *PMESSAGE_REPLY_SEND;

typedef struct _MESSAGE_PROTECT_PATH_SEND
{
	FILTER_MESSAGE_HEADER Header;

	MESSAGE_PROTECT_PATH Message;

} MESSAGE_PROTECT_PATH_SEND, *PMESSAGE_PROTECT_PATH_SEND;

typedef struct _MESSAGE_REDIRECT_PATH_SEND
{
	FILTER_MESSAGE_HEADER Header;

	MESSAGE_REDIRECT_PATH Message;

} MESSAGE_REDIRECT_PATH_SEND, *PMESSAGE_REDIRECT_PATH_SEND;

typedef struct _MESSAGE_PROTECT_PROCESS_SEND
{
	FILTER_MESSAGE_HEADER Header;

	MESSAGE_PROTECT_PROCESS Message;

} MESSAGE_PROTECT_PROCESS_SEND, *PMESSAGE_PROTECT_PROCESS_SEND;


#ifdef __cplusplus
};
#endif

#endif