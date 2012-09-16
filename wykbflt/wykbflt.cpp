
#include "wykbflt.h"

#pragma INITCODE
#ifdef __cplusplus
extern "C"
#endif
NTSTATUS DriverEntry(IN PDRIVER_OBJECT aDriverObject, IN PUNICODE_STRING aRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("wykbflt.sys : enter DriverEntry\n"));

	// 初始化所有派遣函数
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		aDriverObject->MajorFunction[i] = KBFDefaultIRPDispatchRoutine;
	
	// 更改Read派遣函数，通过过滤此IRP获取键盘数据
	aDriverObject->MajorFunction[IRP_MJ_READ] = KBFReadDispatchRoutine;

	aDriverObject->MajorFunction[IRP_MJ_POWER] = KBFPowerDispatchRoutine;
	aDriverObject->MajorFunction[IRP_MJ_PNP] = KBFPnpDispatchRoutine;

	// 卸载函数
	aDriverObject->DriverUnload = KBFDriverUnload;
	gDriverObject = aDriverObject;

	// 注意：这里是关键
	// 绑定所有键盘设备
	status = KBFAttachDevices(aDriverObject, aRegistryPath);

	return status;
}

POBJECT_TYPE IoDriverObjectType;
NTSTATUS KBFAttachDevices(IN PDRIVER_OBJECT aDriverObject, IN PUNICODE_STRING aRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint(("wykbflt.sys : KBFAttachDevices\n"));

	UNICODE_STRING deviceClassName;
	RtlInitUnicodeString(&deviceClassName, KBD_DRIVER_NAME);

	PDRIVER_OBJECT KbdDriverObject = NULL; 
	status = ObReferenceObjectByName(&deviceClassName, OBJ_CASE_INSENSITIVE, 
		NULL, 0, IoDriverObjectType, KernelMode, NULL, (PVOID *)&KbdDriverObject);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("wykbflt.sys : KBFAttachDevices Couldn't get the Device Object\n"));
		return status;
	}
	else
	{
		// 
		ObDereferenceObject(aDriverObject);
	}

	PDEVICE_OBJECT pFilterDeviceObject = NULL; 
	PDEVICE_OBJECT pTargetDeviceObject = NULL; 
	PDEVICE_OBJECT pLowerDeviceObject = NULL;

	pTargetDeviceObject = KbdDriverObject->DeviceObject;

	while (pTargetDeviceObject)
	{
		// 生成一个过滤设备
		status = IoCreateDevice(aDriverObject, sizeof(DEVICE_EXTENSION), NULL,
			pTargetDeviceObject->DeviceType, pTargetDeviceObject->Characteristics,
			FALSE, OUT &pFilterDeviceObject);

		if (!NT_SUCCESS(status))
		{
			KdPrint(("wykbflt.sys : KBFAttachDevices Couldn't create the Filter Device Object\n"));
			return status;
		}

		// 绑定。pLowerDeviceObject是底层物理设备
		pLowerDeviceObject = IoAttachDeviceToDeviceStack(pFilterDeviceObject, pTargetDeviceObject);

		if (!pLowerDeviceObject)
		{
			KdPrint(("wykbflt.sys : Couldn't attach to Device Object\n"));
			IoDeleteDevice(pFilterDeviceObject);
			pFilterDeviceObject = NULL;
			return status;
		}

		// 设备扩展
		PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)pFilterDeviceObject->DeviceExtension;
		KBFInitDeviceExtension(deviceExtension, pFilterDeviceObject, pTargetDeviceObject, pLowerDeviceObject);
		
		// 设置过滤操作
		pFilterDeviceObject->DeviceType = pLowerDeviceObject->DeviceType;	// 要过滤的设备类型跟物理设备类型一致
		pFilterDeviceObject->Characteristics = pLowerDeviceObject->Characteristics;
		pFilterDeviceObject->StackSize = pLowerDeviceObject->StackSize + 1;
		pFilterDeviceObject->Flags |= pLowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE) ;

		pTargetDeviceObject = pTargetDeviceObject->NextDevice;
	}

	return status;
}

