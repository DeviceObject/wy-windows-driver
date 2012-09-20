
/**
 * ����ͷ�ļ�
 *
 */

#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// ����ȫ�ֱ���
typedef struct _WY_GLOAB_DATA
{
	// ��������
	PDRIVER_OBJECT iDriverObject;

	// ����������
	PFLT_FILTER iFilter;

} GLOBAL_DATA, *PGLOBAL_DATA;

extern GLOBAL_DATA globalData;

//////////////////////////////////////////////////////////////////////////////////////////////
// ���������Ľṹ

// 
// ��ʵ��������
typedef struct _WY_INSTANCE_CONTEXT
{
	// ��ǰ�����ĵ�ʵ��
	PFLT_INSTANCE iInstance;

	// ��ǰʵ��������ľ�
	PFLT_VOLUME iVolume;

	// ��ǰʵ��������������
	UNICODE_STRING iVolumeName;

} WY_INSTANCE_CONTEXT, *PWY_INSTANCE_CONTEXT;

#define WY_INSTANCE_CONTEXT_SIZE sizeof(WY_INSTANCE_CONTEXT)

// 
// �ļ�������
typedef struct _WY_FILE_CONTEXT
{
	// �ļ������Ƿ����ȫ·�������ڼ򵥵�ѧϰ���������ȫ·���ˣ�
	UNICODE_STRING iFileName;

	// �ļ�״̬��λ��־��
	ULONG iFileState;

} WY_FILE_CONTEXT, *PWY_FILE_CONTEXT;

#define WY_FILE_CONTEXT_SIZE sizeof(WY_FILE_CONTEXT)

// 
// ��������
typedef struct _WY_STREAM_CONTEXT
{
	// �������Ĺ������ļ�����
	UNICODE_STRING iFileName;

	// �ڴ����ϴ򿪵Ĵ���
	ULONG iCreateCount;

	// �ڴ���������Ĵ���
	ULONG iCleanupCount;

	// �ڴ����ϹرյĴ���
	ULONG iCloseCount;

	// ���������������ĵ���Դ
	PERESOURCE iLockResource;

} WY_STREAM_CONTEXT, *PWY_STREAM_CONTEXT;

#define WY_STREAM_CONTEXT_SIZE sizeof(WY_STREAM_CONTEXT)

// 
// ����������ģ�����Ǹ����õģ�����ȷʵ��֪����
typedef struct _WY_STREAM_HANDLE_CONTEXT
{
	// ����������������ļ���
	UNICODE_STRING iFileName;

	// ���������������ĵ���Դ
	PERESOURCE iLockResource;

} WY_STREAM_HANDLE_CONTEXT, *PWY_STREAM_HANDLE_CONTEXT;

#define WY_STREAM_HANDLE_CONTEXT_SIZE sizeof(WY_STREAM_HANDLE_CONTEXT)

// 
// �������ڴ�����ֱ��
#define WY_INSTANCE_CONTEXT_TAG			'cIxC'
#define WY_FILE_CONTEXT_TAG				'cFxC'
#define WY_STREAM_CONTEXT_TAG			'cSxC'
#define WY_STREAM_HANDLE_CONTEXT_TAG	'cHxC'
#define WY_STRING_TAG					'sSxC'

#endif