#define EXPORT_MODULE

#include <windows.h>
#include "ISISandBox.h"
#include <stdlib.h>
#include <stdio.h>
#include <winioctl.h>
#include <string.h>
#include <crtdbg.h>
#include <assert.h>
#include <fltuser.h>
#include <tchar.h>
#include <atlstr.h>

#define ISISandBoxPortName	L"\\ISISandBoxPort"

// 声明通信标记
#define MESSAGE_DRIVER_START_WORKING		0x00000001
#define MESSAGE_DRIVER_STOP_WORKING			0x00000002
#define MESSAGE_INSERT_PROTECT_PATH			0x10000001
#define MESSAGE_REMOVE_PROTECT_PATH			0x10000002
#define MESSAGE_REMOVE_ALL_PROTECT_PATH		0x10000004
#define MESSAGE_INSERT_REDIRECT_PATH		0x20000001
#define MESSAGE_REMOVE_REDIRECT_PATH		0x20000002
#define MESSAGE_REMOVE_ALL_REDIRECT_PATH	0x20000004
#define MESSAGE_INSERT_PROTECT_PROCESS		0x40000001
#define MESSAGE_REMOVE_PROTECT_PROCESS		0x40000002
#define MESSAGE_REMOVE_ALL_PROTECT_PROCESS	0x40000004

#define MESSAGE_CREATE_DIRECTORY_BY_USERMODE	0x70000001

#define MESSAGE_DEBUG_DEVICENAME_TO_DOSNAME	0x80000001

// 声明通信数据对象
typedef struct _MESSAGE_TYPE
{
	ULONG MessageType;

} MESSAGE_TYPE, *PMESSAGE_TYPE;

typedef struct _MESSAGE_REPLY
{
	ULONG MessageType;

	NTSTATUS status;

} MESSAGE_REPLY, *PMESSAGE_REPLY;

typedef struct _MESSAGE_PROTECT_PATH
{
	ULONG MessageType;

	WCHAR ProtectPath[260];

	ULONG Length;

} MESSAGE_PROTECT_PATH, *PMESSAGE_PROTECT_PATH;

typedef struct _MESSAGE_REDIRECT_PATH
{
	ULONG MessageType;

	WCHAR RedirectPath[260];

	ULONG Length;

} MESSAGE_REDIRECT_PATH, *PMESSAGE_REDIRECT_PATH;

typedef struct _MESSAGE_PROTECT_PROCESS
{
	ULONG MessageType;

	HANDLE ProcessId;

} MESSAGE_PROTECT_PROCESS, *PMESSAGE_PROTECT_PROCESS;

//
typedef struct _MESSAGE_TYPE_REPLY
{
	FILTER_REPLY_HEADER ReplyHeader;

	MESSAGE_TYPE Message;

} MESSAGE_TYPE_REPLY, *PMESSAGE_TYPE_REPLY;

typedef struct _MESSAGE_REPLY_REPLY
{
	FILTER_REPLY_HEADER ReplyHeader;

	MESSAGE_REPLY Message;

} MESSAGE_REPLY_REPLY, *PMESSAGE_REPLY_REPLY;

typedef struct _MESSAGE_PROTECT_PATH_REPLY
{
	FILTER_REPLY_HEADER ReplyHeader;

	MESSAGE_PROTECT_PATH Message;

} MESSAGE_PROTECT_PATH_REPLY, *PMESSAGE_PROTECT_PATH_REPLY;

typedef struct _MESSAGE_REDIRECT_PATH_REPLY
{
	FILTER_REPLY_HEADER ReplyHeader;

	MESSAGE_REDIRECT_PATH Message;

} MESSAGE_REDIRECT_PATH_REPLY, *PMESSAGE_REDIRECT_PATH_REPLY;

typedef struct _MESSAGE_PROTECT_PROCESS_REPLY
{
	FILTER_REPLY_HEADER ReplyHeader;

	MESSAGE_PROTECT_PROCESS Message;

} MESSAGE_PROTECT_PROCESS_REPLY, *PMESSAGE_PROTECT_PROCESS_REPLY;

