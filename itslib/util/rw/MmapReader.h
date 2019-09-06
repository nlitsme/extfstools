#ifndef _UTIL_RW_MMAPREADER_H__
#define _UTIL_RW_MMAPREADER_H__

#include <sys/stat.h>
#ifdef _WIN32
#include <sys/utime.h>
#else
#include <sys/time.h>
#endif

#ifdef TARGET_OS_MAC
#include <sys/disk.h>
#endif
#ifdef __linux__
#include <linux/fs.h>
#endif
#ifndef _WIN32
#include <sys/ioctl.h>
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L 
// http://gcc.1065356.n8.nabble.com/PR-89864-gcc-fails-to-build-bootstrap-with-XCode-10-2-td1578565.html
#  define _Atomic volatile 
#endif 
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/anonymous_shared_memory.hpp>
#include "util/rw/MemoryReader.h"
#include "util/endianutil.h"
#include "err/posix.h"
#include "util/rw/FileReader.h"
namespace ipc= boost::interprocess;

// note: mmapreader will throw an interprocess_exception(size_error) when the file to
// be opened is empty.
class MmapReader : public MemoryReader {
    std::string _filename;

    // helper object, used to resize the file 
    // before the filemap object is created
    // or after the filemap object is destroyed.
    struct file_helper {
        std::string fname;
        uint64_t tsize;
        bool trunc;
        file_helper()
            : tsize(0), trunc(false)
        {
        }
        file_helper(const std::string& name, uint64_t size, bool creatnew, bool opencreate)
            : tsize(0), trunc(false)
        {
            if (creatnew) {
                FileReader w(name, FileReader::createnew);
                //w.truncate(size);
            }
            else if (opencreate) {
                FileReader w(name, FileReader::opencreate);
                //w.truncate(size);
            }
            else {
                FileReader w(name, FileReader::readwrite);
                //w.truncate(size);
            }
        }

        void truncate(const std::string&name, uint64_t size)
        {
            fname= name;
            tsize= size;
            trunc= true;
        }

        ~file_helper()
        {
            if (trunc) {
                FileReader w(fname, FileReader::readwrite);
                w.truncate(tsize);
            }
        }
    };
    file_helper _help;

    ipc::file_mapping _file;
    ipc::mapped_region _region;

    // no default, copy constructor
    MmapReader() { }
    MmapReader(const MmapReader&) { }
public:

    // so we can use make_range
    typedef const uint8_t* const_pointer;

    struct filemode_t {  };
    struct opencreate_t : filemode_t { opencreate_t() { } };
    struct createnew_t : filemode_t { createnew_t() { } };
    struct readonly_t : filemode_t { readonly_t() { }};
    struct readwrite_t : filemode_t { readwrite_t() { } };

    // somehow shared_ptr<MmapReader>(new MmapReader(file, MmapReader::readonly)) does work,
    // while  make_shared<MmapReader>(file, MmapReader::readonly)                 does not work
    // while  make_shared<MmapReader>(file, MmapReader::readonly_t())             does work
    //   -> missing symbol MmapReader::readonly
    static const opencreate_t    opencreate;
    static const createnew_t    createnew;
    static const readonly_t  readonly;
    static const readwrite_t readwrite;

    MmapReader(const std::string& filename, readwrite_t)
        : _filename(filename), _file(filename.c_str(), ipc::read_write), _region(_file, ipc::read_write)
    {
        setbuf((uint8_t*)_region.get_address(), _region.get_size());
    }
    MmapReader(const std::string& filename, readwrite_t, uint64_t size)
        : _filename(filename), 
        _help(filename, size, false, false),
        _file(filename.c_str(), ipc::read_write), 
        _region(_file, ipc::read_write, 0, size)
    {
        setbuf((uint8_t*)_region.get_address(), _region.get_size());
    }

    MmapReader(const std::string& filename, createnew_t, uint64_t size)
        : _filename(filename), 
          _help(filename, size, true, false),
          _file(filename.c_str(), ipc::read_write),
          _region(_file, ipc::read_write, 0, size)
    {
        setbuf((uint8_t*)_region.get_address(), _region.get_size());
    }

