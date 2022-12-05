#include "pch.h"
#include "Main.h"
#include <tchar.h>
#include "MultiSerialPort.h"
#include "BinarySerialPort.h"
#include "StringSerialPort.h"

extern HANDLE _handle = NULL;
extern BOOL _isConnect = FALSE;
extern LogMsg* _logCallback = NULL;
extern char _errorMsg[10240] = { '\0' };

#define LOGD(msg)   do{\
						if(_logCallback != NULL) {\
							_logCallback((char*)msg);\
						}\
					}while(0);\
//std::string errorMsg = GetWin32ErrorMsg(errorCode);
//#define WIN32_ERROR_LOGD(title) do{\
//									memset(_errorMsg, '\0', sizeof(char) * 10240);\
//									DWORD win32Code = GetLastError(); \
//									char *errorMsg = GetWin32ErrorMsg(win32Code); \
//									if(errorMsg != nullptr){\
//										sprintf_s(_errorMsg, "[CPP] %s Error Code : %d\r\nError Message : %s", title, errorCode, errorMsg);\
//										LOGD(_errorMsg)\
//									}\
//								}while (0); \

static BOOL WriteBufferToSerialPort(char* lpBuf, DWORD dwToWrite, uint32_t timeoutMsec);
static char* ReadBufferFromSerialPort(uint32_t timeoutMsec, BOOL& result);
static char* GetWin32ErrorMsg(DWORD& errorMessageID);
static BOOL WaitSerialEvent(uint32_t timeoutMsec);

static inline void WIN32_ERROR_LOGD(const char* title) {
	memset(_errorMsg, '\0', sizeof(char) * 10240);
	DWORD win32Code = GetLastError();
	char* errorMsg = GetWin32ErrorMsg(win32Code); \
		if (errorMsg != nullptr) {
			sprintf_s(_errorMsg, "[CPP] %s Error Code : %d\r\nError Message : %s", title, win32Code, errorMsg);
			LOGD(_errorMsg)
		}
}

void SetLogCallback(LogMsg& logFunc)
{
	_logCallback = &logFunc;
}

BOOL SerialConnect(SERIAL_PORT_VAR& param)
{
	if (_isConnect) {
		LOGD("[CPP] SerialConnect Is Already Connected !!")
			return FALSE;
	}

	if (strlen(param.portName) == 0) {
		LOGD("[CPP] SerialConnect PortName Param Is NULL Error !!")
			return FALSE;
	}

	_handle = CreateFileA(static_cast<LPCSTR>(param.portName),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);

	if (_handle == INVALID_HANDLE_VALUE)
	{
		WIN32_ERROR_LOGD("SerialConnect CreateFileA");
		return FALSE;
	}

	DCB dcbSerialParameters = { 0 };
	if (!GetCommState(_handle, &dcbSerialParameters))
	{
		WIN32_ERROR_LOGD("SerialConnect GetCommState");
		SerialClose();
		return FALSE;
	}

	dcbSerialParameters.BaudRate = param.baudRate;
	dcbSerialParameters.ByteSize = param.byteSize;
	dcbSerialParameters.StopBits = param.stopBit;
	dcbSerialParameters.Parity = param.parity;

	switch (param.hardwareFlowControl) {
	case HARDWARE_FLOW_CONTROL_CTS_ENABLE_RTS_ENABLE:
		dcbSerialParameters.fOutxCtsFlow = true;
		dcbSerialParameters.fRtsControl = RTS_CONTROL_ENABLE;
		break;

	case HARDWARE_FLOW_CONTROL_CTS_ENABLE_RTS_HANDSHAKE:
		dcbSerialParameters.fOutxCtsFlow = true;
		dcbSerialParameters.fRtsControl = RTS_CONTROL_HANDSHAKE;
		break;

	case HARDWARE_FLOW_CONTROL_CTS_ENABLE_RTS_TOGGLE:
		dcbSerialParameters.fOutxCtsFlow = true;
		dcbSerialParameters.fRtsControl = RTS_CONTROL_TOGGLE;
		break;

	default:
		dcbSerialParameters.fOutxCtsFlow = false;
		dcbSerialParameters.fRtsControl = RTS_CONTROL_DISABLE;
		break;
	}

	//dcbSerialParameters.fDtrControl = param.dtrControl;

	if (!SetCommState(_handle, &dcbSerialParameters))
	{
		WIN32_ERROR_LOGD("SerialConnect SetCommState");
		SerialClose();
		return FALSE;
	}

	COMMTIMEOUTS commTimeout;
	if (!GetCommTimeouts(_handle, &commTimeout))
	{
		WIN32_ERROR_LOGD("SerialConnect GetCommTimeouts");
		SerialClose();
		return FALSE;
	}

	commTimeout.ReadIntervalTimeout = MAXDWORD;
	commTimeout.ReadTotalTimeoutMultiplier = 0;
	commTimeout.ReadTotalTimeoutConstant = 0;
	commTimeout.WriteTotalTimeoutMultiplier = 0;
	commTimeout.WriteTotalTimeoutConstant = 0;

	/*commTimeout.ReadIntervalTimeout = 5000;
	commTimeout.ReadTotalTimeoutConstant = 0;
	commTimeout.ReadTotalTimeoutMultiplier = 0;
	commTimeout.WriteTotalTimeoutConstant = 5000;
	commTimeout.WriteTotalTimeoutMultiplier = 0;*/

	if (!SetCommTimeouts(_handle, &commTimeout)) {
		WIN32_ERROR_LOGD("SerialConnect SetCommTimeouts");
		SerialClose();
		return FALSE;
	}

	if (param.txRxBufferSize > 0) {
		if (!SetupComm(_handle, param.txRxBufferSize, param.txRxBufferSize)) {
			WIN32_ERROR_LOGD("SerialConnect SetupComm");
			SerialClose();
			return FALSE;
		}
	}
	else {
		if (!SetupComm(_handle, 1024, 1024)) {
			WIN32_ERROR_LOGD("SerialConnect SetupComm");
			SerialClose();
			return FALSE;
		}
	}

	_isConnect = TRUE;

	if (!ClearBuffer()) {
		LOGD("[CPP] SerialConnect ClearBuffer Error !!")
			SerialClose();
		return FALSE;
	}

	return TRUE;
}

