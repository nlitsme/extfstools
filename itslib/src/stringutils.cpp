/* (C) 2003 XDA Developers  itsme@xs4all.nl
 *
 * $Header: /var/db/cvs/xda-devtools/itsutils/common/stringutils.cpp,v 1.12 2005/06/12 22:52:04 itsme Exp $
 */

#include <util/wintypes.h>

// todo: add optional length to , length==-1 -> use stringlength
// ToString(const chartype* p, size_t length /*=-1*/) { ... conversion code ... }
// to 
#include "stringutils.h"
#ifdef __GNUC__
extern "C" {
int strcasecmp(const char *, const char *);
}
#endif
//#include "debug.h"
#ifdef _MSC_VER
#define va_copy(a,b)  (a)=(b)
#endif

// NOTE:  in the ms version of std::string  'clear' is not implemented,  use 'erase' instead.
#include <stdio.h>
#include <string>
#include <algorithm>

#ifdef __GNUC__
#include <errno.h>
#endif
#ifdef __GNUC__

// MSVC: the buffer is assumed to contain a 0 at buf[size]
// NOTE: with gcc, you'd have to specify str.size()+1
// msvc does not add the terminating 0 when result is size.
int _snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n= vsnprintf(buf, size+1, fmt, ap);
    va_end(ap);
    return n;
}
int _snwprintf(wchar_t *wbuf, size_t size, const wchar_t *wfmt, ...)
{
    va_list ap;
    va_start(ap, wfmt);
    int n= vswprintf(wbuf, size+1, wfmt, ap);
    va_end(ap);
    return n;
}
int _vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    return vsnprintf(buf, size+1, fmt, ap);
}
int _vsnwprintf(wchar_t *wbuf, size_t size, const wchar_t *wfmt, va_list ap) 
{
    return vswprintf(wbuf, size+1, wfmt, ap);
}

#endif


// splits a list of blank separated optionally quoted parameters
// in a list of strings
bool SplitString(const std::string& str, StringList& strlist, bool bWithEscape/*= true*/, const std::string& separator/*= " \t"*/)
{
    std::string::const_iterator pos= str.begin();
    bool bQuoted= false;
    bool bEscaped= false;
    std::string current;

    while (pos != str.end())
    {
        if (bEscaped)
        {
            current += *pos++;
            bEscaped= false;
        }
        else if (bQuoted)
        {
            switch(*pos)
            {
            case '"':
                bQuoted= false;
                strlist.push_back(std::string(current));
                //debug("added %hs\n", current.c_str());
                current.erase();
                ++pos;
                break;
            case '\\':
                if (bWithEscape)
                {
                    bEscaped= true;
                    ++pos;  // skip escape char
                    break;
                }
                // else fall through
            default:
                current += *pos++;
            }
        }
        else    // not escaped, and not quoted
        {
            if (separator.find(*pos)!=separator.npos)
            {
                ++pos;
                if (!current.empty())
                {
                    strlist.push_back(std::string(current));
                    //debug("added %hs\n", current.c_str());
                    current.erase();
                }
            }
            else switch(*pos)
            {
            case '"':
                bQuoted=true;
                ++pos;
                break;
            case '\\':
                if (bWithEscape) {
                    bEscaped= true;
                    ++pos;  // skip escape char
                    break;
                }
                // else fall through
            default:
                current += *pos++;
            }
        }
    }
    if (!current.empty())
    {
        strlist.push_back(std::string(current));
        //debug("added %hs\n", current.c_str());
        current.erase();
    }
    if (bQuoted || bEscaped)
    {
        //debug("ERROR: Unterminated commandline\n");
        return false;
    }

    return true;
}
bool SplitString(const std::Wstring& str, WStringList& strlist, bool bWithEscape/*= true*/, const std::Wstring& separator/*= " \t"*/)
{
    std::Wstring::const_iterator pos= str.begin();
    bool bQuoted= false;
    bool bEscaped= false;
    std::Wstring current;

    while (pos != str.end())
    {
        if (bEscaped)
        {
            current += *pos++;
            bEscaped= false;
        }
        else if (bQuoted)
        {
            switch(*pos)
            {
            case '"':
                bQuoted= false;
                strlist.push_back(std::Wstring(current));
                //debug("added %hs\n", current.c_str());
                current.erase();
                ++pos;
                break;
            case '\\':
                if (bWithEscape)
                {
                    bEscaped= true;
                    ++pos;  // skip escaped char
                }
                // else fall through
            default:
                current += *pos++;
            }
        }
        else    // not escaped, and not quoted
        {
            if (separator.find(*pos)!=separator.npos)
            {
                ++pos;
                if (!current.empty())
                {
                    strlist.push_back(std::Wstring(current));
                    //debug("added %hs\n", current.c_str());
                    current.erase();
                }
            }
            else switch(*pos)
            {
            case '"':
                bQuoted=true;
                ++pos;
                break;
            case '\\':
                if (bWithEscape) {
                    bEscaped= true;
                    ++pos;  // skip escaped char
                    break;
                }
                // else fall through
            default:
                current += *pos++;
            }
        }
    }
    if (!current.empty())
    {
        strlist.push_back(std::Wstring(current));
        //debug("added %hs\n", current.c_str());
        current.erase();
    }
    if (bQuoted || bEscaped)
    {
        //debug("ERROR: Unterminated commandline\n");
        return false;
    }

    return true;
}

