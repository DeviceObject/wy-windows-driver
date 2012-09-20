
// ��������غ�������
#ifdef __cplusplus
extern "C"
{
#endif

VOID FSFContextCleanup(__in PFLT_CONTEXT Context, __in FLT_CONTEXT_TYPE ContextType);

NTSTATUS FSFFindOrCreateFileContext(__in PFLT_CALLBACK_DATA Cbd,
									__in BOOLEAN CreateIfNotFound,
									__in_opt PUNICODE_STRING FileName,
									__deref_out PWY_FILE_CONTEXT *FileContext,
									__out_opt PBOOLEAN ContextCreated);

NTSTATUS FSFCreateFileContext(__in PUNICODE_STRING FileName, __deref_out PWY_FILE_CONTEXT *FileContext);

NTSTATUS FSFCreateOrFindStreamContext(__in PFLT_CALLBACK_DATA Cbd,
									  __in BOOLEAN CreateIfNotFound,
									  __deref_out PWY_STREAM_CONTEXT *StreamContext,
									  __out_opt PBOOLEAN ContextCreated);

NTSTATUS FSFCreateStreamContext(__deref_out PWY_STREAM_CONTEXT *StreamContext);

NTSTATUS FSFUpdateNameInStreamContext(__in PUNICODE_STRING DirectoryName, __inout PWY_STREAM_CONTEXT StreamContext);

NTSTATUS FSFCreateOrReplaceStreamHandleContext(__in PFLT_CALLBACK_DATA Cbd,
											   __in BOOLEAN ReplaceIfExists,
											   __deref_out PWY_STREAM_HANDLE_CONTEXT *StreamHandleContext,
											   __out_opt PBOOLEAN ContextReplaced);

NTSTATUS FSFCreateStreamHandleContext(__deref_out PWY_STREAM_HANDLE_CONTEXT *StreamHandleContext);

NTSTATUS FSFUpdateNameInStreamHandleContxt(__in PUNICODE_STRING DirectoryName, __inout PWY_STREAM_HANDLE_CONTEXT StreamHandleContext);

// �ַ�����غ�������
NTSTATUS CtxAllocateUnicodeString(__inout PUNICODE_STRING String);

NTSTATUS CtxFreeUnicodeString(__inout PUNICODE_STRING String);

#ifdef __cplusplus
}
#endif

// ��Դ��غ�������
#define CTX_RESOURCE_TAG 'cRxC'
FORCEINLINE PERESOURCE CtxAllocateResource(VOID)
{
	// �������뵽�ķǷ�ҳ�ڴ�
	return (PERESOURCE)ExAllocatePoolWithTag(NonPagedPool, sizeof(ERESOURCE), CTX_RESOURCE_TAG);
}

FORCEINLINE VOID CtxFreeResource(__in PERESOURCE Resource)
{
	// �ͷ�ָ����ǵ��ڴ�����
	ExFreePoolWithTag(Resource, CTX_RESOURCE_TAG);
}

FORCEINLINE VOID __drv_acquiresCriticalRegion
CtxAcquireResourceExclusive(__inout PERESOURCE Resource)
{
	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
	ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) || !ExIsResourceAcquiredSharedLite(Resource));

	KeEnterCriticalRegion();
	(VOID)ExAcquireResourceExclusiveLite(Resource, TRUE);
}

FORCEINLINE VOID __drv_acquiresCriticalRegion
CtxAcquireResourceShared(__inout PERESOURCE Resource)
{
	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

	// �����ٽ���
	KeEnterCriticalRegion();
	(VOID)ExAcquireResourceSharedLite( Resource, TRUE );
}

FORCEINLINE VOID __drv_releasesCriticalRegion __drv_mustHoldCriticalRegion
CtxReleaseResource(__inout PERESOURCE Resource)
{
	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
	ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) || ExIsResourceAcquiredSharedLite(Resource));

	ExReleaseResourceLite(Resource);
	KeLeaveCriticalRegion();
}