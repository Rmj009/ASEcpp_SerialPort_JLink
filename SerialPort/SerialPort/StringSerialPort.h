#pragma once
#include "Utils.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <sstream>

#define COMPORT_BUFFER 128
#define MSG_BUFFER 4096

typedef void(LOGD)(char* msg);

class SubStringSerialClient {
public:
	explicit SubStringSerialClient(char* comport, HANDLE comportHandle);

	~SubStringSerialClient();

	void RunRxThread();

	BOOL ClearBuffer();

	BOOL TransferReceive(char* cmd, StringFormatCheck& checkCallback, SERIAL_RESULT& result, uint32_t timeoutMs);

	void SetLogCallback(LOGD* callback);

private:

	inline HANDLE _SafeGetComportHandle() {
		HANDLE handle = nullptr;
		mHandleMutex.lock();
		handle = this->mComportHandle;
		mHandleMutex.unlock();
		return mComportHandle;
	}

	inline bool _SafeCallCheckCallback(char* data) {
		std::lock_guard<std::mutex> lock(mCheckCallbackMutex);
		if (this->mStrCheckCallback != nullptr) {
			return this->mStrCheckCallback(data);
		}
		return false;
	}

	inline void _SafeSetCheckCallback(StringFormatCheck& callback) {
		std::lock_guard<std::mutex> lock(mCheckCallbackMutex);
		this->mStrCheckCallback = &callback;
	}

	inline void _SafeCloseComportHanle() {
		std::lock_guard<std::mutex> lock(mHandleMutex);
		if (this->mComportHandle != nullptr) {
			CloseHandle(this->mComportHandle);
			this->mComportHandle = nullptr;
			this->mComport.clear();
		}
	}

	inline void _SafeClearReceiveData() {
		std::lock_guard<std::mutex> lock(mDataMutex);
		this->mReceiveStream.str("");
	}

	inline void _SafeAppendReceiveData(char* bytes) {
		std::lock_guard<std::mutex> lock(mDataMutex);
		this->mReceiveStream << std::string(bytes);
	}

	inline std::string* _SafeGetReceiveData() {
		std::lock_guard<std::mutex> lock(mDataMutex);
		static std::string tmpData;
		tmpData.clear();
		tmpData.append(this->mReceiveStream.str());
		return &tmpData;
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

private:

	void _LOGD(char* msg);

	BOOL _SafeDispose();

	static DWORD WINAPI _ThreadFunc(LPVOID pParam);

	BOOL _WriteToSerialPort(char* lpBuf, uint32_t timeoutMsec);

	VOID _ReadFromSerialPort();

	BOOL _ClearBufferByHandle(HANDLE& handle);

private:
	std::string mComport;
	HANDLE mComportHandle;
	HANDLE mReceiveEvent;
	HANDLE mThread;
	bool mIsRunningRx;
	std::stringstream mReceiveStream;
	//std::string mReceiveStr;
	StringFormatCheck* mStrCheckCallback;
	std::mutex mCheckCallbackMutex;
	std::mutex mHandleMutex;
	std::mutex mDataMutex;
	std::mutex mStateMutex;
	std::mutex mTransferMutex;
	LOGD* mLogCallback;
};

class StringSerialPort
{
public:
	static StringSerialPort* Instance();
	VOID SetLogCallback(LogMsg& logFunc);
	BOOL SerialConnect(SERIAL_PORT_VAR& param);
	BOOL SerialClose(char* comport);
	BOOL ClearBuffer(char* comport);
	VOID SerialCloseAll();
	BOOL TransferReceive(char* comport, char* cmd, StringFormatCheck& checkCallback, SERIAL_RESULT& result, uint32_t timeoutMs);

public:
	std::mutex mLogMutex;
	LogMsg* mLogCallback;

private:
	static void _MAIN_LOGD(char* msg);

	StringSerialPort() :mLogCallback(nullptr) {}

	inline void _WIN32_ERROR_LOGD(char* comport, const char* title) {
		char _errorMsg[10240] = { '\0' };
		DWORD win32Code = GetLastError();
		char* errorMsg = _GetWin32ErrorMsg(win32Code);
		if (errorMsg != nullptr) {
			sprintf_s(_errorMsg, "[CPP] Comport : %s, %s Error Code : %d\r\nError Message : %s", comport, title, win32Code, errorMsg);
			_MAIN_LOGD(_errorMsg);
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

	std::shared_ptr<SubStringSerialClient> _GetComportObjByPort(char* comport);

private:
	static StringSerialPort* SELF;
	static CRITICAL_SECTION CriticalSection;

	std::mutex mComportMutex;
	std::unordered_map<std::string, std::shared_ptr<SubStringSerialClient>> mMapping;
};
