#ifndef __UTIL_RW_MEMORYREADER_H_
#define __UTIL_RW_MEMORYREADER_H_
#include "util/ReadWriter.h"

class MemoryReader : public ReadWriter {
    bool _growable;
    uint8_t *_mem;
    uint64_t _size;
    uint64_t _curpos;
public:
    MemoryReader()
        : _growable(false), _mem(NULL), _size(0), _curpos(0)
    {
    }
    MemoryReader(uint8_t *mem, uint64_t size)
        : _growable(false), _mem(mem), _size(size), _curpos(0)
    {
    }
    MemoryReader(const uint8_t *mem, uint64_t size)
        : _growable(false), _mem(const_cast<uint8_t*>(mem)), _size(size), _curpos(0)
    {
        setreadonly();
    }

    void setbuf(uint8_t *mem, uint64_t size)
    {
        _mem= const_cast<uint8_t*>(mem);
        _size= size;
    }
    void setbuf(const uint8_t *mem, uint64_t size)
    {
        setbuf(const_cast<uint8_t*>(mem), size);
        setreadonly();
    }
    virtual ~MemoryReader() { }

    virtual size_t read(uint8_t *p, size_t n)
    {
        size_t want= (size_t)std::min(uint64_t(n), size()-_curpos);
        std::copy(_mem+_curpos, _mem+_curpos+want, p);
        _curpos += want;
        return want;
    }
    virtual void write(const uint8_t *p, size_t n)
    {
        if (isreadonly())
            throw "MemoryReader: readonly";
        if (_growable && _curpos+n>size())
            grow(_curpos+n-size());
        size_t want= (size_t)std::min(uint64_t(n), size()-_curpos);
        std::copy(p, p+want, _mem+_curpos);

        _curpos += want;
    }
    virtual void setpos(uint64_t off)
    {
        _curpos= off;
    }
    virtual uint64_t getpos() const
    {
        return _curpos;
    }
    virtual bool eof()
    {
        return _curpos>=size();
    }
    virtual void truncate(uint64_t off)
    {
        _size= off;
    }
    virtual uint64_t size()
    {
        return _size;
    }

    void setgrowable() { _growable= true; }
    virtual void grow(size_t n)
    {
        // by default don't grow.
    }

    // unhide overloaded methods hidden by virtuals
    //  see http://stackoverflow.com/questions/4146499/why-does-a-virtual-function-get-hidden
    //  http://stackoverflow.com/questions/3406482/c-inheriting-overloaded-non-virtual-method-and-virtual-method-both-with-the-sa
    //  http://stackoverflow.com/questions/888235/overriding-a-bases-overloaded-function-in-c/888313#888313
    //  http://stackoverflow.com/questions/3518228/overwriting-pure-virtual-functions-by-using-a-separately-inherited-method
    //  http://gotw.ca/gotw/005.htm
    //  http://gotw.ca/gotw/030.htm
    //
    using ReadWriter::read8;
    using ReadWriter::read16le;
    using ReadWriter::read32le;
    using ReadWriter::read64le;
    using ReadWriter::read16be;
    using ReadWriter::read32be;
    using ReadWriter::read64be;

    using ReadWriter::write64le;
    using ReadWriter::write32le;
    using ReadWriter::write16le;
    using ReadWriter::write64be;
    using ReadWriter::write32be;
    using ReadWriter::write16be;
    using ReadWriter::write8;



    virtual uint8_t read8(uint64_t ofs) { return get8(_mem+ofs); }
    virtual uint16_t read16le(uint64_t ofs) { return get16le(_mem+ofs); }
    virtual uint32_t read32le(uint64_t ofs) { return get32le(_mem+ofs); }
    virtual uint64_t read64le(uint64_t ofs) { return get64le(_mem+ofs); }
    virtual uint16_t read16be(uint64_t ofs) { return get16be(_mem+ofs); }
    virtual uint32_t read32be(uint64_t ofs) { return get32be(_mem+ofs); }
    virtual uint64_t read64be(uint64_t ofs) { return get64be(_mem+ofs); }


    virtual void write64le(uint64_t ofs, uint64_t x) { set64le(_mem+ofs, x); }
    virtual void write32le(uint64_t ofs, uint32_t x) { set32le(_mem+ofs, x); }
    virtual void write16le(uint64_t ofs, uint16_t x) { set16le(_mem+ofs, x); }
    virtual void write64be(uint64_t ofs, uint64_t x) { set64be(_mem+ofs, x); }
    virtual void write32be(uint64_t ofs, uint32_t x) { set32be(_mem+ofs, x); }
    virtual void write16be(uint64_t ofs, uint16_t x) { set16be(_mem+ofs, x); }
    virtual void write8(uint64_t ofs, uint8_t x) { set8(_mem+ofs, x); }


    uint8_t*begin() { return _mem; }
    uint8_t*cur() { return _mem+_curpos; }
    uint8_t*end() { return _mem+size(); }

    const uint8_t*cbegin() { return _mem; }
    const uint8_t*ccur() { return _mem+_curpos; }
    const uint8_t*cend() { return _mem+size(); }
};
#endif
