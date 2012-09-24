
#include "wycomflt.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <ntddk.h>
#include <ntstrsafe.h>
#ifdef __cplusplus
}
#endif

#define NTSTRSAFE_LIB


// 宏定义，主要处理一些标志位
#ifndef SetFlag
#define SetFlag(_F,_SF)       ((_F) |= (_SF))
#endif
#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif
#define CFT_MAX_COM_ID 32

// 过滤设备和真实设备
static PDEVICE_OBJECT global_FileDevice[CFT_MAX_COM_ID] = {0};
static PDEVICE_OBJECT global_RealDevice[CFT_MAX_COM_ID] = {0};

// 打开一个端口设备
PDEVICE_OBJECT CFTOpenCom(ULONG aId, NTSTATUS *aStatus)
{
	UNICODE_STRING nameString;
	static WCHAR name[32] = {0};
	PFILE_OBJECT fileObject = NULL;
	PDEVICE_OBJECT deviceObject = NULL;

	// 输入字符串
	RtlZeroMemory(name, sizeof(WCHAR)*32);
	RtlStringCchPrintfW(name, 32, L"\\Device\\Serial%d", aId);
	RtlInitUnicodeString(&nameString, name);

	// 打开设备对象
	*aStatus = IoGetDeviceObjectPointer(&nameString, FILE_ALL_ACCESS, &fileObject, &deviceObject);
	if (*aStatus == STATUS_SUCCESS)
		ObDereferenceObject(fileObject);

	return deviceObject;
}

NTSTATUS CFTAttachDevice(PDRIVER_OBJECT aDriverObject, PDEVICE_OBJECT aOldDeviceObject, PDEVICE_OBJECT *aFilterDeviceObject, PDEVICE_OBJECT *aNextDeviceObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT topDeviceObject = NULL;

	// 生成过滤层设备对象并绑定
	status = IoCreateDevice(aDriverObject, 0, NULL, aOldDeviceObject->DeviceType, 0, FALSE, aFilterDeviceObject);

	if (!NT_SUCCESS(status))
		return status;

	// 拷贝重要标志位
	if (aOldDeviceObject->Flags & DO_BUFFERED_IO)
		(*aFilterDeviceObject)->Flags |= DO_BUFFERED_IO;

	if (aOldDeviceObject->Flags & DO_DIRECT_IO)
		(*aFilterDeviceObject)->Flags |= DO_DIRECT_IO;

	if (aOldDeviceObject->Characteristics & FILE_DEVICE_SECURE_OPEN)
		(*aFilterDeviceObject)->Characteristics |= FILE_DEVICE_SECURE_OPEN;

	(*aFilterDeviceObject)->Flags |= DO_POWER_PAGABLE;

	// 绑定到另一个设备
	topDeviceObject = IoAttachDeviceToDeviceStack(*aFilterDeviceObject, aOldDeviceObject);
	if (topDeviceObject == NULL)
	{
		// 如果绑定失败，销毁之
		IoDeleteDevice(*aFilterDeviceObject);
		*aFilterDeviceObject = NULL;
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	*aNextDeviceObject = topDeviceObject;

	// 设置此设备已启动
	(*aFilterDeviceObject)->Flags = (*aFilterDeviceObject)->Flags & ~DO_DEVICE_INITIALIZING;
	return STATUS_SUCCESS;
}

VOID CFPAttachAllComs(PDRIVER_OBJECT aDriverObject)
{
	ULONG i = 0;
	PDEVICE_OBJECT comDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;

	for (i = 0; i < CFT_MAX_COM_ID; ++i)
	{
		// 获得设备对象是引用
		comDeviceObject = CFTOpenCom(i, &status);
		if (comDeviceObject == NULL)
			continue;

		// 这里绑定，不管是否绑定成功
		CFTAttachDevice(aDriverObject, comDeviceObject, &global_FileDevice[i], &global_RealDevice[i]);

		// 取消Object引用
	}
}

VOID CFPUnloadDriver(PDRIVER_OBJECT aDriverObject)
{
	ULONG i = 0;
	LARGE_INTEGER interval;
	//interval.LowPart = 0;
	//interval.HighPart = 0;

	// 首先解除绑定
	for (i = 0; i < CFT_MAX_COM_ID; ++i)
	{
		if (global_RealDevice[i] != NULL)
			IoDetachDevice(global_RealDevice[i]);
	}

	// 睡眠5秒，等待所有IRP处理结束
	interval.QuadPart = (5 * 1000 * DELAY_ONE_MILLISECOND);
	KeDelayExecutionThread(KernelMode, FALSE, &interval);

	// 删除这些设备
	for (i = 0; i < CFT_MAX_COM_ID; ++i)
	{
		if (global_FileDevice[i] != NULL)
			IoDeleteDevice(global_FileDevice[i]);	// 这里造成蓝屏
	}

	return ;
}

NTSTATUS CFTDefaultDispatch(PDEVICE_OBJECT aDeviceObject, PIRP aIrp)
{
	PIO_STACK_LOCATION ioStackLocation = IoGetCurrentIrpStackLocation(aIrp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG i = 0, j = 0;

	// 首先需要知道发送给了那哪个设备。
	for (i = 0; i < CFT_MAX_COM_ID; ++i)
	{
		if (global_FileDevice[i] == aDeviceObject)
		{
			// 所有电源操作一律不过滤
			if (ioStackLocation->MajorFunction == IRP_MJ_POWER)
			{
				// 直接发送
				PoStartNextPowerIrp(aIrp);
				IoSkipCurrentIrpStackLocation(aIrp);
				return PoCallDriver(global_RealDevice[i], aIrp);
			}

			// 我们只过滤写请求，获得缓冲区以及长度，打印出来。
			if (ioStackLocation->MajorFunction == IRP_MJ_WRITE)
			{
				// 获得长度
				ULONG len = ioStackLocation->Parameters.Write.Length;

				// 获得缓冲区
				PUCHAR buf = NULL;
				if (aIrp->MdlAddress != NULL)
					buf = (PUCHAR)MmGetSystemAddressForMdlSafe(aIrp->MdlAddress, NormalPagePriority);
				else
					buf = (PUCHAR)aIrp->UserBuffer;

				if (buf == NULL)
					buf = (PUCHAR)aIrp->AssociatedIrp.SystemBuffer;

				// 打印内容
				for (j = 0; j < len; ++j)
					DbgPrint("COM capture : Send-Data : %2x\r\n", buf[j]);
			}
		}

		// 直接下发
		IoSkipCurrentIrpStackLocation(aIrp);
		return IoCallDriver(global_RealDevice[i], aIrp);
	}

	// 如果根本就不在被绑定的设备中，那是有问题的，直接返回参数
	aIrp->IoStatus.Information = 0;
	aIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
	IoCompleteRequest(aIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT aDriverObject, PUNICODE_STRING aRegisterPath)
{
	size_t i = 0;

	// 所有分发函数都设置为默认的
	// 在CFTDefaultDispatch()中，由它来派发不同的IRP请求。实际上其内容可分给相应的函数处理。
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		aDriverObject->MajorFunction[i] = CFTDefaultDispatch;

	// 定义卸载函数，支持动态卸载
	aDriverObject->DriverUnload = CFPUnloadDriver;

	// 绑定所有串口
	CFPAttachAllComs(aDriverObject);

	// 直接返回成功
	return STATUS_SUCCESS;
}