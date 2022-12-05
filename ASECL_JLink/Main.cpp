#include "pch.h"
#include "Main.h"
#include <thread>
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>
#include <winapifamily.h>
#include <setupapi.h>
#include <devguid.h>
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>
#include <avrt.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys.h>
#include<Mmsystem.h>

#pragma comment( lib, "setupapi.lib" )
#pragma comment(lib,"winmm.lib")

#define JLINKAPI __cdecl

#define CMD_BUFFER 8192
#define WRITE_BUFFER 4096

#define LOGD(msg)   do{\
						if(_logCallback != NULL) {\
							_logCallback((TCHAR*)msg);\
						}\
					}while(0);\

extern LogMsg* _logCallback = nullptr;

#if OLDER_VERSION == 1
const TCHAR* JLINK_FILE_NAME = _T("JLinkScript.txt");
extern TCHAR _jlinkExePath[4096] = { L'\0' };
static bool _CheckFileExists(TCHAR* filePath);
static bool _WriteJLinkFile(char* content);
static bool _DeleteJLinkFile(void);
static TCHAR* _ExecuteJLinkScript(bool* result);

#else

//JLINK TIF
#define JLINK_TIF_JTAG          0
#define JLINK_TIF_SWD           1

//RESET TYPE
#define JLINK_RESET_TYPE_NORMAL 0
#define JLINK_RESET_TYPE_CORE   1
#define JLINK_RESET_TYPE_PIN    2

#define JLINK_ERR_FLASH_PROG_COMPARE_FAILED -265
#define JLINK_ERR_FLASH_PROG_PROGRAM_FAILED -266
#define JLINK_ERR_FLASH_PROG_VERIFY_FAILED -267
#define JLINK_ERR_OPEN_FILE_FAILED -268
#define JLINK_ERR_UNKNOWN_FILE_FORMAT -269
#define JLINK_ERR_WRITE_TARGET_MEMORY_FAILED -270

//REGISTER INDEX
/*
0 - 15     R0 - R15(SP=R13, PC=R15)
16          XPSR
17          MSP
18          PSP
19          RAZ
20          CFBP
21          APSR
22          EPSR
23          IPSR
24          PRIMASK
25          BASEPRI
26          FAULTMASK
27          CONTROL
28          BASEPRI_MAX
29          IAPSR
30          EAPSR
31          IEPSR
*/

typedef struct
{
	unsigned short vtarget;
	unsigned char  tck;
	unsigned char  tdi;
	unsigned char  tdo;
	unsigned char  tms;
	unsigned char  tres;
	unsigned char  trst;
} HWSTATUS;

typedef void(*JLINK_LOG)(const char* sError);

typedef DWORD(JLINKAPI* JLINKARM_GetDLLVersion)(void);
typedef DWORD(JLINKAPI* JLINKARM_GetHardwareVersion)(void);
typedef DWORD(JLINKAPI* JLINKARM_GetFirmwareString)(char* buff, int count);
typedef DWORD(JLINKAPI* JLINKARM_GetSN)(void);
typedef BOOL(JLINKAPI* JLINKARM_ExecCommand)(char* cmd, char* error, int bufferSize);
typedef DWORD(JLINKAPI* JLINKARM_TIF_Select)(int type);
typedef void  (JLINKAPI* JLINKARM_SetSpeed)(int speed);
typedef DWORD(JLINKAPI* JLINKARM_GetSpeed)(void);
typedef DWORD(JLINKAPI* JLINKARM_GetId)(void);
typedef DWORD(JLINKAPI* JLINKARM_GetDeviceFamily)(void);
typedef BOOL(JLINKAPI* JLINKARM_Open)(void);
typedef BOOL(JLINKAPI* JLINKARM_OpenEx)(JLINK_LOG pfLog, JLINK_LOG pfErrorOut);
typedef void (JLINKAPI* JLINKARM_EnableLog)(JLINK_LOG LogOutHandler);
typedef void  (JLINKAPI* JLINKARM_Close)(void);
typedef BOOL(JLINKAPI* JLINKARM_IsOpen)(void);
typedef int (JLINKAPI* JLINKARM_Connect)(void);
typedef BOOL(JLINKAPI* JLINKARM_IsConnected)(void);
typedef int   (JLINKAPI* JLINKARM_Halt)(void);
typedef BOOL(JLINKAPI* JLINKARM_IsHalted)(void);
typedef void  (JLINKAPI* JLINKARM_SetResetType)(int type);
typedef int  (JLINKAPI* JLINKARM_Reset)(void);
typedef void  (JLINKAPI* JLINKARM_GoIntDis)(void);
typedef DWORD(JLINKAPI* JLINKARM_ReadReg)(int index);
typedef int   (JLINKAPI* JLINKARM_WriteReg)(int index, DWORD data);
typedef int   (JLINKAPI* JLINKARM_ReadMem)(DWORD addr, int len, void* buf);
typedef int   (JLINKAPI* JLINKARM_WriteMem)(uint32_t addr, int len, void* buf);
typedef int   (JLINKAPI* JLINKARM_WriteU8)(DWORD addr, BYTE data);
typedef int   (JLINKAPI* JLINKARM_WriteU16)(DWORD addr, WORD data);
typedef int   (JLINKAPI* JLINK_EraseChip)(void);
typedef int   (WINAPI* JLINK_DownloadFile)(const char* file, DWORD addr);
typedef void  (JLINKAPI* JLINKARM_BeginDownload)(int index);
typedef int  (JLINKAPI* JLINKARM_EndDownload)(void);
typedef BOOL(JLINKAPI* JLINKARM_HasError)(void);
typedef void  (JLINKAPI* JLINKARM_Go)(void);
typedef void (JLINKAPI* JLINKARM_ClrError)(void);
typedef void (JLINKAPI* JLINKARM_SetErrorOutHandler)(JLINK_LOG pfErrorOut);
typedef void (JLINKAPI* JLINKARM_SetWarnOutHandler)(JLINK_LOG pfWarnOut);
typedef void (JLINKAPI* JLINKARM_SetLogFile)(const char* sFilename);
typedef void (JLINKAPI* JLINKARM_GetOEMString)(char* str);
typedef unsigned (JLINKAPI* JLINKARM_GetEmuCaps)();
typedef int(JLINKAPI* JLINKARM_GetHWStatus)(HWSTATUS* sts);
typedef int(JLINKAPI* JLINKARM_ReadMemU32)(uint32_t Addr, uint32_t NumItems, uint32_t* pData, uint8_t* pStatus);
typedef int(JLINKAPI* JLINKARM_WriteU32)(uint32_t Addr, uint32_t Data);

//======================
static JLINKARM_GetDLLVersion _JLINKARM_GetDLLVersion = nullptr;
static JLINKARM_GetHardwareVersion _JLINKARM_GetHardwareVersion = nullptr;
static JLINKARM_GetFirmwareString _JLINKARM_GetFirmwareString = nullptr;
static JLINKARM_GetSN _JLINKARM_GetSN = nullptr;
static JLINKARM_ExecCommand _JLINKARM_ExecCommand = nullptr;
static JLINKARM_TIF_Select _JLINKARM_TIF_Select = nullptr;
static JLINKARM_SetSpeed _JLINKARM_SetSpeed = nullptr;
static JLINKARM_GetSpeed _JLINKARM_GetSpeed = nullptr;
static JLINKARM_GetId _JLINKARM_GetId = nullptr;
static JLINKARM_GetDeviceFamily _JLINKARM_GetDeviceFamily = nullptr;
static JLINKARM_Open _JLINKARM_Open = nullptr;
static JLINKARM_OpenEx _JLINKARM_OpenEx = nullptr;
static JLINKARM_Close _JLINKARM_Close = nullptr;
static JLINKARM_EnableLog _JLINKARM_EnableLog = nullptr;
static JLINKARM_IsOpen _JLINKARM_IsOpen = nullptr;
static JLINKARM_Connect _JLINKARM_Connect = nullptr;
static JLINKARM_IsConnected _JLINKARM_IsConnected = nullptr;
static JLINKARM_Halt _JLINKARM_Halt = nullptr;
static JLINKARM_IsHalted _JLINKARM_IsHalted = nullptr;
static JLINKARM_SetResetType _JLINKARM_SetResetType = nullptr;
static JLINKARM_Reset _JLINKARM_Reset = nullptr;
static JLINKARM_GoIntDis _JLINKARM_GoIntDis = nullptr;
static JLINKARM_ReadReg _JLINKARM_ReadReg = nullptr;
static JLINKARM_WriteReg _JLINKARM_WriteReg = nullptr;
static JLINKARM_ReadMem _JLINKARM_ReadMem = nullptr;
static JLINKARM_WriteMem _JLINKARM_WriteMem = nullptr;
static JLINKARM_WriteU8 _JLINKARM_WriteU8 = nullptr;
static JLINKARM_WriteU16 _JLINKARM_WriteU16 = nullptr;
static JLINKARM_WriteU32 _JLINKARM_WriteU32 = nullptr;
static JLINK_EraseChip _JLINK_EraseChip = nullptr;
static JLINK_DownloadFile _JLINK_DownloadFile = nullptr;
static JLINKARM_BeginDownload _JLINKARM_BeginDownload = nullptr;
static JLINKARM_EndDownload _JLINKARM_EndDownload = nullptr;
static JLINKARM_HasError _JLINKARM_HasError = nullptr;
static JLINKARM_Go _JLINKARM_Go = nullptr;
static JLINKARM_ClrError _JLINKARM_ClrError = nullptr;
static JLINKARM_SetErrorOutHandler _JLINKARM_SetErrorOutHandler = nullptr;
static JLINKARM_SetWarnOutHandler _JLINKARM_SetWarnOutHandler = nullptr;
static JLINKARM_SetLogFile _JLINKARM_SetLogFile = nullptr;
static JLINKARM_GetOEMString _JLINKARM_GetOEMString = nullptr;
static JLINKARM_GetEmuCaps _JLINKARM_GetEmuCaps = nullptr;
static JLINKARM_GetHWStatus _JLINKARM_GetHWStatus = nullptr;
static JLINKARM_ReadMemU32 _JLINKARM_ReadMemU32 = nullptr;

