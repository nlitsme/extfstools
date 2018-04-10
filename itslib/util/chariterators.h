// Copyright 2012 GSMK mbH, Berlin
// All Rights Reserved
//
// Author: Willem Hengeveld   <itsme@gsmk.de>
//
#ifndef _UTIL_CHARITERATORS_H_
#define _UTIL_CHARITERATORS_H_

#include <iterator>

/*
   UTF8-octets = *( UTF8-char )
   UTF8-char   = UTF8-1 / UTF8-2 / UTF8-3 / UTF8-4
   UTF8-1      = 00-7F                   0xxxxxxx                       [     0,     80 )
   UTF8-2      = C2-DF tail              110xxxxx   x=02..1f  ->        [    80,    800 )
   UTF8-3      = E0 A0-BF tail           11100000 10xxxxxx x=20..3f  -> [   800,   1000 )
                 E1-EC tail tail         1110xxxx        x=01..0c  ->   [  1000,   d000 )
                 ED 80-9F tail           11101101 100xxxxx x=00..1f  -> [  d000,   d800 )
                 EE-EF tail tail         1110xxxx  x=0e..0f   -> val in [  e000,  10000 )
   UTF8-4      = F0 90-BF tail tail      11110000 10xxxxxx   x=10..3f   [ 10000,  40000 )
                 F1-F3 tail tail tail    111100xx   x=01..03            [ 40000, 100000 )
                 F4 80-8F tail tail      11110100 1000xxxx x=00..0f     [100000, 110000 )
   tail        = 80-BF

                max range        excluding
  00-7f        [ 0,      80 )
  c0-df T      [ 0,     800 )  - [0, 80)
  e0-ef TT     [ 0,   10000 )  - [0, 800) - [d800, e000)
  f0-f7 TTT    [ 0,  200000 )  - [0, 10000) - [110000, 200000)

*/
// todo: add 'utf8distance' function
// todo: add variants of algorithms with 'last' iterator
//       [current algorithms all handle NUL terminated strings]

// todo: utf8iterator cannot be used with std::sort, since it expects the iterator to be both assignable and dereferencable

// does not check if chars fall in valid ranges
template<typename P>
bool utf8validator(P p)
{
    while(true)
    {
        uint8_t b= *p++;
        if (b==0)
            break;
        if (b>=0xf8)
            return false;
        int n=0;
        if (b>=0xf0)
            n=3;
        else if (b>=0xe0)
            n=2;
        else if (b>=0xc0) {
            n=1;
        }
        else if (b>=0x80)
            return false;

        while (n--) {
            uint8_t b1= *p++;
            if ( !(b1>=0x80 && b1<0xc0) )
                return false;
        }
    }
    return true;
}

/* alternative interface:
 *     P find_next_strict_invalid(P p, P last=P());
 *
 *     bool strict_utf8validator(P p, P last=P())
 *     {
 *         return find_next_strict_invalid(p,last)==last;
 *     }
 */
template<typename P>
bool strictutf8validator(P p)
{
    while(true)
    {
        uint8_t b= *p++;
        if (b>=0xf8)
            return false;
        int n=0;
        uint32_t val=0;
        uint32_t minval=0;
        uint32_t maxval=0;
        if (b>=0xf0) {
            n=3;
            val= b & 0x07;
            minval= 0x10000;
            maxval= 0x110000;
        }
        else if (b>=0xe0) {
            n=2;
            val= b & 0x0f;
            minval= 0x800;
            maxval= 0x10000;
        }
        else if (b>=0xc0) {
            n=1;
            minval= 0x80;
            maxval= 0x800;
        }
        else if (b>=0x80)
            return false;
        else {
            minval= 0;
            maxval= 0x80;
        }

        while (n--) {
            uint8_t b1= *p++;
            if ( !(b1>=0x80 && b1<0xc0) )
                return false;
            val<<=6;
            val |= b1&0x3f;
        }
        if (val<minval || val>=maxval)
            return false;
        if (val>=0xd000 && val<0xe000)
            return false;
    }
    return true;
}
template<typename P>
class utf8iterator : public std::iterator<std::random_access_iterator_tag, uint32_t> {
    P _ptr;
public:

    utf8iterator()
        : _ptr(NULL)
    {
    }
    utf8iterator(P p)
        : _ptr(p)
    {
    }

    utf8iterator(const utf8iterator&i)
        : _ptr(i._ptr)
    {
    }
    utf8iterator& operator=(const utf8iterator& i)
    {
        _ptr= i._ptr;
        return *this;
    }

