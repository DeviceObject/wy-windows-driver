#ifndef _DATA_STRUCT_H_
#define _DATA_STRUCT_H_

/************************************************************************/
/* 此文件定义了重定向驱动所需的链表结构
/************************************************************************/

#include <fltKernel.h>

// 
// 保护进程链表项
typedef struct _PROTECT_PROCESS
{
	LIST_ENTRY ListEntry;

	HANDLE ProcessId;

} PROTECT_PROCESS, *PPROTECT_PROCESS;

// 
// 保护进程结构
typedef struct _PROTECT_PROCESS_LIST
{
	// 保护进程链表头
	LIST_ENTRY HeadList;

	// 链表自旋锁
	KSPIN_LOCK Lock;

	// 链表Lookaside结构
	PAGED_LOOKASIDE_LIST PageList;

} PROTECT_PROCESS_LIST, *PPROTECT_PROCESS_LIST;

extern PROTECT_PROCESS_LIST ProtectProcessList;

NTSTATUS InitializeProctectProcessList();

VOID DestroyProctectProcessList();

NTSTATUS InsertProtectProcess(__in HANDLE ProcessId);

NTSTATUS CheckProtectProcess(__in HANDLE ProcessId);

NTSTATUS RemoveProtectProcess(__in HANDLE ProcessId);

NTSTATUS CleanupProtectProcess();


//
// 重定向匹配表达式
typedef struct _MATCH_EXPRESSION
{
	LIST_ENTRY ListEntry;

	WCHAR Expression[260];

	ULONG Length;

} MATCH_EXPRESSION, *PMATCH_EXPRESSION;

// 
// 重定向匹配列表
#define PROTECT_PROCESS_LIST		_PROTECT_PROCESS_LIST

typedef PROTECT_PROCESS_LIST		MATCH_EXPRESSION_LIST;
typedef PPROTECT_PROCESS_LIST		PMATCH_EXPRESSION_LIST;

extern MATCH_EXPRESSION_LIST MatchExpressionList;

NTSTATUS InitializeMatchExpressionList();

VOID DestroyMatchExpressionList();

NTSTATUS InsertMatchExpression(__in WCHAR Expression[], __in ULONG ExpressionLength);

NTSTATUS CheckMatchExpression(__in WCHAR Expression[], __in ULONG ExpressionLength);

NTSTATUS RemoveMatchExpression(__in WCHAR Expression[], __in ULONG ExpressionLength);

NTSTATUS DoMatchExpression(PUNICODE_STRING Path);

NTSTATUS CleanupMatchExpression();

// 
// 重定向根目录列表
typedef struct _REDIRECT_DIR
{
	WCHAR RedirectBaseDirectory[260];

	ULONG Length;

	KSPIN_LOCK Lock;

} REDIRECT_DIR, *PREDIRECT_DIR;

extern REDIRECT_DIR RedirectDirectory;

NTSTATUS InitializeRedirectDirectory();

NTSTATUS SetRedirectBaseDirectory(__in WCHAR Directory[], __in ULONG Length);

NTSTATUS CleanupRedirectBaseDirectory();

NTSTATUS GetRedirectBaseDirectory(__inout WCHAR Directory[260], __inout PULONG Length);

NTSTATUS ChechRedirectBaseDirectory();

#endif