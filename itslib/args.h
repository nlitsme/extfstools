/* (C) 2003 XDA Developers  itsme@xs4all.nl
 *
 * $Header$
 */
#ifndef __ARGS_H__

#ifndef _WIN32
#define _strtoi64 strtoll
#endif
#define HANDLEULOPTION(var, type) (argv[i][2] ? var= (type)strtoul(argv[i]+2, 0, 0) : i+1<argc ? var= (type)strtoul(argv[++i], 0, 0) : 0)
#define HANDLELLOPTION(var, type) (argv[i][2] ? var= (type)_strtoi64(argv[i]+2, 0, 0) : i+1<argc ? var= (type)_strtoi64(argv[++i], 0, 0) : 0)
#define HANDLESTROPTION(var) (argv[i][2] ? var= argv[i]+2 : (i+1<argc) ? var= argv[++i] : 0)

// use these when arg is a ptr to the arg string.
#define HANDLEULOPTION2(var, type) (*arg ? var= (type)strtoul(arg, 0, 0) : i+1<argc ? var= (type)strtoul(argv[++i], 0, 0) : 0)
#define HANDLESTROPTION2(var) (*arg ? var= arg : i+1<argc ? var= argv[++i] : 0)

// use these when args is a StringList
#define HANDLESTLSTRCOMBIOPTION(var) (arg[2] ? !(var= arg.substr(2)).empty() : i+1 != args.end() ? !(var= *(++i)).empty() : 0)
#define HANDLESTLSTROPTION(var) (i+1 != args.end() ? !(var= *(++i)).empty() : 0)
#define HANDLESTLULOPTION(var,type) (arg[2] ? var= (type)strtoul(ToString(arg.substr(2)).c_str(),0,0) : i+1 != args.end() ? var= (type)strtoul(ToString(*(++i)).c_str(),0,0) : 0)
#define HANDLESTLLLOPTION(var,type) (arg[2] ? var= (type)_strtoi64(ToString(arg.substr(2)).c_str(),0,0) : i+1 != args.end() ? var= (type)_strtoi64(ToString(*(++i)).c_str(),0,0) : 0)

#include <string>
#include <stdlib.h>
#include "vectorutils.h"
#include "stringutils.h"

template<typename T>
void parsearg(const char*arg, T& value)
{
    // 2 bugs:  unsigned values get truncated
    //          pointers cannot be static casted

    value= static_cast<T>(_strtoi64(arg, 0, 0));
}

template<>
void parsearg<std::string>(const char*arg, std::string& value)
{
    value= arg;
}
template<>
void parsearg<ByteVector>(const char*arg, ByteVector& value)
{
    hex2binary(std::string(arg), value);
}
template<typename ARG, typename V>
void parsecsvarg(const ARG& arg, V& list)
{
  typedef typename V::value_type value_type;
    auto p = std::begin(arg);
    auto pend = std::end(arg);
    while (p<pend && *p) {
        char *q;
        auto res = parseunsigned(p, pend, 0);
        if (res.second == p)
            break;
        value_type value= static_cast<value_type>(res.first);
        list.push_back(value);
        // note: not actually checking for comma.
        p= q+1;
    }
}


//////////////////////////////////////////////////////////////////////////////
// void getarg(..., T& value)   style functions
//////////////////////////////////////////////////////////////////////////////
template<typename T>
void getarg(char **argv, int& i, int argc, T& value)
{
    if (argv[i][2]) {
        parsearg(argv[i]+2, value);
    }
    else if (++i<argc) {
        parsearg(argv[i], value);
    }
    else {
        throw "missing arg";
    }
}
template<typename T>
void getarg(StringList::iterator &i, StringList::iterator end, T& value)
{
    if ((*i).size()>2 && (*i).c_str()[2]) {
        parsearg((*i).c_str()+2, value);
    }
    else if (++i!=end) {
        parsearg((*i).c_str(), value);
    }
    else {
        throw "missing arg";
    }
}


//////////////////////////////////////////////////////////////////////////////
// T get<type>arg(argv,i,argc)   style functions
//////////////////////////////////////////////////////////////////////////////

inline std::string getstrarg(char **argv, int& i, int argc)
{
    std::string str;
    getarg(argv, i, argc, str);
    return str;
}
inline uint64_t getuintarg(char **argv, int& i, int argc)
{
    uint64_t val;
    getarg(argv, i, argc, val);
    return val;
}
inline int64_t getintarg(char **argv, int& i, int argc)
{
    int64_t val;
    getarg(argv, i, argc, val);
    return val;
}
inline int countoptionmultiplicity(char **argv, int& i, int argc)
{
    char *p= argv[i];
    if (*p++ != '-')
        return 0;
    char c= *p++;
    if (c==0)
        return 0;
    while (*p++ == c)
        ;
    return p-argv[i]-2;
}
inline int countoptionmultiplicity(const std::string& arg)
{
    if (arg.size()<2)
        return 0;
    if (arg[0]!='-')
        return 0;
    char c= arg[1];
    int count= 0;
    for (std::string::const_iterator i= arg.begin()+1 ; i!=arg.end() && *i==c ; ++i)
        count++;
    return count;
}

/* todo: implement option counter, for these:

==> /Users/itsme/cvsprj/xda-devtools//dumprom/cpp_editimgfs.cpp <==
        else if (arg.size()>=2 && arg[0]=='-' && arg[1]=='v') { g_verbose+=std::count(arg.begin()+1, arg.end(), 'v');

==> /Users/itsme/cvsprj/xda-devtools//itsutils/src/ppostmsg.cpp <==
        case 'v': for (int j=1 ; argv[i][j]=='v' ; j++) g_verbose++; break;

==> /Users/itsme/cvsprj/xda-devtools//itsutils/wifitools/rc4crack.cpp <==
        case 'v': for (char *p= argv[i]+1 ; *p=='v' ; p++) g_verbose++; break;

*/
#define __ARGS_H__
#endif
