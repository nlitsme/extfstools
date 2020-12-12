/* (C) 2003 XDA Developers  itsme@xs4all.nl
 *
 * $Header$
 */

#ifndef __STRINGUTILS_H_

// make sure everything is compiled with '_MT' defined.
// otherwise this will not link with code that has any of the mfc
// headers included.
//    without _MT this code compiles to non multithreaded templates,
//    with it compiles to templates with threading support.

#include <string>
#include <vector>
#include <iterator>
#include "vectorutils.h"

#include "util/wintypes.h"
#include "util/endianutil.h"
#include "util/chariterators.h"


typedef std::vector<std::string> StringList;
typedef std::vector<std::Wstring> WStringList;

typedef std::vector<std::tstring> TStringList;

#if __cplusplus <= 201103L
namespace std {

#if __cplusplus < 201103L
#pragma message("Note that this code will need at least c++11")
#endif

template<typename V>
    size_t size(const V& v)
    {
        return v.size();
    }
template<typename V>
    bool empty(const V& v)
    {
        return v.empty();
    }
}
#endif


// preprocessor helper macro
#define STR$(s) XSTR$(s)
#define XSTR$(s) #s

template<typename T>
size_t stringlength(const T *str)
{
    size_t len=0;
    while (*str++)
        len++;
    return len;
}

template<typename T>
int stringcompare(const T *a, const T *b)
{
    while (*a && *b && *a == *b) 
    {
        a++;
        b++;
    }
    if (*a<*b)
        return -1;
    if (*a>*b)
        return 1;
    return 0;
}

template<typename T>
T *stringcopy(T *a, const T *b)
{
    while ((*a++ = *b++)!=0)
        ;
    return a-1;
}

template<typename T>
ByteVector ToByteVector(const T*buf)
{
    return ByteVector((const uint8_t*)buf, (const uint8_t*)(buf+stringlength(buf)*sizeof(T)));
}
template<typename T>
ByteVector ToByteVector(const T*buf, size_t n)
{
    return ByteVector((const uint8_t*)buf, (const uint8_t*)(buf+n*sizeof(T)));
}
template<typename T>
ByteVector ToByteVector(const T&buf)
{
    return ByteVector((const uint8_t*)buf.c_str(), (const uint8_t*)(buf.c_str()+buf.size()));
}


//-------------------------------------------------------
//  string conversion functions
//
template<typename T>
std::string ToString(const T* buf)
{
    std::string str;
    size_t len= stringlength(buf);
    str.reserve(len);
    std::copy(utfadaptor<sizeof(T)*8>::adapt(buf), utfadaptor<sizeof(T)*8>::adapt(buf+len),
          utf8_backinserter(back_inserter(str)));
    return str;
}
template<typename T>
std::string ToString(const T* buf, size_t len)
{
    std::string str;
    str.reserve(len);
    std::copy(utfadaptor<sizeof(T)*8>::adapt(buf), utfadaptor<sizeof(T)*8>::adapt(buf+len),
          utf8_backinserter(back_inserter(str)));
    return str;
}

template<typename T>
std::string ToString(const std::basic_string<T>& buf)
{
    std::string str;
    str.reserve(buf.size());
    std::copy(utfadaptor<sizeof(T)*8>::adapt(buf.begin()), utfadaptor<sizeof(T)*8>::adapt(buf.end()),
          utf8_backinserter(back_inserter(str)));
    return str;
}

template<typename T>
std::Wstring ToWString(const T* buf)
{
    std::Wstring str;
    size_t len= stringlength(buf);
    std::copy(utfadaptor<sizeof(T)*8>::adapt(buf), utfadaptor<sizeof(T)*8>::adapt(buf+len),
          utf16_backinserter(back_inserter(str)));
    return str;
}
template<typename T>
std::Wstring ToWString(const T* buf, size_t len)
{
    std::Wstring str;
    str.reserve(len);
    std::copy(utfadaptor<sizeof(T)*8>::adapt(buf), utfadaptor<sizeof(T)*8>::adapt(buf+len),
          utf16_backinserter(back_inserter(str)));
    return str;
}