// sprintf like string formatting
std::string stringformat(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    std::string str= stringvformat(fmt, ap);
    va_end(ap);
    return str;
}

std::string stringvformat(const char *fmt, va_list ap)
{
    va_list ap2;
#ifdef _WIN32_WCE
// unfortunately in CE there is now way of determining the resulting
// length of a formatted string.

    int desired_length= 1024;
#elif defined(_WIN32)
    // or use _scprintf to calculate result length
    // on ANSI-C compliant platforms snprintf will always return the desired length
    va_copy(ap2, ap);
    int desired_length= _vsnprintf(NULL, 0, fmt, ap2);
#else
    int desired_length= 1024;
#endif

    std::string str; str.resize(desired_length);

    while (true) {
        va_copy(ap2, ap);
        int printedlength= _vsnprintf(stringptr(str), str.size(), fmt, ap2);
        // '-1' means the buffer was too small.
        if (printedlength!=-1 && size_t(printedlength)<str.size()) {
            str.resize(printedlength);
            break;
        }
        str.resize(2*str.size()+128);
    }
    return str;
}

std::string tolower(const std::string& str)
{
    std::string lstr; 
    lstr.reserve(str.size());
    for (std::string::const_iterator i=str.begin() ; i!=str.end() ; ++i)
        lstr += (char)tolower(*i);
    return lstr;
}
#if 0
int stringicompare(const std::string& a, const std::string& b)
{
    // todo: make this platform independent
#ifdef _WIN32
    return _stricmp(a.c_str(), b.c_str());
#else
    return strcasecmp(a.c_str(), b.c_str());
#endif
}
#endif

static char nyble2hexchar(int n)
{
    return n<0?'?':n<10?('0'+n) : n<16 ? (n-10+'a') : '?';
}
void byte2hexchars(uint8_t b, char *p)
{
    p[0]= nyble2hexchar((b>>4)&0xf);
    p[1]= nyble2hexchar(b&0xf);
}
void word2hexchars(uint16_t w, char *p)
{
    byte2hexchars((w>>8)&0xff, p);  p+=2;
    byte2hexchars(w&0xff, p);  p+=2;
}
void dword2hexchars(uint32_t d, char *p)
{
    word2hexchars((d>>16)&0xffff, p);  p+=4;
    word2hexchars(d&0xffff, p);  p+=4;
}
void qword2hexchars(uint64_t d, char *p)
{
    dword2hexchars((d>>32), p);  p+=8;
    dword2hexchars(d, p);        p+=8;
}
//----------------------------------------------------------------------------

void binary2hex(std::string &str, const uint8_t *buf, int nLength)
{
    str.resize(str.size()+nLength*3);
    char *p= &str[str.size()-nLength*3];
    while(nLength--)
    {
        byte2hexchars(*buf++, p); p+=2;
    }
}