    uint32_t operator*() const
    {
        P p=_ptr;
        uint8_t b0= *p++;
        uint32_t c;
        int n;
        if (b0>=0xf0) {
            c= b0&7;
            n=3;
        }
        else if (b0>=0xe0) {
            c= b0&15;
            n=2;
        }
        else if (b0>=0xc0) {
            c= b0&31;
            n=1;
        }
        else if (b0>=0x80) {
            return 0;
        }
        else {
            return b0;
        }
        while (n--) {
            c <<= 6;
            c |= *p++ & 0x3f;
        }
        return c;
    }
    utf8iterator& operator++()
    {
        P p=_ptr;
        uint8_t b0= *p++;
        int n;
        if (b0>=0xf0) {
            n=3;
        }
        else if (b0>=0xe0) {
            n=2;
        }
        else if (b0>=0xc0) {
            n=1;
        }
        else {
            n=0;
        }
        while (n--) {
            p++;
        }
        _ptr= p;
        return *this;
    }
    utf8iterator& operator--()
    {
        P p=_ptr;
        while ( (*--p & 0xc0)==0x80 )
            ;
        _ptr= p;

        return *this;
    }
    utf8iterator operator++(int)
    {
        utf8iterator copy(*this);
        ++(*this);
        return copy;
    }
    utf8iterator operator--(int)
    {
        utf8iterator copy(*this);
        --(*this);
        return copy;
    }

    // note: std::advance does exactly this.
    utf8iterator operator+=(difference_type n)
    {
        if (n<0)
            return (*this)-=(-n);
        while (n--)
            ++(*this);
        return *this;
    }
    utf8iterator operator-=(difference_type n)
    {
        if (n<0)
            return (*this)+=(-n);
        while (n--)
            --(*this);
        return *this;
    }
    utf8iterator operator+(difference_type n) const {
        utf8iterator copy(*this);
        copy += n;
        return copy;
    }
    utf8iterator operator-(difference_type n) const {
        utf8iterator copy(*this);
        copy -= n;
        return copy;
    }

    // note: std::distance does exactly this.
    difference_type operator-(const utf8iterator& rhs) const {
        if ((*this) < rhs) {
            difference_type n=0;
            utf8iterator i= *this;
            while (i!=rhs) {
                i++;
                n++;
            }
            return -n;
        }
        if (rhs < (*this)) {
            difference_type n=0;
            utf8iterator i= rhs;
            while (i!=*this) {
                i++;
                n++;
            }
            return n;
        }
        return 0;
    }



    bool operator==(const utf8iterator& rhs) const { return _ptr == rhs._ptr; }
    bool operator>=(const utf8iterator& rhs) const { return !(*this<rhs); }
    bool operator<=(const utf8iterator& rhs) const { return !(*this>rhs); }
    bool operator!=(const utf8iterator& rhs) const { return !(*this==rhs); }
    bool operator>(const utf8iterator& rhs)  const { return _ptr >  rhs._ptr; }
    bool operator<(const utf8iterator& rhs)  const { return _ptr <  rhs._ptr; }
};
template<typename P>
utf8iterator<P> utf8adaptor(P p)
{
    return utf8iterator<P>(p);
}

template<typename P>
class utf8insert_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void> {
    P _p;
public:

    utf8insert_iterator(P p)
        : _p(p)
    {
    }
    P ptr() { return _p; }
    //todo: operator P() { return _p; }

    utf8insert_iterator& operator=(uint32_t x)
    {
        int n;
        uint8_t b;
        if (x<0x80) {
            n=0;
            b=x;
        }
        else if (x<0x800) {
            n=1;
            b=0xc0|(x>>6);
        }
        else if (x<0x10000) {
            if (x>=0xd800 && x<0xe000)
                return *this;
            b=0xe0|(x>>12);
            n=2;
        }
        else if (x<0x110000) {
            b=0xf0|(x>>18);
            n=3;
        }
        else {
            // ignoring invalid value
            return *this;
        }
        *_p++ = b;
        while (n--) {
            b= 0x80 | ((x>>(6*n))&0x3f);
            *_p++ = b;
        }
        return *this;
    }

    utf8insert_iterator& operator*()
    {
        return *this;
    }
    utf8insert_iterator& operator++()
    {
        return *this;
    }
    utf8insert_iterator& operator++(int)
    {
        return *this;
    }
};
template<typename P>
utf8insert_iterator<P> utf8_backinserter(P p)
{
    return utf8insert_iterator<P>(p);
}

