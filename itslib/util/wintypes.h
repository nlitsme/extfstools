// Copyright 2012 GSMK mbH, Berlin
// All Rights Reserved
//
// Author: Willem Hengeveld   <itsme@gsmk.de>
//
#ifndef _ITSUTILS_WINTYPES_H_
#define _ITSUTILS_WINTYPES_H_
#include <stdint.h>
#ifndef _WIN32
#include <errno.h>
inline uint32_t GetLastError() { return errno; }
#endif
#ifdef _MSC_VER
#include <windows.h>
#include <tchar.h>
#elif defined(USING_SYNCE)
#include <synce.h>
using namespace synce;
typedef struct {
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
} GUID;

struct RECT {
    int32_t top;
    int32_t left;
    int32_t bottom;
    int32_t right;
};
struct SYSTEMTIME {
// todo
};
struct TIME_ZONE_INFORMATION {
// todo
};

#else
#define INVALID_HANDLE_VALUE 0xFFFFFFFF

typedef uint16_t WCHAR;
#ifndef __CYGWIN__
typedef struct {
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
} GUID;

//#define __T(x)  L##x
#define __T(x)  x
#define _T(x)  __T(x)
#define TEXT(x)  __T(x)
#else
#include <windows.h>
#endif

#ifdef _UNICODE
    typedef WCHAR TCHAR;
#else
    typedef char TCHAR;
#endif


#endif

#include <string>
namespace std {
typedef std::basic_string<WCHAR> Wstring;
#ifdef _UNICODE
    typedef Wstring tstring;
#else
    typedef string tstring;
#endif
}

#endif
