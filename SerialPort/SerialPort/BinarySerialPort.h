#pragma once
#include "Utils.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>

#define COMPORT_BUFFER 128

class BinarySerialPort
{
public:
	static BinarySerialPort* Instance();
	VOID SetLogCallback(LogMsg& logFunc);
	BOOL SerialConnect(SERIAL_PORT_VAR& param);
	BOOL SerialClose();
	BOOL ClearBuffer();
	BOOL TransferReceive(unsigned char* cmd, unsigned int cmdLen, BinaryFormatCheck& checkCallback, BINARY_SERIAL_RESULT& result, uint32_t timeoutMs);

private:
	static CRITICAL_SECTION CriticalSection;
	LogMsg* mLogCallback;
	std::mutex mLogMutex;
	std::mutex mStateMutex;
	std::mutex mHandleMutex;
	std::mutex mDataMutex;
	std::mutex mBinaryCheckCallbackMutex;
	std::vector<unsigned char> mReceiveData;

	HANDLE mHandle;
	bool mIsRunningRx;
	HANDLE mReceiveEventHandle;

private:

	BinarySerialPort() {
		this->mLogCallback = nullptr;
		this->mHandle = nullptr;
		this->mIsRunningRx = false;
		this->mReceiveData.clear();
		memset(this->mComport, '\0', sizeof(char) * COMPORT_BUFFER);
		this->mReceiveEventHandle = ::CreateEventA(NULL, TRUE, FALSE, "mReceiveEventHandle");
		this->mBinaryFormatCheckCallback = nullptr;
	}

public:
	char mComport[COMPORT_BUFFER];
	BinaryFormatCheck* mBinaryFormatCheckCallback;

	inline void _SafeSetBinaryFormatCheckCallback(BinaryFormatCheck& callback) {
		std::lock_guard<std::mutex> lock(mBinaryCheckCallbackMutex);
		this->mBinaryFormatCheckCallback = &callback;
	}

	inline bool _SafeCallBinaryFormatCallback(unsigned char* data, int dataLen) {
		std::lock_guard<std::mutex> lock(mBinaryCheckCallbackMutex);
		if (this->mBinaryFormatCheckCallback != nullptr) {
			return this->mBinaryFormatCheckCallback(data, dataLen);
		}
		return false;
	}

	inline void _SafeClearReceiveData() {
		std::lock_guard<std::mutex> lock(mDataMutex);
		this->mReceiveData.clear();
	}

	inline void _SafeAppendReceiveData(unsigned char* bytes, size_t len) {
		std::lock_guard<std::mutex> lock(mDataMutex);
		for (size_t i = 0; i < len; i++) {
			this->mReceiveData.push_back(*(bytes + i));
		}
	}

	inline std::vector<unsigned char>* _SafeGetReceiveData() {
		static std::vector<unsigned char> tmpData;
		tmpData.clear();
		std::lock_guard<std::mutex> lock(mDataMutex);
		tmpData.assign(this->mReceiveData.begin(), this->mReceiveData.end());
		if (tmpData.size() == 0) {
			tmpData = { '1','2' };
		}
		return &tmpData;
	}

	inline void _SafeCloseHanle() {
		std::lock_guard<std::mutex> lock(mHandleMutex);
		if (this->mHandle != nullptr) {
			CloseHandle(this->mHandle);
			this->mHandle = nullptr;
		}
	}

	inline void _SafeSetHandle(HANDLE handle) {
		std::lock_guard<std::mutex> lock(mHandleMutex);
		this->mHandle = handle;
	}

	inline HANDLE _SafeGetHandle() {
		HANDLE handle = nullptr;
		this->mHandleMutex.lock();
		handle = this->mHandle;
		this->mHandleMutex.unlock();
		return handle;
	}

	inline void _SafeSetRunningRxState(bool isRunning) {
		std::lock_guard<std::mutex> lock(mStateMutex);
		this->mIsRunningRx = isRunning;
	}

	inline bool _SafeIsRunningRx() {
		bool isRunning = false;
		mStateMutex.lock();
		isRunning = this->mIsRunningRx;
		mStateMutex.unlock();
		return isRunning;
	}

	inline void _LOGD(char* msg) {
		std::lock_guard<std::mutex> lock(mLogMutex);
		if (mLogCallback != NULL) {
			mLogCallback((char*)msg);
		}
	}

	inline void _WIN32_ERROR_LOGD(char* comport, const char* title) {
		char _errorMsg[10240] = { '\0' };
		DWORD win32Code = GetLastError();
		char* errorMsg = _GetWin32ErrorMsg(win32Code);
		if (errorMsg != nullptr) {
			sprintf_s(_errorMsg, "[CPP] Comport : %s, %s Error Code : %d\r\nError Message : %s", comport, title, win32Code, errorMsg);
			_LOGD(_errorMsg);
		}
	}

	inline char* _GetWin32ErrorMsg(DWORD& errorMessageID) {
		//DWORD errorMessageID = ::GetLastError();
		static char* msg = nullptr;
		if (errorMessageID == 0)
			return nullptr; //No error message has been recorded

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		if (size > 0) {
			if (msg != nullptr) {
				free(msg);
				msg = nullptr;
			}

			msg = (char*)malloc(sizeof(char) * (size + 1));
			if (msg != nullptr) {
				sprintf(msg, "%s", messageBuffer);
			}
		}

		//Free the buffer.
		LocalFree(messageBuffer);

		return msg;
	}

	BOOL _WriteBufferToSerialPort(unsigned char* lpBuf, DWORD dwToWrite, uint32_t timeoutMsec);

	VOID _ReadBufferFromSerialPort();
};