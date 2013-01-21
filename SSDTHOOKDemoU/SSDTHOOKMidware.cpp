#include "stdafx.h"
#include "SSDTHOOKMidware.h"

// 提升当前进程权限
BOOL AdjustProcessTokenPrivilege()
{
	LUID luidTmp;
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		OutputDebugString(_TEXT("AdjustProcessTokenPrivilege - OpenProcessToken Failed , Error Code Is {0} , Error Message Is {1} ! \n"));
		return false;
	}

	if(!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidTmp))
	{
		OutputDebugString(_TEXT("AdjustProcessTokenPrivilege - LookupPrivilegeValue Failed , Error Code Is {0} , Error Message Is {1} ! \n"));
		CloseHandle(hToken);
		return FALSE;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luidTmp;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if(!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
	{
		OutputDebugString(_TEXT("AdjustProcessTokenPrivilege - AdjustTokenPrivileges Failed , Error Code Is {0} , Error Message Is {1} ! \n"));
		CloseHandle(hToken);
		return FALSE;
	}
	return true;
}

SSDTHOOKMidware::SSDTHOOKMidware()
: iDriverContext(NULL)
{

}

SSDTHOOKMidware::~SSDTHOOKMidware()
{
	if (iDriverContext)
	{
		CloseHandle(iDriverContext);
	}

	iDriverContext == NULL;
}

int SSDTHOOKMidware::RunSSDTHOOKDriver(TCHAR *aServiceName)
{
	int errCode = ERROR_SUCCESS;

	// 检查参数是否有效
	if (!aServiceName)
		return ERROR_INVALID_PARAMETER;

	// 提升权限
	AdjustProcessTokenPrivilege();

	// 检查服务是否存在
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSCManager)
	{
		errCode = GetLastError();
		OutputDebugString(_T("RunSSDTHOOKDriver Step - 1 : OpenSCManager failed .\n"));
		return errCode;
	}

	// 安装此服务
	// 如果成功了就直接启动
	// 如果失败了，并且错误码为ERROR_SERVICE_EXISTS，直接驱动
	TCHAR binaryPath[MAX_PATH] = {0};
	_tcscpy_s(binaryPath, MAX_PATH, _T("system32\\drivers\\"));
	_tcscat_s(binaryPath, MAX_PATH, aServiceName);
	_tcscat_s(binaryPath, MAX_PATH, _T(".sys"));

	SC_HANDLE hService = CreateService(hSCManager, aServiceName, aServiceName, SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, binaryPath,
		NULL, NULL, NULL, NULL, NULL);
	if (!hService)
	{
		errCode = GetLastError();
		if (ERROR_SERVICE_EXISTS == errCode)
			goto STARTUP_SSDTHOOK_DRIVER;

		// 其他的失败
		OutputDebugString(_T("RunSSDTHOOKDriver Step - 2 : Create Service failed .\n"));
		CloseServiceHandle(hSCManager);
		return errCode;
	}

STARTUP_SSDTHOOK_DRIVER:
	// 安装完成或者服务已存在，则直接启动
	hService = OpenService(hSCManager, aServiceName, SERVICE_ALL_ACCESS);
	if (NULL == hService)
	{
		errCode = GetLastError();
		OutputDebugString(_T("StopSSDTHOOKDriver Step - 2 : OpenService failed .\n"));
		return errCode;
	}

	BOOL ret = StartService(hService, NULL, NULL);
	if (!ret)
	{
		errCode = GetLastError();
		OutputDebugString(_T("RunSSDTHOOKDriver Step - 3 : Start Service failed .\n"));
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		return errCode;
	}

	OutputDebugString(_T("RunSSDTHOOKDriver Run Service succeed .\n"));
	return errCode;
}

int SSDTHOOKMidware::StopSSDTHOOKDriver(TCHAR *aServiceName)
{
	int errCode = ERROR_SUCCESS;

	// 验证参数有效性
	if (!aServiceName)
		return ERROR_INVALID_PARAMETER;

	// 向驱动发送可以卸载的消息，使用DeviceIoControl()

	// 检查服务是否存在
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSCManager)
	{
		errCode = GetLastError();
		OutputDebugString(_T("StopSSDTHOOKDriver Step - 1 : OpenSCManager failed .\n"));
		return errCode;
	}

	SC_HANDLE hService = OpenService(hSCManager, aServiceName, SERVICE_ALL_ACCESS);
	if (NULL == hService)
	{
		errCode = GetLastError();
		OutputDebugString(_T("StopSSDTHOOKDriver Step - 2 : OpenService failed .\n"));
		return errCode;
	}

	SERVICE_STATUS ssStatus; 
	BOOL ret = ControlService(hService, SERVICE_CONTROL_STOP, &ssStatus);
	if (!ret)
	{
		errCode = GetLastError();
		OutputDebugString(_T("StopSSDTHOOKDriver Step - 3 : ControlService::SERVICE_CONTROL_STOP failed .\n"));
		return errCode;
	}

	return ERROR_SUCCESS;
}

int SSDTHOOKMidware::ConnectToDriver(TCHAR *aDeviceName)
{
	// 连接到驱动，发送开始工作的命令
}