static bool _EraseAllNordic52840ByRegister(void);
static bool _ErasePartialNordic52840ByRegister(uint32_t registerPos);

#endif

#define JLINK_FILE_NAME _T("JLINK.log")
static TCHAR* _Char2TCHAR(char* src);
static char* _TCHAR2Char(TCHAR* src);
static bool _JlinkDriverIsExist(void);
static bool _CheckFileExists(TCHAR* filePath);
static bool _DeleteJLinkFile(void);

void SetLogCallback(LogMsg& logFunc)
{
	_logCallback = &logFunc;
}

static TCHAR* _Char2TCHAR(char* src) {
	static TCHAR cvt[8192] = { L'\0' };
	memset(cvt, L'\0', sizeof(TCHAR) * 8192);

	DWORD dBufSize = MultiByteToWideChar(CP_ACP, 0, src, strlen(src), NULL, 0);
	if (dBufSize <= 8191) {
		int nRet = MultiByteToWideChar(CP_ACP, 0, src, strlen(src), cvt, dBufSize);
		if (nRet > 0) {
			cvt[dBufSize] = L'\0';
			return cvt;
		}

		return nullptr;
	}
	return nullptr;
}

static char* _TCHAR2Char(TCHAR* src) {
	static char cvt[8192] = { '\0' };
	memset(cvt, '\0', sizeof(char) * 8192);
	DWORD dBufSize = WideCharToMultiByte(CP_ACP, 0, src, -1, NULL, 0, NULL, FALSE);
	if (dBufSize <= 8191) {
		int nRet = WideCharToMultiByte(CP_ACP, 0, src, -1, cvt, dBufSize, NULL, FALSE);
		if (nRet > 0) {
			cvt[dBufSize] = '\0';
			return cvt;
		}

		return nullptr;
	}
	return nullptr;
}

static bool _JlinkDriverIsExist(void)
{
	int i = 0;
	int res = 0;
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData = { sizeof(DeviceInfoData) };
	//A5DCBF10-6530-11D2-901F-00C04FB951ED
	//{36fc9e60-c465-11cf-8056-444553540000}
	// {9d7debbc-c85d-11d1-9eb4-006008c3a19a}

	//GUID USB_ID = { 0x9d7debbc, 0xc85d, 0x11d1, 0x9e, 0xb4, 0x00, 0x60, 0x08, 0xc3, 0xa1, 0x9a };
	// get device class information handle
	hDevInfo = SetupDiGetClassDevs(0, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		TCHAR msg[2048] = { L'\0' };
		_stprintf(msg, _T("[ ERROR ] DUT ControlQCCUsbDbgDevice GetUSBDevicePath ERR Code : %d"), GetLastError());
		LOGD(msg);
		return FALSE;
	}

	// enumerute device information
	DWORD required_size = 0;
	bool isFindJLink = false;
	for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
	{
		DWORD DataT;
		char deviceDesc[2046] = { 0 };
		DWORD buffersize = 2046;
		DWORD req_bufsize = 0;

		// get device description information
		if (!SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC, &DataT, (LPBYTE)deviceDesc, buffersize, &req_bufsize))
		{
			res = GetLastError();
			continue;
		}

		//J-Link driver
		if (strlen(deviceDesc) > 0 && strcmp(deviceDesc, "J-Link driver") == 0) {
			LOGD(_T("[ JLINK INFO ] Find JLink Driver !!"));
			isFindJLink = true;
			break;
		}
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return isFindJLink;
}

static bool _CheckFileExists(TCHAR* filePath) {
	DWORD dwAttrib = GetFileAttributes(filePath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static bool _DeleteJLinkFile(void) {
	if (!_CheckFileExists((TCHAR*)JLINK_FILE_NAME)) {
		return TRUE;
	}

	BOOL fSuccess = DeleteFile((TCHAR*)JLINK_FILE_NAME);

	if (!fSuccess)
	{
		// Handle the error.
		TCHAR msg[4096] = { L'\0' };
		_swprintf(msg, _T("[ ERROR ] _DeleteJLinkFile Fail, Win32API ErrorCode : %d !!"), GetLastError());
		LOGD(msg);
		return FALSE;
	}
	return TRUE;
}

#if OLDER_VERSION == 1

bool SetJlinkExePos(TCHAR* jlinkPos)
{
	if (!_CheckFileExists(jlinkPos)) {
		LOGD(_T("[ ERROR ] SetJlinkExePos Input File Path Is Not Exists !!"));
		return false;
	}

	lstrcpy(_jlinkExePath, jlinkPos);
}

bool BurnNordic52840FWBySWD(TCHAR* fwPos)
{
	if (!_JlinkDriverIsExist()) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Not Found J-Link Driver !!"));
		return false;
	}

	static char cmd[CMD_BUFFER] = { '\0' };
	if (!_CheckFileExists(_jlinkExePath)) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Jlink Execute Path Is Not Exists !!"));
		return false;
	}

	if (!_CheckFileExists(fwPos)) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD FW Path Is Not Exists !!"));
		return false;
	}

	memset(cmd, L'\0', sizeof(char) * CMD_BUFFER);
	strcpy(cmd, "SelectInterface SWD\r\n");
	strcat(cmd, "Speed 4000\r\n");
	strcat(cmd, "Device NRF52840_XXAA\r\n");
	strcat(cmd, "R\r\n");
	strcat(cmd, "H\r\n");
	strcat(cmd, "Erase\r\n");
	strcat(cmd, "LoadFile ");
	strcat(cmd, _TCHAR2Char(fwPos));
	strcat(cmd, "\r\n");
	strcat(cmd, "R\r\n");
	strcat(cmd, "Exit\r\n");

	if (!_WriteJLinkFile(cmd)) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Write JLink Script Fail !!"));
		_DeleteJLinkFile();
		return false;
	}
	bool isOK = false;
	TCHAR* msg = _ExecuteJLinkScript(&isOK);

	if (!isOK) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Execute JLink Script Fail !!"));
		_DeleteJLinkFile();
		return false;
	}

	if (_tcsstr(msg, _T("Failed")) != nullptr) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Execute JLink Script Fail (Have Failed String)!!"));
		_DeleteJLinkFile();
		return false;
	}

	if (_tcsstr(msg, _T("Cannot connect to target")) != nullptr) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Execute JLink Script Fail (Can't connect to target)!!"));
		_DeleteJLinkFile();
		return false;
	}

	if (!_DeleteJLinkFile()) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Delete JLink Script Fail !!"));
		return false;
	}

	return true;
}

bool EraseNordic52840BySWD(void)
{
	if (!_JlinkDriverIsExist()) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Not Found J-Link Driver !!"));
		return false;
	}

	static char cmd[CMD_BUFFER] = { '\0' };
	if (!_CheckFileExists(_jlinkExePath)) {
		LOGD(_T("[ ERROR ] EraseNordic52840BySWD Jlink Execute Path Is Not Exists !!"));
		return false;
	}

	memset(cmd, L'\0', sizeof(char) * CMD_BUFFER);
	strcpy(cmd, "SelectInterface SWD\r\n");
	strcat(cmd, "Speed 4000\r\n");
	strcat(cmd, "Device NRF52840_XXAA\r\n");
	strcat(cmd, "R\r\n");
	strcat(cmd, "H\r\n");
	strcat(cmd, "Erase\r\n");
	strcat(cmd, "R\r\n");
	strcat(cmd, "Exit\r\n");

	if (!_WriteJLinkFile(cmd)) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Write JLink Script Fail !!"));
		_DeleteJLinkFile();
		return false;
	}

	bool isOK = false;
	TCHAR* msg = _ExecuteJLinkScript(&isOK);

	if (!isOK) {
		LOGD(_T("[ ERROR ] EraseNordic52840BySWD Execute JLink Script Fail !!"));
		_DeleteJLinkFile();
		return false;
	}

	if (_tcsstr(msg, _T("Failed")) != nullptr) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Execute JLink Script Fail (Have Failed String)!!"));
		_DeleteJLinkFile();
		return false;
	}

	if (_tcsstr(msg, _T("Cannot connect to target")) != nullptr) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Execute JLink Script Fail (Can't connect to target)!!"));
		_DeleteJLinkFile();
		return false;
	}

	if (!_DeleteJLinkFile()) {
		LOGD(_T("[ ERROR ] EraseNordic52840BySWD Delete JLink Script Fail !!"));
		return false;
	}

	return true;
}

bool ResetNordic52840BySWD(void)
{
	if (!_JlinkDriverIsExist()) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Not Found J-Link Driver !!"));
		return false;
	}

	static char cmd[CMD_BUFFER] = { '\0' };
	if (!_CheckFileExists(_jlinkExePath)) {
		LOGD(_T("[ ERROR ] ResetNordic52840BySWD Jlink Execute Path Is Not Exists !!"));
		return false;
	}

	memset(cmd, L'\0', sizeof(char) * CMD_BUFFER);
	strcpy(cmd, "SelectInterface SWD\r\n");
	strcat(cmd, "Speed 4000\r\n");
	strcat(cmd, "Device NRF52840_XXAA\r\n");
	strcat(cmd, "R\r\n");
	strcat(cmd, "Exit\r\n");

	if (!_WriteJLinkFile(cmd)) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Write JLink Script Fail !!"));
		_DeleteJLinkFile();
		return false;
	}

	bool isOK = false;
	TCHAR* msg = _ExecuteJLinkScript(&isOK);

	if (!isOK) {
		LOGD(_T("[ ERROR ] ResetNordic52840BySWD Execute JLink Script Fail !!"));
		_DeleteJLinkFile();
		return false;
	}

	if (_tcsstr(msg, _T("Failed")) != nullptr) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Execute JLink Script Fail (Have Failed String)!!"));
		_DeleteJLinkFile();
		return false;
	}

	if (_tcsstr(msg, _T("Cannot connect to target")) != nullptr) {
		LOGD(_T("[ ERROR ] BurnNordic52840FWBySWD Execute JLink Script Fail (Can't connect to target)!!"));
		_DeleteJLinkFile();
		return false;
	}

	if (!_DeleteJLinkFile()) {
		LOGD(_T("[ ERROR ] ResetNordic52840BySWD Delete JLink Script Fail !!"));
		return false;
	}

	return true;
}