// std::wstring w= L"abcdef";
// std::string s;
// std::copy(utf16adaptor(w.begin()), utf16adaptor(w.end()), utf8_backinserter(back_inserter(s)));

/*
   U' = yyyyyyyyyyxxxxxxxxxx
   W1 = 110110yyyyyyyyyy
   W2 = 110111xxxxxxxxxx
*/
template<typename P>
bool utf16validator(P p)
{
    while(true)
    {
        uint16_t b= *p++;
        if (b==0)
            break;
        if (b>=0xd800 && b<0xdc00) {
            uint16_t b1= *p++;
            if (!(b1>=0xdc00 && b1<0xe000))
                return false;
        }
        else if (b>=0xdc00 && b<0xe000)
            return false;
    }
    return true;
}

template<typename P>
class utf16iterator  : public std::iterator<std::random_access_iterator_tag, uint32_t> {
    P _ptr;
public:
    utf16iterator()
        : _ptr(NULL)
    {
    }
    utf16iterator(P p)
        : _ptr(p)
    {
    }

    utf16iterator(const utf16iterator&i)
        : _ptr(i._ptr)
    {
    }
    utf16iterator& operator=(const utf16iterator& i)
    {
        _ptr= i._ptr;
        return *this;
    }

    uint32_t operator*() const
    {
        P p= _ptr;
        uint16_t b0= *p++;
        if (b0>=0xd800 && b0<0xdc00) {
            uint16_t b1= *p++;
            return (((b0&0x3ff)<<10)|(b1&0x3ff))+0x10000;
        }
        else {
            return b0;
        }
    }
    utf16iterator& operator++()
    {
        uint16_t c= *_ptr++;
        if (c>=0xd800 && c<0xdc00)
            _ptr++;

        return *this;
    }
    utf16iterator& operator--()
    {
        uint16_t c= *--_ptr;
        if (c>=0xdc00 && c<0xe000)
            --_ptr;

        return *this;
    }
    utf16iterator operator++(int)
    {
        utf16iterator copy(*this);
        ++(*this);
        return copy;
    }
    utf16iterator operator--(int)
    {
        utf16iterator copy(*this);
        --(*this);
        return copy;
    }
    utf16iterator operator+=(difference_type n)
    {
        if (n<0)
            return (*this)-=(-n);
        while (n--)
            ++(*this);
        return *this;
    }
    utf16iterator operator-=(difference_type n)
    {
        if (n<0)
            return (*this)+=(-n);
        while (n--)
            --(*this);
        return *this;
    }
    utf16iterator operator+(difference_type n) const {
        utf16iterator copy(*this);
        copy += n;
        return copy;
    }
    utf16iterator operator-(difference_type n) const {
        utf16iterator copy(*this);
        copy -= n;
        return copy;
    }
    difference_type operator-(const utf16iterator& rhs) const {
        if ((*this) < rhs) {
            difference_type n=0;
            utf16iterator i= *this;
            while (i!=rhs) {
                i++;
                n++;
            }
            return -n;
        }
        if (rhs < (*this)) {
            difference_type n=0;
            utf16iterator i= rhs;
            while (i!=*this) {
                i++;
                n++;
            }
            return n;
        }
        return 0;
    }
    bool operator==(const utf16iterator& rhs) const { 
        return _ptr == rhs._ptr;
    }
    bool operator>=(const utf16iterator& rhs) const { return !(*this<rhs); }
    bool operator<=(const utf16iterator& rhs) const { return !(*this>rhs); }
    bool operator!=(const utf16iterator& rhs) const { return !(*this==rhs); }
    bool operator>(const utf16iterator& rhs)  const { return _ptr >  rhs._ptr; }
    bool operator<(const utf16iterator& rhs)  const { return _ptr <  rhs._ptr; }
};
template<typename P>
utf16iterator<P> utf16adaptor(P p)
{
    return utf16iterator<P>(p);
}


template<typename P>
class utf16insert_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void> {
    P _p;
public:

    utf16insert_iterator(P p)
        : _p(p)
    {
    }
    P ptr() { return _p; }
    //operator P() { return _p; }