VOID KBFDetach(IN PDEVICE_OBJECT aDeviceObject)
{
	PDEVICE_EXTENSION deviceExtension;
	BOOLEAN NoRequestsOutstanding = FALSE;
	deviceExtension = (PDEVICE_EXTENSION)aDeviceObject->DeviceExtension;

	__try
	{
		__try
		{
			IoDetachDevice(deviceExtension->iTargetDeviceObject);
			deviceExtension->iTargetDeviceObject = NULL;
			IoDetachDevice(aDeviceObject);
			deviceExtension->iFilterDeviceObject = NULL;
			DbgPrint(("wykbflt.sys : Detach finished .\n"));
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			// Do nothing ...
		}
	}
	__finally
	{
		// Do nothing ...
	}

	return ;
}

VOID KBFDriverUnload(IN PDRIVER_OBJECT aDriverObject)
{
	PDEVICE_OBJECT deviceObject = NULL;
	PDEVICE_OBJECT oldDeviceObject = NULL;
	PDEVICE_EXTENSION deviceExtension = NULL;

	LARGE_INTEGER lDelay;
	PRKTHREAD currentThread = NULL;

	// delay some time
	lDelay = RtlConvertLongToLargeInteger(100 * DELAY_ONE_MILLISECOND);
	currentThread = KeGetCurrentThread();

	// 将当前线程设置为低实时模式，以便让它的运行尽量少影响其他程序
	KeSetPriorityThread(currentThread, LOW_REALTIME_PRIORITY);

	UNREFERENCED_PARAMETER(aDriverObject);
	DbgPrint("wykbflt.sys : DriverEntry unloading .\n");

	// 遍历所有设备并一律解除绑定
	deviceObject = aDriverObject->DeviceObject;
	while (deviceObject)
	{
		KBFDetach(deviceObject);
		deviceObject = deviceObject->NextDevice;
	}

	ASSERT(NULL == aDriverObject->DeviceObject);

	while (gC2pKeyCount)
	{
		KeDelayExecutionThread(KernelMode, FALSE, &lDelay);
	}

	DbgPrint("wykbflt.sys : DriverEntry unload ok .\n");
	return ;
}

NTSTATUS KBFDefaultIRPDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp)
{
	// 直接调用IoCallDriver将IRP发送到后续的设备对象
	IoSkipCurrentIrpStackLocation(aIrp);

	return IoCallDriver(((PDEVICE_EXTENSION)aDeviceObject->DeviceExtension)->iLowerDeviceObject, aIrp);
}

NTSTATUS KBFPowerDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp)
{
	PDEVICE_EXTENSION deviceExtension;
	deviceExtension = (PDEVICE_EXTENSION)aDeviceObject->DeviceExtension;

	PoStartNextPowerIrp(aIrp);
	IoSkipCurrentIrpStackLocation(aIrp);

	return PoCallDriver(deviceExtension->iLowerDeviceObject, aIrp);
}

NTSTATUS KBFPnpDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp)
{
	PDEVICE_EXTENSION deviceExtension = NULL;
	PIO_STACK_LOCATION irpStack = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL oldIrql = NULL;
	//KEVENT event = NULL;

	// 获得真实设备
	deviceExtension = (PDEVICE_EXTENSION)(aDeviceObject->DeviceExtension);
	irpStack = IoGetCurrentIrpStackLocation(aIrp);

	switch (irpStack->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE:
		DbgPrint("IRP_MN_REMOVE_DEVICE\n");

		// 1. 将请求下发
		IoSkipCurrentIrpStackLocation(aIrp);
		IoCallDriver(deviceExtension->iLowerDeviceObject, aIrp);

		// 2. 解除绑定
		IoDetachDevice(deviceExtension->iLowerDeviceObject);

		// 删除我们自己生成的虚拟设备
		IoDeleteDevice(aDeviceObject);
		status = STATUS_SUCCESS;

		break;
	default:
		// 其他类型的IRP，直接传递到下一个设备
		IoSkipCurrentIrpStackLocation(aIrp);
		status = IoCallDriver(deviceExtension->iLowerDeviceObject, aIrp);
	}

	return status;
}

