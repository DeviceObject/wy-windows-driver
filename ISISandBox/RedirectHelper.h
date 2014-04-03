#ifndef _REDIRECT_HELPER_H_
#define _REDIRECT_HELPER_H_

/************************************************************************/
/* ���������ַ������ļ�·����صĲ�������
/************************************************************************/

#include <fltkernel.h>

/**
 * �������ļ�·�����͵�
 */
NTSTATUS IoCreateDirectoryInUserMode(__in PUNICODE_STRING RedirectPath);

NTSTATUS IoCreateDirectory(__in PUNICODE_STRING Path, __in PFLT_FILTER Filter, __in PFLT_INSTANCE Instance);

BOOLEAN FsIsRedirectPath(__in PWCH RedirectBase, __in PFLT_FILE_NAME_INFORMATION FileName, __in PFLT_VOLUME Volume);

#endif