VOID SerialClose(void)
{
	if (_isConnect || _handle != NULL)
	{
		_isConnect = FALSE;
		CloseHandle(_handle);
		_handle = NULL;
	}
}

BOOL ClearBuffer(void) {
	if (!_isConnect) {
		LOGD("[CPP] ClearBuffer Serial Is Not Connected !!")
			return FALSE;
	}
	if (!PurgeComm(_handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
		WIN32_ERROR_LOGD("ClearBuffer PurgeComm");
		return FALSE;
	}
}

static BOOL WriteBufferToSerialPort(char* lpBuf, DWORD dwToWrite, uint32_t timeoutMsec) {
	OVERLAPPED osWrite = { 0 };
	DWORD dwWritten;
	DWORD dwRes;
	BOOL fRes;

	// Create this write operation's OVERLAPPED structure's hEvent.
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL) {
		WIN32_ERROR_LOGD("WriteBufferToSerialPort CreateEvent");
		return FALSE;
	}

	// Issue write.
	if (!WriteFile(_handle, lpBuf, dwToWrite, &dwWritten, &osWrite)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			WIN32_ERROR_LOGD("WriteBufferToSerialPort WriteFile");
			// WriteFile failed, but isn't delayed. Report error and abort.
			fRes = FALSE;
		}
		else {
			dwRes = WaitForSingleObject(osWrite.hEvent, timeoutMsec);
			switch (dwRes)
			{
				// OVERLAPPED structure's event has been signaled.
			case WAIT_OBJECT_0:
				if (!GetOverlappedResult(_handle, &osWrite, &dwWritten, FALSE)) {
					WIN32_ERROR_LOGD("WriteBufferToSerialPort GetOverlappedResult");
					fRes = FALSE;
				}
				else {
					// Write operation completed successfully.
					fRes = TRUE;
				}
				break;

			case WAIT_TIMEOUT:
				LOGD("[CPP] WriteBufferToSerialPort WAIT_TIMEOUT Error !!")
					// Operation isn't complete yet. fWaitingOnStatusHandle flag
					// isn't changed since I'll loop back around and I don't want
					// to issue another WaitCommEvent until the first one finishes.
					//
					// This is a good time to do some background work.
					fRes = FALSE;
				break;

			default:
				WIN32_ERROR_LOGD("WriteBufferToSerialPort WaitForSingleObject");
				fRes = FALSE;
				break;
			}
		}
	}
	else {
		// WriteFile completed immediately.
		fRes = TRUE;
	}
	CloseHandle(osWrite.hEvent);
	return fRes;
}

