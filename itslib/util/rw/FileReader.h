#ifndef _UTIL_RW_FILEREADER_H__
#define _UTIL_RW_FILEREADER_H__
#include "err/posix.h"
#ifndef _WIN32_WCE
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <sys/utime.h>
#endif
#endif
#ifndef _WIN32
#include <sys/time.h>
#endif

#ifdef __MACH__
#include <sys/disk.h>
#endif
#ifdef __linux__
#include <linux/fs.h>
#endif
#ifndef _WIN32
#include <sys/ioctl.h>
#endif

#include "util/ReadWriter.h"

class FileReader : public ReadWriter {
    FILE *_f;
    std::string _filename;
    enum { STATE_READING, STATE_WRITING, STATE_FLUSHED } _state;
public:
    struct filemode_t {  };
    struct opencreate_t : filemode_t { };      // open file, create if did not exist
    struct createnew_t : filemode_t { };       // create new file, error if exists
    struct readonly_t : filemode_t { };        // open existing file for reading only
    struct readwrite_t : filemode_t { };       // open existing file for read/write
    static const opencreate_t    opencreate;
    static const createnew_t    createnew;
    static const readonly_t  readonly;
    static const readwrite_t readwrite;

    FileReader(const std::string& filename, readwrite_t)
        : _filename(filename), _state(STATE_FLUSHED)
    {
        _f= fopen(_filename.c_str(), "r+b");
        if (_f==NULL)
            throw posixerror(std::string("opening ")+_filename);
        //printf("readwrite file: %p %s\n", _f, filename.c_str());
    }
    // ignore size arg, for compatibility with MmapReader
    FileReader(const std::string& filename, readwrite_t, uint64_t)
        : _filename(filename), _state(STATE_FLUSHED)
    {
        _f= fopen(_filename.c_str(), "r+b");
        if (_f==NULL)
            throw posixerror(std::string("opening ")+_filename);
        //printf("readwrite file: %p %s\n", _f, filename.c_str());
    }
    FileReader(const std::string& filename, readonly_t)
        : _filename(filename), _state(STATE_READING)
    {
        setreadonly();

        _f= fopen(_filename.c_str(), "rb");
        if (_f==NULL)
            throw posixerror(std::string("opening ")+_filename);
        //printf("readonly file: %p %s\n", _f, filename.c_str());
    }
    FileReader(const std::string& filename, createnew_t)
        : _filename(filename), _state(STATE_FLUSHED)
    {
        _f= fopen(_filename.c_str(), "w+b");
        if (_f==NULL)
            throw posixerror(std::string("creating ")+_filename);
        //printf("create  file: %p %s\n", _f, filename.c_str());
    }

    // ignore size arg, for compatibility with MmapReader
    FileReader(const std::string& filename, createnew_t, uint64_t /*size*/)
        : _filename(filename), _state(STATE_FLUSHED)
    {
        _f= fopen(_filename.c_str(), "w+b");
        if (_f==NULL)
            throw posixerror(std::string("creating ")+_filename);
        //printf("create  file: %p %s\n", _f, filename.c_str());
    }

    // note: with mode 'a+'  fseek does not work!!

    FileReader(const std::string& filename, opencreate_t)
        : _filename(filename), _state(STATE_FLUSHED)
    {
        _f= fopen(_filename.c_str(), "r+b");
        if (_f==NULL) {
            _f= fopen(_filename.c_str(), "w+b");
            if (_f==NULL)
                throw posixerror(std::string("appending ")+_filename);
        }
        //printf("create  file: %p %s\n", _f, filename.c_str());
    }

    // ignore size arg, for compatibility with MmapReader
    FileReader(const std::string& filename, opencreate_t, uint64_t /*size*/)
        : _filename(filename), _state(STATE_FLUSHED)
    {
        _f= fopen(_filename.c_str(), "r+b");
        if (_f==NULL) {
            _f= fopen(_filename.c_str(), "w+b");
            if (_f==NULL)
                throw posixerror(std::string("appending ")+_filename);
        }
        //printf("create  file: %p %s\n", _f, filename.c_str());
    }

