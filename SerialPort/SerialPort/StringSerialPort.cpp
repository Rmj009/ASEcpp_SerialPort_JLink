#include "pch.h"
#include "StringSerialPort.h"

CRITICAL_SECTION StringSerialPort::CriticalSection;
StringSerialPort* StringSerialPort::SELF = nullptr;

StringSerialPort* StringSerialPort::Instance()
{
	volatile static StringSerialPort* pSingleton = NULL;
	if (pSingleton == NULL)
	{
		InitializeCriticalSection(&StringSerialPort::CriticalSection);
		EnterCriticalSection(&StringSerialPort::CriticalSection);

		if (pSingleton == NULL)
		{
			pSingleton = new StringSerialPort;
			StringSerialPort::SELF = (StringSerialPort*)pSingleton;
		}

		LeaveCriticalSection(&StringSerialPort::CriticalSection);
	}

	return const_cast<StringSerialPort*>(pSingleton);
}

VOID StringSerialPort::SetLogCallback(LogMsg& logFunc)
{
	this->mLogCallback = &logFunc;
}

BOOL StringSerialPort::SerialConnect(SERIAL_PORT_VAR& param)
{
	char msg[MSG_BUFFER] = { '\0' };
	std::string portKey = std::string(param.portName);
	portKey.replace(portKey.find("\\\\.\\"), strlen("\\\\.\\"), "");

	auto ptr = this->_GetComportObjByPort((char*)portKey.c_str());
	if (ptr != nullptr) {
		sprintf(msg, "[CPP] Comport : %s Is Already Connect In Queue !!", portKey);
		LOGD(msg);
		ptr = nullptr;
		return FALSE;
	}

	if (strlen(param.portName) == 0) {
		sprintf(msg, "[CPP] SerialConnect PortName Param Is NULL Error !!");
		LOGD(msg);
		return FALSE;
	}

	HANDLE handle = CreateFileA(static_cast<LPCSTR>(param.portName),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		_WIN32_ERROR_LOGD(param.portName, "SerialConnect CreateFileA");
		return FALSE;
	}

	DCB dcbSerialParameters = { 0 };
	if (!GetCommState(handle, &dcbSerialParameters))
	{
		_WIN32_ERROR_LOGD(param.portName, "SerialConnect GetCommState");
		if (handle != NULL)
		{
			CloseHandle(handle);
		}
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

	if (!SetCommState(handle, &dcbSerialParameters))
	{
		_WIN32_ERROR_LOGD(param.portName, "SerialConnect SetCommState");
		if (handle != NULL)
		{
			CloseHandle(handle);
		}
		return FALSE;
	}

	COMMTIMEOUTS commTimeout;
	if (!GetCommTimeouts(handle, &commTimeout))
	{
		_WIN32_ERROR_LOGD(param.portName, "SerialConnect GetCommTimeouts");
		if (handle != NULL)
		{
			CloseHandle(handle);
		}
		return FALSE;
	}

	commTimeout.ReadIntervalTimeout = MAXDWORD;
	commTimeout.ReadTotalTimeoutMultiplier = 0;
	commTimeout.ReadTotalTimeoutConstant = 0;
	commTimeout.WriteTotalTimeoutMultiplier = 0;
	commTimeout.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(handle, &commTimeout)) {
		_WIN32_ERROR_LOGD(param.portName, "SerialConnect SetCommTimeouts");
		if (handle != NULL)
		{
			CloseHandle(handle);
		}
		return FALSE;
	}

	if (param.txRxBufferSize > 0) {
		if (!SetupComm(handle, param.txRxBufferSize, param.txRxBufferSize)) {
			_WIN32_ERROR_LOGD(param.portName, "SerialConnect SetupComm");
			if (handle != NULL)
			{
				CloseHandle(handle);
			}
			return FALSE;
		}
	}
	else {
		if (!SetupComm(handle, 1024, 1024)) {
			_WIN32_ERROR_LOGD(param.portName, "SerialConnect SetupComm");
			if (handle != NULL)
			{
				CloseHandle(handle);
			}
			return FALSE;
		}
	}

	if (!PurgeComm(handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
		_WIN32_ERROR_LOGD(param.portName, "SerialConnect ClearBuffer PurgeComm");
		if (handle != NULL)
		{
			CloseHandle(handle);
		}
		return FALSE;
	}

	// ----
	auto newComport = std::make_shared<SubStringSerialClient>((char*)portKey.c_str(), handle);
	this->mMapping.insert({ portKey, newComport });
	newComport->SetLogCallback(&StringSerialPort::_MAIN_LOGD);
	newComport->RunRxThread();
	return TRUE;
}

BOOL StringSerialPort::SerialClose(char* comport)
{
	std::lock_guard<std::mutex> lock(mComportMutex);
	if (this->mMapping.find(std::string(comport)) == this->mMapping.end()) {
		return TRUE;
	}

	auto ptr = this->mMapping.at(std::string(comport));
	ptr.reset();
	this->mMapping.erase(std::string(comport));
	return TRUE;
}

BOOL StringSerialPort::ClearBuffer(char* comport)
{
	std::lock_guard<std::mutex> lock(mComportMutex);
	if (this->mMapping.find(std::string(comport)) == this->mMapping.end()) {
		return TRUE;
	}

	auto ptr = this->mMapping.at(std::string(comport));
	return ptr->ClearBuffer();
}

VOID StringSerialPort::SerialCloseAll()
{
	std::lock_guard<std::mutex> lock(mComportMutex);
	for (auto& it : this->mMapping) {
		auto ptr = this->mMapping.at(std::string(it.first));
		it.second = nullptr;
		ptr.reset();
	}

	this->mMapping.clear();
}

BOOL StringSerialPort::TransferReceive(char* comport, char* cmd, StringFormatCheck& checkCallback, SERIAL_RESULT& result, uint32_t timeoutMs)
{
	char msg[MSG_BUFFER] = { '\0' };
	auto ptr = this->_GetComportObjByPort(comport);
	if (ptr == nullptr) {
		sprintf(msg, "[CPP] Comport : %s Is Not Connect In Queue !!", comport);
		LOGD(msg);
		ptr = nullptr;
		return FALSE;
	}
	return ptr->TransferReceive(cmd, checkCallback, result, timeoutMs);
}

void StringSerialPort::_MAIN_LOGD(char* msg)
{
	std::lock_guard<std::mutex> lock(StringSerialPort::SELF->mLogMutex);
	if (StringSerialPort::SELF->mLogCallback != NULL) {
		StringSerialPort::SELF->mLogCallback((char*)msg);
	}
}

std::shared_ptr<SubStringSerialClient> StringSerialPort::_GetComportObjByPort(char* comport)
{
	std::lock_guard<std::mutex> lock(mComportMutex);
	if (this->mMapping.find(std::string(comport)) == this->mMapping.end()) {
		return nullptr;
	}
	return this->mMapping.at(std::string(comport));
}

//==================================================================================================

SubStringSerialClient::SubStringSerialClient(char* comport, HANDLE comportHandle) :mThread(nullptr),
mIsRunningRx(false), mStrCheckCallback(nullptr), mLogCallback(nullptr)
{
	this->mReceiveStream.str("");
	this->mComport = std::string(comport);
	this->mComportHandle = comportHandle;
	char eventName[COMPORT_BUFFER] = { '\0' };
	sprintf_s(eventName, "mReceiveEventHandle_%s", comport);
	this->mReceiveEvent = ::CreateEventA(NULL, TRUE, FALSE, eventName);
}

SubStringSerialClient::~SubStringSerialClient()
{
	this->_SafeDispose();
}

void SubStringSerialClient::RunRxThread()
{
	std::lock_guard<std::mutex> lock(this->mTransferMutex);
	if (this->_SafeIsRunningRx()) {
		return;
	}
	char msg[MSG_BUFFER] = { '\0' };
	sprintf_s(msg, "Start to create comport : %s Rx thread !!", this->mComport.c_str());
	this->_SafeClearReceiveData();
	this->_SafeSetRunningRxState(true);
	this->mThread = ::CreateThread(NULL, 0, &SubStringSerialClient::_ThreadFunc, this, 0, NULL);
}

BOOL SubStringSerialClient::ClearBuffer()
{
	std::lock_guard<std::mutex> lock(this->mTransferMutex);
	char msg[MSG_BUFFER] = { '\0' };
	HANDLE handle = this->_SafeGetComportHandle();
	if (handle == nullptr || !this->_SafeIsRunningRx()) {
		sprintf_s(msg, "[CPP] TransferReceive Comport : %s Not Connect !!", (char*)this->mComport.c_str());
		this->_LOGD(msg);
		return FALSE;
	}

	if (!this->_ClearBufferByHandle(handle)) {
		sprintf_s(msg, "[CPP] Comport : %s TransferReceive ClearBuffer Error !!", (char*)this->mComport.c_str());
		this->_LOGD(msg);
		return FALSE;
	}

	return TRUE;
}

BOOL SubStringSerialClient::TransferReceive(char* cmd, StringFormatCheck& checkCallback, SERIAL_RESULT& result, uint32_t timeoutMs)
{
	std::lock_guard<std::mutex> lock(this->mTransferMutex);
	char msg[MSG_BUFFER] = { '\0' };
	HANDLE handle = this->_SafeGetComportHandle();
	if (handle == nullptr || !this->_SafeIsRunningRx()) {
		sprintf_s(msg, "[CPP] TransferReceive Comport : %s Not Connect !!", (char*)this->mComport.c_str());
		this->_LOGD(msg);
		return FALSE;
	}

	if (!this->_ClearBufferByHandle(handle)) {
		sprintf_s(msg, "[CPP] Comport : %s TransferReceive ClearBuffer Error !!", (char*)this->mComport.c_str());
		this->_LOGD(msg);
		return FALSE;
	}

	memset(result.readData, 0, sizeof(char) * READ_STRING_BUFFER);
	this->_SafeSetCheckCallback(checkCallback);
	this->_SafeClearReceiveData();
	this->_WriteToSerialPort(cmd, timeoutMs);
	::ResetEvent(mReceiveEvent);
	::WaitForSingleObject(mReceiveEvent, timeoutMs);

	if (!this->_SafeIsRunningRx()) {
		sprintf_s(msg, "[CPP] Comport : %s Running Rx Fail !!", (char*)this->mComport.c_str());
		this->_LOGD(msg);
		return FALSE;
	}

	std::string* dataVector = this->_SafeGetReceiveData();
	int size = dataVector->size();

	if (size == 0) {
		sprintf_s(msg, "[CPP] Comport : %s Command Timeout Fail !!", (char*)this->mComport.c_str());
		this->_LOGD(msg);
		return FALSE;
	}

	if (size <= (READ_STRING_BUFFER - 1)) {
		int index = 0;
		for (char bytes : *dataVector) {
			result.readData[index] = bytes;
			index++;
		}
		result.readData[index] = '\0';
	}
	else {
		sprintf_s(msg, "[CPP] Comport : %s buffer size %d too high !!", (char*)this->mComport.c_str(), size);
		this->_LOGD(msg);
		return FALSE;
	}
	return TRUE;
}

void SubStringSerialClient::SetLogCallback(LOGD* callback)
{
	this->mLogCallback = callback;
}

void SubStringSerialClient::_LOGD(char* msg)
{
	if (this->mLogCallback != nullptr) {
		this->mLogCallback(msg);
	}
}

BOOL SubStringSerialClient::_SafeDispose()
{
	char msg[MSG_BUFFER] = { '\0' };
	sprintf_s(msg, sizeof(msg), "Sucess to dispose resource comport : %s", this->mComport.c_str());
	this->_LOGD(msg);

	if (this->_SafeIsRunningRx()) {
		this->_SafeSetRunningRxState(false);
		while (this->_SafeGetComportHandle() != nullptr) {
			::Sleep(1);
		}
	}
	else {
		this->_SafeCloseComportHanle();
		this->_SafeSetRunningRxState(false);
	}

	this->mComport.clear();
	if (this->mReceiveEvent != nullptr) {
		CloseHandle(this->mReceiveEvent);
		this->mReceiveEvent = nullptr;
	}

	if (this->mThread != nullptr) {
		CloseHandle(this->mThread);
		this->mThread = nullptr;
	}
	return TRUE;
}

DWORD __stdcall SubStringSerialClient::_ThreadFunc(LPVOID pParam)
{
	char msg[MSG_BUFFER] = { '\0' };
	SubStringSerialClient* self = static_cast<SubStringSerialClient*>(pParam);
	self->_SafeClearReceiveData();

	HANDLE handle = self->_SafeGetComportHandle();
	if (handle == nullptr) {
		return 0;
	}

	BOOL fRes = FALSE;
	OVERLAPPED testEventOverLapped = { 0 };
	DWORD dwCommEvent = 0;

	DWORD dwError = 0;
	COMSTAT comStat = { 0 };
	memset((char*)&comStat, 0, sizeof(COMSTAT));

	testEventOverLapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (testEventOverLapped.hEvent == NULL) {
		self->_WIN32_ERROR_LOGD((char*)self->mComport.c_str(), "WaitSerialEvent EventOverLapped CreateEvent");
		return 0;
	}

	while (self->_SafeIsRunningRx()) {
		memset((char*)&comStat, 0, sizeof(COMSTAT));

		if (!SetCommMask(handle, EV_RXCHAR)) {
			self->_WIN32_ERROR_LOGD((char*)self->mComport.c_str(), "WaitSerialEvent SetCommMask");
			break;
		}

		if (!WaitCommEvent(handle, &dwCommEvent, &testEventOverLapped)) {
			if (GetLastError() != ERROR_IO_PENDING) {
				self->_WIN32_ERROR_LOGD((char*)self->mComport.c_str(), "WaitSerialEvent WaitCommEvent");
				break;
			}
			else {
				DWORD waitRes = WaitForSingleObject(testEventOverLapped.hEvent, (DWORD)WAIT_TIMEOUT_MSEC);
				switch (waitRes)
				{
				case WAIT_OBJECT_0:
					if (!GetOverlappedResult(handle, &testEventOverLapped, &dwCommEvent, FALSE)) {
						self->_WIN32_ERROR_LOGD((char*)self->mComport.c_str(), "ReadBufferFromSerialPort GetOverlappedResult");
						break;
					}
					else {
						// Write operation completed successfully.
						if (!ClearCommError(handle, &dwError, &comStat)) {
							self->_WIN32_ERROR_LOGD((char*)self->mComport.c_str(), "WaitSerialEvent ClearCommError");
							break;
						}
						else {
							if (comStat.cbInQue == 0) {
								//sprintf_s(msg, "[CPP] Comport : %s WaitSerialEvent ClearCommError cbInQue == 0 Warning !!", (char*)self->mComport.c_str());
								//self->_LOGD(msg);
								continue;
							}
							else {
								self->_ReadFromSerialPort();
								// ===
								continue;
							}
						}
					}
					break;

				case WAIT_TIMEOUT:
					continue;

				default:
					self->_WIN32_ERROR_LOGD((char*)self->mComport.c_str(), "ReadBufferFromSerialPort WaitForSingleObject");
					break;
				}
			}
		}
		else {
			if (!ClearCommError(handle, &dwError, &comStat)) {
				self->_WIN32_ERROR_LOGD((char*)self->mComport.c_str(), "WaitSerialEvent ClearCommError");
				break;
			}

			if (comStat.cbInQue == 0) {
				//sprintf_s(msg, "[CPP] Comport : %s WaitSerialEvent ClearCommError cbInQue == 0 Warning !!", (char*)self->mComport.c_str());
				//self->_LOGD(msg);
				continue;
			}
			else {
				self->_ReadFromSerialPort();
				continue;
			}
		}
	}

	if (testEventOverLapped.hEvent != NULL) {
		CloseHandle(testEventOverLapped.hEvent);
		testEventOverLapped.hEvent = NULL;
	}

	self->_SafeSetRunningRxState(false);
	self->_SafeCloseComportHanle();
	return 0;
}

BOOL SubStringSerialClient::_WriteToSerialPort(char* lpBuf, uint32_t timeoutMsec)
{
	char msg[MSG_BUFFER] = { '\0' };

	HANDLE handle = this->_SafeGetComportHandle();
	if (handle == nullptr) {
		sprintf_s(msg, "[CPP] _WriteBufferToSerialPort Not Connect %s !!", (char*)this->mComport.c_str());
		this->_LOGD(msg);
		return FALSE;
	}

	OVERLAPPED osWrite = { 0 };
	DWORD dwWritten;
	DWORD dwRes;
	BOOL fRes;

	// Create this write operation's OVERLAPPED structure's hEvent.
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL) {
		_WIN32_ERROR_LOGD((char*)this->mComport.c_str(), "WriteBufferToSerialPort CreateEvent");
		return FALSE;
	}

	// Issue write.
	if (!WriteFile(handle, lpBuf, strlen(lpBuf), &dwWritten, &osWrite)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			_WIN32_ERROR_LOGD((char*)this->mComport.c_str(), "WriteBufferToSerialPort WriteFile");
			// WriteFile failed, but isn't delayed. Report error and abort.
			fRes = FALSE;
		}
		else {
			dwRes = WaitForSingleObject(osWrite.hEvent, timeoutMsec);
			switch (dwRes)
			{
				// OVERLAPPED structure's event has been signaled.
			case WAIT_OBJECT_0:
				if (!GetOverlappedResult(handle, &osWrite, &dwWritten, FALSE)) {
					_WIN32_ERROR_LOGD((char*)this->mComport.c_str(), "WriteBufferToSerialPort GetOverlappedResult");
					fRes = FALSE;
				}
				else {
					// Write operation completed successfully.
					fRes = TRUE;
				}
				break;

			case WAIT_TIMEOUT:
				sprintf_s(msg, "[CPP] Comport : %s WriteBufferToSerialPort WAIT_TIMEOUT Error !!", (char*)this->mComport.c_str());
				this->_LOGD(msg);
				fRes = FALSE;
				break;

			default:
				_WIN32_ERROR_LOGD((char*)this->mComport.c_str(), "WriteBufferToSerialPort WaitForSingleObject");
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

VOID SubStringSerialClient::_ReadFromSerialPort()
{
	char msg[READ_STRING_BUFFER + 1024] = { '\0' };
	HANDLE handle = this->_SafeGetComportHandle();
	if (handle == nullptr) {
		return;
	}
	static char tmp[READ_BINARY_BUFFER + 1] = { 0 };

	DWORD dwRead = 0;
	OVERLAPPED osReader = { 0 };
	DWORD dwRes = 0;
	BOOL fRes = FALSE;

	// Create the overlapped event. Must be closed before exiting
	// to avoid a handle leak.
	osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (osReader.hEvent == NULL) {
		_WIN32_ERROR_LOGD((char*)this->mComport.c_str(), "ReadBufferFromSerialPort CreateEvent");
		return;
	}

	//long long int dwReceiveCount = 0;

	do {
		dwRead = 0;
		memset(tmp, '\0', sizeof(char) * (READ_BINARY_BUFFER + 1));
		if (!ReadFile(handle, tmp, READ_BINARY_BUFFER, &dwRead, &osReader)) {
			if (GetLastError() != ERROR_IO_PENDING) {
				if (osReader.hEvent == nullptr) {
					CloseHandle(osReader.hEvent);
					osReader.hEvent = nullptr;
				}
				_WIN32_ERROR_LOGD((char*)this->mComport.c_str(), "ReadBufferFromSerialPort ReadFile");
				return;
			}
			else {
				dwRes = WaitForSingleObject(osReader.hEvent, WAIT_TIMEOUT_MSEC);
				switch (dwRes)
				{
					// OVERLAPPED structure's event has been signaled.
				case WAIT_OBJECT_0:
					if (!GetOverlappedResult(handle, &osReader, &dwRead, FALSE)) {
						if (osReader.hEvent == nullptr) {
							CloseHandle(osReader.hEvent);
							osReader.hEvent = nullptr;
						}

						_WIN32_ERROR_LOGD((char*)this->mComport.c_str(), "ReadBufferFromSerialPort GetOverlappedResult");
						return;
					}
					else {
						// Write operation completed successfully.
						if (dwRead > 0) {
							tmp[dwRead] = { '\0' };
							/*	dwReceiveCount += dwRead;
								if (dwReceiveCount > READ_BINARY_BUFFER) {
									break;
								}*/

							this->_SafeAppendReceiveData(tmp);
							/*			sprintf_s(msg, sizeof(msg), "[CPP] Comport : %s Get : %s", (char*)this->mComport.c_str(), tmp);
										this->_LOGD(msg);*/
						}
					}
					break;

				case WAIT_TIMEOUT:
					if (osReader.hEvent == nullptr) {
						CloseHandle(osReader.hEvent);
						osReader.hEvent = nullptr;
					}
					sprintf_s(msg, "[CPP] Comport : %s ReadBufferFromSerialPort WAIT_TIMEOUT Error !!", (char*)this->mComport.c_str());
					this->_LOGD(msg);
					return;

				default:
					if (osReader.hEvent == nullptr) {
						CloseHandle(osReader.hEvent);
						osReader.hEvent = nullptr;
					}
					_WIN32_ERROR_LOGD((char*)this->mComport.c_str(), "ReadBufferFromSerialPort WaitForSingleObject");
					return;
				}
			}
		}
		else {
			// read completed immediately
			if (dwRead > 0) {
				tmp[dwRead] = { '\0' };
				/*	dwReceiveCount += dwRead;
					if (dwReceiveCount > READ_BINARY_BUFFER) {
						break;
					}*/

				this->_SafeAppendReceiveData(tmp);
			}
		}

		if (dwRead > 0) {
			Sleep(1);
		}
	} while (dwRead > 0);

	if (osReader.hEvent == nullptr) {
		CloseHandle(osReader.hEvent);
		osReader.hEvent = nullptr;
	}

	std::string* srcData = this->_SafeGetReceiveData();
#if 0
	sprintf_s(msg, sizeof(msg), "[CPP] Comport : %s Get : %s", (char*)this->mComport.c_str(), srcData->c_str());
	this->_LOGD(msg);
#endif

	if (this->_SafeCallCheckCallback((char*)srcData->c_str())) {
		::SetEvent(mReceiveEvent);
	}
}

BOOL SubStringSerialClient::_ClearBufferByHandle(HANDLE& handle)
{
	if (!PurgeComm(handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
		_WIN32_ERROR_LOGD((char*)this->mComport.c_str(), "ClearBuffer PurgeComm");
		return FALSE;
	}
	return TRUE;
}