    utf16insert_iterator& operator=(uint32_t x)
    {
        if (x>=0xd800 && x<0xe000)
            return *this;
        if (x>=0x110000)
            return *this;
        if (x<0x10000) {
            *_p++ = x;
            return *this;
        }
        x-=0x10000;
        *_p++ = 0xd800|(x>>10);
        *_p++ = 0xdc00|(x&0x3ff);
        return *this;
    }
    utf16insert_iterator& operator*()
    {
        return *this;
    }
    utf16insert_iterator& operator++()
    {
        return *this;
    }
    utf16insert_iterator& operator++(int)
    {
        return *this;
    }
};
template<typename P>
utf16insert_iterator<P> utf16_backinserter(P p)
{
    return utf16insert_iterator<P>(p);
}

// ------ utf32 ----------------
template<typename P>
bool utf32validator(P p)
{
    while(true)
    {
        uint32_t b= *p++;
        if (b==0)
            break;
        if (b>=0xd800 && b<0xe000) {
            return false;
        }
    }
    return true;
}

template<typename P>
class utf32iterator  : public std::iterator<std::random_access_iterator_tag, uint32_t> {
    P _ptr;
public:
    utf32iterator()
        : _ptr(NULL)
    {
    }
    utf32iterator(P p)
        : _ptr(p)
    {
    }

    utf32iterator(const utf32iterator&i)
        : _ptr(i._ptr)
    {
    }
    utf32iterator& operator=(const utf32iterator& i)
    {
        _ptr= i._ptr;
        return *this;
    }

    uint32_t operator*() const
    {
        return *_ptr;
    }
    utf32iterator& operator++()
    {
        _ptr++;

        return *this;
    }
    utf32iterator& operator--()
    {
        --_ptr;

        return *this;
    }
    utf32iterator operator++(int)
    {
        utf32iterator copy(*this);
        ++(*this);
        return copy;
    }
    utf32iterator operator--(int)
    {
        utf32iterator copy(*this);
        --(*this);
        return copy;
    }
    utf32iterator operator+=(difference_type n)
    {
        _ptr += n;
        return *this;
    }
    utf32iterator operator-=(difference_type n)
    {
        _ptr -= n;
        return *this;
    }
    utf32iterator operator+(difference_type n) const {
        utf32iterator copy(*this);
        copy += n;
        return copy;
    }
    utf32iterator operator-(difference_type n) const {
        utf32iterator copy(*this);
        copy -= n;
        return copy;
    }
    difference_type operator-(const utf32iterator& rhs) const {
        return _ptr - rhs._ptr;
    }
    bool operator==(const utf32iterator& rhs) const { 
        return _ptr == rhs._ptr;
    }
    bool operator>=(const utf32iterator& rhs) const { return !(*this<rhs); }
    bool operator<=(const utf32iterator& rhs) const { return !(*this>rhs); }
    bool operator!=(const utf32iterator& rhs) const { return !(*this==rhs); }
    bool operator>(const utf32iterator& rhs)  const { return _ptr >  rhs._ptr; }
    bool operator<(const utf32iterator& rhs)  const { return _ptr <  rhs._ptr; }
};
template<typename P>
utf32iterator<P> utf32adaptor(P p)
{
    return utf32iterator<P>(p);
}


template<typename P>
class utf32insert_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void> {
    P _p;
public:

    utf32insert_iterator(P p)
        : _p(p)
    {
    }
    P ptr() { return _p; }
    //operator P() { return _p; }

    utf32insert_iterator& operator=(uint32_t x)
    {
        *_p++ = x;
        return *this;
    }
    utf32insert_iterator& operator*()
    {
        return *this;
    }
    utf32insert_iterator& operator++()
    {
        return *this;
    }
    utf32insert_iterator& operator++(int)
    {
        return *this;
    }
};
template<typename P>
utf32insert_iterator<P> utf32_backinserter(P p)
{
    return utf32insert_iterator<P>(p);
}


template<typename P>
class utf16beiterator  : public std::iterator<std::random_access_iterator_tag, uint32_t> {
    P _ptr;
public:
    utf16beiterator()
        : _ptr(NULL)
    {
    }
    utf16beiterator(P p)
        : _ptr(p)
    {
    }

    utf16beiterator(const utf16beiterator&i)
        : _ptr(i._ptr)
    {
    }
    utf16beiterator& operator=(const utf16beiterator& i)
    {
        _ptr= i._ptr;
        return *this;
    }

    uint16_t getbe16(P p) const
    {
        uint16_t v= *p;
        return (v>>8)|(v<<8);
    }