template<typename T>
std::Wstring ToWString(const std::basic_string<T>& buf)
{
    std::Wstring str;
    str.reserve(buf.size());
    std::copy(utfadaptor<sizeof(T)*8>::adapt(buf.begin()), utfadaptor<sizeof(T)*8>::adapt(buf.end()),
          utf16_backinserter(back_inserter(str)));
    return str;
}

template<typename T>
std::tstring ToTString(const T* buf)
{
    typedef std::tstring::value_type S;

    std::tstring str;
    size_t len= stringlength(buf);
    str.reserve(len);
    std::copy(utfadaptor<sizeof(T)*8>::adapt(buf), utfadaptor<sizeof(T)*8>::adapt(buf+len),
            utfadaptor<sizeof(S)*8>::insertor(back_inserter(str)));
    return str;
}
template<typename T>
std::tstring ToTString(const T* buf, size_t len)
{
    typedef std::tstring::value_type S;

    std::tstring str;
    str.reserve(len);
    std::copy(utfadaptor<sizeof(T)*8>::adapt(buf), utfadaptor<sizeof(T)*8>::adapt(buf+len),
            utfadaptor<sizeof(S)*8>::insertor(back_inserter(str)));
    return str;
}


template<typename T>
std::tstring ToTString(const std::basic_string<T>& buf)
{
    typedef std::tstring::value_type S;

    std::tstring str;
    str.reserve(buf.size());
    std::copy(utfadaptor<sizeof(T)*8>::adapt(buf.begin()), utfadaptor<sizeof(T)*8>::adapt(buf.end()),
            utfadaptor<sizeof(S)*8>::insertor(back_inserter(str)));
    return str;
}


// trivial conversions
template<>
inline std::string ToString<char>(const char* buf)
{
    return std::string(buf);
}

template<>
inline std::string ToString<char>(const std::basic_string<char>& buf)
{
    return buf;
}

template<>
inline std::Wstring ToWString<WCHAR>(const WCHAR* buf)
{
    return std::Wstring(buf);
}

template<>
inline std::Wstring ToWString<WCHAR>(const std::basic_string<WCHAR>& buf)
{
    return buf;
}

template<>
inline std::tstring ToTString<std::tstring::value_type>(const std::tstring::value_type* buf)
{
    return std::tstring(buf);
}


template<>
inline std::tstring ToTString<std::tstring::value_type>(const std::basic_string<std::tstring::value_type>& buf)
{
    return buf;
}




//-------------------------------------------------------
//  string manipulation functions

// removes cr, lf, whitespace from end of string
template<typename T>
void chomp(std::basic_string<T>& str)
{
    for (int i=str.size()-1 ; i>=0 && isspace(str.at(i)) ; --i)
    {
        str.erase(i);
    }
}

// removes cr, lf, whitespace from end of string
template<typename T>
void chomp(T *str)
{
    T *p= str+stringlength(str)-1;

    while (p>=str && isspace(*p))
    {
        *p--= 0;
    }
}


bool SplitString(const std::string& str, StringList& strlist, bool bWithEscape= true, const std::string& separator=" \t");
bool SplitString(const std::Wstring& str, WStringList& strlist, bool bWithEscape= true, const std::Wstring& separator=(const WCHAR*)L" \t");

template<typename STRING, typename SEPSTRING>
STRING JoinStringList(const std::vector<STRING>& strlist, const SEPSTRING& sep)
{
    if (strlist.empty())
        return STRING();

    STRING sep2= sep;
    size_t size=sep2.size()*(strlist.size()-1);
    for (typename std::vector<STRING>::const_iterator i=strlist.begin() ; i!=strlist.end() ; ++i)
        size += (*i).size();
    STRING result;
    result.reserve(size);
    //debug("join(%d, '%hs')\n", strlist.size(), sep.c_str());
    for (typename std::vector<STRING>::const_iterator i=strlist.begin() ; i!=strlist.end() ; ++i)
    {
        if (!result.empty())
            result += sep2;
        result += *i;
        //debug("  added %hs\n", (*i).c_str());
    }
    return result;
}
#ifndef _NO_OLD_STRINGFORMAT
std::string stringformat(const char *fmt, ...);
std::string stringvformat(const char *fmt, va_list ap);
#endif