static bool _CheckFileExists(TCHAR* filePath) {
	DWORD dwAttrib = GetFileAttributes(filePath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static bool _WriteJLinkFile(char* content) {
	if (!_DeleteJLinkFile()) {
		return FALSE;
	}

	HANDLE hFile = CreateFile((TCHAR*)JLINK_FILE_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,          // open with exclusive access
		NULL,       // no security attributes
		CREATE_NEW, // creating a new temp file
		0,          // not overlapped index/O
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		TCHAR msg[4096] = { L'\0' };
		_swprintf(msg, _T("[ ERROR ] CreateFile Fail, Win32API ErrorCode : %d !!"), GetLastError());
		LOGD(msg);
		return FALSE;
	}

	DWORD totalSize = strlen(content);
	DWORD remainSize = totalSize;
	DWORD dwNumBytesWritten = 0;
	DWORD currentPos = 0;

	do {
		BOOL fSuccess = WriteFile(hFile,
			content + currentPos,
			totalSize - currentPos,
			&dwNumBytesWritten,
			NULL);  // sync operation.
		if (!fSuccess)
		{
			TCHAR msg[4096] = { L'\0' };
			_swprintf(msg, _T("[ ERROR ] WriteFile Fail, Win32API ErrorCode : %d !!"), GetLastError());
			LOGD(msg);
			CloseHandle(hFile);
			return FALSE;
		}
		currentPos += dwNumBytesWritten;
		remainSize -= currentPos;
	} while (remainSize > 0);

	FlushFileBuffers(hFile);
	CloseHandle(hFile);
	return TRUE;
}

static bool _DeleteJLinkFile(void) {
	if (!_CheckFileExists((TCHAR*)JLINK_FILE_NAME)) {
		return TRUE;
	}

	BOOL fSuccess = DeleteFile((TCHAR*)JLINK_FILE_NAME);

	if (!fSuccess)
	{
		// Handle the error.
		TCHAR msg[4096] = { L'\0' };
		_swprintf(msg, _T("[ ERROR ] _DeleteJLinkFile Fail, Win32API ErrorCode : %d !!"), GetLastError());
		LOGD(msg);
		return FALSE;
	}
	return TRUE;
}

static TCHAR* _ExecuteJLinkScript(bool* result) {
	*result = FALSE;
	static TCHAR returnMsg[15360] = { L'\0' };
	memset(returnMsg, L'\0', sizeof(TCHAR) * 15360);

	if (!_CheckFileExists((TCHAR*)JLINK_FILE_NAME)) {
		LOGD(_T("_ExecuteJLinkScript Script Not Found Fail!!"));
		return nullptr;
	}

	HANDLE hstdInRead, hstdInWrite;
	HANDLE hstdOutRead, hstdOutWrite;
	HANDLE hstdErrorRead, hstdErrorWrite;

	SECURITY_ATTRIBUTES sa;
	sa.bInheritHandle = true;
	sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof(sa);

	if (!CreatePipe(&hstdInRead, &hstdInWrite, &sa, 0)) {
		LOGD(_T("[ ERROR ] _ExecuteJLinkScript CreatePipe hstdInRead TFL_ERROR"));
		return nullptr;
	}

	if (!SetHandleInformation(hstdInRead, HANDLE_FLAG_INHERIT, 0)) {
		LOGD(_T("[ ERROR ] _ExecuteJLinkScript SetHandleInformation hstdInWrite TFL_ERROR"));
		return nullptr;
	}

	if (!CreatePipe(&hstdOutRead, &hstdOutWrite, &sa, 0)) {
		LOGD(_T("[ ERROR ] _ExecuteJLinkScript CreatePipe hstdOutRead TFL_ERROR"));
		return nullptr;
	}

	if (!SetHandleInformation(hstdOutRead, HANDLE_FLAG_INHERIT, 0)) {
		LOGD(_T("[ ERROR ] _ExecuteJLinkScript SetHandleInformation hstdOutRead TFL_ERROR"));
		return nullptr;
	}

	if (!CreatePipe(&hstdErrorRead, &hstdErrorWrite, &sa, 0)) {
		LOGD(_T("[ ERROR ] _ExecuteJLinkScript CreatePipe hstdErrorRead TFL_ERROR"));
		return nullptr;
	}

	if (!SetHandleInformation(hstdErrorRead, HANDLE_FLAG_INHERIT, 0)) {
		LOGD(_T("[ ERROR ] _ExecuteJLinkScript SetHandleInformation hstdErrorRead TFL_ERROR"));
		return nullptr;
	}

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = hstdErrorWrite;
	siStartInfo.hStdOutput = hstdOutWrite;
	siStartInfo.hStdInput = hstdInRead;
	siStartInfo.dwFlags = STARTF_USESTDHANDLES;
	siStartInfo.wShowWindow = SW_HIDE;

	TCHAR cmdLine[512] = { L'\0' };
	_swprintf(cmdLine, _T("C:\\Windows\\System32\\cmd.exe /C %s %s"), _jlinkExePath, JLINK_FILE_NAME);

	TCHAR cmdTmp[4096] = { L'\0' };
	_swprintf(cmdTmp, _T("Execute Cmd : %s"), cmdLine);
	LOGD(cmdTmp);

	//this->mLogFunc(cmdTmp);
	//_T("C:\\Windows\\System32\\cmd.exe /C dir"),
	if (!CreateProcess(NULL,
		cmdLine,
		NULL,
		NULL,
		TRUE,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		&siStartInfo,
		&piProcInfo)) {
		LOGD(_T("[ ERROR ] DUT EraseFlashFillZero CreateProcess TFL_ERROR"));
		return FALSE;
	}

	volatile static bool isRunProcessFinish = false;
	isRunProcessFinish = false;

	std::function<void(HANDLE&)> readIOCallback = [&](HANDLE& handle) {
		DWORD dwRead = 0;
		DWORD canRead = 0;
		BOOL isSuccess = FALSE;
		char buf[4096] = { '\0' };
		static TCHAR msg[8192] = { L'\0' };
		int loop = 10000;
		int curLoop = 1;
		do {
			PeekNamedPipe(handle, buf, sizeof(char) * 4095, &canRead, 0, 0);
			if (canRead != 0) {
				curLoop = 0;
				isSuccess = ReadFile(handle, buf, sizeof(char) * 4095, &dwRead, 0);
				if (isSuccess && dwRead > 0) {
					buf[dwRead] = '\0';
					TCHAR* cvt = _Char2TCHAR(buf);
					lstrcpy(msg, cvt);
					LOGD(msg);

					if (lstrlen(returnMsg) > 0) {
						lstrcat(returnMsg, cvt);
					}
					else {
						lstrcpy(returnMsg, cvt);
					}
				}
			}
			else {
				if (!isRunProcessFinish) {
					curLoop++;
					if (loop == curLoop)
						break;
				}
				else {
					break;
				}
			}
			Sleep(1);
		} while (true);
	};

	std::thread normalOutputThread = std::thread([&]() {
		readIOCallback(hstdOutRead);
		});

	DWORD exit;
	while (GetExitCodeProcess(piProcInfo.hProcess, &exit)) {
		if (exit == 0 || exit == 1) {
			isRunProcessFinish = true;
			break;
		}
	}

	normalOutputThread.join();
	exit = 0;
	GetExitCodeProcess(piProcInfo.hProcess, &exit);
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
	CloseHandle(hstdOutWrite);
	CloseHandle(hstdErrorWrite);
	CloseHandle(hstdInRead);
	CloseHandle(hstdInWrite);
	CloseHandle(hstdErrorRead);
	CloseHandle(hstdOutRead);

	if (exit == 1) {
		LOGD(_T("[ ERROR ] DUT EraseFlashFillZero Execute CMD Failed"));
		return nullptr;
	}
	Sleep(100);
	*result = TRUE;
	return returnMsg;
}
#else

static TCHAR _lastErrorMsg[MSG_BUFFER] = { L'\0' };
static HMODULE  _hModule = nullptr;
static double _CheckVoltage_mv = 2000;

#define CHECK_GetProcAddress(funcPointer,jlinkClass, jlinkName)   do{\
						funcPointer = (##jlinkClass)GetProcAddress(_hModule, jlinkName);\
						if(funcPointer == NULL) {\
							TCHAR *tCallbackName = _Char2TCHAR((char*)jlinkName);\
							memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);\
							_stprintf(_lastErrorMsg, _T("[ ERROR ] InitJlinkModule GetProcAddress %s Fail !!"), tCallbackName);\
							LOGD(_lastErrorMsg);\
							FreeJLinkModule();\
							return FALSE;\
						}\
					}while(0);\

static TCHAR* _GetJLinkInfo(void) {
	static TCHAR msg[4096] = { L'\0' };
	TCHAR tmp[1024] = { L'\0' };
	_stprintf(tmp, _T("DLL Version : % u\r\n"), _JLINKARM_GetDLLVersion());
	lstrcpy(msg, tmp);
	_stprintf(tmp, _T("HW Version : % u\r\n"), _JLINKARM_GetHardwareVersion());
	lstrcat(msg, tmp);
	_stprintf(tmp, _T("SerialNumber : % u\r\n"), _JLINKARM_GetSN());
	lstrcat(msg, tmp);
	return msg;
}

static bool _ShowJLinkInfo(const TCHAR* deviceName) {
	TCHAR tmp[1024] = { L'\0' };
	_stprintf(tmp, _T("[ JLINK INFO ] JLink Select Device: %s, CPU ID: 0x%08X"), deviceName, _JLINKARM_GetId());
	LOGD(tmp);

	char fwstr[0x100] = { '\0' };
	_JLINKARM_GetFirmwareString(fwstr, 0x100);
	if (fwstr[0] == 0)
	{
		memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);
		_stprintf(_lastErrorMsg, _T("[ JLINK ERR ] Can't Read JLink FW Version !!"));
		LOGD(_lastErrorMsg);
		return false;
	}

	TCHAR* tmpTCHAR = _Char2TCHAR(fwstr);
	memset(tmp, _T('\0'), sizeof(TCHAR) * 1024);
	_stprintf(tmp, _T("[ JLINK INFO ] JLink FW: %s"), tmpTCHAR);
	LOGD(tmp);

	unsigned hwver = _JLINKARM_GetHardwareVersion();
	if (hwver == 0) {
		memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);
		_stprintf(_lastErrorMsg, _T("[ JLINK ERR ] Can't Read JLink HW Version !!"));
		LOGD(_lastErrorMsg);
		return false;
	}

	memset(tmp, _T('\0'), sizeof(TCHAR) * 1024);
	_stprintf(tmp, _T("[ JLINK INFO ] JLink HW: %d.%d.%d"), hwver / 10000, (hwver % 10000) / 100, hwver % 100);
	LOGD(tmp);

	unsigned dllver = _JLINKARM_GetDLLVersion();
	if (dllver == 0) {
		memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);
		_stprintf(_lastErrorMsg, _T("[ JLINK ERR ] Can't Read JLink DLL Version !!"));
		LOGD(_lastErrorMsg);
		return false;
	}

	memset(tmp, _T('\0'), sizeof(TCHAR) * 1024);
	_stprintf(tmp, _T("[ JLINK INFO ] JLink DLL: %d.%d.%d"), dllver / 10000, (dllver % 10000) / 100, dllver % 100);
	LOGD(tmp);

	char oemstr[0x100] = { '\0' };
	_JLINKARM_GetOEMString(oemstr);
	if (strlen(oemstr) > 0) {
		tmpTCHAR = _Char2TCHAR(oemstr);
		memset(tmp, _T('\0'), sizeof(TCHAR) * 1024);
		_stprintf(tmp, _T("[ JLINK INFO ] JLink OEM: %s"), tmpTCHAR);
		LOGD(tmp);
	}

	unsigned caps = _JLINKARM_GetEmuCaps();
	memset(tmp, _T('\0'), sizeof(TCHAR) * 1024);
	_stprintf(tmp, _T("[ JLINK INFO ] JLink Capabilities: %X"), caps);
	LOGD(tmp);

	return true;
}