static char* ReadBufferFromSerialPort(uint32_t timeoutMsec, BOOL& result) {
	const int RETURN_VALUE_LEN = 10240;
	const int TMP_LEN = 1025;
	static char returnValue[RETURN_VALUE_LEN] = { '\0' };
	static char tmp[TMP_LEN] = { '\0' };
	memset(returnValue, '\0', sizeof(char) * RETURN_VALUE_LEN);
	result = false;
	DWORD dwRead;
	OVERLAPPED osReader = { 0 };
	DWORD dwRes;
	BOOL fRes = FALSE;

	// Create the overlapped event. Must be closed before exiting
	// to avoid a handle leak.
	osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (osReader.hEvent == NULL) {
		WIN32_ERROR_LOGD("ReadBufferFromSerialPort CreateEvent");
		result = false;
		return nullptr;
	}

	long long int currentTotalSize = 0;

	do {
		dwRead = 0;
		memset(tmp, '\0', sizeof(char) * TMP_LEN);
		if (!ReadFile(_handle, tmp, (TMP_LEN - 1), &dwRead, &osReader)) {
			if (GetLastError() != ERROR_IO_PENDING) {
				CloseHandle(osReader.hEvent);
				WIN32_ERROR_LOGD("ReadBufferFromSerialPort ReadFile");
				result = false;
				return nullptr;
			}
			else {
				dwRes = WaitForSingleObject(osReader.hEvent, timeoutMsec);
				switch (dwRes)
				{
					// OVERLAPPED structure's event has been signaled.
				case WAIT_OBJECT_0:
					if (!GetOverlappedResult(_handle, &osReader, &dwRead, FALSE)) {
						CloseHandle(osReader.hEvent);
						WIN32_ERROR_LOGD("ReadBufferFromSerialPort GetOverlappedResult");
						result = false;
						return nullptr;
					}
					else {
						// Write operation completed successfully.
						result = true;
						if (dwRead > 0) {
							currentTotalSize += dwRead;
							if (currentTotalSize > RETURN_VALUE_LEN) {
								break; // overflow
							}

							tmp[dwRead] = '\0';
							if (strlen(returnValue) == 0) {
								strcpy_s(returnValue, tmp);
							}
							else {
								strcat_s(returnValue, tmp);
							}
						}
					}
					break;

				case WAIT_TIMEOUT:
					CloseHandle(osReader.hEvent);
					LOGD("[CPP] ReadBufferFromSerialPort WAIT_TIMEOUT Error !!")
						result = false;
					return nullptr;

				default:
					CloseHandle(osReader.hEvent);
					WIN32_ERROR_LOGD("ReadBufferFromSerialPort WaitForSingleObject");
					result = false;
					return nullptr;
				}
			}
		}
		else {
			// read completed immediately
			result = true;
			if (dwRead > 0) {
				currentTotalSize += dwRead;
				if (currentTotalSize > RETURN_VALUE_LEN) {
					break; // overflow
				}
				tmp[dwRead] = '\0';
				if (strlen(returnValue) == 0) {
					strcpy_s(returnValue, tmp);
				}
				else {
					strcat_s(returnValue, tmp);
				}
			}
		}

		if (dwRead > 0) {
			Sleep(1);
		}
	} while (dwRead > 0);

	CloseHandle(osReader.hEvent);

	return returnValue;
}

