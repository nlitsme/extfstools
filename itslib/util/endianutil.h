#ifndef __UTIL_ENDIANUTIL_H__
#define __UTIL_ENDIANUTIL_H__

#include <stdint.h>
//#include "vectorutils.h"
#include <iterator>
#include <algorithm>

#ifdef __LITTLE_ENDIAN__ 

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER  __LITTLE_ENDIAN

#elif defined __BIG_ENDIAN__ 

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER  __BIG_ENDIAN

#elif defined(_WIN32)

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER  __LITTLE_ENDIAN

#elif defined( __FreeBSD__)
#include <sys/endian.h>
//inline uint32_t swab32(uint32_t x) { return bswap32(x); }
//inline uint16_t swab16(uint16_t x) { return bswap16(x); }
#else
#include <endian.h>
#endif
inline uint16_t swab16(uint16_t x) { return ((x&0xff)<<8)                    | ((x>>8)&0xff); }
inline uint32_t swab32(uint32_t x) { return (swab16(x&0xffff)<<16)           | swab16((x>>16)&0xffff); }
inline uint64_t swab64(uint64_t x) { return (uint64_t(swab32(x&0xffff))<<32) | swab32((x>>32)&0xffff); }

// note: BYTEITER must be a randomaccess iterator with the current implementation.
//
// an implementation for forwarditerators would look like this:
//
// get32le(FWDITER p) { uint32_t val= *p++; val |= (*p++)<<8;  val |= (*p++)<<16; val |= (*p++)<<24;  return val; }
// get32be(FWDITER p) { uint32_t val= *p++; val<<=8; val |= (*p++); val<<=8; val |= (*p++); val<<=8; val |= (*p++);  return val; }
//
// then i would need some template construction with enable_if  to choose the FWDITER or RANITER  implementation

template<typename BYTEITER> inline uint8_t get8(BYTEITER p) { return *p; }
#ifdef __amd64__
// note:  *... &*   for the case where BYTEITER is not a pointer but a c++ container iterator
template<typename BYTEITER> inline uint16_t get16le(BYTEITER p) { return *(uint16_t*)&*p; }
template<typename BYTEITER> inline uint32_t get32le(BYTEITER p) { return *(uint32_t*)&*p; }
template<typename BYTEITER> inline uint64_t get64le(BYTEITER p) { return *(uint64_t*)&*p; }
#else
template<typename BYTEITER> inline uint16_t get16le(BYTEITER p) { return (get8(p+1)<<8)              +get8(p); }
template<typename BYTEITER> inline uint32_t get32le(BYTEITER p) { return (get16le(p+2)<<16)          +get16le(p); }
template<typename BYTEITER> inline uint64_t get64le(BYTEITER p) { return (uint64_t(get32le(p+4))<<32)+get32le(p); }
#endif

template<typename BYTEITER> inline uint32_t get24le(BYTEITER p) { return (get8(p+2)<<16)             +get16le(p); }
template<typename BYTEITER> inline uint64_t get48le(BYTEITER p) { return (uint64_t(get16le(p+4))<<32)+get32le(p); }


#ifdef __amd64__
template<typename BYTEITER> inline uint16_t get16be(BYTEITER p) { return __builtin_bswap16(get16le(p)); }
template<typename BYTEITER> inline uint32_t get32be(BYTEITER p) { return __builtin_bswap32(get32le(p)); }
template<typename BYTEITER> inline uint64_t get64be(BYTEITER p) { return __builtin_bswap64(get64le(p)); }
#else
template<typename BYTEITER> inline uint16_t get16be(BYTEITER p) { return (get8(p)<<8)                +get8(p+1); }
template<typename BYTEITER> inline uint32_t get32be(BYTEITER p) { return (get16be(p)<<16)            +get16be(p+2); }
template<typename BYTEITER> inline uint64_t get64be(BYTEITER p) { return (uint64_t(get32be(p))<<32)  +get32be(p+4); }
#endif

template<typename BYTEITER> inline uint32_t get24be(BYTEITER p) { return (get8(p)<<16)               +get16be(p+1); } // for ssl
template<typename BYTEITER> inline uint64_t get48be(BYTEITER p) { return (uint64_t(get16be(p))<<32)  +get32be(p+2); }

template<typename BYTEITER> inline void set8(BYTEITER p, uint8_t v) { *p= v; }

#ifdef __amd64__
template<typename BYTEITER> inline void set16le(BYTEITER p, uint16_t v) { *(uint16_t*)p = v; }
template<typename BYTEITER> inline void set32le(BYTEITER p, uint32_t v) { *(uint32_t*)p = v; }
template<typename BYTEITER> inline void set64le(BYTEITER p, uint64_t v) { *(uint64_t*)p = v; }
#else
template<typename BYTEITER> inline void set16le(BYTEITER p, uint16_t v) { set8(p, v);      set8(p+1, v>>8); }
template<typename BYTEITER> inline void set32le(BYTEITER p, uint32_t v) { set16le(p, v);   set16le(p+2, v>>16); }
template<typename BYTEITER> inline void set64le(BYTEITER p, uint64_t v) { set32le(p, v);   set32le(p+4, v>>32); }
#endif

template<typename BYTEITER> inline void set24le(BYTEITER p, uint32_t v) { set16le(p, v);   set8(p+2, v>>16); }
template<typename BYTEITER> inline void set48le(BYTEITER p, uint64_t v) { set32le(p, v);   set16le(p+4, v>>32); }


