#ifndef _SSDT_HOOK_MIDWARE_H_
#define _SSDT_HOOK_MIDWARE_H_

#include <tchar.h>
#include <WINDOWS.H>

/**
 * �ļ�˵����
 *	���ļ�������SSDT HOOK�м������
 *	�˶����ṩ�˻������û�̬������ͨ���¼�
 * ���ߣ�
 *	wangy
 * ���ڣ�
 *	2013-01-17
 */

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
	 * ���ӹҹ�����
	 * ������
	 *	@aHookType		�ҹ�����
	 *	@aProcessId		����ID
	 * ����ֵ��
	 *	0 - ��ӳɹ�
	 */
	int AddSSDTHOOKProcess(int aHookType, DWORD aProcessId);

	/**
	 * �Ƴ��ҹ�����
	 * ������
	 *	@aHookType		�ҹ�����
	 *	@aProcessId		����ID
	 * ����ֵ��
	 *	0 - �Ƴ��ɹ�
	 */
	int RemoveSSDTHOOKProcess(int aHookType, DWORD aProcessId);

private:
	/**
	 * ����ͨ��������
	 */
	void *iDriverContext;
};

#endif