NTSTATUS KBFReadCompleteRoutine(IN PDEVICE_OBJECT aDeviceObjec, IN PIRP aIrp, IN PVOID aContext)
{
	PIO_STACK_LOCATION irpStackLocation;
	ULONG_PTR buflen = 0;
	PUCHAR buf = NULL;

	irpStackLocation = IoGetCurrentIrpStackLocation(aIrp);

	if (NT_SUCCESS(aIrp->IoStatus.Status))
	{
		// 获得读取请求完成后输出的缓冲区
		buf = (PUCHAR)aIrp->AssociatedIrp.SystemBuffer;

		// 获得缓冲区长度
		buflen = aIrp->IoStatus.Information;

		for (size_t i = 0; i < buflen; ++i)
		{
			DbgPrint("wykbflt.sys : %2x\r\n", buf[i]);
		}
	}

	--gC2pKeyCount;

	if (aIrp->PendingReturned)
	{
		IoMarkIrpPending(aIrp);
	}

	return aIrp->IoStatus.Status;
}

NTSTATUS KBFReadDispatchRoutine(IN PDEVICE_OBJECT aDeviceObject, IN PIRP aIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION deviceExtension = NULL;
	PIO_STACK_LOCATION currentIrpStack = NULL;
	KEVENT waitEvent;

	// 初始化内核事件
	KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);

	if (aIrp->CurrentLocation == 1)
	{
		ULONG ReturnInformation = 0;
		DbgPrint("wykbflt.sys : Dispatch encountered bogus current location .\n");
		status = STATUS_INVALID_DEVICE_REQUEST;
		aIrp->IoStatus.Status = status;
		aIrp->IoStatus.Information = ReturnInformation;
		IoCompleteRequest(aIrp, IO_NO_INCREMENT);
		return status;
	}

	// 全局变量计数器增加
	++gC2pKeyCount;

	// 得到设备扩展，方便获取下一个设备的指针
	deviceExtension = (PDEVICE_EXTENSION)aDeviceObject->DeviceExtension;

	// 
	currentIrpStack = IoGetCurrentIrpStackLocation(aIrp);
	IoCopyCurrentIrpStackLocationToNext(aIrp);
	IoSetCompletionRoutine(aIrp, KBFReadCompleteRoutine, aDeviceObject, TRUE, TRUE, TRUE);

	return IoCallDriver(deviceExtension->iLowerDeviceObject, aIrp);
}

NTSTATUS KBFInitDeviceExtension(IN PDEVICE_EXTENSION aDeviceExtension, IN PDEVICE_OBJECT aFilterDeviceObject,
								IN PDEVICE_OBJECT aTargetDeviceObject, IN PDEVICE_OBJECT aLowerDeviceObject)
{ 
	memset(aDeviceExtension, 0, sizeof(DEVICE_EXTENSION)); 
	aDeviceExtension->iSize = sizeof(DEVICE_EXTENSION); 
	aDeviceExtension->iFilterDeviceObject = aFilterDeviceObject; 
	KeInitializeSpinLock(&(aDeviceExtension->iIoRequestsSpinLock)); 
	KeInitializeEvent(&(aDeviceExtension->iIoInProcessEvent), NotificationEvent, FALSE); 
	aDeviceExtension->iTargetDeviceObject = aTargetDeviceObject; 
	aDeviceExtension->iLowerDeviceObject = aLowerDeviceObject; 
	return( STATUS_SUCCESS ); 
}