//
typedef struct _MESSAGE_TYPE_SEND
{
	FILTER_MESSAGE_HEADER Header;

	MESSAGE_TYPE Message;

} MESSAGE_TYPE_SEND, *PMESSAGE_TYPE_SEND;

typedef struct _MESSAGE_REPLY_SEND
{
	FILTER_MESSAGE_HEADER Header;

	MESSAGE_REPLY Message;

} MESSAGE_REPLY_SEND, *PMESSAGE_REPLY_SEND;

typedef struct _MESSAGE_PROTECT_PATH_SEND
{
	FILTER_MESSAGE_HEADER Header;

	MESSAGE_PROTECT_PATH Message;

} MESSAGE_PROTECT_PATH_SEND, *PMESSAGE_PROTECT_PATH_SEND;

typedef struct _MESSAGE_REDIRECT_PATH_SEND
{
	FILTER_MESSAGE_HEADER Header;

	MESSAGE_REDIRECT_PATH Message;

} MESSAGE_REDIRECT_PATH_SEND, *PMESSAGE_REDIRECT_PATH_SEND;

typedef struct _MESSAGE_PROTECT_PROCESS_SEND
{
	FILTER_MESSAGE_HEADER Header;

	MESSAGE_PROTECT_PROCESS Message;

} MESSAGE_PROTECT_PROCESS_SEND, *PMESSAGE_PROTECT_PROCESS_SEND;


// 函数实现
int InstallISISandBox(const wchar_t *aServiceName)
{
	BOOL bOK = FALSE;

	SC_HANDLE hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hManager == NULL)
		goto InstallISISandBox_Error;

	DWORD dwTag = 5;
	SC_HANDLE hService = CreateService (hManager, _T("ISISandBox"), _T("ISISandBox"),
		SERVICE_ALL_ACCESS, SERVICE_FILE_SYSTEM_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
		_T("System32\\drivers\\ISISandBox.sys"), _T("FSFilter Virtualization"), &dwTag, _T("FltMgr"), NULL, NULL);

	if (hService == NULL)
		goto InstallISISandBox_Error;
	else
		CloseServiceHandle (hService);

	// 写注册表（MiniFilter规定的注册表）
	{
		TCHAR chPath[MAX_PATH];
		ZeroMemory(chPath, sizeof(TCHAR)*MAX_PATH);

		CString szAltitude = _T("135678");
		DWORD dwFlags = 0;
		CString szInst = _T("ISISandBox Instance");
		_tcscpy(chPath, szInst.GetString());
		CString szRegPath = _T("SYSTEM\\CurrentControlSet\\Services\\ISISandBox\\Instances");
		SHRegSetUSValue(szRegPath, _T("DefaultInstance"), REG_SZ, chPath, szInst.GetLength()*sizeof(TCHAR), SHREGSET_FORCE_HKLM);

		szRegPath = _T("SYSTEM\\CurrentControlSet\\Services\\ISISandBox\\Instances\\ISISandBox Instance");
		ZeroMemory(chPath, sizeof(TCHAR)*MAX_PATH);
		_tcscpy(chPath, szAltitude.GetString());
		DWORD dwLength = szAltitude.GetLength()*2;
		SHRegSetUSValue(szRegPath, _T(""), REG_SZ, chPath, 0, SHREGSET_FORCE_HKLM);
		SHRegSetUSValue(szRegPath, _T("Altitude"), REG_SZ, chPath, dwLength, SHREGSET_FORCE_HKLM);
		SHRegSetUSValue(szRegPath, _T("Flags"), REG_DWORD, &dwFlags, sizeof(DWORD), SHREGSET_FORCE_HKLM);
	}

	hService = OpenService (hManager, _T("ISISandBox"), SERVICE_ALL_ACCESS);
	if (hService == NULL)
		goto InstallISISandBox_Error;

	BOOL bRet = StartService (hService, 0, NULL);
	if (bRet == FALSE)
		goto InstallISISandBox_Error;

	bOK = TRUE;

