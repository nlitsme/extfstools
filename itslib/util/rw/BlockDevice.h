#ifndef _UTIL_RW_BLOCKDEVICE_H__
#define _UTIL_RW_BLOCKDEVICE_H__
#include "err/posix.h"
#ifndef _WIN32_WCE
#include <sys/stat.h>
#endif
#ifndef _WIN32
#include <sys/time.h>
#endif

#ifdef __MACH__
#ifdef __APPLE__
#include "TargetConditionals.h"
#endif
#if !TARGET_OS_IPHONE
// sys/disk.h is not in the ios sdk
#include <sys/disk.h>
#endif
#endif
#if defined(_ANDROID) || defined(__linux__)
#include <linux/fs.h>
#include <sys/time.h>
#endif
#ifndef _WIN32
#include <unistd.h>     // ftruncate
#include <sys/ioctl.h>
#endif

#include <fcntl.h>

class BlockDevice : public ReadWriter {
    int _f;
    std::string _filename;
    uint64_t _bkcount;
    uint32_t _bksize;
    uint64_t _curpos;

    uint64_t _bufferpos;
    std::vector<uint8_t> _buffer;
public:
    struct filemode_t {  };
    struct readonly_t : filemode_t { };
    struct readwrite_t : filemode_t { };
    static const readonly_t  readonly;
    static const readwrite_t readwrite;

#ifndef _WIN32
    enum { O_BINARY=0 };
#endif

    BlockDevice(const std::string& filename, readwrite_t)
        : _filename(filename)
    {
        _f= open(_filename.c_str(), O_RDWR|O_BINARY);
        if (_f==-1)
            throw posixerror(std::string("opening ")+_filename);
        initdev();
    }
    BlockDevice(const std::string& filename, readonly_t)
        : _filename(filename)
    {
        setreadonly();
        _f= open(_filename.c_str(), O_RDONLY|O_BINARY);
        if (_f==-1)
            throw posixerror(std::string("opening ")+_filename);
        initdev();
    }
    void initdev()
    {
#ifdef DKIOCGETBLOCKCOUNT
        if (-1==ioctl(_f, DKIOCGETBLOCKCOUNT, &_bkcount))
            throw posixerror("ioctl(DKIOCGETBLOCKCOUNT)");
        if (-1==ioctl(_f, DKIOCGETBLOCKSIZE, &_bksize))
            throw posixerror("ioctl(DKIOCGETBLOCKSIZE)");
#endif
#ifdef BLKGETSIZE
        uint64_t devsize;
        if (-1==ioctl(_f, BLKGETSIZE64, &devsize))
            throw posixerror("ioctl(BLKGETSIZE64)");
        if (-1==ioctl(_f, BLKBSZGET, &_bksize))
               throw posixerror("ioctl(BLKBSZGET)");
        _bkcount = devsize / _bksize;
#endif

        _curpos= 0;
        _bufferpos= 0;
    }

    virtual ~BlockDevice()
    {
        if (_f)
            close(_f);
    }
    virtual size_t read(uint8_t *p, size_t n)
    {
        size_t total= 0;
        while (n) {
            if (_curpos>=_bufferpos && _curpos<_bufferpos+_buffer.size()) {
                size_t want= std::min(n, size_t(_buffer.size()-(_curpos-_bufferpos)));

                if (want) {
                    std::copy(&_buffer[_curpos-_bufferpos], &_buffer[_curpos-_bufferpos+want], p);
                    n -= want;
                    p += want;
                    _curpos += want;
                    total += want;
                }
            }
            if (n) {
                _bufferpos= _curpos&~uint64_t(_bksize-1);
                if (-1==lseek(_f, _bufferpos, SEEK_SET))
                    throw posixerror("fseek");

                size_t bufwant= std::min(uint64_t(_bksize)*16384, _bkcount*_bksize-_bufferpos);
                _buffer.resize(bufwant);
                if (bufwant==0)
                    return total;
                size_t r= ::read(_f, &_buffer[0], _buffer.size());
                if (r==size_t(-1))
                    throw posixerror("read");
                _buffer.resize(r);
                if (r==0)
                    return total;
            }
        }
        return total;
    }
    virtual void write(const uint8_t *p, size_t n)
    {
        throw "blockdev write not supported";
    }
    virtual void setpos(uint64_t off)
    {
        _curpos= off;
    }
    virtual void truncate(uint64_t off)
    {
        throw "blockdev trunc not supported";
    }
    virtual uint64_t size()
    {
        return _bkcount*_bksize;
    }
    virtual uint64_t getpos() const
    {
        return _curpos;
    }
    virtual bool eof()
    {
        return _curpos>=size();
    }
    uint64_t getunixtime() const
    {
        struct stat st;
        if (-1==fstat(_f, &st))
            throw posixerror(std::string("statting ")+_filename);
        return st.st_mtime;
    }
    void setunixtime(uint64_t t)
    {
        throw "bloockdev settime not supported";
    }
};

#ifdef _MSC_VER
// msvc requires explicit allocation
const BlockDevice::readonly_t  BlockDevice::readonly;
const BlockDevice::readwrite_t BlockDevice::readwrite;
#endif

#endif

