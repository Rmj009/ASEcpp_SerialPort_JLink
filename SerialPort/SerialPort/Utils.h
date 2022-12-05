#pragma once
#include <Windows.h>
#include <tchar.h>
#include <iostream>

typedef void(_stdcall LogMsg)(char*);
typedef bool(_stdcall BinaryFormatCheck)(unsigned char* data, int len);
typedef bool(_stdcall StringFormatCheck)(char* data);
typedef unsigned int uint32_t;
#define TRUE 1
#define FALSE 0
#define WAIT_TIMEOUT_MSEC 100
#define READ_BINARY_BUFFER 512
#define READ_STRING_BUFFER 15360

#define HARDWARE_FLOW_CONTROL_DISABLE 0x00
#define HARDWARE_FLOW_CONTROL_CTS_ENABLE_RTS_ENABLE 0x01
#define HARDWARE_FLOW_CONTROL_CTS_ENABLE_RTS_HANDSHAKE 0x02
#define HARDWARE_FLOW_CONTROL_CTS_ENABLE_RTS_TOGGLE 0x03

#pragma pack(push, type_aion_packed, 1)

struct SERIAL_PORT_VAR {
	char portName[512];
	uint32_t baudRate;
	uint32_t byteSize;
	uint32_t stopBit;
	uint32_t parity;
	//uint32_t ReadIntervalTimeout;
	//uint32_t ReadTotalTimeoutConstant;
	//uint32_t ReadTotalTimeoutMultiplier;
	//uint32_t WriteTotalTimeoutConstant;
	//uint32_t WriteTotalTimeoutMultiplier;
	uint32_t txRxBufferSize;
	uint32_t hardwareFlowControl;
};

struct SERIAL_RESULT {
	char readData[READ_STRING_BUFFER];
};

struct BINARY_SERIAL_RESULT {
	unsigned char readData[READ_BINARY_BUFFER];
	int readLen;
};

#pragma pack(pop, type_aion_packed)