static bool _CheckJLinkHWVoltage(void) {
	TCHAR tmp[1024] = { L'\0' };
	HWSTATUS hwsts;
	memset(&hwsts, 0, sizeof(HWSTATUS));
	if (_JLINKARM_GetHWStatus(&hwsts) == 1)
	{
		memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);
		_stprintf(_lastErrorMsg, _T("[ JLINK ERR ] Can't Read JLink HW Voltage Status !!"));
		LOGD(_lastErrorMsg);
		return false;
	}

#if 0
	if (hwsts.tck == 0) {
		memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);
		_stprintf(_lastErrorMsg, _T("[ JLINK ERR ] JLink HW Voltage TCK Low Fail !!"));
		LOGD(_lastErrorMsg);
		return false;
	}

	if (hwsts.trst == 0) {
		memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);
		_stprintf(_lastErrorMsg, _T("[ JLINK ERR ] JLink HW Voltage TRST Low Fail !!"));
		LOGD(_lastErrorMsg);
		return false;
	}

#else

	memset(tmp, L'\0', sizeof(TCHAR) * 1024);
	_stprintf(tmp, _T("[ JLINK INFO ] JLink HW TCK: %d"), hwsts.tck);
	LOGD(tmp);

	memset(tmp, L'\0', sizeof(TCHAR) * 1024);
	_stprintf(tmp, _T("[ JLINK INFO ] JLink HW TDI: %d"), hwsts.tdi);
	LOGD(tmp);

	memset(tmp, L'\0', sizeof(TCHAR) * 1024);
	_stprintf(tmp, _T("[ JLINK INFO ] JLink HW TMS: %d"), hwsts.tms);
	LOGD(tmp);

	memset(tmp, L'\0', sizeof(TCHAR) * 1024);
	_stprintf(tmp, _T("[ JLINK INFO ] JLink HW TRES: %d"), hwsts.tres);
	LOGD(tmp);

	memset(tmp, L'\0', sizeof(TCHAR) * 1024);
	_stprintf(tmp, _T("[ JLINK INFO ] JLink HW TRST: %d"), hwsts.trst);
	LOGD(tmp);

#endif

	if (hwsts.vtarget < _CheckVoltage_mv)
	{
		memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);
		_stprintf(_lastErrorMsg, _T("[ JLINK ERR ] JLink HW Voltage Too Low Fail, VTarget : %d.%dV !! ( < %fV)"), hwsts.vtarget / 1000, hwsts.vtarget % 1000, _CheckVoltage_mv / 1000);
		LOGD(_lastErrorMsg);
		return false;
	}

	memset(tmp, _T('\0'), sizeof(TCHAR) * 1024);
	_stprintf(tmp, _T("[ JLINK INFO ] VTarget = %d.%dV"), hwsts.vtarget / 1000, hwsts.vtarget % 1000);
	LOGD(tmp);

	return true;
}

static void _ErrorOutHandler(const char* sError) {
	static TCHAR msg[4096] = { L'\0' };
	memset(msg, L'\0', sizeof(TCHAR) * 4096);
	TCHAR* tError = _Char2TCHAR((char*)sError);
	_swprintf(msg, _T("[ ERROR TOOTIP ] %s"), tError);
	LOGD(msg);
}

static void _WarnOutHandler(const char* sError) {
	static TCHAR msg[4096] = { L'\0' };
	memset(msg, L'\0', sizeof(TCHAR) * 4096);
	TCHAR* tWarn = _Char2TCHAR((char*)sError);
	_swprintf(msg, _T("[ WARNNING TOOTIP ] %s"), tWarn);
	LOGD(msg);
}

// Initial Fuction Pointer For DLL
bool InitJLinkModule(void) {
	FreeJLinkModule();
	_hModule = LoadLibrary(_T("JLinkARM.dll"));
	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] InitJlinkModule Can't LoadLibrary JLinkARM.dll !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	CHECK_GetProcAddress(_JLINKARM_GetDLLVersion, JLINKARM_GetDLLVersion, "JLINKARM_GetDLLVersion");
	CHECK_GetProcAddress(_JLINKARM_GetHardwareVersion, JLINKARM_GetHardwareVersion, "JLINKARM_GetHardwareVersion");
	CHECK_GetProcAddress(_JLINKARM_GetFirmwareString, JLINKARM_GetFirmwareString, "JLINKARM_GetFirmwareString");
	CHECK_GetProcAddress(_JLINKARM_GetSN, JLINKARM_GetSN, "JLINKARM_GetSN");
	CHECK_GetProcAddress(_JLINKARM_ExecCommand, JLINKARM_ExecCommand, "JLINKARM_ExecCommand");
	CHECK_GetProcAddress(_JLINKARM_TIF_Select, JLINKARM_TIF_Select, "JLINKARM_TIF_Select");
	CHECK_GetProcAddress(_JLINKARM_SetSpeed, JLINKARM_SetSpeed, "JLINKARM_SetSpeed");
	CHECK_GetProcAddress(_JLINKARM_GetSpeed, JLINKARM_GetSpeed, "JLINKARM_GetSpeed");
	CHECK_GetProcAddress(_JLINKARM_GetId, JLINKARM_GetId, "JLINKARM_GetId");
	CHECK_GetProcAddress(_JLINKARM_GetDeviceFamily, JLINKARM_GetDeviceFamily, "JLINKARM_GetDeviceFamily");
	CHECK_GetProcAddress(_JLINKARM_Open, JLINKARM_Open, "JLINKARM_Open");
	CHECK_GetProcAddress(_JLINKARM_Close, JLINKARM_Close, "JLINKARM_Close");
	CHECK_GetProcAddress(_JLINKARM_IsOpen, JLINKARM_IsOpen, "JLINKARM_IsOpen");
	CHECK_GetProcAddress(_JLINKARM_Connect, JLINKARM_Connect, "JLINKARM_Connect");
	CHECK_GetProcAddress(_JLINKARM_IsConnected, JLINKARM_IsConnected, "JLINKARM_IsConnected");
	CHECK_GetProcAddress(_JLINKARM_Halt, JLINKARM_Halt, "JLINKARM_Halt");
	CHECK_GetProcAddress(_JLINKARM_IsHalted, JLINKARM_IsHalted, "JLINKARM_IsHalted");
	CHECK_GetProcAddress(_JLINKARM_SetResetType, JLINKARM_SetResetType, "JLINKARM_SetResetType");
	CHECK_GetProcAddress(_JLINKARM_Reset, JLINKARM_Reset, "JLINKARM_Reset");
	CHECK_GetProcAddress(_JLINKARM_Go, JLINKARM_Go, "JLINKARM_Go");
	CHECK_GetProcAddress(_JLINKARM_GoIntDis, JLINKARM_GoIntDis, "JLINKARM_GoIntDis");
	CHECK_GetProcAddress(_JLINKARM_ReadReg, JLINKARM_ReadReg, "JLINKARM_ReadReg");
	CHECK_GetProcAddress(_JLINKARM_WriteReg, JLINKARM_WriteReg, "JLINKARM_WriteReg");
	CHECK_GetProcAddress(_JLINKARM_ReadMem, JLINKARM_ReadMem, "JLINKARM_ReadMem");
	CHECK_GetProcAddress(_JLINKARM_WriteMem, JLINKARM_WriteMem, "JLINKARM_WriteMem");
	CHECK_GetProcAddress(_JLINKARM_WriteU8, JLINKARM_WriteU8, "JLINKARM_WriteU8");
	CHECK_GetProcAddress(_JLINKARM_WriteU16, JLINKARM_WriteU16, "JLINKARM_WriteU16");
	CHECK_GetProcAddress(_JLINKARM_WriteU32, JLINKARM_WriteU32, "JLINKARM_WriteU32");
	CHECK_GetProcAddress(_JLINK_EraseChip, JLINK_EraseChip, "JLINK_EraseChip");
	CHECK_GetProcAddress(_JLINK_DownloadFile, JLINK_DownloadFile, "JLINK_DownloadFile");
	CHECK_GetProcAddress(_JLINKARM_BeginDownload, JLINKARM_BeginDownload, "JLINKARM_BeginDownload");
	CHECK_GetProcAddress(_JLINKARM_EndDownload, JLINKARM_EndDownload, "JLINKARM_EndDownload");
	CHECK_GetProcAddress(_JLINKARM_HasError, JLINKARM_HasError, "JLINKARM_HasError");
	CHECK_GetProcAddress(_JLINKARM_Go, JLINKARM_Go, "JLINKARM_Go");
	CHECK_GetProcAddress(_JLINKARM_ClrError, JLINKARM_ClrError, "JLINKARM_ClrError");
	CHECK_GetProcAddress(_JLINKARM_SetWarnOutHandler, JLINKARM_SetWarnOutHandler, "JLINKARM_SetWarnOutHandler");
	CHECK_GetProcAddress(_JLINKARM_SetErrorOutHandler, JLINKARM_SetErrorOutHandler, "JLINKARM_SetErrorOutHandler");
	CHECK_GetProcAddress(_JLINKARM_OpenEx, JLINKARM_OpenEx, "JLINKARM_OpenEx");
	CHECK_GetProcAddress(_JLINKARM_EnableLog, JLINKARM_EnableLog, "JLINKARM_EnableLog");
	CHECK_GetProcAddress(_JLINKARM_SetLogFile, JLINKARM_SetLogFile, "JLINKARM_SetLogFile");
	CHECK_GetProcAddress(_JLINKARM_GetOEMString, JLINKARM_GetOEMString, "JLINKARM_GetOEMString");
	CHECK_GetProcAddress(_JLINKARM_GetEmuCaps, JLINKARM_GetEmuCaps, "JLINKARM_GetEmuCaps");
	CHECK_GetProcAddress(_JLINKARM_GetHWStatus, JLINKARM_GetHWStatus, "JLINKARM_GetHWStatus");

	CHECK_GetProcAddress(_JLINKARM_BeginDownload, JLINKARM_BeginDownload, "JLINKARM_BeginDownload");
	CHECK_GetProcAddress(_JLINKARM_GetHWStatus, JLINKARM_GetHWStatus, "JLINKARM_GetHWStatus");
	CHECK_GetProcAddress(_JLINKARM_GetHWStatus, JLINKARM_GetHWStatus, "JLINKARM_GetHWStatus");
	CHECK_GetProcAddress(_JLINKARM_ReadMemU32, JLINKARM_ReadMemU32, "JLINKARM_ReadMemU32");