InstallISISandBox_Error:

	DWORD dwError = GetLastError();
	if (dwError == ERROR_SERVICE_EXISTS)
	{
		hService = OpenService (hManager, _T("ISISandBox"), SERVICE_ALL_ACCESS);
		if (hService == NULL)
			goto InstallISISandBox_Error;

		bRet = StartService (hService, 0, NULL);
		if (bRet == FALSE)
			goto InstallISISandBox_Error;

		bOK = TRUE;
	}

	if (bOK == FALSE && dwError != ERROR_SERVICE_ALREADY_RUNNING)
	{
		bOK = FALSE;
	}
	else
	{
		bOK = TRUE;
		dwError = 0;
	}

	if (hService != NULL)
		CloseServiceHandle (hService);

	if (hManager != NULL)
		CloseServiceHandle (hManager);

	return dwError;
}

int UninstallISISandBox(const wchar_t *aServiceName)
{
	SC_HANDLE hManager, hService = NULL;
	BOOL bOK = FALSE, bRet;
	DWORD dwError = 0;
	SERVICE_STATUS serviceStatus;
	ZeroMemory(&serviceStatus, sizeof(SERVICE_STATUS));

	hManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hManager == NULL)
		goto error;

	hService = OpenService (hManager, _T("ISISandBox"), SERVICE_ALL_ACCESS);
	if (hService == NULL)
		goto error;

	bRet = ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);
	if (bRet == FALSE)
		goto error;

	bRet = DeleteService(hService);
	if (bRet == FALSE)
		goto error;

	bOK = TRUE;

error:
	dwError = GetLastError();
	if (bOK == FALSE)
	{
		bOK = FALSE;
	}
	else
	{
		bOK = TRUE;
		dwError = 0;
	}

	if (hService != NULL)
		CloseServiceHandle (hService);

	if (hManager != NULL)
		CloseServiceHandle (hManager);

	return dwError;
}

int ISISandBoxSetProtectPath(const wchar_t *aProtectPath)
{
	int errCode = ERROR_SUCCESS;
	HANDLE driverPort = NULL;

	__try
	{
		if (!aProtectPath)
		{
			errCode = ERROR_INVALID_PARAMETER;
			__leave;
		}

		if (wcslen(aProtectPath) > 260)
		{
			errCode = ERROR_BAD_LENGTH;
			__leave;
		}

		// 测试连接
		HRESULT ret = FilterConnectCommunicationPort(ISISandBoxPortName, 0, NULL, 0, NULL, &driverPort);
		if (IS_ERROR(ret))
		{
			errCode = ERROR_NOT_CONNECTED;
			__leave;
		}

		MESSAGE_PROTECT_PATH_SEND message;
		message.Message.MessageType = MESSAGE_INSERT_PROTECT_PATH;
		wcscpy_s(message.Message.ProtectPath, 260, aProtectPath);
		_wcsupr_s(message.Message.ProtectPath, 260);			// 在用户态强制改为大写
		message.Message.Length = (ULONG)wcslen(aProtectPath);

		DWORD replyLength = 0;
		MESSAGE_REPLY reply;

		ret = FilterSendMessage(driverPort, (PFILTER_MESSAGE_HEADER)&message, sizeof(MESSAGE_PROTECT_PATH_SEND), (PVOID)&reply, sizeof(MESSAGE_REPLY), &replyLength);
		if (ret != S_OK)
		{
			errCode = GetLastError();
			__leave;
		}

		if (reply.MessageType != MESSAGE_INSERT_PROTECT_PATH && reply.status != 0)
		{
			errCode = ERROR_NOT_SUPPORTED;
			__leave;
		}
	}
	__finally
	{
		CloseHandle(driverPort);
		driverPort = NULL;
	}
	
	return errCode;
}

