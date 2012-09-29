
#ifndef _CONTEXT_H_
#define _CONTEXT_H_

typedef struct _CTX_STREAM_CONTEXT
{
	// �ļ���
	UNICODE_STRING iFileName;

	// �ļ������ü���
	ULONG iReferenceCount;

	// �ļ�״̬
	ULONG iState;
} CTX_STREAM_CONTEXT, *PCTX_STREAM_CONTEXT;

#define CTX_STREAM_CONTEXT_TAG 'cSxC'
#define CTX_STREAM_CONTEXT_SIZE	sizeof(CTX_STREAM_CONTEXT)



#endif