BOOL TransferReceive(char* cmd, SERIAL_RESULT& result, uint32_t timeoutMs)
{
	if (!_isConnect) {
		LOGD("[CPP] WriteBufferToSerialPort Serial Is Not Connected !!")
			return FALSE;
	}

	if (!ClearBuffer()) {
		LOGD("[CPP] TransferReceive ClearBuffer Error !!")
			return FALSE;
	}

	if (!WriteBufferToSerialPort(cmd, strlen(cmd), timeoutMs)) {
		return FALSE;
	}

	if (!WaitSerialEvent(timeoutMs)) {
		return FALSE;
	}

	BOOL readResult = FALSE;
	char* getValue = ReadBufferFromSerialPort(timeoutMs, readResult);
	if (!readResult) {
		return FALSE;
	}
	strcpy_s(result.readData, getValue);
	return TRUE;
}

static BOOL WaitSerialEvent(uint32_t timeoutMsec) {
	BOOL fRes = FALSE;
	OVERLAPPED testEventOverLapped = { 0 };
	DWORD dwCommEvent = 0;

	testEventOverLapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (testEventOverLapped.hEvent == NULL) {
		WIN32_ERROR_LOGD("WaitSerialEvent EventOverLapped CreateEvent");
		return FALSE;
	}

	if (!SetCommMask(_handle, EV_RXCHAR)) {
		CloseHandle(testEventOverLapped.hEvent);
		return FALSE;
	}

	while (TRUE) {
		if (!WaitCommEvent(_handle, &dwCommEvent, &testEventOverLapped)) {
			if (GetLastError() != ERROR_IO_PENDING) {
				WIN32_ERROR_LOGD("WaitSerialEvent WaitCommEvent");
				if (testEventOverLapped.hEvent != NULL) {
					CloseHandle(testEventOverLapped.hEvent);
					testEventOverLapped.hEvent = NULL;
				}
				return FALSE;
			}
			else {
				DWORD waitRes = WaitForSingleObject(testEventOverLapped.hEvent, (DWORD)timeoutMsec);
				switch (waitRes)
				{
				case WAIT_OBJECT_0:
					if (!GetOverlappedResult(_handle, &testEventOverLapped, &dwCommEvent, FALSE)) {
						WIN32_ERROR_LOGD("ReadBufferFromSerialPort GetOverlappedResult");
						if (testEventOverLapped.hEvent != NULL) {
							CloseHandle(testEventOverLapped.hEvent);
							testEventOverLapped.hEvent = NULL;
						}
						return FALSE;
					}
					else {
						// Write operation completed successfully.
						DWORD dwError = 0;
						COMSTAT comStat = { 0 };
						memset((char*)&comStat, 0, sizeof(COMSTAT));
						if (!ClearCommError(_handle, &dwError, &comStat)) {
							WIN32_ERROR_LOGD("WaitSerialEvent ClearCommError");
							if (testEventOverLapped.hEvent != NULL) {
								CloseHandle(testEventOverLapped.hEvent);
								testEventOverLapped.hEvent = NULL;
							}
							return FALSE;
						}
						else {
							if (comStat.cbInQue == 0) {
								LOGD("[CPP] WaitSerialEvent ClearCommError cbInQue == 0 Warning !!")
									continue;
							}
							else {
								if (testEventOverLapped.hEvent != NULL) {
									CloseHandle(testEventOverLapped.hEvent);
									testEventOverLapped.hEvent = NULL;
								}
								return TRUE;
							}
						}
					}
					break;

				case WAIT_TIMEOUT:
					LOGD("[CPP] WaitSerialEvent WaitCommEvent WAIT_TIMEOUT Error !!")
						if (testEventOverLapped.hEvent != NULL) {
							CloseHandle(testEventOverLapped.hEvent);
							testEventOverLapped.hEvent = NULL;
						}
					return FALSE;

				default:
					WIN32_ERROR_LOGD("ReadBufferFromSerialPort WaitForSingleObject");
					if (testEventOverLapped.hEvent != NULL) {
						CloseHandle(testEventOverLapped.hEvent);
						testEventOverLapped.hEvent = NULL;
					}
					return FALSE;
				}
			}
		}
		else {
			DWORD dwError1 = 0;
			COMSTAT comStat1 = { 0 };
			memset((char*)&comStat1, 0, sizeof(COMSTAT));
			if (!ClearCommError(_handle, &dwError1, &comStat1)) {
				WIN32_ERROR_LOGD("WaitSerialEvent ClearCommError");
				if (testEventOverLapped.hEvent != NULL) {
					CloseHandle(testEventOverLapped.hEvent);
					testEventOverLapped.hEvent = NULL;
				}
				return FALSE;
			}

			if (comStat1.cbInQue == 0) {
				LOGD("[CPP] WaitSerialEvent ClearCommError cbInQue == 0 Warning !!")
					continue;
			}
			else {
				if (testEventOverLapped.hEvent != NULL) {
					CloseHandle(testEventOverLapped.hEvent);
					testEventOverLapped.hEvent = NULL;
				}
				return TRUE;
			}
		}
	}

	if (testEventOverLapped.hEvent != NULL) {
		CloseHandle(testEventOverLapped.hEvent);
		testEventOverLapped.hEvent = NULL;
	}

	return FALSE;
}