int ISISandBoxRemoveProtectPath(const wchar_t *aProtectPath)
{
	int errCode = ERROR_SUCCESS;
	HANDLE driverPort = NULL;

	__try
	{
		if (!aProtectPath)
		{
			errCode = ERROR_INVALID_PARAMETER;
			__leave;
		}

		if (wcslen(aProtectPath) > 260)
		{
			errCode = ERROR_BAD_LENGTH;
			__leave;
		}

		// 测试连接
		HRESULT ret = FilterConnectCommunicationPort(ISISandBoxPortName, 0, NULL, 0, NULL, &driverPort);
		if (IS_ERROR(ret))
		{
			errCode = ERROR_NOT_CONNECTED;
			__leave;
		}

		MESSAGE_PROTECT_PATH_SEND message;
		message.Message.MessageType = MESSAGE_REMOVE_PROTECT_PATH;
		wcscpy_s(message.Message.ProtectPath, 260, aProtectPath);
		_wcsupr_s(message.Message.ProtectPath, 260);			// 在用户态强制改为大写
		message.Message.Length = (ULONG)wcslen(aProtectPath);

		DWORD replyLength = 0;
		MESSAGE_REPLY reply;

		ret = FilterSendMessage(driverPort, (PFILTER_MESSAGE_HEADER)&message, sizeof(MESSAGE_PROTECT_PATH_SEND), (PVOID)&reply, sizeof(MESSAGE_REPLY), &replyLength);
		if (ret != S_OK)
		{
			errCode = GetLastError();
			__leave;
		}

		if (reply.MessageType != MESSAGE_INSERT_PROTECT_PATH && reply.status != 0)
		{
			errCode = ERROR_NOT_SUPPORTED;
			__leave;
		}
	}
	__finally
	{
		CloseHandle(driverPort);
		driverPort = NULL;
	}

	return errCode;
}

int ISISandBoxRemoveAllProtectPath()
{
	int errCode = ERROR_SUCCESS;
	HANDLE driverPort = NULL;

	__try
	{
		// 测试连接
		HRESULT ret = FilterConnectCommunicationPort(ISISandBoxPortName, 0, NULL, 0, NULL, &driverPort);
		if (IS_ERROR(ret))
		{
			errCode = ERROR_NOT_CONNECTED;
			__leave;
		}

		MESSAGE_PROTECT_PATH_SEND message;
		message.Message.MessageType = MESSAGE_REMOVE_ALL_PROTECT_PATH;

		DWORD replyLength = 0;
		MESSAGE_REPLY reply;

		ret = FilterSendMessage(driverPort, (PFILTER_MESSAGE_HEADER)&message, sizeof(MESSAGE_PROTECT_PATH_SEND), (PVOID)&reply, sizeof(MESSAGE_REPLY), &replyLength);
		if (ret != S_OK)
		{
			errCode = GetLastError();
			__leave;
		}

		if (reply.MessageType != MESSAGE_INSERT_PROTECT_PATH && reply.status != 0)
		{
			errCode = ERROR_NOT_SUPPORTED;
			__leave;
		}
	}
	__finally
	{
		CloseHandle(driverPort);
		driverPort = NULL;
	}

	return errCode;
}

int ISISandBoxSetRedirectPath(const wchar_t *aRedirectPath)
{
	int errCode = ERROR_SUCCESS;
	HANDLE driverPort = NULL;

	__try
	{
		if (!aRedirectPath)
		{
			errCode = ERROR_INVALID_PARAMETER;
			__leave;
		}

		if (wcslen(aRedirectPath) > 260)
		{
			errCode = ERROR_BAD_LENGTH;
			__leave;
		}

		// 测试连接
		HRESULT ret = FilterConnectCommunicationPort(ISISandBoxPortName, 0, NULL, 0, NULL, &driverPort);
		if (IS_ERROR(ret))
		{
			errCode = ERROR_NOT_CONNECTED;
			__leave;
		}

		MESSAGE_REDIRECT_PATH_SEND message;
		message.Message.MessageType = MESSAGE_INSERT_REDIRECT_PATH;
		wcscpy_s(message.Message.RedirectPath, 260, aRedirectPath);
		_wcsupr_s(message.Message.RedirectPath, 260);			// 在用户态强制改为大写
		message.Message.Length = (ULONG)wcslen(aRedirectPath);

		DWORD replyLength = 0;
		MESSAGE_REPLY reply;

		ret = FilterSendMessage(driverPort, (PFILTER_MESSAGE_HEADER)&message, sizeof(MESSAGE_PROTECT_PATH_SEND), (PVOID)&reply, sizeof(MESSAGE_REPLY), &replyLength);
		if (ret != S_OK)
		{
			errCode = GetLastError();
			__leave;
		}

		if (reply.MessageType != MESSAGE_INSERT_REDIRECT_PATH && reply.status != 0)
		{
			errCode = ERROR_NOT_SUPPORTED;
			__leave;
		}
	}
	__finally
	{
		CloseHandle(driverPort);
		driverPort = NULL;
	}
	
	return errCode;
}

