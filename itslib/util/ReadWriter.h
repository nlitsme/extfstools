#ifndef _UTIL_READWRITER_H__
#define _UTIL_READWRITER_H__
#include <string>
#include <stdio.h>
#include <algorithm>
#include <memory>

#include "util/endianutil.h"

class ReadWriter {
    bool _readonly;
public:
    ReadWriter() : _readonly(false) { }
    virtual ~ReadWriter() { }
    virtual size_t read(uint8_t *p, size_t n)= 0;
    virtual void write(const uint8_t *p, size_t n)= 0;
    virtual void setpos(uint64_t off)= 0;
    virtual void truncate(uint64_t off)= 0;
    virtual uint64_t size()= 0;
    virtual uint64_t getpos() const= 0;
    virtual bool eof()= 0;
    bool isreadonly() const { return _readonly; }
    void setreadonly() { _readonly= true; }

    // see n2798, page231 : virtual func can hide fns with same name
    uint64_t read64le() { uint8_t v[sizeof(uint64_t)]; if (sizeof(v)!=read(v, sizeof(v))) throw "read64le"; return get64le(v); }
    uint32_t read32le() { uint8_t v[sizeof(uint32_t)]; if (sizeof(v)!=read(v, sizeof(v))) throw "read32le"; return get32le(v); }
    uint16_t read16le() { uint8_t v[sizeof(uint16_t)]; if (sizeof(v)!=read(v, sizeof(v))) throw "read16le"; return get16le(v); }
    uint64_t read64be() { uint8_t v[sizeof(uint64_t)]; if (sizeof(v)!=read(v, sizeof(v))) throw "read64be"; return get64be(v); }
    uint32_t read32be() { uint8_t v[sizeof(uint32_t)]; if (sizeof(v)!=read(v, sizeof(v))) throw "read32be"; return get32be(v); }
    uint16_t read16be() { uint8_t v[sizeof(uint16_t)]; if (sizeof(v)!=read(v, sizeof(v))) throw "read16be"; return get16be(v); }
    uint8_t read8() { uint8_t v[sizeof(uint8_t)]; if (sizeof(v)!=read(v, sizeof(v))) throw "read8"; return v[0]; }

    void write64le(uint64_t x) { uint8_t v[sizeof(uint64_t)]; set64le(v, x); write(v, sizeof(v)); }
    void write32le(uint32_t x) { uint8_t v[sizeof(uint32_t)]; set32le(v, x); write(v, sizeof(v)); }
    void write16le(uint16_t x) { uint8_t v[sizeof(uint16_t)]; set16le(v, x); write(v, sizeof(v)); }
    void write64be(uint64_t x) { uint8_t v[sizeof(uint64_t)]; set64be(v, x); write(v, sizeof(v)); }
    void write32be(uint32_t x) { uint8_t v[sizeof(uint32_t)]; set32be(v, x); write(v, sizeof(v)); }
    void write16be(uint16_t x) { uint8_t v[sizeof(uint16_t)]; set16be(v, x); write(v, sizeof(v)); }
    void write8(uint8_t x) { uint8_t v[sizeof(uint8_t)]; v[0]= x; write(v, sizeof(v)); }

#if 0
    void vectorwrite8(const ByteVector& v)
    {
        write(&v.front(), v.size());
    }
    void vectorwrite32le(const DwordVector& v)
    {
        for (DwordVector::const_iterator i= v.begin() ; i!=v.end() ; ++i)
            write32le(*i);
    }
    void writestr(const std::string& str)
    {
        write((const uint8_t*)str.c_str(), str.size());
    }
    size_t vectorread32le(DwordVector& v, size_t n)
    {
        v.resize(n);
        size_t nr= read((uint8_t*)&v[0], n*sizeof(uint32_t));
        if (nr%sizeof(uint32_t))
            throw "read partial uint32_t";
        v.resize(nr/sizeof(uint32_t));
#if __BYTE_ORDER == __BIG_ENDIAN
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        std::for_each(v.begin(), v.end(), [](uint32_t& x) { x= swab32(x);});
#else
        throw "need c++0x";
#endif
#endif
        return v.size();
    }
    size_t vectorread16le(WordVector& v, size_t n)
    {
        v.resize(n);
        size_t nr= read((uint8_t*)&v[0], n*sizeof(uint16_t));
        if (nr%sizeof(uint16_t))
            throw "read partial uint16_t";
        v.resize(nr/sizeof(uint16_t));
#if __BYTE_ORDER == __BIG_ENDIAN
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        std::for_each(v.begin(), v.end(), [](uint16_t& x) { x= swab16(x);});
#else
        throw "need c++0x";
#endif
#endif
        return v.size();
    }
    size_t vectorread32be(DwordVector& v, size_t n)
    {
        v.resize(n);
        size_t nr= read((uint8_t*)&v[0], n*sizeof(uint32_t));
        if (nr%sizeof(uint32_t))
            throw "read partial uint32_t";
        v.resize(nr/sizeof(uint32_t));
#if __BYTE_ORDER == __LITTLE_ENDIAN
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        std::for_each(v.begin(), v.end(), [](uint32_t& x) { x= swab32(x);});
#else
        throw "need c++0x";
#endif
#endif
        return v.size();
    }
    size_t vectorread16be(WordVector& v, size_t n)
    {
        v.resize(n);
        size_t nr= read((uint8_t*)&v[0], n*sizeof(uint16_t));
        if (nr%sizeof(uint16_t))
            throw "read partial uint16_t";
        v.resize(nr/sizeof(uint16_t));
#if __BYTE_ORDER == __LITTLE_ENDIAN
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        std::for_each(v.begin(), v.end(), [](uint16_t& x) { x= swab16(x);});
#else
        throw "need c++0x";
#endif
#endif
        return v.size();
    }
    size_t vectorread8(ByteVector& v, size_t n)
    {
        v.resize(n);
        size_t nr= read((uint8_t*)&v[0], n);
        v.resize(nr);
        return v.size();
    }
    size_t readstr(std::string& v, size_t n)
    {
        v.resize(n);
        size_t nr= read((uint8_t*)&v[0], n);
        v.resize(nr);
        v.resize(stringlength(&v[0]));
        return v.size();
    }
    // read NUL terminated string
    std::string readstr()
    {
        std::string str;
        while(true)
        {
            str.resize(str.size()+16);
            size_t n= read((uint8_t*)&str[str.size()-16], 16);
            str.resize(str.size()-16+n);
            if (n==0)
                return str;
            size_t i0= str.find(char(0), str.size()-n);
            if (i0!=str.npos)
            {
                str.resize(i0);
                return str;
            }
        }
    }
    // note: confusing param list, you might be confused
    // to use readstr(len)   instead of  readstr(ofs,len)
    std::string readstr(uint32_t ofs, size_t n=size_t(-1))
    {
        setpos(ofs);
        if (n==size_t(-1))
            return readstr();
        else {
            std::string str;
            readstr(str, n);
            return str;
        }
    }
    void writeutf16le(const std::Wstring& v)
    {
#if __BYTE_ORDER == __BIG_ENDIAN
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        std::for_each(v.begin(), v.end(), [this](uint16_t x) { write16le(x); });
#else
        throw "need c++0x";
#endif
#else  // __LITTLE_ENDIAN
        write((const uint8_t*)v.c_str(), v.size()*sizeof(uint16_t));
#endif


    }
    size_t readutf16le(std::Wstring& v, size_t n)
    {
        v.resize(n);
        size_t nr= read((uint8_t*)&v[0], n*sizeof(uint16_t));
        if (nr%sizeof(uint16_t))
            throw "read partial uint16_t";
        v.resize(nr/sizeof(uint16_t));
        v.resize(stringlength(&v[0]));
#if __BYTE_ORDER == __BIG_ENDIAN
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        std::for_each(v.begin(), v.end(), [](uint16_t& x) { x= swab16(x);});
#else
        throw "need c++0x";
#endif
#endif

        return v.size();
    }

