#pragma once

#include "Utils.h"
#include <iostream>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>

class MultiSerialPort
{
public:
	static MultiSerialPort* Instance();
	VOID SetLogCallback(LogMsg& logFunc);
	BOOL SerialConnect(SERIAL_PORT_VAR& param);
	BOOL SerialClose(char* comport);
	BOOL ClearBuffer(char* comport);
	VOID SerialCloseAll();
	BOOL TransferReceive(char* comport, char* cmd, SERIAL_RESULT& result, uint32_t timeoutMs, uint32_t echoRepeatWaitDelayMs);

private:
	static CRITICAL_SECTION CriticalSection;
	std::unordered_map<std::string, HANDLE> mMapping;
	LogMsg* mlogCallback;
	std::mutex mMutex;
	std::mutex mSerialMutex;

private:

	VOID LOGD(char* msg) {
		std::lock_guard<std::mutex> lock(mMutex);
		if (mlogCallback != NULL) {
			mlogCallback((char*)msg);
		}
	}

	inline void _WIN32_ERROR_LOGD(char* comport, const char* title) {
		char _errorMsg[10240] = { '\0' };
		DWORD win32Code = GetLastError();
		char* errorMsg = _GetWin32ErrorMsg(win32Code);
		if (errorMsg != nullptr) {
			sprintf_s(_errorMsg, "[CPP] Comport : %s, %s Error Code : %d\r\nError Message : %s", comport, title, win32Code, errorMsg);
			LOGD(_errorMsg);
		}
	}

	inline char* _GetWin32ErrorMsg(DWORD& errorMessageID) {
		//DWORD errorMessageID = ::GetLastError();
		char* msg = nullptr;
		if (errorMessageID == 0)
			return nullptr; //No error message has been recorded

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

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

	inline BOOL _GetComportHandle(char* comport, HANDLE& handle) {
		std::lock_guard<std::mutex> lock(mSerialMutex);
		if (this->mMapping.find(std::string(comport)) == this->mMapping.end()) {
			return FALSE;
		}
		handle = this->mMapping.at(std::string(comport));
		return handle != NULL;
	}

	BOOL _ClearBufferByHandle(char* comport, HANDLE& handle);

	BOOL _WriteBufferToSerialPortByHandle(char* comport, HANDLE& handle, char* lpBuf, DWORD dwToWrite, uint32_t timeoutMsec);

	BOOL _WaitSerialEventByHandle(char* comport, HANDLE& handle, uint32_t timeoutMsec);

	std::string _ReadBufferFromSerialPortByHandle(char* comport, HANDLE& handle, uint32_t timeoutMsec, BOOL& result);
};
