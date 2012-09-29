
#ifndef _CONTEXT_H_
#define _CONTEXT_H_

typedef struct _CTX_STREAM_CONTEXT
{
	// 文件名
	UNICODE_STRING iFileName;

	// 文件的引用计数
	ULONG iReferenceCount;

	// 文件状态
	ULONG iState;
} CTX_STREAM_CONTEXT, *PCTX_STREAM_CONTEXT;

#define CTX_STREAM_CONTEXT_TAG 'cSxC'
#define CTX_STREAM_CONTEXT_SIZE	sizeof(CTX_STREAM_CONTEXT)



#endif