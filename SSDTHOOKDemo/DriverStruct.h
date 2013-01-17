#ifndef _DRVICE_STRUCT_H_
#define _DRVICE_STRUCT_H_

//=====================================================================================//
// 此文件是此驱动的实际逻辑部分
// 创建不同的SSDT HOOK工程时只需要改动此部分的函数即可
//=====================================================================================//

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "HookLogic.h"
#include <stdlib.h>
#include <WINDEF.H>

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING  pRegistryPath);

VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject);

/**
 * 派遣函数
 */
NTSTATUS CreateDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS CloseDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS ReadDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS WriteDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DeviceIoControlDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

NTSTATUS GeneralDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);


#ifdef __cplusplus
};
#endif // __cplusplus

#endif	// _DRVICE_STRUCT_H_