    MmapReader(const std::string& filename, opencreate_t, uint64_t size)
        : _filename(filename), 
          _help(filename, size, false, true),
          _file(filename.c_str(), ipc::read_write),
          _region(_file, ipc::read_write, 0, size)
    {
        setbuf((uint8_t*)_region.get_address(), _region.get_size());
    }

    // note: will fail for empty file!!
    MmapReader(const std::string& filename, readonly_t)
        : _filename(filename), _file(filename.c_str(), ipc::read_only), _region(_file, ipc::read_only)
    {
        setreadonly();

        setbuf((uint8_t*)_region.get_address(), _region.get_size());
    }

    // create an anonymous fixed size mmapped file ( basically a large malloc )
    MmapReader(uint64_t size)
        : _region(ipc::anonymous_shared_memory(size))
    {
        setgrowable();
        setbuf((uint8_t*)_region.get_address(), _region.get_size());
    }
    virtual ~MmapReader()
    {
        if (!isreadonly())
            _help.truncate(_filename, size());
    }
#ifndef _WIN32
    void advise(ipc::mapped_region::advice_types adv)
    {
        _region.advise(adv);
    }
#endif
    virtual void grow(size_t n)
    {
        ipc::mapped_region newregion(ipc::anonymous_shared_memory(size()*2));
        memcpy(newregion.get_address(), _region.get_address(), _region.get_size());

        _region.swap(newregion);
        setbuf((uint8_t*)_region.get_address(), _region.get_size());
    }
    uint64_t getunixtime() const {
        if (_filename.empty())
            throw "can't get time for anonymous mmap";
        struct stat st;
        if (-1==stat(_filename.c_str(), &st))
            throw posixerror(std::string("statting ")+_filename);
        return st.st_mtime;
    }
    void setunixtime(uint64_t t)
    {
        if (_filename.empty())
            throw "can't set time for anonymous mmap";
#ifdef _WIN32
        struct _utimbuf times;
        times.actime= t;
        times.modtime= t;
        if (-1==_utime(_filename.c_str(),&times))
            throw posixerror(std::string("m setting file time: ")+_filename);
#else
        timeval times[2];
        times[0].tv_sec = times[1].tv_sec = t;
        times[0].tv_usec = times[1].tv_usec = 0;
        if (-1==utimes(_filename.c_str(), times))
            throw posixerror(std::string("m setting file time: ")+_filename);
#endif
    }
#ifndef __GXX_EXPERIMENTAL_CXX0X__
    static bool iscrorlf(char c) { return c==10 || c==13; }
#endif

    // special algorithm enumerating text lines
    // exit enumerator by returning false
    // the linefn is called with ptr to first char and ptr to first eol char.
    template<typename linefn>
    void line_enumerator(linefn f)
    {
        char *curpos= (char*)cur();
        char *endpos= (char*)end();
        while (curpos < endpos) {
#ifdef __GXX_EXPERIMENTAL_CXX0X__
            char *eoln= std::find_if(curpos, endpos, [](char c) { return c==10 || c==13; });
#else
            char *eoln= std::find_if(curpos, endpos, iscrorlf);
#endif
            if (!f(curpos, eoln))
                break;
            if (eoln>=endpos)
                break;

            // skip eoln :  /\r*\n/
            while (eoln<endpos && *eoln==13)
                eoln++;
            if (eoln>=endpos)
                break;
            if (eoln<endpos && *eoln==10)
                eoln++;

            curpos= eoln;
        }
    }

    std::string name() const { return _filename; }
};
#ifdef _MSC_VER
// msvc requires explicit allocation
const MmapReader::opencreate_t    MmapReader::opencreate;
const MmapReader::createnew_t    MmapReader::createnew;
const MmapReader::readonly_t  MmapReader::readonly;
const MmapReader::readwrite_t MmapReader::readwrite;
#endif


#endif
