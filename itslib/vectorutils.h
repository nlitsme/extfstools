/* (C) 2003 XDA Developers  itsme@xs4all.nl
 *
 * $Header$
 */
#ifndef __VECTORUTILS_H_


#include "util/wintypes.h"

#include <vector>
#include <string>
#include <limits>
#include <stdarg.h>
#include <stdint.h>

//#include <SafeAllocator.h>
//typedef std::vector<uint8_t,std::Safe_Allocator<uint8_t> > ByteVector;
typedef std::vector<uint8_t> ByteVector;

//typedef std::vector<TCHAR> TCharVector;
//typedef std::vector<LPCTSTR> LPCTSTRVector;

typedef std::vector<uint16_t> WordVector;
typedef std::vector<uint32_t> DwordVector;

typedef std::vector<int> IntVector;


// used for easy instantiation of arrays
// todo: replace this with c++11 initializer
template<class T> std::vector<T> MakeVector(int n, ...)
{
    std::vector<T> v;

    va_list ap;
    va_start(ap, n);
    while (n--)
        v.push_back((T)va_arg(ap, int));
    va_end(ap);

    return v;
}

#ifndef _WIN32
// this causes:
// fatal error C1001: An internal error has occurred in the compiler.
// (compiler file 'msc1.cpp', line 1393)
template<> std::vector<std::string> MakeVector(int n, ...);
#endif

#define vectorptr(v)  ((v).empty()?NULL:&(v)[0])

// NOTE: the checked stl cannot take iteratorptr(v.end())
#define iteratorptr(v)  (&(*(v)))

void DV_AppendPtr(DwordVector& v, void *ptr);
void *DV_GetPtr(DwordVector::const_iterator& i);
void BV_AppendByte(ByteVector& v, uint8_t b);
void BV_AppendBytes(ByteVector& v, const uint8_t *b, int len);
void BV_AppendWord(ByteVector& v, uint16_t b);
void BV_AppendDword(ByteVector& v, uint32_t b);
void BV_AppendQword(ByteVector& v, uint64_t b);
void BV_AppendNetWord(ByteVector& v, uint16_t b);
void BV_AppendNetDword(ByteVector& v, uint32_t b);
void BV_AppendVector(ByteVector& v1, const ByteVector& v2);
void BV_AppendString(ByteVector& v, const std::string& s);
void BV_AppendTString(ByteVector& v, const std::tstring& s);
void BV_AppendWString(ByteVector& v, const std::Wstring& s);
void BV_AppendRange(ByteVector& v, const ByteVector::const_iterator& begin, const ByteVector::const_iterator& end);

ByteVector BV_FromBuffer(uint8_t* buf, int len);
ByteVector BV_FromDword(uint32_t value);
ByteVector BV_FromString(const std::string& str);
ByteVector BV_FromWString(const std::Wstring& wstr);

uint8_t BV_GetByte(ByteVector::const_iterator &i);
uint16_t BV_GetWord(ByteVector::const_iterator &i);
uint16_t BV_GetNetWord(ByteVector::const_iterator &i);
uint32_t BV_GetDword(ByteVector::const_iterator &i);
uint64_t BV_GetQword(ByteVector::const_iterator &i);
uint32_t BV_GetNetDword(ByteVector::const_iterator &i);

ByteVector BV_GetByteVector(ByteVector::const_iterator &i, ByteVector::const_iterator end);
std::string BV_GetString(ByteVector::const_iterator &i, int len);
std::tstring BV_GetTString(ByteVector::const_iterator &i, int len);
std::Wstring BV_GetWString(ByteVector::const_iterator &i, int len);
std::string BV_GetString(const ByteVector& bv, int len);
std::Wstring BV_GetWString(const ByteVector& bv, int len);

ByteVector* BV_MakeByteVector(ByteVector::const_iterator &i, ByteVector::const_iterator end);
std::string* BV_MakeString(ByteVector::const_iterator &i, int len);
std::tstring* BV_MakeTString(ByteVector::const_iterator &i, int len);
std::Wstring* BV_MakeWString(ByteVector::const_iterator &i, int len);
std::string* BV_MakeString(const ByteVector& bv, int len);
std::Wstring* BV_MakeWString(const ByteVector& bv, int len);

uint8_t BV_GetByte(const ByteVector& bv);
uint16_t BV_GetWord(const ByteVector& bv);
uint16_t BV_GetNetWord(const ByteVector& bv);
uint32_t BV_GetDword(const ByteVector& bv);
uint32_t BV_GetNetDword(const ByteVector& bv);

// bufpack/bufunpack are vaguely based on perl's pack/unpack
void vbufpack(ByteVector& buf, const char *fmt, va_list ap);
void bufpack(ByteVector& buf, const char *fmt, ...);
DwordVector bufunpack(const ByteVector& buf, const char *fmt);

ByteVector bufpack(const char*fmt, ...);
void bufunpack2(const ByteVector& bv, const char*fmt, ...);


template<typename T>
bool beginswith(const T& v, const T& w)
{
    return v.size()>=w.size() && std::equal(w.begin(), w.end(), v.begin());
    //return std::size(v)>=std::size(w) && std::equal(std::begin(w), std::end(w), std::begin(v));
}

#define __VECTORUTILS_H_
#endif