#if 0

	if (!_DeleteJLinkFile()) {
		memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);
		_stprintf(_lastErrorMsg, _T("[ ERROR ] InitJlinkModule DeleteJLinkFile %s Fail !!"), JLINK_FILE_NAME);
		LOGD(_lastErrorMsg);
		FreeJLinkModule();
		return FALSE;
	}

	char* jlinkCharStr = _TCHAR2Char((TCHAR*)JLINK_FILE_NAME);
	_JLINKARM_SetLogFile(jlinkCharStr);

#endif

	return TRUE;
}

bool FreeJLinkModule(void)
{
	if (_hModule != nullptr) {
		FreeLibrary(_hModule);
		_hModule = nullptr;
	}
	return TRUE;
}

TCHAR* GetJLinkInfo(bool* result) {
	*result = FALSE;
	if (!_JlinkDriverIsExist()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] GetJLinkInfo JLink Driver Is Not Connect PC !!"));
		LOGD(_lastErrorMsg);
		return nullptr;
	}

	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] GetJLinkInfo JLink DLL Is Not Initial !!"));
		LOGD(_lastErrorMsg);
		return nullptr;
	}

	*result = TRUE;
	return _GetJLinkInfo();
}

void SetVoltageCheckThresholdByMV(double voltage_mv)
{
	_CheckVoltage_mv = voltage_mv;
}

static void  _OpenLogHandler(const char* sLog) {
	static TCHAR msg[4096] = { L'\0' };
	memset(msg, L'\0', sizeof(TCHAR) * 4096);
	TCHAR* tWarn = _Char2TCHAR((char*)sLog);
	_swprintf(msg, _T("[ OPEN INFO ] %s"), tWarn);
	LOGD(msg);
}

static void  _OpenErrorOutHandler(const char* sError) {
	static TCHAR msg[4096] = { L'\0' };
	memset(msg, L'\0', sizeof(TCHAR) * 4096);
	TCHAR* tWarn = _Char2TCHAR((char*)sError);
	_swprintf(msg, _T("[ OPEN ERR ] %s"), tWarn);
	LOGD(msg);
}

bool BurnNordic52840FWBySWD(TCHAR* fwPos, DWORD flashOffset) {
	_JLINKARM_ClrError();
	memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);

	if (!_JlinkDriverIsExist()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Driver Is Not Connect PC !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink DLL Is Not Initial !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	if (!_CheckFileExists(fwPos)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD Input File Pos Is Not Found !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_SetWarnOutHandler(_WarnOutHandler);
	_JLINKARM_EnableLog(_OpenLogHandler);

	if (_JLINKARM_OpenEx(_OpenLogHandler, _OpenErrorOutHandler)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Open Fail : "));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_EnableLog(_OpenLogHandler);
	_JLINKARM_SetErrorOutHandler(_ErrorOutHandler);

	/*if (_JLINK_Open() != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Open Fail !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}*/

	char acOut[256] = { '\0' };
	int r = _JLINKARM_ExecCommand((char*)"DEVICE = NRF52840_XXAA", acOut, sizeof(acOut));
	if (acOut[0] != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD Set DEVICE = NRF52840_XXAA Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_TIF_Select(JLINK_TIF_SWD);
	// default kHz, target 4MHz
	_JLINKARM_SetSpeed(4000);

	if (!_ShowJLinkInfo(_T("NRF52840_XXAA"))) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_CheckJLinkHWVoltage()) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Connect() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Connect Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_IsHalted() != 0)
	{
		_JLINKARM_Halt();
	}

	if (_JLINK_EraseChip() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Erase Chip Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	char* cFWPos = _TCHAR2Char(fwPos);
	r = _JLINK_DownloadFile(cFWPos, flashOffset);

	if (r < 0) {
		if (r == JLINK_ERR_FLASH_PROG_COMPARE_FAILED) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Download JLINK_ERR_FLASH_PROG_COMPARE_FAILED !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}
		else if (r == JLINK_ERR_FLASH_PROG_PROGRAM_FAILED) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Download JLINK_ERR_FLASH_PROG_PROGRAM_FAILED !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}
		else if (r == JLINK_ERR_FLASH_PROG_VERIFY_FAILED) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Download JLINK_ERR_FLASH_PROG_VERIFY_FAILED !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}
		else if (r == JLINK_ERR_OPEN_FILE_FAILED) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Download JLINK_ERR_OPEN_FILE_FAILED !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}
		else if (r == JLINK_ERR_UNKNOWN_FILE_FORMAT) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Download JLINK_ERR_UNKNOWN_FILE_FORMAT !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}
		else if (r == JLINK_ERR_WRITE_TARGET_MEMORY_FAILED) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Download JLINK_ERR_WRITE_TARGET_MEMORY_FAILED !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}
		else {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Download Fail !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_Close();

	LOGD(_T("[ JLINK INFO ] Burn Image OK !!"));

	return TRUE;
}

