#ifndef _SSDT_HOOK_MIDWARE_H_
#define _SSDT_HOOK_MIDWARE_H_

#include <tchar.h>
#include <WINDOWS.H>
#include <winioctl.h>

/**
 * �ļ�˵����
 *	���ļ�������SSDT HOOK�м������
 *	�˶����ṩ�˻������û�̬������ͨ���¼�
 * ���ߣ�
 *	wangy
 * ���ڣ�
 *	2013-01-17
 */

#define SYMBO_LINK_NAME_T	_T("\\\\.\\SSDTHOOKDemo")

// SSDT��������
#define	SSDT_DEVICE_TYPE				FILE_DEVICE_UNKNOWN

//��������Ӧ�ó������������ͨ�ŵĺ꣬����ʹ�õ��ǻ�������д��ʽ
#define IO_START_WORKING				(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_STOP_WORKING					(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x702, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_INSERT_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_REMOVE_HIDE_PROCESS			(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_INSERT_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IO_REMOVE_PROTECT_PROCESS		(ULONG) CTL_CODE(SSDT_DEVICE_TYPE, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)


// ���ﶨ��ϵͳ�������ͣ��ɽ���λ����������
#define TerminateProcessProtect		0x00000001		// ���̱���
#define QueryProcessInfoProtect		0x00000010		// ��������

// ����״̬
typedef struct _PROCRSS_STATE
{
	// ����ID
	DWORD processId;

	// ����״̬��ǣ�λ���
	unsigned int processState;

} PROCESS_STATE_EX, *PPROCESS_STATE_EX;

class SSDTHOOKMidware
{
public:
	SSDTHOOKMidware();
	~SSDTHOOKMidware();

public:
	/**
	 * ����SSDT HOOK����
	 * �������δ��װ����װ����������ֱ��������
	 * ������
	 *	@aServiceName	������������
	 * ����ֵ��
	 *	0 - �����ɹ��������Ѿ��ɹ�����
	 */
	int RunSSDTHOOKDriver(TCHAR *aServiceName);

	/**
	 * ֹͣSSDT HOOK����
	 * ������
	 *	@aServiceName	������������
	 * ����ֵ��
	 *	0 - �����ɹ��������Ѿ��ɹ�ֹͣ
	 */
	int StopSSDTHOOKDriver(TCHAR *aServiceName);

	/**
	 * ���ӵ������豸����
	 * ������
	 *	@aDeviceName	�豸�����������
	 * ����ֵ��
	 *	0 - ���ӳɹ�
	 */
	int ConnectToDriver(TCHAR *aDeviceName);

	/**
	 * �Ͽ������豸����ͨ��
	 * ����ֵ��
	 *	0 - �Ͽ��ɹ�
	 */
	int DisconnectToDriver();

	/**
	 * ��������״̬
	 */
	int StartSSDTHOOK();

	/**
	 * ֹͣ����״̬
	 */
	int StopSSDTHOOK();

	/**
	 * ���ӹҹ�����
	 * ������
	 *	@aHookType		�ҹ�����
	 *	@aProcessId		����ID
	 * ����ֵ��
	 *	�ɹ�������Ӧ��λ
	 */
	int AddSSDTHOOKProcess(int aHookType, DWORD aProcessId);

	/**
	 * �Ƴ��ҹ�����
	 * ������
	 *	@aHookType		�ҹ�����
	 *	@aProcessId		����ID
	 * ����ֵ��
	 *	�ɹ�������Ӧ��λ
	 */
	int RemoveSSDTHOOKProcess(int aHookType, DWORD aProcessId);

private:
	/**
	 * ����ͨ��������
	 */
	void *iDriverContext;
};

#endif