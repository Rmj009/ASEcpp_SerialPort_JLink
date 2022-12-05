#include "pch.h"
#include "MultiSerialPort.h"

CRITICAL_SECTION MultiSerialPort::CriticalSection;

MultiSerialPort* MultiSerialPort::Instance()
{
	volatile static MultiSerialPort* pSingleton = NULL;
	if (pSingleton == NULL)
	{
		InitializeCriticalSection(&MultiSerialPort::CriticalSection);
		EnterCriticalSection(&MultiSerialPort::CriticalSection);

		if (pSingleton == NULL)
		{
			pSingleton = new MultiSerialPort;
		}

		LeaveCriticalSection(&MultiSerialPort::CriticalSection);
	}

	return const_cast<MultiSerialPort*>(pSingleton);
}

VOID MultiSerialPort::SetLogCallback(LogMsg& logFunc)
{
	this->mlogCallback = &logFunc;
}

BOOL MultiSerialPort::SerialConnect(SERIAL_PORT_VAR& param)
{
	std::lock_guard<std::mutex> lock(mSerialMutex);
	char msg[8192] = { '\0' };
	std::string portKey = std::string(param.portName);
	portKey.replace(portKey.find("\\\\.\\"), strlen("\\\\.\\"), "");
	if (this->mMapping.find(std::string(portKey)) != this->mMapping.end()) {
		sprintf(msg, "[CPP] Comport : %s Is Already Connect In Queue !!", portKey);
		LOGD(msg);
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

	this->mMapping.insert({ portKey, handle });

	return TRUE;
}

BOOL MultiSerialPort::SerialClose(char* comport)
{
	std::lock_guard<std::mutex> lock(mSerialMutex);
	char msg[8192] = { '\0' };
	if (this->mMapping.find(std::string(comport)) == this->mMapping.end()) {
		_stprintf(msg, "[CPP] Comport : %s Is Not Connect In Queue !!", comport);
		LOGD(msg);
		return FALSE;
	}

	HANDLE currentHandle = this->mMapping.at(std::string(comport));
	if (NULL != currentHandle) {
		CloseHandle(currentHandle);
	}

	this->mMapping.erase(std::string(comport));
	return TRUE;
}

BOOL MultiSerialPort::ClearBuffer(char* comport)
{
	char msg[8192] = { '\0' };
	HANDLE currentHandle = NULL;
	if (!this->_GetComportHandle(comport, currentHandle)) {
		sprintf(msg, "[CPP] Comport : %s Is Not Connect In Queue !!", comport);
		LOGD(msg);
		return FALSE;
	}

	if (currentHandle != NULL) {
		if (!PurgeComm(currentHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
			_WIN32_ERROR_LOGD(comport, "ClearBuffer PurgeComm");
			return FALSE;
		}
	}
	return TRUE;
}

VOID MultiSerialPort::SerialCloseAll()
{
	std::lock_guard<std::mutex> lock(mSerialMutex);
	for (auto& it : this->mMapping) {
		// Do stuff
		HANDLE handle = this->mMapping.at(std::string(it.first));
		if (NULL != handle) {
			CloseHandle(handle);
		}
	}

	this->mMapping.clear();
}

BOOL MultiSerialPort::TransferReceive(char* comport, char* cmd, SERIAL_RESULT& result, uint32_t timeoutMs, uint32_t echoRepeatWaitDelayMs)
{
	HANDLE currentHandle = NULL;
	char msg[8192] = { '\0' };
	if (!this->_GetComportHandle(comport, currentHandle)) {
		sprintf(msg, "[CPP] Comport : %s Is Not Connect In Queue !!", comport);
		LOGD(msg);
		return FALSE;
	}

	if (!_ClearBufferByHandle(comport, currentHandle)) {
		sprintf(msg, "[CPP] Comport : %s TransferReceive ClearBuffer Error !!", comport);
		LOGD(msg);
		return FALSE;
	}

	if (!_WriteBufferToSerialPortByHandle(comport, currentHandle, cmd, strlen(cmd), timeoutMs)) {
		return FALSE;
	}

	if (!_WaitSerialEventByHandle(comport, currentHandle, timeoutMs)) {
		return FALSE;
	}

	if (echoRepeatWaitDelayMs > 0) {
		Sleep(echoRepeatWaitDelayMs);
	}

	BOOL readResult = FALSE;
	std::string getValue = _ReadBufferFromSerialPortByHandle(comport, currentHandle, timeoutMs, readResult);
	if (!readResult) {
		return FALSE;
	}
	strcpy_s(result.readData, getValue.c_str());
	return TRUE;
}

BOOL MultiSerialPort::_ClearBufferByHandle(char* comport, HANDLE& handle)
{
	if (!PurgeComm(handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
		_WIN32_ERROR_LOGD(comport, "ClearBuffer PurgeComm");
		return FALSE;
	}
	return TRUE;
}

BOOL MultiSerialPort::_WriteBufferToSerialPortByHandle(char* comport, HANDLE& handle, char* lpBuf, DWORD dwToWrite, uint32_t timeoutMsec)
{
	char msg[8192] = { '\0' };
	OVERLAPPED osWrite = { 0 };
	DWORD dwWritten;
	DWORD dwRes;
	BOOL fRes;

	// Create this write operation's OVERLAPPED structure's hEvent.
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL) {
		_WIN32_ERROR_LOGD(comport, "WriteBufferToSerialPort CreateEvent");
		return FALSE;
	}

	// Issue write.
	if (!WriteFile(handle, lpBuf, dwToWrite, &dwWritten, &osWrite)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			_WIN32_ERROR_LOGD(comport, "WriteBufferToSerialPort WriteFile");
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
					_WIN32_ERROR_LOGD(comport, "WriteBufferToSerialPort GetOverlappedResult");
					fRes = FALSE;
				}
				else {
					// Write operation completed successfully.
					fRes = TRUE;
				}
				break;

			case WAIT_TIMEOUT:
				sprintf(msg, "[CPP] Comport : %s WriteBufferToSerialPort WAIT_TIMEOUT Error !!", comport);
				LOGD(msg);
				fRes = FALSE;
				break;

			default:
				_WIN32_ERROR_LOGD(comport, "WriteBufferToSerialPort WaitForSingleObject");
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

BOOL MultiSerialPort::_WaitSerialEventByHandle(char* comport, HANDLE& handle, uint32_t timeoutMsec)
{
	char msg[8192] = { '\0' };
	BOOL fRes = FALSE;
	OVERLAPPED testEventOverLapped = { 0 };
	DWORD dwCommEvent = 0;

	testEventOverLapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (testEventOverLapped.hEvent == NULL) {
		_WIN32_ERROR_LOGD(comport, "WaitSerialEvent EventOverLapped CreateEvent");
		return FALSE;
	}

	if (!SetCommMask(handle, EV_RXCHAR)) {
		CloseHandle(testEventOverLapped.hEvent);
		return FALSE;
	}

	while (TRUE) {
		if (!WaitCommEvent(handle, &dwCommEvent, &testEventOverLapped)) {
			if (GetLastError() != ERROR_IO_PENDING) {
				_WIN32_ERROR_LOGD(comport, "WaitSerialEvent WaitCommEvent");
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
					if (!GetOverlappedResult(handle, &testEventOverLapped, &dwCommEvent, FALSE)) {
						_WIN32_ERROR_LOGD(comport, "ReadBufferFromSerialPort GetOverlappedResult");
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
						if (!ClearCommError(handle, &dwError, &comStat)) {
							_WIN32_ERROR_LOGD(comport, "WaitSerialEvent ClearCommError");
							if (testEventOverLapped.hEvent != NULL) {
								CloseHandle(testEventOverLapped.hEvent);
								testEventOverLapped.hEvent = NULL;
							}
							return FALSE;
						}
						else {
							if (comStat.cbInQue == 0) {
								//sprintf(msg, "[CPP] Comport : %s WaitSerialEvent ClearCommError cbInQue == 0 Warning !!", comport);
								//LOGD(msg);
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
					sprintf(msg, "[CPP] Comport : %s WaitSerialEvent WaitCommEvent WAIT_TIMEOUT Error !!", comport);
					LOGD(msg);
					if (testEventOverLapped.hEvent != NULL) {
						CloseHandle(testEventOverLapped.hEvent);
						testEventOverLapped.hEvent = NULL;
					}
					return FALSE;

				default:
					_WIN32_ERROR_LOGD(comport, "ReadBufferFromSerialPort WaitForSingleObject");
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
			if (!ClearCommError(handle, &dwError1, &comStat1)) {
				_WIN32_ERROR_LOGD(comport, "WaitSerialEvent ClearCommError");
				if (testEventOverLapped.hEvent != NULL) {
					CloseHandle(testEventOverLapped.hEvent);
					testEventOverLapped.hEvent = NULL;
				}
				return FALSE;
			}

			if (comStat1.cbInQue == 0) {
				//sprintf(msg, "[CPP] Comport : %s WaitSerialEvent ClearCommError cbInQue == 0 Warning !!", comport);
				//LOGD(msg);
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

std::string MultiSerialPort::_ReadBufferFromSerialPortByHandle(char* comport, HANDLE& handle, uint32_t timeoutMsec, BOOL& result)
{
	char msg[8192] = { '\0' };
	const int RECEIVE_COUNT = 15360;

	//static char* returnValue = nullptr;
	static char returnValue[RECEIVE_COUNT] = { '\0' };
	static char tmp[1025] = { '\0' };

	/*if (returnValue == nullptr) {
		returnValue = (char*)malloc(sizeof(char) * RECEIVE_COUNT);
	}*/

	if (returnValue != nullptr) {
		memset(returnValue, '\0', sizeof(char) * RECEIVE_COUNT);
	}

	/*char msg[8192] = { '\0' };
	char returnValue[10240] = { '\0' };
	char tmp[1024] = { '\0' };*/
	result = false;
	DWORD dwRead;
	OVERLAPPED osReader = { 0 };
	DWORD dwRes;
	BOOL fRes = FALSE;

	// Create the overlapped event. Must be closed before exiting
	// to avoid a handle leak.
	osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (osReader.hEvent == NULL) {
		_WIN32_ERROR_LOGD(comport, "ReadBufferFromSerialPort CreateEvent");
		result = false;
		return std::string("");
	}

	//size_t currentTotalSize = 0;
	long long int dwReceiveCount = 0;

	do {
		//Sleep(10);
		dwRead = 0;
		memset(tmp, '\0', sizeof(char) * 1025);
		if (!ReadFile(handle, tmp, 1024, &dwRead, &osReader)) {
			if (GetLastError() != ERROR_IO_PENDING) {
				CloseHandle(osReader.hEvent);
				_WIN32_ERROR_LOGD(comport, "ReadBufferFromSerialPort ReadFile");
				result = false;
				return std::string("");
			}
			else {
				dwRes = WaitForSingleObject(osReader.hEvent, timeoutMsec);
				switch (dwRes)
				{
					// OVERLAPPED structure's event has been signaled.
				case WAIT_OBJECT_0:
					if (!GetOverlappedResult(handle, &osReader, &dwRead, FALSE)) {
						CloseHandle(osReader.hEvent);
						_WIN32_ERROR_LOGD(comport, "ReadBufferFromSerialPort GetOverlappedResult");
						result = false;
						return std::string("");
					}
					else {
						// Write operation completed successfully.
						result = true;
						if (dwRead > 0) {
							//tmp[dwRead - 1] = '\0';
							dwReceiveCount += dwRead;

							//printf("Get count 1 : %d \n", dwReceiveCount);

							if (dwReceiveCount > RECEIVE_COUNT) {
								break;
							}

							tmp[dwRead] = '\0';

							if (strlen(returnValue) == 0) {
								strcpy(returnValue, tmp);
							}
							else {
								strcat(returnValue, tmp);
							}
						}
					}
					break;

				case WAIT_TIMEOUT:
					CloseHandle(osReader.hEvent);
					sprintf(msg, "[CPP] Comport : %s ReadBufferFromSerialPort WAIT_TIMEOUT Error !!", comport);
					LOGD(msg);
					result = false;
					return std::string("");

				default:
					CloseHandle(osReader.hEvent);
					_WIN32_ERROR_LOGD(comport, "ReadBufferFromSerialPort WaitForSingleObject");
					result = false;
					return std::string("");
				}
			}
		}
		else {
			// read completed immediately
			result = true;
			if (dwRead > 0) {
				//currentTotalSize += dwRead;
				dwReceiveCount += dwRead;
				//printf("Get count 2 : %d \n", dwReceiveCount);
				if (dwReceiveCount > RECEIVE_COUNT) {
					break;
				}

				tmp[dwRead] = '\0';
				if (strlen(returnValue) == 0) {
					//strcpy_s(returnValue, tmp);
					strcpy(returnValue, tmp);
				}
				else {
					//strcat_s(returnValue, tmp);
					strcat(returnValue, tmp);
				}
			}
		}

		if (dwRead > 0) {
			Sleep(1);
		}
	} while (dwRead > 0);

	CloseHandle(osReader.hEvent);

	std::string str = std::string(returnValue);
	return str;
}