bool EraseNordic52840BySWD(void) {
	_JLINKARM_ClrError();
	memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);

	if (!_JlinkDriverIsExist()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWD JLink Driver Is Not Connect PC !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWD JLink DLL Is Not Initial !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_SetWarnOutHandler(_WarnOutHandler);
	_JLINKARM_EnableLog(_OpenLogHandler);

	if (_JLINKARM_OpenEx(_OpenLogHandler, _OpenErrorOutHandler)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Open Fail : "));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_EnableLog(_OpenLogHandler);
	_JLINKARM_SetErrorOutHandler(_ErrorOutHandler);

	char acOut[256] = { '\0' };
	int r = _JLINKARM_ExecCommand((char*)"DEVICE = NRF52840_XXAA", acOut, sizeof(acOut));
	if (acOut[0] != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWD Set DEVICE = NRF52840_XXAA Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_TIF_Select(JLINK_TIF_SWD);
	// default kHz, target 4MHz
	_JLINKARM_SetSpeed(4000);

	if (!_ShowJLinkInfo(_T("NRF52840_XXAA"))) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_CheckJLinkHWVoltage()) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Connect() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWD JLink Connect Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_IsHalted() != 0)
	{
		_JLINKARM_Halt();
	}

	if (_JLINK_EraseChip() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWD JLink Erase Chip Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWD JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_Close();

	LOGD(_T("[ JLINK INFO ] Erase All Chip OK !!"));

	return TRUE;
}

#if 1
// https://www.twblogs.net/a/5b894d5f2b71775d1ce115ea
// https://infocenter.nordicsemi.com/index.jsp?topic=%2Fps_nrf52840%2Fnvmc.html&anchor=register.ERASEPAGE
bool ErasePageNordic52840BySWD(uint32_t pageAddr)
{
	_JLINKARM_ClrError();
	memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);

	if (!_JlinkDriverIsExist()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD JLink Driver Is Not Connect PC !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD JLink DLL Is Not Initial !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_SetWarnOutHandler(_WarnOutHandler);
	_JLINKARM_EnableLog(_OpenLogHandler);

	if (_JLINKARM_OpenEx(_OpenLogHandler, _OpenErrorOutHandler)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD JLink Open Fail : "));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_EnableLog(_OpenLogHandler);
	_JLINKARM_SetErrorOutHandler(_ErrorOutHandler);

	char acOut[256] = { '\0' };
	int r = _JLINKARM_ExecCommand((char*)"DEVICE = NRF52840_XXAA", acOut, sizeof(acOut));
	if (acOut[0] != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD Set DEVICE = NRF52840_XXAA Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_TIF_Select(JLINK_TIF_SWD);
	// default kHz, target 4MHz
	_JLINKARM_SetSpeed(4000);

	if (!_ShowJLinkInfo(_T("NRF52840_XXAA"))) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_CheckJLinkHWVoltage()) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Connect() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD JLink Connect Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	const uint32_t NORDIC_CONFIG_REGISTER = 0x4001E504; // REN = 0 (Read only access), WEN = 1 (Write enabled), Een = 2 (Erase enabled)
	const uint32_t NORDIC_CONFIG_REGISTER_REN = 0x0000;
	const uint32_t NORDIC_CONFIG_REGISTER_WEN = 0x0001;
	const uint32_t NORDIC_CONFIG_REGISTER_Een = 0x0002;

	const uint32_t NORDIC_ERASEALL_REGISTER = 0x4001E50C; // NoOperation = 0 (No operation), Erase = 1 (Start chip erase)
	const uint32_t NORDIC_ERASEALL_REGISTER_NoOperation = 0x0000;
	const uint32_t NORDIC_ERASEALL_REGISTER_Erase = 0x0001;

	const uint32_t NORDIC_ERASEPAGE_REGISTER = 0x4001E508;

	const uint32_t NORDIC_READY_REGISTER = 0x4001E400; // Busy = 0 (NVMC is busy), Ready = 1 (NVMC is ready)
	uint32_t NORDIC_READY_REGISTER_Busy = 0x0000;
	uint32_t NORDIC_READY_REGISTER_Ready = 0x0001;

	// Erase Enable . . .
	int jlinkR = _JLINKARM_WriteU32(NORDIC_CONFIG_REGISTER, NORDIC_CONFIG_REGISTER_Een);

	if (jlinkR != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD _JLINKARM_WriteU32 NORDIC_CONFIG_REGISTER NORDIC_CONFIG_REGISTER_Een Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// Wait Register Ready ...
	do {
		uint32_t pData = 0;
		uint8_t pStatus = 0;
		jlinkR = _JLINKARM_ReadMemU32(NORDIC_READY_REGISTER, 0x0001, &pData, &pStatus);

		if (jlinkR < 0) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD _JLINKARM_ReadMemU32 NORDIC_READY_REGISTER Fail !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}

		if (pData == NORDIC_READY_REGISTER_Ready) {
			break;
		}
	} while (true);

	// Erase ALL  ...
	/*jlinkR = _JLINKARM_WriteU32(NORDIC_ERASEALL_REGISTER, NORDIC_ERASEALL_REGISTER_Erase);

	if (jlinkR != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePartitionNordic52840BySWD _JLINKARM_WriteU32 NORDIC_ERASEALL_REGISTER NORDIC_ERASEALL_REGISTER_Erase Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}*/

	// Erase Page Register  ...
	jlinkR = _JLINKARM_WriteU32(NORDIC_ERASEPAGE_REGISTER, pageAddr);
	if (jlinkR != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD _JLINKARM_WriteU32 NORDIC_ERASEPAGE_REGISTER Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// Wait Register Ready ...
	do {
		uint32_t pData = 0;
		uint8_t pStatus = 0;
		jlinkR = _JLINKARM_ReadMemU32(NORDIC_READY_REGISTER, 0x0001, &pData, &pStatus);

		if (jlinkR < 0) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD _JLINKARM_ReadMemU32 NORDIC_READY_REGISTER Fail !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}

		if (pData == NORDIC_READY_REGISTER_Ready) {
			break;
		}
	} while (true);

	// Erase Disable . . .
	jlinkR = _JLINKARM_WriteU32(NORDIC_CONFIG_REGISTER, NORDIC_CONFIG_REGISTER_REN);

	if (jlinkR != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD _JLINKARM_WriteU32 NORDIC_CONFIG_REGISTER NORDIC_CONFIG_REGISTER_REN Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ErasePageNordic52840BySWD JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_Close();

	LOGD(_T("[ JLINK INFO ] Erase Page Chip OK !!"));

	return TRUE;
}

#endif

// ---- Register ----

static bool _EraseAllNordic52840ByRegister(void) {
	const uint32_t NORDIC_CONFIG_REGISTER = 0x4001E504; // REN = 0 (Read only access), WEN = 1 (Write enabled), Een = 2 (Erase enabled)
	const uint32_t NORDIC_CONFIG_REGISTER_REN = 0x0000;
	const uint32_t NORDIC_CONFIG_REGISTER_WEN = 0x0001;
	const uint32_t NORDIC_CONFIG_REGISTER_Een = 0x0002;

	const uint32_t NORDIC_ERASEALL_REGISTER = 0x4001E50C; // NoOperation = 0 (No operation), Erase = 1 (Start chip erase)
	const uint32_t NORDIC_ERASEALL_REGISTER_NoOperation = 0x0000;
	const uint32_t NORDIC_ERASEALL_REGISTER_Erase = 0x0001;

	const uint32_t NORDIC_ERASEPAGE_REGISTER = 0x4001E508;

	const uint32_t NORDIC_READY_REGISTER = 0x4001E400; // Busy = 0 (NVMC is busy), Ready = 1 (NVMC is ready)
	uint32_t NORDIC_READY_REGISTER_Busy = 0x0000;
	uint32_t NORDIC_READY_REGISTER_Ready = 0x0001;

	// Erase Enable . . .
	int jlinkR = _JLINKARM_WriteU32(NORDIC_CONFIG_REGISTER, NORDIC_CONFIG_REGISTER_Een);

	if (jlinkR != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] _EraseAllNordic52840ByRegister _JLINKARM_WriteU32 NORDIC_CONFIG_REGISTER NORDIC_CONFIG_REGISTER_Een Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// Wait Register Ready ...
	do {
		uint32_t pData = 0;
		uint8_t pStatus = 0;
		jlinkR = _JLINKARM_ReadMemU32(NORDIC_READY_REGISTER, 0x0001, &pData, &pStatus);

		if (jlinkR < 0) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] _EraseAllNordic52840ByRegister _JLINKARM_ReadMemU32 NORDIC_READY_REGISTER Fail !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}

		if (pData == NORDIC_READY_REGISTER_Ready) {
			break;
		}
	} while (true);

	jlinkR = _JLINKARM_WriteU32(NORDIC_ERASEALL_REGISTER, NORDIC_ERASEALL_REGISTER_Erase);
	if (jlinkR != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] _EraseAllNordic52840ByRegister _JLINKARM_WriteU32 NORDIC_ERASEALL_REGISTER Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// Wait Register Ready ...
	do {
		uint32_t pData = 0;
		uint8_t pStatus = 0;
		jlinkR = _JLINKARM_ReadMemU32(NORDIC_READY_REGISTER, 0x0001, &pData, &pStatus);

		if (jlinkR < 0) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] _EraseAllNordic52840ByRegister _JLINKARM_ReadMemU32 NORDIC_READY_REGISTER Fail !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}

		if (pData == NORDIC_READY_REGISTER_Ready) {
			break;
		}
	} while (true);

	// Erase Disable . . .
	jlinkR = _JLINKARM_WriteU32(NORDIC_CONFIG_REGISTER, NORDIC_CONFIG_REGISTER_REN);

	if (jlinkR != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] _EraseAllNordic52840ByRegister _JLINKARM_WriteU32 NORDIC_CONFIG_REGISTER NORDIC_CONFIG_REGISTER_REN Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] _EraseAllNordic52840ByRegister JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	return TRUE;
}

static bool _ErasePartialNordic52840ByRegister(uint32_t registerPos) {
	const uint32_t NORDIC_CONFIG_REGISTER = 0x4001E504; // REN = 0 (Read only access), WEN = 1 (Write enabled), Een = 2 (Erase enabled)
	const uint32_t NORDIC_CONFIG_REGISTER_REN = 0x0000;
	const uint32_t NORDIC_CONFIG_REGISTER_WEN = 0x0001;
	const uint32_t NORDIC_CONFIG_REGISTER_Een = 0x0002;

	const uint32_t NORDIC_ERASEPAGEPARTIAL_REGISTER = 0x4001E518;

	const uint32_t NORDIC_READY_REGISTER = 0x4001E400; // Busy = 0 (NVMC is busy), Ready = 1 (NVMC is ready)
	uint32_t NORDIC_READY_REGISTER_Busy = 0x0000;
	uint32_t NORDIC_READY_REGISTER_Ready = 0x0001;

	// Erase Enable . . .
	int jlinkR = _JLINKARM_WriteU32(NORDIC_CONFIG_REGISTER, NORDIC_CONFIG_REGISTER_Een);

	if (jlinkR != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] _ErasePartialNordic52840ByRegister _JLINKARM_WriteU32 NORDIC_CONFIG_REGISTER NORDIC_CONFIG_REGISTER_Een Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// Wait Register Ready ...
	do {
		uint32_t pData = 0;
		uint8_t pStatus = 0;
		jlinkR = _JLINKARM_ReadMemU32(NORDIC_READY_REGISTER, 0x0001, &pData, &pStatus);

		if (jlinkR < 0) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] _ErasePartialNordic52840ByRegister _JLINKARM_ReadMemU32 NORDIC_READY_REGISTER Fail !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}

		if (pData == NORDIC_READY_REGISTER_Ready) {
			break;
		}
	} while (true);

	jlinkR = _JLINKARM_WriteU32(NORDIC_ERASEPAGEPARTIAL_REGISTER, registerPos);
	if (jlinkR != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] _ErasePartialNordic52840ByRegister _JLINKARM_WriteU32 NORDIC_ERASEPAGEPARTIAL_REGISTER Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// Wait Register Ready ...
	do {
		uint32_t pData = 0;
		uint8_t pStatus = 0;
		jlinkR = _JLINKARM_ReadMemU32(NORDIC_READY_REGISTER, 0x0001, &pData, &pStatus);

		if (jlinkR < 0) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] _ErasePartialNordic52840ByRegister _JLINKARM_ReadMemU32 NORDIC_READY_REGISTER Fail !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			return FALSE;
		}

		if (pData == NORDIC_READY_REGISTER_Ready) {
			break;
		}
	} while (true);

	// Erase Disable . . .
	jlinkR = _JLINKARM_WriteU32(NORDIC_CONFIG_REGISTER, NORDIC_CONFIG_REGISTER_REN);

	if (jlinkR != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] _ErasePartialNordic52840ByRegister _JLINKARM_WriteU32 NORDIC_CONFIG_REGISTER NORDIC_CONFIG_REGISTER_REN Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] _ErasePartialNordic52840ByRegister JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	return TRUE;
}

