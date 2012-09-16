/**
�ļ�˵����
	���������ڹ��˴��ڲ���
���ߣ�
	����
ʱ�䣺
	2012-09-12
**/

#pragma once

// ��C���򵼳�/����
#ifdef __cplusplus
extern "C"
{
#endif
#include <wdm.h>
#ifdef __cplusplus
}
#endif

// ������һЩ���ֵĶ���
#define INITDATA data_seg("INIT")
#define LOCKEDDATA data_seg()
#define PAGEDDATA data_seg("PAGE")

#define INITCODE code_seg("INIT")
#define LOCKEDCODE code_seg()
#define PAGEDCODE code_seg("PAGE")

// ���ݽṹ����
enum 
{
	CFT_IRP_PASS = 0,
	CFT_IRP_COMPLETED = 1,
	CFT_IRP_GO_ON = 2
};

#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)

// ��������
// ��ں���
#ifdef __cplusplus
extern "C"
#endif
NTSTATUS DriverEntry(PDRIVER_OBJECT aDriverObject, PUNICODE_STRING aRegisterPath);

// ж������
VOID CFPUnloadDriver(PDRIVER_OBJECT aDriverObject);

// �����д���
VOID CFPAttachAllComs(PDRIVER_OBJECT aDriverObject);

// ʹ�ܺ���
VOID CFPSetEnable(BOOLEAN aEnable);

// ����IRP
BOOLEAN CFPFileIrpFilter(PDEVICE_OBJECT aNextDevice, PIRP aIrp, PIO_STACK_LOCATION aIrpStackLocation, NTSTATUS *aStatus);



