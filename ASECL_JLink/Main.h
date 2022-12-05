#pragma once

#include "Utils.h"

#define OLDER_VERSION 0

#ifdef MAINLIBRARY_EXPORTS
#define MAINLIBRARY_API __declspec(dllexport)
#else
#define MAINLIBRARY_API __declspec(dllimport)
#endif

LogMsg* _logCallback;

extern "C" 	MAINLIBRARY_API void SetLogCallback(LogMsg & logFunc);

#if OLDER_VERSION == 1

TCHAR _jlinkExePath[4096];

extern "C" 	MAINLIBRARY_API bool SetJlinkExePos(TCHAR * jlinkPos);

extern "C" 	MAINLIBRARY_API bool BurnNordic52840FWBySWD(TCHAR * fwPos);

extern "C" 	MAINLIBRARY_API bool EraseNordic52840BySWD(void);

extern "C" 	MAINLIBRARY_API bool ResetNordic52840BySWD(void);

#else

extern "C" 	MAINLIBRARY_API bool InitJLinkModule(void);

extern "C" 	MAINLIBRARY_API bool FreeJLinkModule(void);

extern "C" MAINLIBRARY_API TCHAR * GetJLinkInfo(bool* result);

extern "C" MAINLIBRARY_API void SetVoltageCheckThresholdByMV(double voltage_mv);

extern "C" 	MAINLIBRARY_API bool BurnNordic52840FWBySWD(TCHAR * fwPos, DWORD flashOffset);

extern "C" 	MAINLIBRARY_API bool EraseNordic52840BySWD(void);

extern "C" 	MAINLIBRARY_API bool ErasePageNordic52840BySWD(uint32_t pageAddr);

// ---- Register ----

extern "C" 	MAINLIBRARY_API bool EraseNordic52840BySWDRegister(void);

#if 0
extern "C" 	MAINLIBRARY_API bool BurnNordic52840FWBySWDRegister(TCHAR * fwPos, DWORD flashOffset, BurnProcess & process);
#endif

extern "C" 	MAINLIBRARY_API bool WriteDataToUICR_Nordic52840BySWDRegister(unsigned char* datas, uint32_t dataLen, uint32_t UICRStartPos);

extern "C" 	MAINLIBRARY_API bool ReadDataFromUICR_Nordic52840BySWDRegister(struct UICR_READ_DATA* outputData, uint32_t UICRStartPos);

extern "C" 	MAINLIBRARY_API bool ReadFICR_DeviceID_Nordic52840BySWDRegister(uint64_t * outputData);

// ------------------

extern "C" 	MAINLIBRARY_API bool ResetNordic52840BySWD(void);

extern "C" MAINLIBRARY_API TCHAR * GetLastErrorStr(void);

#endif