std::string hash_as_string(const ByteVector& hash)
{
    std::string str;
    str.resize(hash.size()*2);
    char *p= &str[str.size()-hash.size()*2];
    for (size_t i=0 ; i<hash.size() ; ++i)
    {
        byte2hexchars(hash[i], p); p+=2;
    }
    return str;
}

void dumpascii(std::string &str, const unsigned char *buf, size_t n)
{
    str.resize(str.size()+n);
    std::string::iterator sp= str.end()-n;
    while (sp<str.end())
    {
        char c= (char)get8(buf++);
        *sp++ = (c>=' ' && c<='~')?c:'.';
    }
}
void writespaces(std::string &str, int n)
{
    str.resize(str.size()+n, ' ');
}

//----------------------------------------------------------------------------
// various ways of generating a hexdump of binary data.


// dumps data with a limited nr of items per line, followed by ascii data, prefixed with offsets.
std::string hexdump(int64_t llOffset, const uint8_t *buf, int nLength, int nDumpUnitSize /*=1*/, int nMaxUnitsPerLine /*=16*/)
{
// 1 for LF terminating line,
// 8-16 for offset preceeding line,
// 2 for spaces separating hex from ascii
//  .. 1 spare
    int nCharsInLine= 20
               +nMaxUnitsPerLine*(
            nDumpUnitSize==1?4:       // 2 for hex chars 1 for space next to hexchar 1 for asciidump
            nDumpUnitSize==2?7:
            nDumpUnitSize==4?13:
            nDumpUnitSize==8?25:
            13);
    int nCharsInResult= nCharsInLine*(nLength/nDumpUnitSize/nMaxUnitsPerLine+1);

    std::string all; all.reserve(nCharsInResult);

    while(nLength>0)
    {
        std::string line;
        // is rounding correct here?
        int nUnitsInLine= nLength/nDumpUnitSize;
        int leftover=0;
        if (nMaxUnitsPerLine<=nUnitsInLine) {
            nUnitsInLine= nMaxUnitsPerLine;
        }
        else {
            leftover= nLength-nDumpUnitSize*nUnitsInLine;
        }
        
        line.reserve(nCharsInLine);

        if (llOffset>>32)
            line +=stringformat("%x", static_cast<uint32_t>(llOffset>>32));
        line += stringformat("%08x", static_cast<uint32_t>(llOffset));

        switch(nDumpUnitSize)
        {
            case 1: hexdumpbytes(line, buf, nUnitsInLine); break;
            case 2: hexdumpwords(line, buf, nUnitsInLine); break;
            case 4: hexdumpdwords(line, buf, nUnitsInLine); break;
            case 8: hexdumpqwords(line, buf, nUnitsInLine); break;
        }
        int extra=0;
        if (leftover>0 && leftover<nDumpUnitSize) {
            // handle incomplete last dword or word
            if (nDumpUnitSize==2)
                line += " ????";
            else if (nDumpUnitSize==4)
                line += " ????????";
            // this never occurs for nDumpUnitSize==1

            extra=1;
        }

        if (nUnitsInLine+extra<nMaxUnitsPerLine)
            writespaces(line, (nMaxUnitsPerLine-nUnitsInLine-extra)*(2*nDumpUnitSize+1));

        line += "  ";

        dumpascii(line, buf, nUnitsInLine*nDumpUnitSize+leftover);
        if (nUnitsInLine<nMaxUnitsPerLine)
            writespaces(line, nDumpUnitSize*(nMaxUnitsPerLine-nUnitsInLine)-leftover);

        all += line;
        all += "\n";

        nLength -= (nUnitsInLine+extra)*nDumpUnitSize;
        llOffset += (nUnitsInLine+extra)*nDumpUnitSize;
        buf += (nUnitsInLine+extra)*nDumpUnitSize;
    }

    return all;
}

