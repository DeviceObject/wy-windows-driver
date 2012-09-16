/**
文件说明：
	此驱动用于过滤串口操作
作者：
	王煜
时间：
	2012-09-12
**/

#pragma once

// 以C程序导出/导入
#ifdef __cplusplus
extern "C"
{
#endif
#include <wdm.h>
#ifdef __cplusplus
}
#endif

// 这里是一些文字的定义
#define INITDATA data_seg("INIT")
#define LOCKEDDATA data_seg()
#define PAGEDDATA data_seg("PAGE")

#define INITCODE code_seg("INIT")
#define LOCKEDCODE code_seg()
#define PAGEDCODE code_seg("PAGE")

// 数据结构定义
enum 
{
	CFT_IRP_PASS = 0,
	CFT_IRP_COMPLETED = 1,
	CFT_IRP_GO_ON = 2
};

#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)

// 函数定义
// 入口函数
#ifdef __cplusplus
extern "C"
#endif
NTSTATUS DriverEntry(PDRIVER_OBJECT aDriverObject, PUNICODE_STRING aRegisterPath);

// 卸载驱动
VOID CFPUnloadDriver(PDRIVER_OBJECT aDriverObject);

// 绑定所有串口
VOID CFPAttachAllComs(PDRIVER_OBJECT aDriverObject);

// 使能函数
VOID CFPSetEnable(BOOLEAN aEnable);

// 过滤IRP
BOOLEAN CFPFileIrpFilter(PDEVICE_OBJECT aNextDevice, PIRP aIrp, PIO_STACK_LOCATION aIrpStackLocation, NTSTATUS *aStatus);



