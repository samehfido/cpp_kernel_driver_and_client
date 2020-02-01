#pragma once
#include <ntddk.h> // Библиотека драйверов

extern "C"
{
	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(
		IN PEPROCESS		SourceProcess,
		IN PVOID			SourceAddress,
		IN PEPROCESS		TargetProcess,
		IN PVOID			TargetAddress,
		IN SIZE_T			BufferSize,
		IN KPROCESSOR_MODE  PreviousMode,
		OUT PSIZE_T			ReturnSize
	);

	NTKERNELAPI NTSTATUS PsLookupProcessByProcessId(
		IN HANDLE			ProcessId,
		OUT PEPROCESS* Process
	);

	NTKERNELAPI PVOID PsGetProcessSectionBaseAddress(
		IN PEPROCESS		Process
	);
}

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;