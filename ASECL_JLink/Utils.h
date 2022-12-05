#pragma once
#include <Windows.h>
#include <tchar.h>
#include <iostream>

#define MSG_BUFFER 8192

typedef void(_stdcall LogMsg)(TCHAR*);

typedef void(_stdcall BurnProcess)(int percent);

#pragma pack(push, type_aion_packed, 1)
struct UICR_READ_DATA {
	unsigned char datas[12];
};
#pragma pack(pop, type_aion_packed)