#ifdef __amd64__
template<typename BYTEITER> inline void set16be(BYTEITER p, uint16_t v) { set16le(p, __builtin_bswap16(v)); }
template<typename BYTEITER> inline void set32be(BYTEITER p, uint32_t v) { set32le(p, __builtin_bswap32(v)); }
template<typename BYTEITER> inline void set64be(BYTEITER p, uint64_t v) { set64le(p, __builtin_bswap64(v)); }
#else
template<typename BYTEITER> inline void set16be(BYTEITER p, uint16_t v) { set8(p+1, v);    set8(p, v>>8); }
template<typename BYTEITER> inline void set32be(BYTEITER p, uint32_t v) { set16be(p+2, v); set16be(p, v>>16); }
template<typename BYTEITER> inline void set64be(BYTEITER p, uint64_t v) { set32be(p+4, v); set32be(p, v>>32); }
#endif

template<typename BYTEITER> inline void set24be(BYTEITER p, uint32_t v) { set16be(p+1, v); set8(p, v>>16); }    // for ssl
template<typename BYTEITER> inline void set48be(BYTEITER p, uint64_t v) { set32be(p+2, v); set16be(p, v>>32); }

template<typename BYTEITER, typename V> 
    size_t vectorget16le(BYTEITER p, V& v, size_t n)
    {
        v.resize(n);
        memcpy(&v.front(), p, n*sizeof(typename V::value_type));
#if __BYTE_ORDER == __BIG_ENDIAN
        std::for_each(v.begin(), v.end(), [](typename V::value_type& x) { x= swab16(x);});
#endif
        return v.size();
    }

template<int size> struct intsizes;
template<> struct intsizes<8> { typedef uint8_t u; };
template<> struct intsizes<16> { typedef uint16_t u; };
template<> struct intsizes<32> { typedef uint32_t u; };
template<> struct intsizes<64> { typedef uint64_t u; };

template<int bits> inline typename intsizes<bits>::u reversebyteorder(typename intsizes<bits>::u x)
{
    return reversebyteorder( typename intsizes<bits/2>::u(x>>(bits/2)) ) | (reversebyteorder( typename intsizes<bits/2>::u(x>>(bits/2)) )<<(bits/2));
}
template<> inline uint8_t reversebyteorder<8>(uint8_t x) { return x; }

template<typename P>
class bigendianiterator  : public std::iterator<std::random_access_iterator_tag, typename std::iterator_traits<P>::value_type> {
    P _ptr;
    typedef typename std::iterator_traits<P>::value_type value_type;
    typedef typename std::iterator_traits<P>::difference_type difference_type;
public:
    bigendianiterator()
        : _ptr(NULL)
    {
    }
    bigendianiterator(P p)
        : _ptr(p)
    {
    }

    bigendianiterator(const bigendianiterator&i)
        : _ptr(i._ptr)
    {
    }
    bigendianiterator& operator=(const bigendianiterator& i)
    {
        _ptr= i._ptr;
        return *this;
    }

    value_type operator*() const
    {
#if __BYTE_ORDER==__LITTLE_ENDIAN
        return reversebyteorder<sizeof(value_type)*8>(*_ptr);
#else
        return *_ptr;
#endif
    }
    bigendianiterator& operator++()
    {
        ++_ptr;

        return *this;
    }
    bigendianiterator& operator--()
    {
        --_ptr;

        return *this;
    }
    bigendianiterator operator++(int)
    {
        bigendianiterator copy(*this);
        ++(*this);
        return copy;
    }
    bigendianiterator operator--(int)
    {
        bigendianiterator copy(*this);
        --(*this);
        return copy;
    }
    bigendianiterator operator+=(difference_type n)
    {
        if (n<0)
            return (*this)-=(-n);
        while (n--)
            ++(*this);
        return *this;
    }
    bigendianiterator operator-=(difference_type n)
    {
        if (n<0)
            return (*this)+=(-n);
        while (n--)
            --(*this);
        return *this;
    }
    bigendianiterator operator+(difference_type n) const {
        bigendianiterator copy(*this);
        copy += n;
        return copy;
    }
    bigendianiterator operator-(difference_type n) const {
        bigendianiterator copy(*this);
        copy -= n;
        return copy;
    }
    difference_type operator-(const bigendianiterator& rhs) const {
        if ((*this) < rhs) {
            difference_type n=0;
            bigendianiterator i= *this;
            while (i!=rhs) {
                i++;
                n++;
            }
            return -n;
        }
        if (rhs < (*this)) {
            difference_type n=0;
            bigendianiterator i= rhs;
            while (i!=*this) {
                i++;
                n++;
            }
            return n;
        }
        return 0;
    }
    bool operator==(const bigendianiterator& rhs) const { 
        return _ptr == rhs._ptr;
    }
    bool operator>=(const bigendianiterator& rhs) const { return !(*this<rhs); }
    bool operator<=(const bigendianiterator& rhs) const { return !(*this>rhs); }
    bool operator!=(const bigendianiterator& rhs) const { return !(*this==rhs); }
    bool operator>(const bigendianiterator& rhs)  const { return _ptr >  rhs._ptr; }
    bool operator<(const bigendianiterator& rhs)  const { return _ptr <  rhs._ptr; }
};



#endif