int ISISandBoxSetProtectProcess(DWORD aProcessId)
{
	int errCode = ERROR_SUCCESS;
	HANDLE driverPort = NULL;

	__try
	{
		// 测试连接
		HRESULT ret = FilterConnectCommunicationPort(ISISandBoxPortName, 0, NULL, 0, NULL, &driverPort);
		if (IS_ERROR(ret))
		{
			errCode = ERROR_NOT_CONNECTED;
			__leave;
		}

		MESSAGE_PROTECT_PROCESS_SEND message;
		message.Message.MessageType = MESSAGE_INSERT_PROTECT_PROCESS;
		message.Message.ProcessId = (HANDLE)aProcessId;

		DWORD replyLength = 0;
		MESSAGE_REPLY reply;

		ret = FilterSendMessage(driverPort, (PFILTER_MESSAGE_HEADER)&message, sizeof(MESSAGE_PROTECT_PATH_SEND), (PVOID)&reply, sizeof(MESSAGE_REPLY), &replyLength);
		if (ret != S_OK)
		{
			errCode = GetLastError();
			__leave;
		}

		if (reply.MessageType != MESSAGE_INSERT_PROTECT_PROCESS && reply.status != 0)
		{
			errCode = ERROR_NOT_SUPPORTED;
			__leave;
		}
	}
	__finally
	{
		CloseHandle(driverPort);
		driverPort = NULL;
	}

	return errCode;
}

int ISISandBoxRemoveProtectProcess(DWORD aProcessId)
{
	int errCode = ERROR_SUCCESS;
	HANDLE driverPort = NULL;

	__try
	{
		// 测试连接
		HRESULT ret = FilterConnectCommunicationPort(ISISandBoxPortName, 0, NULL, 0, NULL, &driverPort);
		if (IS_ERROR(ret))
		{
			errCode = ERROR_NOT_CONNECTED;
			__leave;
		}

		MESSAGE_PROTECT_PROCESS_SEND message;
		message.Message.MessageType = MESSAGE_REMOVE_PROTECT_PROCESS;
		message.Message.ProcessId = (HANDLE)aProcessId;

		DWORD replyLength = 0;
		MESSAGE_REPLY reply;

		ret = FilterSendMessage(driverPort, (PFILTER_MESSAGE_HEADER)&message, sizeof(MESSAGE_PROTECT_PATH_SEND), (PVOID)&reply, sizeof(MESSAGE_REPLY), &replyLength);
		if (ret != S_OK)
		{
			errCode = GetLastError();
			__leave;
		}

		if (reply.MessageType != MESSAGE_INSERT_PROTECT_PROCESS && reply.status != 0)
		{
			errCode = ERROR_NOT_SUPPORTED;
			__leave;
		}
	}
	__finally
	{
		CloseHandle(driverPort);
		driverPort = NULL;
	}

	return ERROR_SUCCESS;
}