    uint32_t operator*() const
    {
        P p= _ptr;
        uint16_t b0= getbe16(p++);
        if (b0>=0xd800 && b0<0xdc00) {
            uint16_t b1= getbe16(p++);
            return (((b0&0x3ff)<<10)|(b1&0x3ff))+0x10000;
        }
        else {
            return b0;
        }
    }
    utf16beiterator& operator++()
    {
        uint16_t c= getbe16(_ptr++);
        if (c>=0xd800 && c<0xdc00)
            _ptr++;

        return *this;
    }
    utf16beiterator& operator--()
    {
        uint16_t c= getbe16(--_ptr);
        if (c>=0xdc00 && c<0xe000)
            --_ptr;

        return *this;
    }
    utf16beiterator operator++(int)
    {
        utf16beiterator copy(*this);
        ++(*this);
        return copy;
    }
    utf16beiterator operator--(int)
    {
        utf16beiterator copy(*this);
        --(*this);
        return copy;
    }
    utf16beiterator operator+=(difference_type n)
    {
        if (n<0)
            return (*this)-=(-n);
        while (n--)
            ++(*this);
        return *this;
    }
    utf16beiterator operator-=(difference_type n)
    {
        if (n<0)
            return (*this)+=(-n);
        while (n--)
            --(*this);
        return *this;
    }
    utf16beiterator operator+(difference_type n) const {
        utf16beiterator copy(*this);
        copy += n;
        return copy;
    }
    utf16beiterator operator-(difference_type n) const {
        utf16beiterator copy(*this);
        copy -= n;
        return copy;
    }
    difference_type operator-(const utf16beiterator& rhs) const {
        if ((*this) < rhs) {
            difference_type n=0;
            utf16beiterator i= *this;
            while (i!=rhs) {
                i++;
                n++;
            }
            return -n;
        }
        if (rhs < (*this)) {
            difference_type n=0;
            utf16beiterator i= rhs;
            while (i!=*this) {
                i++;
                n++;
            }
            return n;
        }
        return 0;
    }
    bool operator==(const utf16beiterator& rhs) const { 
        return _ptr == rhs._ptr;
    }
    bool operator>=(const utf16beiterator& rhs) const { return !(*this<rhs); }
    bool operator<=(const utf16beiterator& rhs) const { return !(*this>rhs); }
    bool operator!=(const utf16beiterator& rhs) const { return !(*this==rhs); }
    bool operator>(const utf16beiterator& rhs)  const { return _ptr >  rhs._ptr; }
    bool operator<(const utf16beiterator& rhs)  const { return _ptr <  rhs._ptr; }
};
template<typename P>
utf16beiterator<P> utf16beadaptor(P p)
{
    return utf16beiterator<P>(p);
}

// bitsize parameterized templates for utf convertors

template<int bits> struct utfadaptor { };

template<> struct utfadaptor<8> {
    typedef uint8_t value_type;

    template<typename P> struct iterator {
        typedef utf8iterator<P> itertype;
        typedef utf8insert_iterator<P> instype;
    };

    template<typename P>
    static utf8iterator<P> adapt(P p) { return utf8iterator<P>(p); }

    template<typename P>
    static utf8insert_iterator<P> insertor(P p) { return utf8insert_iterator<P>(p); }
};

template<> struct utfadaptor<16> {
    typedef uint16_t value_type;

    template<typename P> struct iterator {
        typedef utf16iterator<P> itertype;
        typedef utf16insert_iterator<P> instype;
    };

    template<typename P>
    static utf16iterator<P> adapt(P p) { return utf16iterator<P>(p); }

    template<typename P>
    static utf16insert_iterator<P> insertor(P p) { return utf16insert_iterator<P>(p); }

};
/*
template<int bits> struct utfbeadaptor { };

template<> struct utfbeadaptor<16> {
    typedef int16_t value_type;

    template<typename P> struct iterator {
        typedef utf16beiterator<P> itertype;
        typedef utf16beinsert_iterator<P> instype;
    };

    template<typename P>
    static utf16beiterator<P> adapt(P p) { return utf16beiterator<P>(p); }

    template<typename P>
    static utf16beinsert_iterator<P> insertor(P p) { return utf16beinsert_iterator<P>(p); }

};
*/

template<> struct utfadaptor<32> {
    typedef uint32_t value_type;

    template<typename P> struct iterator {
        typedef utf32iterator<P> itertype;
        typedef utf32insert_iterator<P> instype;
    };

    template<typename P>
    static utf32iterator<P> adapt(P p) { return utf32iterator<P>(p); }

    template<typename P>
    static utf32insert_iterator<P> insertor(P p) { return utf32insert_iterator<P>(p); }

};


#endif
