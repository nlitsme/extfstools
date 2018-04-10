#ifndef _ERR_POSIX_H__
#define _ERR_POSIX_H__
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <string>
class posixerror {
    std::string _msg;
    int _err;
public:
    posixerror(const std::string& msg) : _msg(msg), _err(errno)
    {
    }
    ~posixerror() { fprintf(stderr,"ERROR (%d)%s: %s\n", _err, strerror(_err), _msg.c_str()); }
};
#endif