static char* GetWin32ErrorMsg(DWORD& errorMessageID) {
	//DWORD errorMessageID = ::GetLastError();
	static char* msg = nullptr;
	if (errorMessageID == 0)
		return nullptr; //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	if (nullptr != msg) {
		free(msg);
		msg = nullptr;
	}

	if (size > 0) {
		msg = (char*)malloc(sizeof(char) * (size + 1));
		if (msg != nullptr) {
			sprintf(msg, "%s", messageBuffer);
		}
	}

	//Free the buffer.
	LocalFree(messageBuffer);

	return msg;
}

//=============  Multi

VOID Multi_SetLogCallback(LogMsg& logFunc)
{
	MultiSerialPort::Instance()->SetLogCallback(logFunc);
}

BOOL Multi_SerialConnect(SERIAL_PORT_VAR& param)
{
	return MultiSerialPort::Instance()->SerialConnect(param);
}

BOOL Multi_SerialClose(char* comport)
{
	return MultiSerialPort::Instance()->SerialClose(comport);
}

VOID Multi_SerialCloseAll(void)
{
	MultiSerialPort::Instance()->SerialCloseAll();
}

BOOL Multi_ClearBuffer(char* comport)
{
	return MultiSerialPort::Instance()->ClearBuffer(comport);
}

BOOL Multi_TransferReceive(char* comport, char* cmd, SERIAL_RESULT& result, uint32_t timeoutMs, uint32_t echoRepeatWaitDelayMs)
{
	return MultiSerialPort::Instance()->TransferReceive(comport, cmd, result, timeoutMs, echoRepeatWaitDelayMs);
}

VOID Binary_SetLogCallback(LogMsg& logFunc)
{
	BinarySerialPort::Instance()->SetLogCallback(logFunc);
}

BOOL Binary_SerialConnect(SERIAL_PORT_VAR& param)
{
	return BinarySerialPort::Instance()->SerialConnect(param);
}

BOOL Binary_SerialClose(void)
{
	return BinarySerialPort::Instance()->SerialClose();
}

BOOL Binary_ClearBuffer(void)
{
	return BinarySerialPort::Instance()->ClearBuffer();
}

BOOL Binary_TransferReceive(unsigned char* cmd, unsigned int cmdLen, BinaryFormatCheck& checkCallback, BINARY_SERIAL_RESULT& result, uint32_t timeoutMs)
{
	return BinarySerialPort::Instance()->TransferReceive(cmd, cmdLen, checkCallback, result, timeoutMs);
}

VOID Multi_SetLogCallbackV2(LogMsg& logFunc)
{
	StringSerialPort::Instance()->SetLogCallback(logFunc);
}

BOOL Multi_SerialConnectV2(SERIAL_PORT_VAR& param)
{
	return StringSerialPort::Instance()->SerialConnect(param);
}

BOOL Multi_SerialCloseV2(char* comport)
{
	return StringSerialPort::Instance()->SerialClose(comport);
}

VOID Multi_SerialCloseAllV2(void)
{
	StringSerialPort::Instance()->SerialCloseAll();
}

BOOL Multi_ClearBufferV2(char* comport)
{
	return StringSerialPort::Instance()->ClearBuffer(comport);
}

BOOL Multi_TransferReceiveV2(char* comport, char* cmd, StringFormatCheck& checkCallback, SERIAL_RESULT& result, uint32_t timeoutMs)
{
	return StringSerialPort::Instance()->TransferReceive(comport, cmd, checkCallback, result, timeoutMs);
}