    virtual ~FileReader()
    {
        if (_f)
            fclose(_f);
    }
    virtual size_t read(uint8_t *p, size_t n)
    {
        if (_state==STATE_WRITING)
            flush();
        _state= STATE_READING;
        size_t r= fread(p, 1, n, _f);
        if (r<n && ferror(_f))
            throw posixerror(std::string("reading ")+_filename);
        return r;
    }
    virtual void write(const uint8_t *p, size_t n)
    {
        if (_state==STATE_READING)
            flush();
        _state= STATE_WRITING;
        //printf("writing %p  [%p, %x]\n", _f, p, n);
        size_t r= fwrite(p, 1, n, _f);
        if (r<n)
            throw posixerror(std::string("writing ")+_filename);
    }
    virtual void setpos(uint64_t off)
    {
#if defined(_WIN32) && !defined(_WIN32_WCE)
        if (-1==_fseeki64(_f, off, SEEK_SET))
            throw posixerror(std::string("seeking ")+_filename);
#else
        if (-1==fseek(_f, off, SEEK_SET))
            throw posixerror(std::string("seeking ")+_filename);
#endif
        _state= STATE_FLUSHED;
    }
    virtual void truncate(uint64_t off)
    {
#ifdef _WIN32_WCE
        throw "truncate not supported under wince";
#elif defined(_WIN32)
        if (-1==_chsize_s(fileno(_f), off))
            throw posixerror(std::string("truncating ")+_filename);
#else
        if (-1==ftruncate(fileno(_f), off)) {
            printf("err=%d\n", errno);
            throw posixerror(std::string("truncating ")+_filename);
        }
#endif
    }
    void flush()
    {
        _state= STATE_FLUSHED;
        if (-1==fflush(_f))
            throw posixerror(std::string("flushing ")+_filename);
    }
    virtual uint64_t size()
    {
#ifdef _WIN32_WCE
        throw "file::size not supported under wince";
#else
        // if we don't fflush, there may be unwritten bytes in the file buffer
        // which are not counted in the size yet
        if (!isreadonly())
            flush();

        int h= fileno(_f);
        struct stat data;
        if (-1==fstat(h, &data))
            throw posixerror("fstat");
        if (data.st_mode&S_IFREG) {
            return data.st_size;
        }
#if !defined(_WIN32) && !defined(__FreeBSD__)
        else if (data.st_mode&S_IFBLK) {
#ifdef __MACH__
            uint64_t bkcount;
            uint32_t bksize;
            if (-1==ioctl(h, DKIOCGETBLOCKCOUNT, &bkcount))
                throw posixerror("ioctl(DKIOCGETBLOCKCOUNT)");
            if (-1==ioctl(h, DKIOCGETBLOCKSIZE, &bksize))
                throw posixerror("ioctl(DKIOCGETBLOCKSIZE)");
            return bkcount*bksize;
#else
            uint64_t devsize;
            if (-1==ioctl(h, BLKGETSIZE64, &devsize))
                throw posixerror("ioctl(BLKGETSIZE64)");
            return devsize;
#endif
        }
#endif
        else {
            throw "not a file or blockdev";
        }
#endif
    }
    virtual uint64_t getpos() const
    {
#ifdef _WIN32
        return _ftelli64(_f);
#else
        return ftello(_f);
#endif
    }
    virtual bool eof()
    {
        return feof(_f);
    }

    uint64_t getunixtime() const {

#ifdef _WIN32_WCE
        throw "file::mtime not supported under wince";
#else
        struct stat st;
        if (-1==fstat(fileno(_f), &st))
            throw posixerror(std::string("statting ")+_filename);
        return st.st_mtime;
#endif
    }
    void setunixtime(uint64_t t)
    {
        // note: if we don't flush, the timestamp may be changed again when the file is really closed
        if (!isreadonly() && -1==fflush(_f))
            throw posixerror(std::string("flushing ")+_filename);

#ifdef _WIN32_WCE
        throw "file::setunixtime not supported on wince";
#elif defined(_WIN32)
        struct _utimbuf times;
        times.actime= t;
        times.modtime= t;
        if (-1==_futime(fileno(_f),&times))
            throw posixerror(std::string("f setting file time: ")+_filename);
#else
        timeval times[2];
        times[0].tv_sec = times[1].tv_sec = t;
        times[0].tv_usec = times[1].tv_usec = 0;
        if (-1==futimes(fileno(_f), times))
            throw posixerror(std::string("f setting file time: ")+_filename);
#endif
    }
    static bool isfile(const std::string& fname)
    {
        struct stat st;
        if (-1==stat(fname.c_str(), &st))
            throw posixerror(std::string("statting ")+fname);
        return st.st_mode&S_IFREG;
    }
    static bool isdir(const std::string& fname)
    {
        struct stat st;
        if (-1==stat(fname.c_str(), &st))
            throw posixerror(std::string("statting ")+fname);
        return st.st_mode&S_IFDIR;
    }
    static bool isblockdev(const std::string& fname)
    {
#ifndef _WIN32
        struct stat st;
        if (-1==stat(fname.c_str(), &st))
            throw posixerror(std::string("statting ")+fname);
        return st.st_mode&S_IFBLK;
#else
        return false;
#endif
    }
};
typedef boost::shared_ptr<FileReader> FileReader_ptr;
#ifdef _MSC_VER
// msvc requires explicit allocation
const FileReader::opencreate_t    FileReader::opencreate;
const FileReader::createnew_t    FileReader::createnew;
const FileReader::readonly_t  FileReader::readonly;
const FileReader::readwrite_t FileReader::readwrite;
#endif

#endif
