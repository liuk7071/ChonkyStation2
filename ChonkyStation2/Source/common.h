#pragma once
// Common headers
#include <stdio.h>
#include <stdint.h>
#include <cstdarg>
#include <cstring>
#include <stdlib.h>
#include <optional>
#include <string>
#include "../Third party/Dolphin/BitField.hpp"
// Macros / typedefs
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
union u128 {
	u64 b64[2];
	u32 b32[4];
	u16 b16[8];
	u8  b8[16];
};

#define UNITS
#define KB *1024
#define MB *1 KB*1024

// Helper functions
namespace Helpers {
	static const char* Logstr[] = { "[ELF] ", "[EE] ", "[MEM] ", "[DMA] ", "[GIF] ", "[GS] ", "" };
	enum class Log {
		ELFd,
		EEd,
		MEMd,
		DMAd,
		GIFd,
		GSd,
		NOCOND
	};

// What should we log?
#define LOG_ELF
//#define LOG_EE
#define LOG_MEM
#define LOG_DMA
#define LOG_GIF
#define LOG_GS
	static bool ShouldLog(Log log) {
		if (log == Log::ELFd) {
#ifdef LOG_ELF
			return true;
#else
			return false;
#endif
		}
		if (log == Log::EEd) {
#ifdef LOG_EE
			return true;
#else
			return false;
#endif
		}
		if (log == Log::MEMd) {
#ifdef LOG_MEM
			return true;
#else
			return false;
#endif
		}
		if (log == Log::GIFd) {
#ifdef LOG_GIF
			return true;
#else
			return false;
#endif
		}
		if (log == Log::DMAd) {
#ifdef LOG_DMA
			return true;
#else
			return false;
#endif
		}
		if (log == Log::GSd) {
#ifdef LOG_GS
			return true;
#else
			return false;
#endif
		}
		return true; // NOCOND
	}

	static void Debug(Log log, const char* fmt, ...) {
		if (ShouldLog(log)) {
			va_list args;
			printf(Logstr[(int)log]);
			va_start(args, fmt);
			vprintf(fmt, args);
			va_end(args);
		}
	}
	static void Panic(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		exit(0);
	}
} // End namespace Helpers