//int stringicompare(const std::string& a, const std::string& b);
template<typename T>
int charicompare(T a,T b)
{
    a=(T)tolower(a);
    b=(T)tolower(b);
    if (a<b) return -1;
    if (a>b) return 1;
    return 0;
}

template<class T>
int stringicompare(const T& a, const T& b)
{
    typename T::const_iterator pa= a.begin();
    typename T::const_iterator pa_end= a.end();
    typename T::const_iterator pb= b.begin();
    typename T::const_iterator pb_end= b.end();
    while (pa!=pa_end && pb!=pb_end && charicompare(*pa, *pb)==0)
    {
        pa++;
        pb++;
    }

    if (pa==pa_end && pb==pb_end)
        return 0;
    if (pa==pa_end)
        return -1;
    if (pb==pb_end)
        return 1;
    return charicompare(*pa, *pb);
}

std::string tolower(const std::string& str);

#define stringptr(v)  ((v).empty()?NULL:&(v)[0])


// utility functions for 'hexdump'
void byte2hexchars(uint8_t b, char *p);
void word2hexchars(uint16_t w, char *p);
void dword2hexchars(uint32_t d, char *p);
void qword2hexchars(uint64_t d, char *p);


template<typename I>
std::string hexstring(I buf, int nLength, char sep=0)
{
    if (nLength==0)
        return "";
    std::string str;
    str.resize(nLength*2+(sep ? (nLength-1):0));
    char *p= &str[0];
    while(nLength--)
    {
        byte2hexchars(*buf++, p); p+=2;
        if (sep && nLength)
            *p++ = sep;
    }
    return str;
}
template<typename V>
std::string hexstring(const V& v, char sep=0)
{
    return hexstring(&v[0], v.size(), sep);
}

template<typename I>
void hexdumpgroups(std::string &str, I buf, size_t nLength, int groupsize, char sep=' ')
{
    str.resize(str.size()+nLength*3);
    char *p= &str[str.size()-nLength*3];
    int i= 0;
    while(nLength--)
    {
        byte2hexchars(get8(buf), p); p+=2; buf++;

        if (i++ == groupsize-1) {
            *p++ = sep;
            i= 0;
        }
    }
}
template<typename I>
void hexdumpbytes(std::string &str, I buf, size_t nLength, char sep=' ')
{
    str.resize(str.size()+nLength*3);
    char *p= &str[str.size()-nLength*3];
    while(nLength--)
    {
        *p++ = sep;
        byte2hexchars(get8(buf), p); p+=2; buf++;
    }
}
template<typename I>
void hexdumpwords(std::string &str, I buf, size_t nLength, char sep=' ')
{
    str.resize(str.size()+nLength*5);
    char *p= &str[str.size()-nLength*5];
    while(nLength--)
    {
        *p++ = sep;
        word2hexchars(get16le(buf), p); p+=4; buf+=2;
    }
}
template<typename I>
void hexdumpdwords(std::string &str, I buf, size_t nLength, char sep=' ')
{
    str.resize(str.size()+nLength*9);
    char *p= &str[str.size()-nLength*9];
    while(nLength--)
    {
        *p++ = sep;
        dword2hexchars(get32le(buf), p); p+=8; buf+=4;
    }
}
template<typename I>
void hexdumpqwords(std::string &str, I buf, size_t nLength)
{
    str.resize(str.size()+nLength*17);
    char *p= &str[str.size()-nLength*17];
    while(nLength--)
    {
        *p++ = ' ';
        qword2hexchars(get64le(buf), p); p+=16; buf+=8;
    }
}


template<typename T>
bool isnullptr(T* p) { return p==NULL; }
template<typename T>
bool isnullptr(const T* p) { return p==NULL; }
template<typename I>
bool isnullptr(I /*p*/) { return false; }

enum EndianType { ET_BIGENDIAN, ET_LITTLEENDIAN };