// todo: also recognize unicode strings
std::string ascdump(const uint8_t *first, size_t size, const std::string& escaped, bool bBreakOnEol/*= false*/)
{
    std::string result;
    bool bQuoted= false;
    bool bLastWasEolChar= false;

    bool bWarnedLarge= false;
    const uint8_t *p= first;
    const uint8_t *last= first+size;
    while (p<last)
    {
        if (result.size()>0x1000000 && !bWarnedLarge) {
            fprintf(stderr, "WARNING: ascdump with large output(buf=%d, res=%d\n", (int)size, (int)result.size());
            fprintf(stderr, "hex: %s\n", hexdump(first, size).c_str());
            throw "ascdump error";
        }

        bool bNeedsEscape= escaped.find((char)*p)!=escaped.npos 
            || *p=='\"' 
            || *p=='\\';

        bool bThisIsEolChar= (*p==0x0a || *p==0x0d || *p==0);

        if ((p>first+1) && p[-2]==*p && p[-1]==*p && (*p==0 || *p==0xff)) {
            const uint8_t *seqstart= p-2;
            while (p<last && *p==*seqstart)
                p++;
            result += stringformat(" [x%d]", p-seqstart);
            p--;
        }
        if (bLastWasEolChar && !bThisIsEolChar && bBreakOnEol) {
            if (bQuoted)
                result += "\"";
            bQuoted= false;
            result += "\n";
        }

        if (isprint(*p) || bNeedsEscape) {
            if (!bQuoted) {
                if (!result.empty() && *result.rbegin()!='\n')
                    result += ",";
                result += "\"";
                bQuoted= true;
            }
            if (bNeedsEscape) {
                std::string escapecode;
                switch(*p) {
                    case '\n': escapecode= "\\n"; break;
                    case '\r': escapecode= "\\r"; break;
                    case '\t': escapecode= "\\t"; break;
                    case '\"': escapecode= "\\\""; break;
                    case '\\': escapecode= "\\\\"; break;
                    default:
                       escapecode= stringformat("\\x%02x", *p);
                }
                result += escapecode;
            }
            else {
                result += (char) *p;
            }
        }
        else {
            if (bQuoted) {
                result += "\"";
                bQuoted= false;
            }
            if (!result.empty())
                result += ",";
            result += stringformat("%02x", *p);
        }
        bLastWasEolChar= bThisIsEolChar;

        p++;
    }

    if (bQuoted) {
        result += "\"";
        bQuoted= false;
    }

    return result;
}

std::string GuidToString(const GUID *guid)
{
    return stringformat("{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1],
            guid->Data4[2] , guid->Data4[3] , guid->Data4[4],
            guid->Data4[5] , guid->Data4[6] , guid->Data4[7]);
}


// -1: invalid
// -2: end
static const int char2b64[]= {
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,
-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

ByteVector base64_decode(const std::string& str)
{
    size_t b64size= int((str.size()+3)/4)*3;

    int n=0;
    uint32_t value= 0;
    ByteVector data; data.reserve(b64size);
    for (std::string::const_iterator i= str.begin() ; i!=str.end() ; i++)
    {
        int b= char2b64[((unsigned)*i)&0xff];
        if (b>=0) {
            value= (value<<6) | b;
            n++;
            if (n==4) {
                data.push_back(value>>16);
                data.push_back(value>>8);
                data.push_back(value);
                n= 0;
                value= 0;
            }
        }
        else if (b==-2)
            break;
    }
    switch(n)
    {
        // fall through all!!
        case 1: value <<= 6;
        case 2: value <<= 6;
        case 3: value <<= 6;
        case 4: break;
        case 0: break;
    } 
    switch(n)
    {
        case 4:
                data.push_back(value>>16);
                data.push_back(value>>8);
                data.push_back(value);
                break;
        case 3:
                data.push_back(value>>16);
                data.push_back(value>>8);
                break;
        case 2:
                data.push_back(value>>16);
                break;
        case 1:
                // note: 1 base64 char should not happen
        case 0:
                break;
    }
    return data;
}
std::string utf8forchar(WCHAR c)
{
    std::string str;
    if (c<0x80) {
        str += (char)c;
    }
    else if (c<0x800) {
        str += (char)(0xc0|(c>>6));
        str += (char)(0x80|(c&0x3f));
    }
    else {  // c<0x10000
        str += (char)(0xe0|(c>>12));
        str += (char)(0x80|((c>>6)&0x3f));
        str += (char)(0x80|(c&0x3f));
    }
    // not handling the case c<0x110000
    return str;
}


