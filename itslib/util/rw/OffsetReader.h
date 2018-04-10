#ifndef _UTIL_RW_OFFSETREADER_H__
#define _UTIL_RW_OFFSETREADER_H__

// very low overhead, only adds baseoffset to positions.
// does not check size.
class OffsetReader : public ReadWriter {
    ReadWriter_ptr  _r;
    uint64_t _baseoff;
    uint64_t _size;
public:
    OffsetReader(ReadWriter_ptr r, uint64_t off, uint64_t size)
        : _r(r), _baseoff(off), _size(size)
    {
        if (size > _r->size()-_baseoff) {
            printf("off=%08llx, size=%08llx,  src: %08llx\n", off, size, _r->size());
            throw "Offsetreader larger then source disk";
        }
    }
    virtual ~OffsetReader() { }
    virtual size_t read(uint8_t *p, size_t n)
    {
        return _r->read(p, n);
    }
    virtual void write(const uint8_t *p, size_t n)
    {
        return _r->write(p,n);
    }
    virtual void setpos(uint64_t off)
    {
        _r->setpos(off+_baseoff);
    }
    virtual void truncate(uint64_t off)
    {
        _r->truncate(off+_baseoff);
    }
    virtual uint64_t size()
    {
        return _size;
    }
    virtual uint64_t getpos() const
    {
        return _r->getpos()-_baseoff;
    }
    virtual bool eof()
    {
        return getpos()>=_size;
    }
};
#endif