// dumps bytes, shorts or longs from a uint8_t ptr + length, in one long line.
template<typename I>
std::string hexdump(I buf, int nLength, int nDumpUnitSize=1, enum EndianType et=ET_LITTLEENDIAN )
{
    if (nLength<0)
        return "hexdump-ERROR";
    if (nLength>0 && isnullptr(buf))
        return "(null)";
    if (nLength==0)
        return "";

    int nCharsInResult= nLength*(nDumpUnitSize==1?3:
                                 nDumpUnitSize==2?5:
                                 nDumpUnitSize==4?9:
                                 nDumpUnitSize==8?17:
                                 9);
    std::string line;

    line.reserve(nCharsInResult);

    if (et==ET_LITTLEENDIAN)
        switch(nDumpUnitSize)
        {
            case 1: hexdumpbytes(line, buf, nLength); break;
            case 2: hexdumpwords(line, buf, nLength); break;
            case 4: hexdumpdwords(line, buf, nLength); break;
            case 8: hexdumpqwords(line, buf, nLength); break;
        }
    else {
        hexdumpgroups(line, buf, nLength*nDumpUnitSize, nDumpUnitSize);
    }
    return line;
}
// dumps bytes, shorts, longs from a bytevector.
// in one long line, without offsets printed
inline std::string hexdump(const ByteVector& buf, int nDumpUnitSize=1)
{
    return hexdump(vectorptr(buf), buf.size()/nDumpUnitSize, nDumpUnitSize);
}


std::string hexdump(int64_t llOffset, const uint8_t *buf, int nLength, int nDumpUnitSize=1, int nMaxUnitsPerLine=16);

template<typename T>
std::string vhexdump(const T& buf)
{
    return hexdump((const uint8_t*)&buf[0], buf.size(), sizeof(typename T::value_type));
}

template<typename T>
std::string vhexdump(const T& buf, int nDumpUnitSize)
{
    return hexdump((const uint8_t*)&buf[0], (sizeof(typename T::value_type)*buf.size())/nDumpUnitSize, nDumpUnitSize);
}
template<typename R>
std::string rhexdump(R range)
{
    return hexdump(std::begin(range), std::size(range), sizeof(*std::begin(range)));
}

// produce exact representation of data, representing nonprintable characters
//  escaped or as hex dumps
std::string ascdump(const uint8_t *first, size_t size, const std::string& escaped= "", bool bBreakOnEol= false);

template<typename V>
inline std::string ascdump(const V& buf, const std::string& escaped= "", bool bBreakOnEol= false)
{
    return ascdump((const uint8_t*)&buf[0], buf.size(), escaped, bBreakOnEol);
}

// replaces nonprintable with dots
template<typename P>
std::string asciidump(P buf, size_t bytelen)
{
    std::string str;
    str.reserve(bytelen);
    while(bytelen--)
    {
        uint8_t c= *buf++;
        str += (c>=' ' && c<='~')?c:'.';
    }

    return str;
}


std::string hash_as_string(const ByteVector& hash);
std::string GuidToString(const GUID *guid);

// ========================================================================================
// base64 functions
template<typename PSTR>
void base64_encode_chunk(const int*chunk, PSTR enc)
{
    // note: facebook, youtube use a modified version with  tr "+/"  "-_"
    static const char*b642char= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    int c0= chunk[0]>>2;
    int c1= (chunk[0]&3)<<4;
    int c2= 0;
    int c3= 0;
    if (chunk[1]>=0) { c1 |= (chunk[1]&0xf0)>>4;  c2 |= (chunk[1]&0x0f)<<2; } else { c2=64; }
    if (chunk[2]>=0) { c2 |= (chunk[2]&0xc0)>>6;  c3 |= chunk[2]&0x3f; } else { c3=64; }

    *enc++= b642char[c0];
    *enc++= b642char[c1];
    *enc++= b642char[c2];
    *enc++= b642char[c3];
}

template<typename P>
std::string base64_encode(P data, size_t n)
{
    P p= data;
    P pend= data+n;
    size_t b64size= int((n+2)/3)*4;
    std::string b64; b64.resize(b64size);

    std::string::iterator pout= b64.begin();
    std::string::iterator outend= b64.end();

    int chunk[3];

    for (unsigned j=0 ; p<pend ;  j+=4)
    while (pout < outend)
    {
        chunk[0]= (p<pend) ? *p++ : -1;
        chunk[1]= (p<pend) ? *p++ : -1;
        chunk[2]= (p<pend) ? *p++ : -1;
        base64_encode_chunk(chunk, pout);

        pout += 4;
    }
    return b64;
}
template<typename R>
std::string base64_encode(const R& data)
{
    if (std::empty(data))
        return "";
    return base64_encode((const uint8_t*)&data[0], data.size());
}

