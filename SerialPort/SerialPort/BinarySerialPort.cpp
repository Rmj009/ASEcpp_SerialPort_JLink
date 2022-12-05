#include "pch.h"
#include "BinarySerialPort.h"

CRITICAL_SECTION BinarySerialPort::CriticalSection;

BinarySerialPort* BinarySerialPort::Instance()
{
	volatile static BinarySerialPort* pSingleton = NULL;
	if (pSingleton == NULL)
	{
		InitializeCriticalSection(&BinarySerialPort::CriticalSection);
		EnterCriticalSection(&BinarySerialPort::CriticalSection);

		if (pSingleton == NULL)
		{
			pSingleton = new BinarySerialPort;
		}

		LeaveCriticalSection(&BinarySerialPort::CriticalSection);
	}

	return const_cast<BinarySerialPort*>(pSingleton);
}

VOID BinarySerialPort::SetLogCallback(LogMsg& logFunc)
{
	this->mLogCallback = &logFunc;
}

static DWORD WINAPI ThreadFunc(LPVOID pParam) {
	char msg[1024] = { '\0' };
	BinarySerialPort* self = static_cast<BinarySerialPort*>(pParam);
	self->_SafeClearReceiveData();

	HANDLE handle = self->_SafeGetHandle();
	if (handle == nullptr) {
		return FALSE;
	}

	BOOL fRes = FALSE;
	OVERLAPPED testEventOverLapped = { 0 };
	DWORD dwCommEvent = 0;

	DWORD dwError = 0;
	COMSTAT comStat = { 0 };
	memset((char*)&comStat, 0, sizeof(COMSTAT));

	testEventOverLapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (testEventOverLapped.hEvent == NULL) {
		self->_WIN32_ERROR_LOGD(self->mComport, "WaitSerialEvent EventOverLapped CreateEvent");
		return 0;
	}

	while (self->_SafeIsRunningRx()) {
		memset((char*)&comStat, 0, sizeof(COMSTAT));

		if (!SetCommMask(handle, EV_RXCHAR)) {
			self->_WIN32_ERROR_LOGD(self->mComport, "WaitSerialEvent SetCommMask");
			break;
		}

		if (!WaitCommEvent(handle, &dwCommEvent, &testEventOverLapped)) {
			if (GetLastError() != ERROR_IO_PENDING) {
				self->_WIN32_ERROR_LOGD(self->mComport, "WaitSerialEvent WaitCommEvent");
				break;
			}
			else {
				DWORD waitRes = WaitForSingleObject(testEventOverLapped.hEvent, (DWORD)WAIT_TIMEOUT_MSEC);
				switch (waitRes)
				{
				case WAIT_OBJECT_0:
					if (!GetOverlappedResult(handle, &testEventOverLapped, &dwCommEvent, FALSE)) {
						self->_WIN32_ERROR_LOGD(self->mComport, "ReadBufferFromSerialPort GetOverlappedResult");
						break;
					}
					else {
						// Write operation completed successfully.
						if (!ClearCommError(handle, &dwError, &comStat)) {
							self->_WIN32_ERROR_LOGD(self->mComport, "WaitSerialEvent ClearCommError");
							break;
						}
						else {
							if (comStat.cbInQue == 0) {
								//sprintf_s(msg, "[CPP] Comport : %s WaitSerialEvent ClearCommError cbInQue == 0 Warning !!", self->mComport);
								//self->_LOGD(msg);
								continue;
							}
							else {
								self->_ReadBufferFromSerialPort();

								// ===

								continue;
							}
						}
					}
					break;

				case WAIT_TIMEOUT:
					continue;

				default:
					self->_WIN32_ERROR_LOGD(self->mComport, "ReadBufferFromSerialPort WaitForSingleObject");
					break;
				}
			}
		}
		else {
			if (!ClearCommError(handle, &dwError, &comStat)) {
				self->_WIN32_ERROR_LOGD(self->mComport, "WaitSerialEvent ClearCommError");
				break;
			}

			if (comStat.cbInQue == 0) {
				//sprintf_s(msg, "[CPP] Comport : %s WaitSerialEvent ClearCommError cbInQue == 0 Warning !!", self->mComport);
				//self->_LOGD(msg);
				continue;
			}
			else {
				self->_ReadBufferFromSerialPort();
				continue;
			}
		}
	}

	if (testEventOverLapped.hEvent != NULL) {
		CloseHandle(testEventOverLapped.hEvent);
		testEventOverLapped.hEvent = NULL;
	}

	self->_SafeSetRunningRxState(false);
	self->_SafeCloseHanle();
	return 0;
}