    size_t readutf16be(std::Wstring& v, size_t n)
    {
        v.resize(n);
        size_t nr= read((uint8_t*)&v[0], n*sizeof(uint16_t));
        if (nr%sizeof(uint16_t))
            throw "read partial uint16_t";
        v.resize(nr/sizeof(uint16_t));
        v.resize(stringlength(&v[0]));
#if __BYTE_ORDER == __LITTLE_ENDIAN
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        std::for_each(v.begin(), v.end(), [](uint16_t& x) { x= swab16(x);});
#else
        throw "need c++0x";
#endif
#endif

        return v.size();
    }
    void readall(ByteVector& data)
    {
        data.resize(size());
        size_t n= read(&data[0], data.size());
        if (n!=data.size()) {
            printf("WARNING: file size(%d) != max read(%d)\n", int(data.size()), int(n));
            data.resize(n);
        }
    }

    void copyto(ReadWriter_ptr w)
    {
        ByteVector buffer(1024*1024);
        while (1)
        {
            size_t nr= read(&buffer[0], buffer.size());
            if (nr==0)
                break;
            w->write(&buffer[0], nr);
        }
    }
    void copyto(ReadWriter_ptr w, uint64_t size)
    {
        ByteVector buffer(1024*1024);
        while (size)
        {
            size_t want= std::min(size, uint64_t(buffer.size()));
            size_t nr= read(&buffer[0], want);
            w->write(&buffer[0], nr);

            if (nr<want)
                break;
            size -= nr;
        }
    }
#endif

    // overridable direct ptr access
    virtual uint64_t read64le(uint64_t ofs) { setpos(ofs); return read64le(); }
    virtual uint32_t read32le(uint64_t ofs) { setpos(ofs); return read32le(); }
    virtual uint16_t read16le(uint64_t ofs) { setpos(ofs); return read16le(); }
    virtual uint64_t read64be(uint64_t ofs) { setpos(ofs); return read64be(); }
    virtual uint32_t read32be(uint64_t ofs) { setpos(ofs); return read32be(); }
    virtual uint16_t read16be(uint64_t ofs) { setpos(ofs); return read16be(); }
    virtual uint8_t read8(uint64_t ofs) { setpos(ofs); return read8(); }

    virtual void write64le(uint64_t ofs, uint64_t x) { setpos(ofs); write64le(x); }
    virtual void write32le(uint64_t ofs, uint32_t x) { setpos(ofs); write32le(x); }
    virtual void write16le(uint64_t ofs, uint16_t x) { setpos(ofs); write16le(x); }
    virtual void write64be(uint64_t ofs, uint64_t x) { setpos(ofs); write64be(x); }
    virtual void write32be(uint64_t ofs, uint32_t x) { setpos(ofs); write32be(x); }
    virtual void write16be(uint64_t ofs, uint16_t x) { setpos(ofs); write16be(x); }
    virtual void write8(uint64_t ofs, uint8_t x) { setpos(ofs); write8(x); }

};

typedef std::shared_ptr<ReadWriter> ReadWriter_ptr;

#endif