ByteVector base64_decode(const std::string& str);


template<typename T>
int char2nyble(T c)
{
    if (c<'0') return -1;
    if (c<='9') return c-'0';
    if (c<'A') return -1;
    if (c<='Z') return 10+c-'A';
    if (c<'a') return -1;
    if (c<='z') return 10+c-'a';
    return -1;
}

// parse unsigned int of specified base
// don't care about where parsing ends.
template<typename I>
uint64_t parseuint(I first, I last, int base)
{
    I i= first;
    while (i!=last && isspace(*i))
        i++;
    uint64_t val= 0;
    while (i!=last)
    {
        int x= char2nyble(*i);
        if (x<0 || x>=base)
            return val;
        val *= base;
        val += x;

        i++;
    }
    return val;
}
// parse (un)signed int of specified base
// return iterator where parsing ends.
template<typename I, typename V>
I parseint(I first, I last, int base, V& val)
{
    I i= first;
    while (i!=last && isspace(*i))
        i++;

    bool negative= false;
    if (i!=last && *i=='-') {
        negative= true;
        i++;
    }
    val= 0;
    while (i!=last)
    {
        int x= char2nyble(*i);
        if (x<0 || x>=base)
            break;
        val *= base;
        val += x;

        i++;
    }
    if (negative)
        val = -val;

    return i;
}
template<typename P>
std::pair<uint64_t, P> parseunsigned(P first, P last, int base)
{
    int state = 0;
    uint64_t num = 0;
    auto p = first;
    while (p<last)
    {
        int n = char2nyble(*p);
        if (base==0) {
            if (state==0) {
                if (1<=n && n<=9) {
                    base = 10;
                    num = n;  // is first real digit
                }
                else if (n==0) {
                    // expect 0<octal>, 0b<binary>, 0x<hex>
                    state++;
                }
                else {
                    // invalid
                    state = 2;
                    break;
                }
            }
            else if (state==1) {
                if (*p == 'x') {
                    base = 16;
                }
                else if (*p == 'b') {
                    base = 2;
                }
                else if (0<=n && n<=7) {
                    base = 8;
                    num = n;  // is first real digit
                }
                else if (n>=8) {
                    // invalid
                    state = 2;
                    break;
                }
            }
            else {
                // invalid
                state = 2;
                break;
            }
        }
        else {
            if (n<0 || n>=base) {
                // end of number
                break;
            }
            num *= base;
            num += n;
        }

        ++p;
    }
    if (state==2)
        return std::make_pair(0, first);

    // todo: currently  "0x" and "0b" are valid numbers too.
    return std::make_pair(num, p);
}
template<typename P>
std::pair<int64_t, P> parsesigned(P first, P last, int base)
{
    auto p = first;
    bool negative = false;
    if (*p == '-') {
        negative = true;
        ++p;
    }
    auto res = parseunsigned(p, last, base);
    if (res.second == p)
        return std::make_pair(0, first);

    uint64_t num = negative ? -res.first : res.first;

    return std::make_pair(num, res.second);
}

template<typename P>
std::pair<uint64_t, const P*> parseunsigned(const P* str, int base)
{
    return parseunsigned(str, str+stringlength(str), base);
}


template<typename P>
std::pair<int64_t, const P*> parsesigned(const P* str, int base)
{
    return parsesigned(str, str+stringlength(str), base);
}

template<typename T>
std::pair<uint64_t, typename T::const_iterator> parseunsigned(const T& str, int base)
{
    return parseunsigned(std::begin(str), std::end(str), base);
}

template<typename T>
std::pair<uint64_t, typename T::const_iterator> parsesigned(const T& str, int base)
{
    return parsesigned(std::begin(str), std::end(str), base);
}