BOOL BinarySerialPort::SerialConnect(SERIAL_PORT_VAR& param)
{
	this->SerialClose();

	char msg[8192] = { '\0' };

	if (strlen(param.portName) == 0) {
		sprintf_s(msg, "[CPP] SerialConnect PortName Param Is NULL Error !!");
		_LOGD(msg);
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

	this->_SafeSetHandle(handle);
	this->_SafeClearReceiveData();

	strcpy_s(this->mComport, param.portName);

	this->_SafeSetRunningRxState(true);
	::CreateThread(NULL, 0, ThreadFunc, this, 0, NULL);
	return TRUE;
}

BOOL BinarySerialPort::SerialClose()
{
	if (this->_SafeIsRunningRx()) {
		this->_SafeSetRunningRxState(false);
		while (this->_SafeGetHandle() != nullptr) {
			::Sleep(1);
		}
	}
	else {
		this->_SafeCloseHanle();
		this->_SafeSetRunningRxState(false);
	}

	memset(this->mComport, '\0', sizeof(char) * COMPORT_BUFFER);

	return TRUE;
}

BOOL BinarySerialPort::ClearBuffer()
{
	char msg[8192] = { '\0' };
	HANDLE handle = this->_SafeGetHandle();
	if (handle == nullptr) {
		sprintf_s(msg, "[CPP] ClearBuffer Not Connect !!");
		this->_LOGD(msg);
		return FALSE;
	}

	if (!PurgeComm(handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
		_WIN32_ERROR_LOGD(this->mComport, "ClearBuffer PurgeComm");
		return FALSE;
	}
	return TRUE;
}

BOOL BinarySerialPort::TransferReceive(unsigned char* cmd, unsigned int cmdLen, BinaryFormatCheck& checkCallback, BINARY_SERIAL_RESULT& result, uint32_t timeoutMs)
{
	char msg[8192] = { '\0' };
	HANDLE handle = this->_SafeGetHandle();
	if (handle == nullptr || !this->_SafeIsRunningRx()) {
		sprintf_s(msg, "[CPP] TransferReceive Not Connect !!");
		this->_LOGD(msg);
		return FALSE;
	}

	if (!this->ClearBuffer()) {
		sprintf_s(msg, "[CPP] Comport : %s TransferReceive ClearBuffer Error !!", this->mComport);
		this->_LOGD(msg);
		return FALSE;
	}

	memset(result.readData, 0, sizeof(unsigned char) * READ_BINARY_BUFFER);
	result.readLen = 0;

	this->_SafeSetBinaryFormatCheckCallback(checkCallback);
	this->_SafeClearReceiveData();
	this->_WriteBufferToSerialPort(cmd, cmdLen, timeoutMs);
	::ResetEvent(mReceiveEventHandle);
	::WaitForSingleObject(mReceiveEventHandle, timeoutMs);

	if (!this->_SafeIsRunningRx()) {
		sprintf_s(msg, "[CPP] Running Rx Fail !!");
		this->_LOGD(msg);
		return FALSE;
	}

	std::vector<unsigned char>* dataVector = this->_SafeGetReceiveData();
	int size = dataVector->size();

	if (size == 0) {
		sprintf_s(msg, "[CPP] Command Timeout Fail !!");
		this->_LOGD(msg);
		return FALSE;
	}

	if (size <= READ_BINARY_BUFFER) {
		int index = 0;
		for (unsigned char bytes : *dataVector) {
			result.readData[index] = bytes;
			index++;
		}
		result.readLen = size;
	}

	//if (!this->_SafeCallBinaryFormatCallback(result.readData, size)) {
	//	sprintf_s(msg, "[CPP] Binary Format Fail !!");
	//	this->_LOGD(msg);
	//	return FALSE;
	//}

	return TRUE;
}

BOOL BinarySerialPort::_WriteBufferToSerialPort(unsigned char* lpBuf, DWORD dwToWrite, uint32_t timeoutMsec)
{
	char msg[8192] = { '\0' };

	HANDLE handle = this->_SafeGetHandle();
	if (handle == nullptr) {
		sprintf_s(msg, "[CPP] _WriteBufferToSerialPort Not Connect !!");
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
		_WIN32_ERROR_LOGD(this->mComport, "WriteBufferToSerialPort CreateEvent");
		return FALSE;
	}

	// Issue write.
	if (!WriteFile(handle, lpBuf, dwToWrite, &dwWritten, &osWrite)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			_WIN32_ERROR_LOGD(this->mComport, "WriteBufferToSerialPort WriteFile");
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
					_WIN32_ERROR_LOGD(this->mComport, "WriteBufferToSerialPort GetOverlappedResult");
					fRes = FALSE;
				}
				else {
					// Write operation completed successfully.
					fRes = TRUE;
				}
				break;

			case WAIT_TIMEOUT:
				sprintf_s(msg, "[CPP] Comport : %s WriteBufferToSerialPort WAIT_TIMEOUT Error !!", this->mComport);
				this->_LOGD(msg);
				fRes = FALSE;
				break;

			default:
				_WIN32_ERROR_LOGD(this->mComport, "WriteBufferToSerialPort WaitForSingleObject");
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

VOID BinarySerialPort::_ReadBufferFromSerialPort()
{
	HANDLE handle = this->_SafeGetHandle();
	if (handle == nullptr) {
		return;
	}

	char msg[8192] = { '\0' };
	static unsigned char tmp[READ_BINARY_BUFFER] = { 0 };

	DWORD dwRead;
	OVERLAPPED osReader = { 0 };
	DWORD dwRes;
	BOOL fRes = FALSE;

	// Create the overlapped event. Must be closed before exiting
	// to avoid a handle leak.
	osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (osReader.hEvent == NULL) {
		_WIN32_ERROR_LOGD(this->mComport, "ReadBufferFromSerialPort CreateEvent");
		return;
	}

	long long int dwReceiveCount = 0;

	do {
		dwRead = 0;
		memset(tmp, 0, sizeof(unsigned char) * READ_BINARY_BUFFER);
		if (!ReadFile(handle, tmp, READ_BINARY_BUFFER, &dwRead, &osReader)) {
			if (GetLastError() != ERROR_IO_PENDING) {
				if (osReader.hEvent == nullptr) {
					CloseHandle(osReader.hEvent);
					osReader.hEvent = nullptr;
				}
				_WIN32_ERROR_LOGD(this->mComport, "ReadBufferFromSerialPort ReadFile");
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

						_WIN32_ERROR_LOGD(this->mComport, "ReadBufferFromSerialPort GetOverlappedResult");
						return;
					}
					else {
						// Write operation completed successfully.
						if (dwRead > 0) {
							dwReceiveCount += dwRead;
							if (dwReceiveCount > READ_BINARY_BUFFER) {
								break;
							}

							this->_SafeAppendReceiveData(tmp, dwRead);
						}
					}
					break;

				case WAIT_TIMEOUT:
					if (osReader.hEvent == nullptr) {
						CloseHandle(osReader.hEvent);
						osReader.hEvent = nullptr;
					}
					sprintf_s(msg, "[CPP] Comport : %s ReadBufferFromSerialPort WAIT_TIMEOUT Error !!", this->mComport);
					this->_LOGD(msg);
					return;

				default:
					if (osReader.hEvent == nullptr) {
						CloseHandle(osReader.hEvent);
						osReader.hEvent = nullptr;
					}
					_WIN32_ERROR_LOGD(this->mComport, "ReadBufferFromSerialPort WaitForSingleObject");
					return;
				}
			}
		}
		else {
			// read completed immediately
			if (dwRead > 0) {
				dwReceiveCount += dwRead;
				if (dwReceiveCount > READ_BINARY_BUFFER) {
					break;
				}

				this->_SafeAppendReceiveData(tmp, dwRead);
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

	unsigned char* srcData = &this->_SafeGetReceiveData()->at(0);
	int srcLen = this->_SafeGetReceiveData()->size();

	if (this->_SafeCallBinaryFormatCallback(srcData, srcLen)) {
		::SetEvent(mReceiveEventHandle);
	}
}