bool EraseNordic52840BySWDRegister(void)
{
	_JLINKARM_ClrError();
	memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);

	if (!_JlinkDriverIsExist()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister JLink Driver Is Not Connect PC !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister JLink DLL Is Not Initial !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_SetWarnOutHandler(_WarnOutHandler);
	_JLINKARM_EnableLog(_OpenLogHandler);

	if (_JLINKARM_OpenEx(_OpenLogHandler, _OpenErrorOutHandler)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister JLink Open Fail : "));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_EnableLog(_OpenLogHandler);
	_JLINKARM_SetErrorOutHandler(_ErrorOutHandler);

	char acOut[256] = { '\0' };
	int r = _JLINKARM_ExecCommand((char*)"DEVICE = NRF52840_XXAA", acOut, sizeof(acOut));
	if (acOut[0] != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister Set DEVICE = NRF52840_XXAA Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_TIF_Select(JLINK_TIF_SWD);
	// default kHz, target 4MHz
	_JLINKARM_SetSpeed(4000);

	if (!_ShowJLinkInfo(_T("NRF52840_XXAA"))) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_CheckJLinkHWVoltage()) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Connect() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister JLink Connect Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_EraseAllNordic52840ByRegister()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister _EraseAllNordic52840ByRegister Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_Close();

	LOGD(_T("[ JLINK INFO ] Erase All Chip OK !!"));

	return TRUE;
}

bool WriteDataToUICR_Nordic52840BySWDRegister(unsigned char* datas, uint32_t dataLen, uint32_t UICRStartPos)
{
	// Max Write 12 Bytes
	const uint32_t UICR_CUSTOMER_12BYTES_LIMIT_ADDR = 0x000000F4;
	const uint32_t UICR_BASE_ADDR = 0x10001000;
	const uint32_t UICR_ADDR_OFFSET = 4;
	const uint32_t UICR_MAX_LEN = 12;

	if (UICRStartPos > UICR_CUSTOMER_12BYTES_LIMIT_ADDR || dataLen != UICR_MAX_LEN) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister UICR Start Pos Is Over!! (Write 12Bytes)"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_ClrError();
	memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);

	if (!_JlinkDriverIsExist()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister JLink Driver Is Not Connect PC !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister JLink DLL Is Not Initial !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_SetWarnOutHandler(_WarnOutHandler);
	_JLINKARM_EnableLog(_OpenLogHandler);

	if (_JLINKARM_OpenEx(_OpenLogHandler, _OpenErrorOutHandler)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister JLink Open Fail : "));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_EnableLog(_OpenLogHandler);
	_JLINKARM_SetErrorOutHandler(_ErrorOutHandler);

	char acOut[256] = { '\0' };
	int r = _JLINKARM_ExecCommand((char*)"DEVICE = NRF52840_XXAA", acOut, sizeof(acOut));
	if (acOut[0] != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister Set DEVICE = NRF52840_XXAA Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_TIF_Select(JLINK_TIF_SWD);
	// default kHz, target 4MHz
	_JLINKARM_SetSpeed(4000);

	if (!_ShowJLinkInfo(_T("NRF52840_XXAA"))) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_CheckJLinkHWVoltage()) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Connect() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister JLink Connect Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// ---- erase uicr customer addr
	uint32_t firstPos = UICR_BASE_ADDR | UICRStartPos;
	uint32_t secondPos = firstPos + UICR_ADDR_OFFSET;
	uint32_t thirdPos = secondPos + UICR_ADDR_OFFSET;

	// ---- delete 12 bytes addr data
	if (!_ErasePartialNordic52840ByRegister(firstPos)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister _ErasePartialNordic52840ByRegister first Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_ErasePartialNordic52840ByRegister(secondPos)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister _ErasePartialNordic52840ByRegister second Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_ErasePartialNordic52840ByRegister(thirdPos)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister _ErasePartialNordic52840ByRegister third Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// ---- write 12 bytes
	uint32_t writeFirstBytes = datas[0] << 24 | datas[1] << 16 | datas[2] << 8 | datas[3];
	uint32_t writeSecondBytes = datas[4] << 24 | datas[5] << 16 | datas[6] << 8 | datas[7];
	uint32_t writeThirdBytes = datas[8] << 24 | datas[9] << 16 | datas[10] << 8 | datas[11];

	int result = _JLINKARM_WriteMem(firstPos, UICR_ADDR_OFFSET, &writeFirstBytes);
	if (result < 0) {
		_swprintf(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister JLink MEM first Fail !! Code : %d"), result);
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	result = _JLINKARM_WriteMem(secondPos, UICR_ADDR_OFFSET, &writeSecondBytes);
	if (result < 0) {
		_swprintf(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister JLink MEM second Fail !! Code : %d"), result);
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	result = _JLINKARM_WriteMem(thirdPos, UICR_ADDR_OFFSET, &writeThirdBytes);
	if (result < 0) {
		_swprintf(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister JLink MEM third Fail !! Code : %d"), result);
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] WriteDataToUICR_Nordic52840BySWDRegister JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_Close();

	LOGD(_T("[ JLINK INFO ] Write UICR To Chip OK !!"));
	return TRUE;
}

bool ReadDataFromUICR_Nordic52840BySWDRegister(struct UICR_READ_DATA* outputData, uint32_t UICRStartPos)
{
	const uint32_t UICR_CUSTOMER_12BYTES_LIMIT_ADDR = 0x000000F4;
	const uint32_t UICR_BASE_ADDR = 0x10001000;
	const uint32_t UICR_ADDR_OFFSET = 4;
	const uint32_t UICR_MAX_LEN = 12;

	if (UICRStartPos > UICR_CUSTOMER_12BYTES_LIMIT_ADDR) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadDataFromUICR_Nordic52840BySWDRegister UICR Start Pos Is Over!! (Write 12Bytes)"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_ClrError();
	memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);

	if (!_JlinkDriverIsExist()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadDataFromUICR_Nordic52840BySWDRegister JLink Driver Is Not Connect PC !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadDataFromUICR_Nordic52840BySWDRegister JLink DLL Is Not Initial !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_SetWarnOutHandler(_WarnOutHandler);
	_JLINKARM_EnableLog(_OpenLogHandler);

	if (_JLINKARM_OpenEx(_OpenLogHandler, _OpenErrorOutHandler)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadDataFromUICR_Nordic52840BySWDRegister JLink Open Fail : "));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_EnableLog(_OpenLogHandler);
	_JLINKARM_SetErrorOutHandler(_ErrorOutHandler);

	char acOut[256] = { '\0' };
	int r = _JLINKARM_ExecCommand((char*)"DEVICE = NRF52840_XXAA", acOut, sizeof(acOut));
	if (acOut[0] != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadDataFromUICR_Nordic52840BySWDRegister Set DEVICE = NRF52840_XXAA Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_TIF_Select(JLINK_TIF_SWD);
	// default kHz, target 4MHz
	_JLINKARM_SetSpeed(4000);

	if (!_ShowJLinkInfo(_T("NRF52840_XXAA"))) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_CheckJLinkHWVoltage()) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Connect() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadDataFromUICR_Nordic52840BySWDRegister JLink Connect Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadDataFromUICR_Nordic52840BySWDRegister JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// ---- erase uicr customer addr
	uint32_t firstPos = UICR_BASE_ADDR | UICRStartPos;
	uint32_t secondPos = firstPos + UICR_ADDR_OFFSET;
	uint32_t thirdPos = secondPos + UICR_ADDR_OFFSET;

	// ---- write 12 bytes
	uint32_t readFirstBytes = 0;
	uint32_t readSecondBytes = 0;
	uint32_t readThirdBytes = 0;

	static uint32_t readTmpFirstBytes = 0;
	static uint32_t readTmpSecondBytes = 0;
	static uint32_t readTmpThirdBytes = 0;
	static uint8_t pStatus = 0;

	int result = _JLINKARM_ReadMemU32(firstPos, UICR_ADDR_OFFSET, &readTmpFirstBytes, &pStatus);
	if (result < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadDataFromUICR_Nordic52840BySWDRegister _JLINKARM_ReadMemU32 first Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	readFirstBytes = readTmpFirstBytes;

	result = _JLINKARM_ReadMemU32(secondPos, UICR_ADDR_OFFSET, &readTmpSecondBytes, &pStatus);
	if (result < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadDataFromUICR_Nordic52840BySWDRegister _JLINKARM_ReadMemU32 second Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	readSecondBytes = readTmpSecondBytes;

	result = _JLINKARM_ReadMemU32(thirdPos, UICR_ADDR_OFFSET, &readTmpThirdBytes, &pStatus);
	if (result < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadDataFromUICR_Nordic52840BySWDRegister _JLINKARM_ReadMemU32 third Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	readThirdBytes = readTmpThirdBytes;

	memset(outputData->datas, 0, sizeof(outputData->datas));

	outputData->datas[0] = (readFirstBytes >> 24) & 0xff;
	outputData->datas[1] = (readFirstBytes >> 16) & 0xff;
	outputData->datas[2] = (readFirstBytes >> 8) & 0xff;
	outputData->datas[3] = (readFirstBytes) & 0xff;
	outputData->datas[4] = (readSecondBytes >> 24) & 0xff;
	outputData->datas[5] = (readSecondBytes >> 16) & 0xff;
	outputData->datas[6] = (readSecondBytes >> 8) & 0xff;
	outputData->datas[7] = (readSecondBytes) & 0xff;
	outputData->datas[8] = (readThirdBytes >> 24) & 0xff;
	outputData->datas[9] = (readThirdBytes >> 16) & 0xff;
	outputData->datas[10] = (readThirdBytes >> 8) & 0xff;
	outputData->datas[11] = (readThirdBytes) & 0xff;

	_JLINKARM_Close();

	LOGD(_T("[ JLINK INFO ] Read UICR To Chip OK !!"));
	return TRUE;
}

bool ReadFICR_DeviceID_Nordic52840BySWDRegister(uint64_t* outputData)
{
	const uint32_t FICR_DEVICE_ID_BASE_ADDR = 0x10000060;
	const uint32_t FICR_ADDR_OFFSET = 4;

	_JLINKARM_ClrError();
	memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);

	if (!_JlinkDriverIsExist()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadFICR_DeviceID_Nordic52840BySWDRegister JLink Driver Is Not Connect PC !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadFICR_DeviceID_Nordic52840BySWDRegister JLink DLL Is Not Initial !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_SetWarnOutHandler(_WarnOutHandler);
	_JLINKARM_EnableLog(_OpenLogHandler);

	if (_JLINKARM_OpenEx(_OpenLogHandler, _OpenErrorOutHandler)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadFICR_DeviceID_Nordic52840BySWDRegister JLink Open Fail : "));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_EnableLog(_OpenLogHandler);
	_JLINKARM_SetErrorOutHandler(_ErrorOutHandler);

	char acOut[256] = { '\0' };
	int r = _JLINKARM_ExecCommand((char*)"DEVICE = NRF52840_XXAA", acOut, sizeof(acOut));
	if (acOut[0] != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadFICR_DeviceID_Nordic52840BySWDRegister Set DEVICE = NRF52840_XXAA Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_TIF_Select(JLINK_TIF_SWD);
	// default kHz, target 4MHz
	_JLINKARM_SetSpeed(4000);

	if (!_ShowJLinkInfo(_T("NRF52840_XXAA"))) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_CheckJLinkHWVoltage()) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Connect() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadFICR_DeviceID_Nordic52840BySWDRegister JLink Connect Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadFICR_DeviceID_Nordic52840BySWDRegister JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// ---- erase uicr customer addr
	uint32_t firstPos = FICR_DEVICE_ID_BASE_ADDR;
	uint32_t secondPos = FICR_DEVICE_ID_BASE_ADDR + 4;
	uint32_t readFirstBytes = 0;
	uint32_t readSecondBytes = 0;

	static uint32_t readTmpFirstBytes = 0;
	static uint32_t readTmpSecondBytes = 0;
	static uint8_t pStatus = 0;

	int result = _JLINKARM_ReadMemU32(firstPos, FICR_ADDR_OFFSET, &readTmpFirstBytes, &pStatus);
	if (result < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadFICR_DeviceID_Nordic52840BySWDRegister _JLINKARM_ReadMemU32 first Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	readFirstBytes = readTmpFirstBytes;

	result = _JLINKARM_ReadMemU32(secondPos, FICR_ADDR_OFFSET, &readTmpSecondBytes, &pStatus);
	if (result < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ReadFICR_DeviceID_Nordic52840BySWDRegister _JLINKARM_ReadMemU32 second Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	readSecondBytes = readTmpSecondBytes;

	*outputData = (readFirstBytes << 32) | readSecondBytes;

	_JLINKARM_Close();

	LOGD(_T("[ JLINK INFO ] Read UICR To Chip OK !!"));
	return TRUE;
}

#if 0
bool BurnNordic52840FWBySWDRegister(TCHAR* fwPos, DWORD flashOffset, BurnProcess& process)
{
	_JLINKARM_ClrError();
	memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);

	if (!_JlinkDriverIsExist()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWDRegister JLink Driver Is Not Connect PC !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWDRegister JLink DLL Is Not Initial !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_SetWarnOutHandler(_WarnOutHandler);
	_JLINKARM_EnableLog(_OpenLogHandler);

	if (_JLINKARM_OpenEx(_OpenLogHandler, _OpenErrorOutHandler)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWDRegister JLink Open Fail : "));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_EnableLog(_OpenLogHandler);
	_JLINKARM_SetErrorOutHandler(_ErrorOutHandler);

	char acOut[256] = { '\0' };
	int r = _JLINKARM_ExecCommand((char*)"DEVICE = NRF52840_XXAA", acOut, sizeof(acOut));
	if (acOut[0] != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWDRegister Set DEVICE = NRF52840_XXAA Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_TIF_Select(JLINK_TIF_SWD);
	// default kHz, target 4MHz
	_JLINKARM_SetSpeed(4000);

	if (!_ShowJLinkInfo(_T("NRF52840_XXAA"))) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_CheckJLinkHWVoltage()) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Connect() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWDRegister JLink Connect Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWDRegister JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	// ====== erase all ======
	if (!_EraseAllNordic52840ByRegister()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister _EraseAllNordic52840ByRegister Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	//===- Burn
	HANDLE pFile = nullptr;
	DWORD totalFileSize = 0;

	pFile = CreateFile(fwPos, GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,        //打开已存在的文件
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (pFile == INVALID_HANDLE_VALUE)
	{
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister Open Fw File Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		CloseHandle(pFile);
		return FALSE;
	}

	totalFileSize = GetFileSize(pFile, NULL);

	static unsigned char writeBuffer[WRITE_BUFFER] = { 0 };

	DWORD dwBytesToRead = 0;
	uint32_t tempWriteSize = totalFileSize;
	uint32_t currentWriteBlock = 0;
	uint32_t tempAddr = flashOffset;
	uint32_t totalWriteSize = 0;

	BurnProcess* pProcess = &process;

	_JLINKARM_BeginDownload(0);
	do {
		currentWriteBlock = tempWriteSize >= WRITE_BUFFER ? WRITE_BUFFER : tempWriteSize;

		memset(writeBuffer, 0, sizeof(writeBuffer));
		dwBytesToRead = 0;
		if (!ReadFile(pFile, writeBuffer, currentWriteBlock, &dwBytesToRead, NULL)) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister ReadFile Fail !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			CloseHandle(pFile);
			return FALSE;
		}

		if (currentWriteBlock != dwBytesToRead) {
			_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister ReadFile Size Not Match Fail !!"));
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			CloseHandle(pFile);
			return FALSE;
		}

		int result = _JLINKARM_WriteMem(tempAddr, currentWriteBlock, writeBuffer);
		if (result < 0) {
			_swprintf(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister JLink MEM Fail !! Code : %d"), result);
			LOGD(_lastErrorMsg);
			_JLINKARM_Close();
			CloseHandle(pFile);
			return FALSE;
		}

		totalWriteSize += result;
		tempAddr += currentWriteBlock;
		tempWriteSize -= currentWriteBlock;

		if (pProcess != nullptr) {
			double percentDouble = ((double)totalWriteSize / totalFileSize) * 100;
			pProcess((int)percentDouble);
		}
	} while (tempWriteSize > 0);

	CloseHandle(pFile);

	if (totalWriteSize != totalFileSize) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister JLink Download Count Not Match Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	int result = _JLINKARM_EndDownload();
	if (result < 0) {
		_swprintf(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister JLink EndDownload Fail !! Code : %d"), result);
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] EraseNordic52840BySWDRegister JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_Close();

	LOGD(_T("[ JLINK INFO ] Burn Image To Chip OK !!"));

	return TRUE;
}

#endif
// ------------------

bool ResetNordic52840BySWD(void) {
	_JLINKARM_ClrError();
	memset(_lastErrorMsg, L'\0', sizeof(TCHAR) * MSG_BUFFER);

	if (!_JlinkDriverIsExist()) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ResetNordic52840BySWD JLink Driver Is Not Connect PC !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	if (_hModule == nullptr) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ResetNordic52840BySWD JLink DLL Is Not Initial !!"));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_SetWarnOutHandler(_WarnOutHandler);
	_JLINKARM_EnableLog(_OpenLogHandler);

	if (_JLINKARM_OpenEx(_OpenLogHandler, _OpenErrorOutHandler)) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] BurnNordic52840FWBySWD JLink Open Fail : "));
		LOGD(_lastErrorMsg);
		return FALSE;
	}

	_JLINKARM_EnableLog(_OpenLogHandler);
	_JLINKARM_SetErrorOutHandler(_ErrorOutHandler);

	char acOut[256] = { '\0' };
	int r = _JLINKARM_ExecCommand((char*)"DEVICE = NRF52840_XXAA", acOut, sizeof(acOut));
	if (acOut[0] != 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ResetNordic52840BySWD Set DEVICE = NRF52840_XXAA Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_TIF_Select(JLINK_TIF_SWD);
	// default kHz, target 4MHz
	_JLINKARM_SetSpeed(4000);

	if (!_ShowJLinkInfo(_T("NRF52840_XXAA"))) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (!_CheckJLinkHWVoltage()) {
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Connect() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ResetNordic52840BySWD JLink Connect Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	if (_JLINKARM_Reset() < 0) {
		_tcscpy(_lastErrorMsg, _T("[ ERROR ] ResetNordic52840BySWD JLink Reset Fail !!"));
		LOGD(_lastErrorMsg);
		_JLINKARM_Close();
		return FALSE;
	}

	_JLINKARM_Close();

	LOGD(_T("[ JLINK INFO ] Reset Chip OK !!"));

	return TRUE;
}

TCHAR* GetLastErrorStr(void)
{
	return _lastErrorMsg;
}

#endif