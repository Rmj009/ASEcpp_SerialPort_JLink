#pragma once

#include "Utils.h"

#ifdef MAINLIBRARY_EXPORTS
#define MAINLIBRARY_API __declspec(dllexport)
#else
#define MAINLIBRARY_API __declspec(dllimport)
#endif

HANDLE _handle;
BOOL _isConnect;
LogMsg* _logCallback;
char _errorMsg[10240];

extern "C" 	MAINLIBRARY_API void SetLogCallback(LogMsg & logFunc);
extern "C" MAINLIBRARY_API BOOL SerialConnect(SERIAL_PORT_VAR & param);
extern "C" MAINLIBRARY_API VOID SerialClose(void);
extern "C" MAINLIBRARY_API BOOL ClearBuffer(void);
extern "C" MAINLIBRARY_API BOOL TransferReceive(char* cmd, SERIAL_RESULT & result, uint32_t timeoutMs);

extern "C" 	MAINLIBRARY_API VOID Multi_SetLogCallback(LogMsg & logFunc);
extern "C" MAINLIBRARY_API BOOL Multi_SerialConnect(SERIAL_PORT_VAR & param);
extern "C" MAINLIBRARY_API BOOL Multi_SerialClose(char* comport);
extern "C" 	MAINLIBRARY_API VOID Multi_SerialCloseAll(void);
extern "C" MAINLIBRARY_API BOOL Multi_ClearBuffer(char* comport);
extern "C" MAINLIBRARY_API BOOL Multi_TransferReceive(char* comport, char* cmd, SERIAL_RESULT & result, uint32_t timeoutMs, uint32_t echoRepeatWaitDelayMs = 0);

extern "C" 	MAINLIBRARY_API VOID Binary_SetLogCallback(LogMsg & logFunc);
extern "C" MAINLIBRARY_API BOOL Binary_SerialConnect(SERIAL_PORT_VAR & param);
extern "C" MAINLIBRARY_API BOOL Binary_SerialClose(void);
extern "C" MAINLIBRARY_API BOOL Binary_ClearBuffer(void);
extern "C" MAINLIBRARY_API BOOL Binary_TransferReceive(unsigned char* cmd, unsigned int cmdLen, BinaryFormatCheck & checkCallback, BINARY_SERIAL_RESULT & result, uint32_t timeoutMs);

extern "C" 	MAINLIBRARY_API VOID Multi_SetLogCallbackV2(LogMsg & logFunc);
extern "C" MAINLIBRARY_API BOOL Multi_SerialConnectV2(SERIAL_PORT_VAR & param);
extern "C" MAINLIBRARY_API BOOL Multi_SerialCloseV2(char* comport);
extern "C" 	MAINLIBRARY_API VOID Multi_SerialCloseAllV2(void);
extern "C" MAINLIBRARY_API BOOL Multi_ClearBufferV2(char* comport);
extern "C" MAINLIBRARY_API BOOL Multi_TransferReceiveV2(char* comport, char* cmd, StringFormatCheck & checkCallback, SERIAL_RESULT & result, uint32_t timeoutMs);