template<typename P1,typename P2>
size_t hex2binary(P1 strfirst, P1 strlast, P2 first, P2 last)
{
    int shift=0;
    typename std::iterator_traits<P2>::value_type word=0;
    P2 o= first;
    for (P1 i= strfirst ; i!=strlast && o!=last ; i++)
    {
        int n= char2nyble(*i);
        if (n>=0 && n<16) {
            if (shift==0) {
                word= typename std::iterator_traits<P2>::value_type(n);
                shift+=4;
            }
            else {
                word<<=4;
                word|= n;
                shift+=4;
            }
            if (shift==sizeof(word)*8) {
                *o++= word;

                shift=0;
            }
        }
    }
    return o-first;
}

template<typename C,typename T>
void hex2binary(const std::basic_string<C>& hex, std::vector<T>& v)
{
    v.resize(hex.size()/2/sizeof(T));
    size_t n= hex2binary(hex.begin(), hex.end(), v.begin(), v.end());
    v.resize(n);
}

template<typename P,typename T>
void hex2binary(P first, P last, std::vector<T>& v)
{
    v.resize((last-first)/2/sizeof(T));
    size_t n= hex2binary(first, last, v.begin(), v.end());
    v.resize(n);
}


void binary2hex(std::string &str, const uint8_t *buf, int nLength);

template<typename C,typename T>
void hex2binary(const C*hex, std::vector<T>& v)
{
    hex2binary(std::basic_string<C>(hex), v);
}

std::string utf8forchar(WCHAR c);


#ifndef _NO_OLD_STRINGFORMAT
template<typename S>
S cstrunescape(const S& str)
{
    S result;
    size_t pos= 0;
    bool bEscaped= false;
    uint64_t val;
    while (pos<str.size())
    {
        if (bEscaped)
        {
            switch(str[pos])
            {
                case 'a': result+='\a'; break;  // 0x07 alert
                case 'b': result+='\b'; break;  // 0x08 backspace
                case 't': result+='\t'; break;  // 0x09 horizontal tab
                case 'n': result+='\n'; break;  // 0x0a linefeed
                case 'v': result+='\v'; break;  // 0x0b vertical tab
                case 'f': result+='\f'; break;  // 0x0c formfeed
                case 'r': result+='\r'; break;  // 0x0d carriage return
                case 'x': 
                          val= 0;
                          ++pos;
                          while (pos<str.size())
                          {
                              int x= char2nyble(str[pos]);
                              if (x==-1 || x>=16) {
                                  pos--;
                                  break;
                              }
                              val <<= 4;
                              val |= x;
                              pos++;
                          }
                          result += (typename S::value_type)val; 
                          break;
                case '0':
                case '1':
                case '2':
                          {
                          val= char2nyble(str[pos]);
                          ++pos;
                          int ndigits=1;
                          while (pos<str.size() && ndigits<3)
                          {
                              int x= char2nyble(str[pos]);
                              if (x==-1 || x>=8) {
                                  pos--;
                                  break;
                              }
                              val <<= 3;
                              val |= x;
                              pos++;
                              ndigits++;
                          }
                          result += (typename S::value_type)val;  
                          break;
                          }
                default:
                          result+=str[pos];
            }
            bEscaped= false;
        }
        else if (str[pos]=='\\')
        {
            bEscaped= true;
        }
        else {
            result += str[pos];
        }
        ++pos;
    }

    return result;
}
template<typename S>
S cstrescape(const S& str)
{
    S ostr; ostr.reserve(str.size()+(str.size()>>6));
    for (typename S::const_iterator i= str.begin() ; i!=str.end() ; ++i)
    {
        typename S::value_type c= *i;
        if (unsigned(c)<unsigned(' ') || c=='"' || c=='\\') switch(c) {
            case '\n': ostr += "\\n"; break;
            case '\r': ostr += "\\r"; break;
            case '\t': ostr += "\\t"; break;
            case '\\': ostr += "\\\\"; break;
            case '"': ostr += "\\\""; break;
            default: ostr += stringformat("\\x%02x", unsigned(c)&0xff);
        }
        else {
            ostr += c;
        }
    }
    return ostr;
}
#endif

#define __STRINGUTILS_H_
#endif