int ISISandBoxRemoveAllProtectProcess()
{
	int errCode = ERROR_SUCCESS;
	HANDLE driverPort = NULL;

	__try
	{
		// 测试连接
		HRESULT ret = FilterConnectCommunicationPort(ISISandBoxPortName, 0, NULL, 0, NULL, &driverPort);
		if (IS_ERROR(ret))
		{
			errCode = ERROR_NOT_CONNECTED;
			__leave;
		}

		MESSAGE_PROTECT_PROCESS_SEND message;
		message.Message.MessageType = MESSAGE_REMOVE_ALL_PROTECT_PROCESS;

		DWORD replyLength = 0;
		MESSAGE_REPLY reply;

		ret = FilterSendMessage(driverPort, (PFILTER_MESSAGE_HEADER)&message, sizeof(MESSAGE_PROTECT_PATH_SEND), (PVOID)&reply, sizeof(MESSAGE_REPLY), &replyLength);
		if (ret != S_OK)
		{
			errCode = GetLastError();
			__leave;
		}

		if (reply.MessageType != MESSAGE_INSERT_PROTECT_PROCESS && reply.status != 0)
		{
			errCode = ERROR_NOT_SUPPORTED;
			__leave;
		}
	}
	__finally
	{
		CloseHandle(driverPort);
		driverPort = NULL;
	}

	return errCode;
}

int ISISandBoxStart()
{
	int errCode = ERROR_SUCCESS;
	HANDLE driverPort = NULL;

	__try
	{
		// 测试连接
		HRESULT ret = FilterConnectCommunicationPort(ISISandBoxPortName, 0, NULL, 0, NULL, &driverPort);
		if (IS_ERROR(ret))
		{
			errCode = ERROR_NOT_CONNECTED;
			__leave;
		}

		MESSAGE_TYPE_SEND message;
		message.Message.MessageType = MESSAGE_DRIVER_START_WORKING;

		DWORD replyLength = 0;
		MESSAGE_REPLY reply;

		ret = FilterSendMessage(driverPort, (PFILTER_MESSAGE_HEADER)&message, sizeof(MESSAGE_TYPE_SEND), (PVOID)&reply, sizeof(MESSAGE_REPLY), &replyLength);
		if (ret != S_OK)
		{
			errCode = GetLastError();
			__leave;
		}

		if (reply.MessageType != MESSAGE_DRIVER_START_WORKING && reply.status != 0)
		{
			errCode = ERROR_NOT_SUPPORTED;
			__leave;
		}
	}
	__finally
	{
		CloseHandle(driverPort);
		driverPort = NULL;
	}

	return errCode;
}

int ISISandBoxStop()
{
	int errCode = ERROR_SUCCESS;
	HANDLE driverPort = NULL;

	__try
	{
		// 测试连接
		HRESULT ret = FilterConnectCommunicationPort(ISISandBoxPortName, 0, NULL, 0, NULL, &driverPort);
		if (IS_ERROR(ret))
		{
			errCode = ERROR_NOT_CONNECTED;
			__leave;
		}

		MESSAGE_TYPE_SEND message;
		message.Message.MessageType = MESSAGE_DRIVER_STOP_WORKING;

		printf("message length : %d\n", sizeof(MESSAGE_TYPE_SEND));

		DWORD replyLength = 0;
		MESSAGE_REPLY reply;

		ret = FilterSendMessage(driverPort, (PFILTER_MESSAGE_HEADER)&message, sizeof(MESSAGE_TYPE_SEND), (PVOID)&reply, sizeof(MESSAGE_REPLY), &replyLength);
		if (ret != S_OK)
		{
			errCode = GetLastError();
			__leave;
		}

		if (reply.MessageType != MESSAGE_DRIVER_STOP_WORKING && reply.status != 0)
		{
			errCode = ERROR_NOT_SUPPORTED;
			__leave;
		}
	}
	__finally
	{
		CloseHandle(driverPort);
		driverPort = NULL;
	}

	return errCode;
}

int CreateDirectoryEx(TCHAR *aDirctoryPath)
{
	if (!aDirctoryPath || _tcslen(aDirctoryPath) <= 3)
		return -1;

	TCHAR createDir[MAX_PATH] = {0};
	createDir[0] = aDirctoryPath[0];
	createDir[1] = aDirctoryPath[1];
	createDir[2] = aDirctoryPath[2];

	size_t pathLength = _tcslen(aDirctoryPath);
	for (size_t index = 3; index < pathLength; ++index)
	{
		createDir[index] = aDirctoryPath[index];
		if (createDir[index] == _T('\\'))
		{
			CreateDirectory(createDir, NULL);
		}
	}

	return 0;
}