#ifndef _SSDT_HOOK_H_
#define _SSDT_HOOK_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <ntddk.h>

//定义 SSDT(系统服务描述表) 中服务个数的最大数目
//这里定义为 1024 个，实际上在 XP SP3 是 0x0128 个
#define MAX_SYSTEM_SERVICE_NUMBER 1024

//用来保存 SSDT 中所有的原始的服务函数的地址
extern ULONG oldSysServiceAddr[MAX_SYSTEM_SERVICE_NUMBER];

#define	SSDT_DEVICE_TYPE				FILE_DEVICE_UNKNOWN

//定义用于应用程序和驱动程序通信的宏，这里使用的是缓冲区读写方式
#define IO_START_WORKING				(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_STOP_WORKING					(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x702, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_INSERT_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_REMOVE_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_INSERT_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_REMOVE_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)


//=====================================================================================//
// KSYSTEM_SERVICE_TABLE 和 KSERVICE_TABLE_DESCRIPTOR
// 说明: 定义 SSDT 结构
// 此文件定义了SSDT HOOK的基本框架,其他SSDT工程均可以此文件为模板创建
//=====================================================================================//
typedef struct _KSYSTEM_SERVICE_TABLE
{
	PULONG  ServiceTableBase;			// SSDT (System Service Dispatch Table)的基地址
	PULONG  ServiceCounterTableBase;	// 包含 SSDT 中每个服务被调用的次数
	ULONG   NumberOfService;			// 服务函数的个数, NumberOfService * 4 就是整个地址表的大小
	ULONG   ParamTableBase;				// SSPT(System Service Parameter Table)的基地址

} KSYSTEM_SERVICE_TABLE, *PKSYSTEM_SERVICE_TABLE;

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
	KSYSTEM_SERVICE_TABLE   ntoskrnl;	// ntoskrnl.exe 的服务函数
	KSYSTEM_SERVICE_TABLE   win32k;		// win32k.sys 的服务函数(GDI32.dll/User32.dll 的内核支持)
	KSYSTEM_SERVICE_TABLE   notUsed1;
	KSYSTEM_SERVICE_TABLE   notUsed2;

} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

//导出由 ntoskrnl.exe 所导出的 SSDT
extern PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable;

//根据 Zw_ServiceFunction 获取 Zw_ServiceFunction 在 SSDT 中所对应的服务的索引号
#define SYSCALL_INDEX(ServiceFunction) (*(PULONG)((PUCHAR)ServiceFunction + 1))

//根据 Zw_ServiceFunction 来获得服务在 SSDT 中的索引号，
//然后再通过该索引号来获取 Nt_ServiceFunction的地址
#define SYSCALL_FUNCTION(ServiceFunction) KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[SYSCALL_INDEX(ServiceFunction)]

//备份 SSDT 中所有系统服务的地址
VOID BackupSysServicesTable();

//安装 Hook
NTSTATUS InstallSysServiceHook(ULONG oldService, ULONG newService);

//解除 Hook
NTSTATUS UnInstallSysServiceHook(ULONG oldService);


// 还需要注意的是，SSDT 中保存的地址不是说你想写就可以写的，
// SSDT 在内存中是具有只读属性保护的，如果你想修改 SSDT 中的内，你必须先要解除只读属性，
// 也就是要赋予 SSDT 所在的这块内存具有可写属性才行，不然回馈你的将是一个无情的蓝屏(内存写入错误)

//禁止写入保护，也就是恢复到只读
VOID DisableWriteProtect(ULONG oldAttr);

//允许写入保护，也就是设置为可写
VOID EnableWriteProtect(PULONG pOldAttr);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // _SSDT_HOOK_H_