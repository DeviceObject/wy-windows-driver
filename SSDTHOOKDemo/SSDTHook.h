#ifndef _SSDT_HOOK_H_
#define _SSDT_HOOK_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <ntddk.h>

//���� SSDT(ϵͳ����������) �з�������������Ŀ
//���ﶨ��Ϊ 1024 ����ʵ������ XP SP3 �� 0x0128 ��
#define MAX_SYSTEM_SERVICE_NUMBER 1024

//�������� SSDT �����е�ԭʼ�ķ������ĵ�ַ
extern ULONG oldSysServiceAddr[MAX_SYSTEM_SERVICE_NUMBER];

#define	SSDT_DEVICE_TYPE				FILE_DEVICE_UNKNOWN

//��������Ӧ�ó������������ͨ�ŵĺ꣬����ʹ�õ��ǻ�������д��ʽ
#define IO_START_WORKING				(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_STOP_WORKING					(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x702, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_INSERT_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_REMOVE_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_INSERT_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_REMOVE_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)


//=====================================================================================//
// KSYSTEM_SERVICE_TABLE �� KSERVICE_TABLE_DESCRIPTOR
// ˵��: ���� SSDT �ṹ
// ���ļ�������SSDT HOOK�Ļ������,����SSDT���̾����Դ��ļ�Ϊģ�崴��
//=====================================================================================//
typedef struct _KSYSTEM_SERVICE_TABLE
{
	PULONG  ServiceTableBase;			// SSDT (System Service Dispatch Table)�Ļ���ַ
	PULONG  ServiceCounterTableBase;	// ���� SSDT ��ÿ�����񱻵��õĴ���
	ULONG   NumberOfService;			// �������ĸ���, NumberOfService * 4 ����������ַ��Ĵ�С
	ULONG   ParamTableBase;				// SSPT(System Service Parameter Table)�Ļ���ַ

} KSYSTEM_SERVICE_TABLE, *PKSYSTEM_SERVICE_TABLE;

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
	KSYSTEM_SERVICE_TABLE   ntoskrnl;	// ntoskrnl.exe �ķ�����
	KSYSTEM_SERVICE_TABLE   win32k;		// win32k.sys �ķ�����(GDI32.dll/User32.dll ���ں�֧��)
	KSYSTEM_SERVICE_TABLE   notUsed1;
	KSYSTEM_SERVICE_TABLE   notUsed2;

} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

//������ ntoskrnl.exe �������� SSDT
extern PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable;

//���� Zw_ServiceFunction ��ȡ Zw_ServiceFunction �� SSDT ������Ӧ�ķ����������
#define SYSCALL_INDEX(ServiceFunction) (*(PULONG)((PUCHAR)ServiceFunction + 1))

//���� Zw_ServiceFunction ����÷����� SSDT �е������ţ�
//Ȼ����ͨ��������������ȡ Nt_ServiceFunction�ĵ�ַ
#define SYSCALL_FUNCTION(ServiceFunction) KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[SYSCALL_INDEX(ServiceFunction)]

//���� SSDT ������ϵͳ����ĵ�ַ
VOID BackupSysServicesTable();

//��װ Hook
NTSTATUS InstallSysServiceHook(ULONG oldService, ULONG newService);

//��� Hook
NTSTATUS UnInstallSysServiceHook(ULONG oldService);


// ����Ҫע����ǣ�SSDT �б���ĵ�ַ����˵����д�Ϳ���д�ģ�
// SSDT ���ڴ����Ǿ���ֻ�����Ա����ģ���������޸� SSDT �е��ڣ��������Ҫ���ֻ�����ԣ�
// Ҳ����Ҫ���� SSDT ���ڵ�����ڴ���п�д���Բ��У���Ȼ������Ľ���һ�����������(�ڴ�д�����)

//��ֹд�뱣����Ҳ���ǻָ���ֻ��
VOID DisableWriteProtect(ULONG oldAttr);

//����д�뱣����Ҳ��������Ϊ��д
VOID EnableWriteProtect(PULONG pOldAttr);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // _SSDT_HOOK_H_