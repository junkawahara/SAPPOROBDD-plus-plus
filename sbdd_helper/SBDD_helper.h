/*
One header library for SAPPOROBDD C/C++ version
version 1.3.0

Copyright (c) 2017 -- 2026 Jun Kawahara

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef SBDD_HELPER_H
#define SBDD_HELPER_H

#ifdef __cplusplus

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cfloat>
#include <cmath>
#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstring>

#include <new>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <iterator>

/* The C++11 random classes; without them the library falls back on
   rand() (see sbddh_randBelow in utility.h). */
#if __cplusplus >= 201103L
#include <random>
#endif

#ifdef SBDDH_GMP
#include <gmp.h>
#include <gmpxx.h>
#endif

#else /* __cplusplus */

#ifdef SBDDH_GMP
/* The GMP features use mpz_class of gmpxx.h, which is a C++ class. */
#error The SBDDH_GMP features are available only in C++.
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <float.h> /* DBL_MAX_10_EXP used by the snprintf fallback */
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>

#endif /* __cplusplus */

#ifdef __cplusplus
namespace sbddh {
#ifdef SAPPOROBDD_PLUS_PLUS
using namespace sapporobdd;
#endif
#endif

typedef long long int llint;
typedef unsigned long long int ullint;

#define sbddextended_unused(a) (void)(a)

/* inline function qualifier for gcc */
/* if a compile error occurs, change the qualifier */
#define sbddextended_INLINE_FUNC static inline

#define sbddextended_BDDNODE_START 2
#define sbddextended_NUMBER_OF_CHILDREN 2

#define sbddextended_BUFSIZE 1024

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef SBDDH_GMP
    const mpz_class sbddextended_VALUE_ZERO(0);
    const mpz_class sbddextended_VALUE_ONE(1);
#else
    static const llint sbddextended_VALUE_ZERO = 0;
    static const llint sbddextended_VALUE_ONE = 1;
#endif


#ifdef __cplusplus

/* Returns the c_str() of every element of vec. The result points into
   the strings of vec, so vec must outlive it. A std::vector is used
   rather than a malloc'ed array so that the memory is released even when
   the caller throws before it would have freed it. It always holds at
   least one element so that &arr[0] is valid. */
sbddextended_INLINE_FUNC
std::vector<const char*> sbddextended_strVectorToArray(
                                    const std::vector<std::string>& vec)
{
    std::vector<const char*> arr(vec.size() > 0 ? vec.size() : 1,
                                    (const char*)NULL);
    for (size_t i = 0; i < vec.size(); ++i) {
        arr[i] = vec[i].c_str();
    }
    return arr;
}

#endif

/* Returns bddvarmax, the largest variable index (equivalently, the
   largest level) that SAPPOROBDD can represent, as an int. The levels
   and the numbers of variables of this library are ints, while
   bddvarmax is an unsigned macro whose value depends on the build of
   SAPPOROBDD: the B_EXTEND build of SAPPOROBDD++ sets it to 2^32 - 2,
   which no int can hold, and (int)bddvarmax is negative there, so a
   range check written against that cast rejects every value. The result
   is therefore clamped to INT_MAX - 1, which also keeps the "bound + 1"
   that some of the callers compute inside int. bddvarmax fits in an int
   in every other build, so the clamp does not change the bound there. */
sbddextended_INLINE_FUNC
int sbddextended_varMaxAsInt(void)
{
    if ((ullint)bddvarmax > (ullint)(INT_MAX - 1)) {
        return INT_MAX - 1;
    }
    return (int)bddvarmax;
}

/* Returns 1 if s consists only of whitespace (the empty string does). */
/* The importers use it to check that nothing follows the tokens that */
/* a line of the format is made of. */
sbddextended_INLINE_FUNC
int sbddextended_isBlankString(const char* s)
{
    while (*s != '\0') {
        if (!isspace((int)(unsigned char)*s)) {
            return 0;
        }
        ++s;
    }
    return 1;
}

/* XOR shift */
sbddextended_INLINE_FUNC
ullint sbddextended_getXRand(ullint* state)
{
    ullint v = *state;
    /* 0 is a fixed point of the XOR shift, that is, the state would stay
       0 and the generator would return 0 forever. Replace it with the
       non-zero value Marsaglia used as the default seed. */
    if (v == 0) {
        v = 88172645463325252ull;
    }
    v ^= (v << 13);
    v ^= (v >> 7);
    v ^= (v << 17);
    *state = v;
    return v;
}

/* rand() is uniform on [0, RAND_MAX], where RAND_MAX may be as small as */
/* 32767, so one call cannot reach every element of a large family and */
/* its remainder is biased unless RAND_MAX + 1 is a multiple of the */
/* range. The two functions below assemble a value from several calls */
/* and reject the draws that would bias the result. They are used only */
/* where neither GMP nor the random classes of C++11 are available. */

/* the largest b such that 2^b - 1 <= RAND_MAX */
sbddextended_INLINE_FUNC
int sbddh_randBitsPerCall(void)
{
    int b = 1;
    while (b < 31 && ((1ll << (b + 1)) - 1) <= (llint)RAND_MAX) {
        ++b;
    }
    return b;
}

/* a value uniformly distributed on [0, range) */
sbddextended_INLINE_FUNC
ullint sbddh_randBelow(ullint range)
{
    const int step = sbddh_randBitsPerCall();
    const ullint chunk_bound = 1ull << step;
    int bits = 0;
    int got, take;
    ullint m, v, r;

    assert(range >= 1);
    if (range == 0) {
        /* A precondition violation, but do not let it become an endless
           rejection loop in NDEBUG builds ("range - 1" would wrap to
           the maximum ullint and no draw could ever be below 0). */
        return 0;
    }
    for (m = range - 1; m > 0; m >>= 1) {
        ++bits;
    }
    for (;;) {
        v = 0;
        got = 0;
        while (got < bits) {
            /* Keep only the draws below the largest power of two that
               fits in [0, RAND_MAX], whose bits are then uniform. */
            do {
                r = (ullint)rand();
            } while (r >= chunk_bound);
            take = (bits - got < step ? bits - got : step);
            v = (v << take) | (r & ((1ull << take) - 1ull));
            got += take;
        }
        if (v < range) {
            return v;
        }
    }
}

#ifdef __cplusplus

/* Several functions have an overload that takes a container of variable
   numbers and one that takes the number of variables as an int. The
   container one is a template, so it matches a scalar argument exactly
   while the int one needs a conversion, and it therefore wins for an
   argument of any integer type other than int, for instance the bddvar
   (unsigned int) that this library uses for a variable number. Its body
   then fails to compile because a scalar has no begin(). C++98 has no
   <type_traits>, so recognise a container by its member type
   const_iterator and keep the template out of the overload resolution
   for anything else. */
template<typename T>
class sbddh_IsContainer {
private:
    typedef char yes_type[1];
    typedef char no_type[2];
    template<typename U> static yes_type& test(typename U::const_iterator*);
    template<typename U> static no_type& test(...);
public:
    enum { value = (sizeof(test<T>(0)) == sizeof(yes_type)) };
};

template<bool B, typename T>
struct sbddh_EnableIfContainer { };

template<typename T>
struct sbddh_EnableIfContainer<true, T> { typedef T type; };

template<typename value_t>
double sbddh_divide(const value_t& op1, const value_t& op2)
{
    return static_cast<double>(op1) / static_cast<double>(op2);
}

/* Computes op1 / (op1 + op2) as a double. The sum is formed in double
   rather than in value_t because value_t may not be able to hold it:
   with value_t == ullint, a family of exactly 2^64 sets (for instance
   the power set of 64 variables) makes op1 + op2 wrap around to 0, and
   the division by zero would give an infinity or a NaN and thus a
   degenerate probability. A double holds every such sum precisely
   enough for a sampling probability. */
template<typename value_t>
double sbddh_divideBySum(const value_t& op1, const value_t& op2)
{
    const double d1 = static_cast<double>(op1);
    const double d2 = static_cast<double>(op2);
    const double sum = d1 + d2;
    if (!(sum > 0.0)) { /* no information; also excludes a NaN */
        return 0.5;
    }
    return d1 / sum;
}

/* The "strict" argument of the getK*ZBDD functions is used only as a
   three-valued flag (negative, zero, positive), and getKHeaviestZBDD
   needs it inverted. Invert it by cases: -strict would be a signed
   overflow, that is, undefined behavior, for INT_MIN. */
sbddextended_INLINE_FUNC
int sbddh_invertStrict(int strict)
{
    if (strict > 0) {
        return -1;
    } else if (strict < 0) {
        return 1;
    }
    return 0;
}

#ifdef SBDDH_GMP

template<>
inline
double sbddh_divide(const mpz_class& op1, const mpz_class& op2)
{
    mpf_class f1(op1.get_str());
    mpf_class f2(op2.get_str());
    mpf_class result;
    mpf_div(result.get_mpf_t(), f1.get_mpf_t(), f2.get_mpf_t());
    return result.get_d();
}

/* mpz_class has no conversion to double, and its sum cannot overflow
   anyway, so add first and divide with the overload above. */
template<>
inline
double sbddh_divideBySum(const mpz_class& op1, const mpz_class& op2)
{
    const mpz_class sum = op1 + op2;
    if (sum <= 0) {
        return 0.5;
    }
    return sbddh_divide<mpz_class>(op1, sum);
}

/* assume that v is non-negative */
sbddextended_INLINE_FUNC
ullint sbddh_mpz_to_ullint(const mpz_class& v)
{
    /* Not function-local statics: their initialization is not
       thread-safe before C++11, and building the two values by
       shifting costs almost nothing. */
    const mpz_class two32 = mpz_class(1) << 32; /* 2^32 */
    const mpz_class two64 = mpz_class(1) << 64; /* 2^64 */
    assert(v >= 0);
    if (v < two32) {
        return static_cast<ullint>(v.get_ui());
    } else {
        mpz_class vv = v;
        if (v >= two64) { /* return remainder */
            mpz_class qq = v / two64;
            mpz_class rr = v - qq * two64;
            vv = rr;
        }
        assert(vv < two64);
        mpz_class q = vv / two32;
        mpz_class r = vv - q * two32;
        assert(q < two32);
        assert(r < two32);
        return static_cast<ullint>(q.get_ui()) * 4294967296ull
            + r.get_ui();
    }
}

/* mpz_class supports no conversion from llint/ullint. Formatting the
   value into a std::stringstream and parsing the string would make the
   result depend on the global C++ locale, whose numpunct may insert
   thousands separators that the GMP parser rejects, so build the value
   from its 32-bit chunks, each of which unsigned long can always hold. */
sbddextended_INLINE_FUNC
mpz_class sbddh_ullint_to_mpz(ullint v)
{
    const int chunk_bits = 32;
    const int num_chunks = (int)((sizeof(ullint) * CHAR_BIT + chunk_bits - 1)
                                    / chunk_bits);
    mpz_class result(0);
    int i;

    if (v <= static_cast<ullint>(UINT_MAX)) {
        return mpz_class(static_cast<unsigned int>(v));
    }
    for (i = num_chunks - 1; i >= 0; --i) {
        result <<= chunk_bits;
        result += static_cast<unsigned long>(
            (v >> (i * chunk_bits)) & 0xffffffffull);
    }
    return result;
}

sbddextended_INLINE_FUNC
mpz_class sbddh_llint_to_mpz(llint v)
{
    if (static_cast<llint>(INT_MIN) <= v &&
            v <= static_cast<llint>(INT_MAX)) {
        return mpz_class(static_cast<int>(v));
    } else if (v >= 0) {
        return sbddh_ullint_to_mpz(static_cast<ullint>(v));
    } else {
        /* -LLONG_MIN cannot be represented as llint, so take the
           absolute value in ullint. */
        return -sbddh_ullint_to_mpz(static_cast<ullint>(-(v + 1)) + 1);
    }
}

#endif /* SBDDH_GMP */

/* Signed overflow is undefined behavior in C++, so the weight
   computations in DDIndex check the range before each operation and
   report an error instead of overflowing. */
sbddextended_INLINE_FUNC
llint sbddh_checkedAdd(llint v1, llint v2)
{
    if ((v2 > 0 && v1 > LLONG_MAX - v2)
            || (v2 < 0 && v1 < LLONG_MIN - v2)) {
        std::cerr << "The weight computation causes an overflow of "
                     "long long int." << std::endl;
        exit(1);
    }
    return v1 + v2;
}

/* Multiplication of a weight and a number of sets. */
sbddextended_INLINE_FUNC
llint sbddh_checkedMul(llint v1, ullint v2)
{
    if (v1 == 0 || v2 == 0) {
        return 0;
    }
    if (v1 > 0) {
        if (v2 > static_cast<ullint>(LLONG_MAX) / static_cast<ullint>(v1)) {
            std::cerr << "The weight computation causes an overflow of "
                         "long long int." << std::endl;
            exit(1);
        }
        return v1 * static_cast<llint>(v2);
    } else {
        /* -LLONG_MIN cannot be represented as llint, so compute the
           absolute value of v1 in ullint. */
        const ullint abs_v1 = static_cast<ullint>(-(v1 + 1)) + 1;
        const ullint limit = static_cast<ullint>(LLONG_MAX) + 1;
        if (v2 > limit / abs_v1) {
            std::cerr << "The weight computation causes an overflow of "
                         "long long int." << std::endl;
            exit(1);
        }
        const ullint product = abs_v1 * v2;
        if (product == limit) {
            return LLONG_MIN;
        }
        return -static_cast<llint>(product);
    }
}

template<typename value_t>
value_t sbddh_getZero()
{
    return value_t(0);
}

template<typename value_t>
value_t sbddh_getOne()
{
    return value_t(1);
}

template<typename value_t>
value_t sbddh_getCard(const ZBDD& f);

#ifdef SBDDH_GMP
template<typename value_t>
value_t sbddh_getValueFromMpz(const mpz_class& v);

template<>
inline
mpz_class sbddh_getValueFromMpz<mpz_class>(const mpz_class& v)
{
    return v;
}

template<>
inline
ullint sbddh_getValueFromMpz<ullint>(const mpz_class& v)
{
    return sbddh_mpz_to_ullint(v);
}

#else
template<typename value_t>
value_t sbddh_getValueFromMpz(value_t v);

template<>
inline
ullint sbddh_getValueFromMpz<ullint>(ullint v)
{
    return v;
}

#endif

#endif /* __cplusplus */


/* snprintf entered the standard in C99 and in C++11, but it is also */
/* declared by every hosted implementation that provides the C99 or the */
/* POSIX.1-2001 library, which includes glibc in the C++98 mode. Detect */
/* the function as widely as possible and prefer it; where it cannot be */
/* detected (e.g. strict C++98), the macros below fall back on */
/* sbddextended_snprintf, a bounded implementation of this file. */
/* The detection below is heuristic (it looks at the compiler and the */
/* feature-test macros, which do not fully determine what the C library */
/* of the target declares), so the user can override it by defining */
/* SBDDH_HAS_SNPRINTF (the C library provides snprintf) or */
/* SBDDH_NO_SNPRINTF (always use the fallback) before including this */
/* header. */
#if defined(SBDDH_NO_SNPRINTF)
    /* use the fallback */
#elif defined(SBDDH_HAS_SNPRINTF)
    #define SBDDH_SNPRINTF_EXISTS
#else

#ifdef __cplusplus /* C++ */
    #if __cplusplus >= 201103L /* C++11 */
        #define SBDDH_SNPRINTF_EXISTS
    #endif
#else /* C */
    #ifdef __STDC_VERSION__
        #if __STDC_VERSION__ >= 199901L /* C99 */
            #define SBDDH_SNPRINTF_EXISTS
        #endif
    #endif
#endif

#ifdef __clang_major__
    #if __clang_major__ >= 13 /* clang version 13 */
        #define SBDDH_SNPRINTF_EXISTS
    #endif
#endif

#ifdef __USE_ISOC99 /* glibc declares the C99 functions */
    #define SBDDH_SNPRINTF_EXISTS
#endif

#ifdef _POSIX_C_SOURCE
    #if _POSIX_C_SOURCE >= 200112L /* POSIX.1-2001 */
        #define SBDDH_SNPRINTF_EXISTS
    #endif
#endif

#ifdef _XOPEN_SOURCE
    #if _XOPEN_SOURCE >= 600 /* XPG6 */
        #define SBDDH_SNPRINTF_EXISTS
    #endif
#endif

#ifdef __APPLE__
    #define SBDDH_SNPRINTF_EXISTS
#endif

#endif /* SBDDH_NO_SNPRINTF / SBDDH_HAS_SNPRINTF */

/* The following two functions are used to append a string to a buffer of */
/* sbddextended_BUFSIZE bytes in which n characters have already been */
/* written. Note that n may be negative or may exceed the buffer size */
/* because snprintf returns the number of characters that would have been */
/* written if the buffer were large enough. The write position and the */
/* remaining size are clamped so that the appended write never goes */
/* outside the buffer. */
sbddextended_INLINE_FUNC
size_t sbddextended_bufPos(int n)
{
    if (n < 0) {
        return 0;
    } else if ((size_t)n >= (size_t)sbddextended_BUFSIZE) {
        return (size_t)sbddextended_BUFSIZE - 1;
    }
    return (size_t)n;
}

sbddextended_INLINE_FUNC
size_t sbddextended_bufRest(int n)
{
    return (size_t)sbddextended_BUFSIZE - sbddextended_bufPos(n);
}

/* The functions below implement the part of snprintf that a strict */
/* C++98 environment lacks. They are compiled unconditionally so that */
/* every build checks them and the tests can exercise them, but the */
/* sbddextended_snprintfN macros call them only when the snprintf of */
/* the C library has not been detected above. */
/* Supported: the conversions d, i, u, o, x, X, c, s, p, f, F, e, E, */
/* g, G and %%, the flags "-+ #0", a width and a precision (both */
/* possibly '*') and the length modifiers hh, h, l, ll and z. A long */
/* double (the L modifier) is formatted with the range and the */
/* precision of a double, and the precision of a floating conversion */
/* is treated as at most 128 (a larger one is used as 128, in both the */
/* output and the returned length; this is a documented deviation from */
/* snprintf). An unsupported conversion without an argument is printed */
/* verbatim; for a and A the double argument is consumed and the */
/* conversion is printed verbatim. For n, the wide conversions lc and */
/* ls, and the length modifiers j and t, there is no portable way to */
/* consume the argument (its exact type is not known here), so the */
/* rest of the format is printed verbatim and processing stops - */
/* continuing would make the following conversions read the wrong */
/* arguments, which is undefined behavior. */
/* Like snprintf, the functions write at most size bytes */
/* including the terminating null, null-terminate the output whenever */
/* size >= 1 and return the length that the complete output needs. */

typedef struct tagsbddh_SnprintfBuf {
    char* str;
    size_t size;
    size_t total; /* the length the output needs so far; the characters
                     at the positions >= size - 1 are counted, not
                     stored */
} sbddh_SnprintfBuf;

typedef struct tagsbddh_SnprintfSpec {
    int minus;
    int plus;
    int space;
    int hash;
    int zero;
    int width;     /* 0 when not specified */
    int precision; /* -1 when not specified */
} sbddh_SnprintfSpec;

sbddextended_INLINE_FUNC
void sbddh_snPutChar(sbddh_SnprintfBuf* buf, char c)
{
    if (buf->str != NULL && buf->size > 0 && buf->total < buf->size - 1) {
        buf->str[buf->total] = c;
    }
    /* saturate instead of wrapping around, as sbddh_snPutPadding
       does: the counter can already be at its maximum there, and a
       wrap would turn the "the output does not fit in an int" result
       into a small positive length */
    if (buf->total != (size_t)-1) {
        ++buf->total;
    }
}

sbddextended_INLINE_FUNC
void sbddh_snPutPadding(sbddh_SnprintfBuf* buf, char c, size_t len)
{
    size_t i;
    size_t storable = 0;

    /* Store only the characters that fit and count the rest in one
       step: a '*' width can request about 2^31 characters, and putting
       them one by one would spin that long even though the buffer is
       already full (or str is NULL). */
    if (buf->str != NULL && buf->size > 0 && buf->total < buf->size - 1) {
        storable = buf->size - 1 - buf->total;
        if (storable > len) {
            storable = len;
        }
        for (i = 0; i < storable; ++i) {
            buf->str[buf->total + i] = c;
        }
    }
    /* saturate instead of wrapping around; the caller then reports the
       overlong result through the "total > INT_MAX" check */
    if (len > (size_t)-1 - buf->total) {
        buf->total = (size_t)-1;
    } else {
        buf->total += len;
    }
}

sbddextended_INLINE_FUNC
void sbddh_snPutChars(sbddh_SnprintfBuf* buf, const char* s, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i) {
        sbddh_snPutChar(buf, s[i]);
    }
}

/* The fixed scratch buffers of the two formatting helpers below assume
   that ullint is at most 64 bits wide (digits[24] holds its octal form)
   and that the integer part of the widest finite double has at most 309
   decimal digits (scratch[512] holds the %f form). Both hold on every
   supported platform, but neither is guaranteed by the language, so
   state them as compile-time conditions (a negative array size) rather
   than overflow the buffers on an exotic implementation. */
typedef char sbddh_snprintfUllintFitsDigits[
    (sizeof(ullint) * CHAR_BIT <= 64) ? 1 : -1];
typedef char sbddh_snprintfDoubleFitsScratch[
    (DBL_MAX_10_EXP <= 308) ? 1 : -1];

sbddextended_INLINE_FUNC
void sbddh_snPutInteger(sbddh_SnprintfBuf* buf,
                        const sbddh_SnprintfSpec* spec,
                        int is_signed, int negative, ullint magnitude,
                        ullint base, int uppercase)
{
    const char* digit_chars = (uppercase ? "0123456789ABCDEF"
                                            : "0123456789abcdef");
    char digits[24]; /* 64 bits need at most 22 octal digits; the width
                        of ullint is checked at the definition of
                        sbddh_snprintfUllintFitsDigits below */
    size_t ndigits = 0;
    size_t nzeros = 0;
    size_t left_spaces = 0;
    size_t right_spaces = 0;
    size_t len;
    size_t npad;
    int with_prefix;
    char sign = '\0';

    while (magnitude > 0) {
        digits[ndigits] = digit_chars[magnitude % base];
        magnitude /= base;
        ++ndigits;
    }
    if (spec->precision >= 0) {
        if ((size_t)spec->precision > ndigits) {
            nzeros = (size_t)spec->precision - ndigits;
        }
    } else if (ndigits == 0) {
        nzeros = 1; /* the default precision is one */
    }
    /* '#' forces the leading zero of the octal form */
    if (spec->hash && base == 8 && nzeros == 0) {
        nzeros = 1;
    }
    /* '#' prefixes a non-zero hexadecimal value with 0x */
    with_prefix = (spec->hash && base == 16 && ndigits > 0);
    if (is_signed) { /* '+' and ' ' apply only to d and i */
        if (negative) {
            sign = '-';
        } else if (spec->plus) {
            sign = '+';
        } else if (spec->space) {
            sign = ' ';
        }
    }
    len = (sign != '\0' ? 1u : 0u) + (with_prefix ? 2u : 0u)
            + nzeros + ndigits;
    npad = ((size_t)spec->width > len ? (size_t)spec->width - len : 0u);
    if (spec->minus) {
        right_spaces = npad;
    } else if (spec->zero && spec->precision < 0) {
        /* '0' pads with zeros after the sign and the prefix, and is */
        /* ignored when a precision is given */
        nzeros += npad;
    } else {
        left_spaces = npad;
    }

    sbddh_snPutPadding(buf, ' ', left_spaces);
    if (sign != '\0') {
        sbddh_snPutChar(buf, sign);
    }
    if (with_prefix) {
        sbddh_snPutChar(buf, '0');
        sbddh_snPutChar(buf, (char)(uppercase ? 'X' : 'x'));
    }
    sbddh_snPutPadding(buf, '0', nzeros);
    while (ndigits > 0) {
        --ndigits;
        sbddh_snPutChar(buf, digits[ndigits]);
    }
    sbddh_snPutPadding(buf, ' ', right_spaces);
}

sbddextended_INLINE_FUNC
void sbddh_snPutFloating(sbddh_SnprintfBuf* buf,
                         const sbddh_SnprintfSpec* spec,
                         char conv, double value)
{
    /* sprintf is reached through a plain function pointer, which */
    /* carries no format attribute, so that the computed format below */
    /* is not warned about by -Wformat-nonliteral. */
    int (* const psprintf)(char*, const char*, ...) = sprintf;
    /* Both bounds below keep the sprintf result inside scratch: the */
    /* widest double in the %f form has 309 digits before the decimal */
    /* point, and the precision adds at most 128 digits after it. */
    char scratch[512];
    char fmt[8];
    size_t fmt_len = 0;
    int prec = spec->precision;
    size_t len = 0;
    size_t npad;
    size_t start = 0;
    int numeric = 1;

    if (prec < 0) {
        prec = 6; /* the default precision */
    } else if (prec > 128) {
        /* The supported maximum (see the contract above the buffer
           struct): both the output and the returned length use 128
           then, which keeps the sprintf result inside scratch. */
        prec = 128;
    }
    fmt[fmt_len++] = '%';
    if (spec->plus) {
        fmt[fmt_len++] = '+';
    } else if (spec->space) {
        fmt[fmt_len++] = ' ';
    }
    if (spec->hash) {
        fmt[fmt_len++] = '#';
    }
    fmt[fmt_len++] = '.';
    fmt[fmt_len++] = '*';
    /* %F entered the standard with C99; produce it from %f below */
    fmt[fmt_len++] = (char)(conv == 'F' ? 'f' : conv);
    fmt[fmt_len] = '\0';
    psprintf(scratch, fmt, prec, value);
    while (scratch[len] != '\0') {
        char c = scratch[len];
        /* inf and nan contain one of these; a number never does. */
        /* %E and %G make sprintf produce the uppercase INF and NAN, */
        /* so check both cases. */
        if (c == 'i' || c == 'n' || c == 'a'
                || c == 'I' || c == 'N' || c == 'A') {
            numeric = 0;
        }
        if (conv == 'F' && c >= 'a' && c <= 'z') {
            scratch[len] = (char)(c - ('a' - 'A'));
        }
        ++len;
    }
    npad = ((size_t)spec->width > len ? (size_t)spec->width - len : 0u);
    if (spec->minus) {
        sbddh_snPutChars(buf, scratch, len);
        sbddh_snPutPadding(buf, ' ', npad);
    } else if (spec->zero && numeric) {
        /* the zeros go after the sign; inf and nan are padded with */
        /* spaces below instead */
        if (scratch[0] == '-' || scratch[0] == '+'
                || scratch[0] == ' ') {
            sbddh_snPutChar(buf, scratch[0]);
            start = 1;
        }
        sbddh_snPutPadding(buf, '0', npad);
        sbddh_snPutChars(buf, scratch + start, len - start);
    } else {
        sbddh_snPutPadding(buf, ' ', npad);
        sbddh_snPutChars(buf, scratch, len);
    }
}

sbddextended_INLINE_FUNC
int sbddextended_vsnprintf(char* str, size_t size, const char* format,
                           va_list args)
{
    sbddh_SnprintfBuf buf;
    const char* p = format;

    buf.str = str;
    buf.size = size;
    buf.total = 0;

    while (*p != '\0') {
        const char* spec_start;
        sbddh_SnprintfSpec spec;
        int lmod; /* -2 hh, -1 h, 0 none, 1 l, 2 ll, 3 z, 4 L */
        char conv;

        if (*p != '%') {
            sbddh_snPutChar(&buf, *p);
            ++p;
            continue;
        }
        spec_start = p;
        ++p;
        spec.minus = 0;
        spec.plus = 0;
        spec.space = 0;
        spec.hash = 0;
        spec.zero = 0;
        spec.width = 0;
        spec.precision = -1;
        for (;;) {
            if (*p == '-') {
                spec.minus = 1;
            } else if (*p == '+') {
                spec.plus = 1;
            } else if (*p == ' ') {
                spec.space = 1;
            } else if (*p == '#') {
                spec.hash = 1;
            } else if (*p == '0') {
                spec.zero = 1;
            } else {
                break;
            }
            ++p;
        }
        if (*p == '*') {
            spec.width = va_arg(args, int);
            if (spec.width < 0) { /* a negative width means '-' */
                spec.minus = 1;
                spec.width = (spec.width == INT_MIN ? INT_MAX
                                                    : -spec.width);
            }
            ++p;
        } else {
            while (*p >= '0' && *p <= '9') {
                if (spec.width <= (INT_MAX - 9) / 10) {
                    spec.width = spec.width * 10 + (*p - '0');
                }
                ++p;
            }
        }
        if (*p == '.') {
            ++p;
            spec.precision = 0;
            if (*p == '*') {
                spec.precision = va_arg(args, int);
                if (spec.precision < 0) { /* means not specified */
                    spec.precision = -1;
                }
                ++p;
            } else {
                while (*p >= '0' && *p <= '9') {
                    if (spec.precision <= (INT_MAX - 9) / 10) {
                        spec.precision = spec.precision * 10
                                            + (*p - '0');
                    }
                    ++p;
                }
            }
        }
        lmod = 0;
        if (*p == 'h') {
            lmod = -1;
            ++p;
            if (*p == 'h') {
                lmod = -2;
                ++p;
            }
        } else if (*p == 'l') {
            lmod = 1;
            ++p;
            if (*p == 'l') {
                lmod = 2;
                ++p;
            }
        } else if (*p == 'z') {
            lmod = 3;
            ++p;
        } else if (*p == 'L') {
            lmod = 4;
            ++p;
        } else if (*p == 'j' || *p == 't') {
            lmod = 5; /* unsupported: intmax_t and ptrdiff_t are not
                         available in every environment that needs this
                         fallback (strict C++98) */
            ++p;
        }
        conv = *p;
        /* a and A are not supported; the double argument is consumed */
        /* so that the following conversions read their own arguments, */
        /* and the conversion is printed verbatim. */
        if ((conv == 'a' || conv == 'A') && lmod != 5) {
            const char* q;
            if (lmod == 4) {
                (void)va_arg(args, long double);
            } else {
                (void)va_arg(args, double);
            }
            for (q = spec_start; q != p; ++q) {
                sbddh_snPutChar(&buf, *q);
            }
            sbddh_snPutChar(&buf, *p);
            ++p;
            continue;
        }
        /* For n, the wide conversions lc and ls, and the j and t */
        /* length modifiers, there is no portable way to consume the */
        /* argument: va_arg must name the exact (promoted) type that */
        /* the caller passed, and reading e.g. an int* argument of %n */
        /* as void*, or an intmax_t argument of %jd as some other */
        /* type, is undefined behavior. Print the rest of the format */
        /* verbatim and stop, so that the following conversions cannot */
        /* read misaligned arguments. */
        if (conv == 'n' || lmod == 5
                || ((conv == 'c' || conv == 's') && lmod == 1)) {
            while (*spec_start != '\0') {
                sbddh_snPutChar(&buf, *spec_start);
                ++spec_start;
            }
            break;
        }
        switch (conv) {
        case 'd':
        case 'i':
        {
            llint v;
            ullint magnitude;
            int negative = 0;
            if (lmod == 2) {
                v = va_arg(args, llint);
            } else if (lmod == 1) {
                v = (llint)va_arg(args, long);
            } else if (lmod == 3) {
                /* The signed companion of size_t is not available
                   before C99, and reading a negative signed argument
                   through va_arg(args, size_t) is undefined behavior
                   (a negative value is not representable in the
                   unsigned type), so pick the signed type of the same
                   width as size_t. */
                if (sizeof(size_t) == sizeof(int)) {
                    v = (llint)va_arg(args, int);
                } else if (sizeof(size_t) == sizeof(long)) {
                    v = (llint)va_arg(args, long);
                } else {
                    v = va_arg(args, llint);
                }
            } else {
                v = (llint)va_arg(args, int);
            }
            if (lmod == -1) {
                v = (llint)(short)v;
            } else if (lmod == -2) {
                v = (llint)(signed char)v;
            }
            if (v < 0) {
                negative = 1;
                magnitude = (ullint)0 - (ullint)v;
            } else {
                magnitude = (ullint)v;
            }
            sbddh_snPutInteger(&buf, &spec, 1, negative, magnitude,
                                10u, 0);
            ++p;
            break;
        }
        case 'u':
        case 'o':
        case 'x':
        case 'X':
        {
            ullint magnitude;
            ullint base = (conv == 'u' ? 10u : (conv == 'o' ? 8u
                                                            : 16u));
            if (lmod == 2) {
                magnitude = va_arg(args, ullint);
            } else if (lmod == 1) {
                magnitude = (ullint)va_arg(args, unsigned long);
            } else if (lmod == 3) {
                magnitude = (ullint)va_arg(args, size_t);
            } else {
                magnitude = (ullint)va_arg(args, unsigned int);
            }
            if (lmod == -1) {
                magnitude = (ullint)(unsigned short)magnitude;
            } else if (lmod == -2) {
                magnitude = (ullint)(unsigned char)magnitude;
            }
            sbddh_snPutInteger(&buf, &spec, 0, 0, magnitude, base,
                                (conv == 'X' ? 1 : 0));
            ++p;
            break;
        }
        case 'c':
        {
            char c = (char)va_arg(args, int);
            size_t npad = (spec.width > 1 ? (size_t)spec.width - 1u
                                            : 0u);
            if (!spec.minus) {
                sbddh_snPutPadding(&buf, ' ', npad);
            }
            sbddh_snPutChar(&buf, c);
            if (spec.minus) {
                sbddh_snPutPadding(&buf, ' ', npad);
            }
            ++p;
            break;
        }
        case 's':
        {
            const char* s = va_arg(args, const char*);
            size_t len = 0;
            size_t npad;
            if (s == NULL) { /* print the null pointer like glibc */
                s = "(null)";
            }
            /* with a precision the array may end without a null */
            /* character, so do not go beyond it */
            while ((spec.precision < 0
                        || len < (size_t)spec.precision)
                    && s[len] != '\0') {
                ++len;
            }
            npad = ((size_t)spec.width > len
                        ? (size_t)spec.width - len : 0u);
            if (!spec.minus) {
                sbddh_snPutPadding(&buf, ' ', npad);
            }
            sbddh_snPutChars(&buf, s, len);
            if (spec.minus) {
                sbddh_snPutPadding(&buf, ' ', npad);
            }
            ++p;
            break;
        }
        case 'p':
        {
            const void* ptr = va_arg(args, void*);
            spec.hash = 1; /* print the address in the 0x... form */
            sbddh_snPutInteger(&buf, &spec, 0, 0,
                                (ullint)(size_t)ptr, 16u, 0);
            ++p;
            break;
        }
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
        {
            double v;
            if (lmod == 4) {
                v = (double)va_arg(args, long double);
            } else {
                v = va_arg(args, double);
            }
            sbddh_snPutFloating(&buf, &spec, conv, v);
            ++p;
            break;
        }
        case '%':
        {
            sbddh_snPutChar(&buf, '%');
            ++p;
            break;
        }
        default: /* print an unsupported conversion verbatim */
        {
            const char* q;
            for (q = spec_start; q != p; ++q) {
                sbddh_snPutChar(&buf, *q);
            }
            if (*p != '\0') {
                sbddh_snPutChar(&buf, *p);
                ++p;
            }
            break;
        }
        }
    }
    if (str != NULL && size > 0) {
        str[buf.total < size - 1 ? buf.total : size - 1] = '\0';
    }
    if (buf.total > (size_t)INT_MAX) {
        return -1;
    }
    return (int)buf.total;
}

sbddextended_INLINE_FUNC
#ifdef __GNUC__
__attribute__((format(printf, 3, 4)))
#endif
int sbddextended_snprintf(char* str, size_t size, const char* format,
                          ...)
{
    int v;
    va_list args;

    va_start(args, format);
    v = sbddextended_vsnprintf(str, size, format, args);
    va_end(args);
    return v;
}

#ifdef SBDDH_SNPRINTF_EXISTS

/* We use the following macros instead of vsnprintf because passing */
/* a variable to the third argument of vsnprintf outputs warning */
/* "format string is not a string literal [-Wformat-nonliteral]". */

#define sbddextended_snprintf0(str, size, format) \
snprintf(str, size, format)
#define sbddextended_snprintf1(str, size, format, arg1) \
snprintf(str, size, format, arg1)
#define sbddextended_snprintf2(str, size, format, arg1, arg2) \
snprintf(str, size, format, arg1, arg2)
#define sbddextended_snprintf3(str, size, format, arg1, arg2, arg3) \
snprintf(str, size, format, arg1, arg2, arg3)
#define sbddextended_snprintf4(str, size, format, arg1, arg2, arg3, arg4) \
snprintf(str, size, format, arg1, arg2, arg3, arg4)
#define sbddextended_snprintf5(str, size, format, arg1, arg2, arg3, arg4, \
arg5) \
snprintf(str, size, format, arg1, arg2, arg3, arg4, arg5)

#else /* SBDDH_SNPRINTF_EXISTS */

#define sbddextended_snprintf0(str, size, format) \
sbddextended_snprintf(str, size, format)
#define sbddextended_snprintf1(str, size, format, arg1) \
sbddextended_snprintf(str, size, format, arg1)
#define sbddextended_snprintf2(str, size, format, arg1, arg2) \
sbddextended_snprintf(str, size, format, arg1, arg2)
#define sbddextended_snprintf3(str, size, format, arg1, arg2, arg3) \
sbddextended_snprintf(str, size, format, arg1, arg2, arg3)
#define sbddextended_snprintf4(str, size, format, arg1, arg2, arg3, arg4) \
sbddextended_snprintf(str, size, format, arg1, arg2, arg3, arg4)
#define sbddextended_snprintf5(str, size, format, arg1, arg2, arg3, arg4, \
arg5) \
sbddextended_snprintf(str, size, format, arg1, arg2, arg3, arg4, arg5)

#endif /* SBDDH_SNPRINTF_EXISTS */

#define sbddextended_MyVector_INITIAL_BUFSIZE 1024

/* Internal type. It is not part of the public API.                    */
/* Raw struct assignment (e.g. "u = v;") must not be used because it   */
/* copies only the owning pointer ("vec" or "buf"), which leads to     */
/* use-after-free and double free. Use sbddextended_MyVector_copy      */
/* instead.                                                            */
/* On memory exhaustion, the C version prints an error and calls       */
/* exit(1), while the C++ version propagates std::bad_alloc and leaves */
/* the vector consistent (vec->size() == count) and usable; C++        */
/* callers that own a MyVector must deinitialize it when an exception  */
/* propagates through them.                                            */
typedef struct tagsbddextended_MyVector {
#ifdef __cplusplus
    std::vector<llint>* vec;
#endif
    /* in the C++ version, always vec.size() == count */
    size_t count;
#ifndef __cplusplus
    size_t capacity;
    llint* buf;
#endif
} sbddextended_MyVector;

/* "v" must be uninitialized or deinitialized. Calling this on an      */
/* already initialized vector leaks the previously owned buffer        */
/* (this function cannot distinguish garbage from an owned pointer,    */
/* so it never frees the previous content).                            */
sbddextended_INLINE_FUNC
void sbddextended_MyVector_initialize(sbddextended_MyVector* v)
{
#ifdef __cplusplus
    /* Put "v" into the deinitialized state first so that if "new"
       throws, the caller can safely call
       sbddextended_MyVector_deinitialize on "v". */
    v->vec = NULL;
    v->count = 0;
    v->vec = new std::vector<llint>();
#else
    v->count = 0;
    v->capacity = sbddextended_MyVector_INITIAL_BUFSIZE;
    v->buf = (llint*)malloc(v->capacity * sizeof(llint));
    if (v->buf == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
#endif
}

/* After this function returns, the only functions that may be called  */
/* on "v" are sbddextended_MyVector_initialize (to reuse "v") and      */
/* sbddextended_MyVector_deinitialize (that is, deinitializing twice   */
/* is safe).                                                           */
sbddextended_INLINE_FUNC
void sbddextended_MyVector_deinitialize(sbddextended_MyVector* v)
{
#ifdef __cplusplus
    delete v->vec;
    v->vec = NULL;
    v->count = 0;
#else
    free(v->buf);
    v->buf = NULL;
    v->count = 0;
    v->capacity = 0;
#endif
}

/* "v_index" must be in the range [0, count). This precondition is     */
/* checked only by assert, so it is the caller's responsibility        */
/* (out-of-range access is undefined behavior in NDEBUG builds).       */
sbddextended_INLINE_FUNC
llint sbddextended_MyVector_get(const sbddextended_MyVector* v, llint v_index)
{
#ifdef __cplusplus
    assert(0 <= v_index && (size_t)v_index < v->vec->size());
    return (*v->vec)[(size_t)v_index];
#else
    assert(0 <= v_index && (size_t)v_index < v->count);
    return v->buf[v_index];
#endif
}

/* "v_index" must be in the range [0, count). This precondition is     */
/* checked only by assert, so it is the caller's responsibility        */
/* (out-of-range access is undefined behavior in NDEBUG builds).       */
sbddextended_INLINE_FUNC
void sbddextended_MyVector_set(sbddextended_MyVector* v,
                                llint v_index, llint value)
{
#ifdef __cplusplus
    assert(0 <= v_index && (size_t)v_index < v->vec->size());
    (*v->vec)[(size_t)v_index] = value;
#else
    assert(0 <= v_index && (size_t)v_index < v->count);
    v->buf[v_index] = value;
#endif
}

sbddextended_INLINE_FUNC
void sbddextended_MyVector_add(sbddextended_MyVector* v, llint value)
{
#ifdef __cplusplus
    (*v->vec).push_back(value);
    ++v->count;
    assert(v->vec->size() == static_cast<size_t>(v->count));
#else
    if (v->count >= v->capacity) {
        /* guard against the overflow of both "capacity * 2" and */
        /* "capacity * 2 * sizeof(llint)" */
        if (v->capacity > (size_t)-1 / 2 / sizeof(llint)) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        v->capacity *= 2;
        assert(v->count < v->capacity);
        v->buf = (llint*)realloc(v->buf, v->capacity * sizeof(llint));
        if (v->buf == NULL) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
    }
    v->buf[v->count] = value;
    ++v->count;
#endif
}

/* "dest" must be initialized before calling this function. */
/* The current content of "dest" is discarded. */
sbddextended_INLINE_FUNC
void sbddextended_MyVector_copy(sbddextended_MyVector* dest,
                                const sbddextended_MyVector* src)
{
#ifndef __cplusplus
    llint* buf;
    size_t capacity;
#endif

    if (dest == src) {
        return;
    }
#ifdef __cplusplus
    /* Copy into a temporary and swap so that dest keeps the invariant */
    /* "vec.size() == count" when the copy throws std::bad_alloc. */
    std::vector<llint> tmp(*src->vec);
    dest->vec->swap(tmp);
    dest->count = src->count;
#else
    capacity = sbddextended_MyVector_INITIAL_BUFSIZE;
    if (capacity < src->count) {
        capacity = src->count;
    }
    /* "capacity * sizeof(llint)" cannot overflow because "src->buf" */
    /* already holds "src->count" elements */
    buf = (llint*)malloc(capacity * sizeof(llint));
    if (buf == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    memcpy(buf, src->buf, src->count * sizeof(llint));
    free(dest->buf);
    dest->buf = buf;
    dest->count = src->count;
    dest->capacity = capacity;
#endif
}

/* The vector must not be empty. This precondition is checked only by  */
/* assert (popping an empty vector is undefined behavior in NDEBUG     */
/* builds).                                                            */
sbddextended_INLINE_FUNC
void sbddextended_MyVector_pop_back(sbddextended_MyVector* v)
{
#ifdef __cplusplus
    assert(!v->vec->empty());
    v->vec->pop_back();
    --v->count;
#else
    assert(v->count > 0);
    --v->count;
#endif
}

#ifndef __cplusplus

/* The dictionary is an AVL tree. Without the rebalancing, inserting the
   keys in the increasing order, which is what the importers of the file
   formats do, would build a tree of height n and make each insertion and
   each lookup take O(n) time. */

typedef struct tagsbddextended_MyDictNode {
    struct tagsbddextended_MyDictNode* left;
    struct tagsbddextended_MyDictNode* right;
    llint key;
    llint value;
    /* the height of the subtree whose root is this node (a leaf has 1) */
    int height;
} sbddextended_MyDictNode;

/* An AVL tree of height h has at least Fib(h + 2) - 1 nodes, so a tree of
   height 92 has more than 2^63 nodes and cannot be built in practice. */
#define sbddextended_MYDICT_MAXHEIGHT 92

sbddextended_INLINE_FUNC
sbddextended_MyDictNode* sbddextended_MyDictNode_makeNewNode(llint key,
                                                                llint value)
{
    sbddextended_MyDictNode* node;

    node = (sbddextended_MyDictNode*)malloc(sizeof(sbddextended_MyDictNode));
    if (node == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    node->left = NULL;
    node->right = NULL;
    node->key = key;
    node->value = value;
    node->height = 1;
    return node;
}

sbddextended_INLINE_FUNC
int sbddextended_MyDictNode_height(const sbddextended_MyDictNode* node)
{
    return (node == NULL ? 0 : node->height);
}

sbddextended_INLINE_FUNC
void sbddextended_MyDictNode_updateHeight(sbddextended_MyDictNode* node)
{
    int hl;
    int hr;

    hl = sbddextended_MyDictNode_height(node->left);
    hr = sbddextended_MyDictNode_height(node->right);
    node->height = (hl > hr ? hl : hr) + 1;
}

sbddextended_INLINE_FUNC
sbddextended_MyDictNode* sbddextended_MyDictNode_rotateLeft(
                                        sbddextended_MyDictNode* node)
{
    sbddextended_MyDictNode* r;

    r = node->right;
    node->right = r->left;
    r->left = node;
    sbddextended_MyDictNode_updateHeight(node);
    sbddextended_MyDictNode_updateHeight(r);
    return r;
}

sbddextended_INLINE_FUNC
sbddextended_MyDictNode* sbddextended_MyDictNode_rotateRight(
                                        sbddextended_MyDictNode* node)
{
    sbddextended_MyDictNode* l;

    l = node->left;
    node->left = l->right;
    l->right = node;
    sbddextended_MyDictNode_updateHeight(node);
    sbddextended_MyDictNode_updateHeight(l);
    return l;
}

/* Rebalance the subtree whose root is "node". Both children must be
   balanced and their heights must differ by at most 2. The root of the
   rebalanced subtree is returned. */
sbddextended_INLINE_FUNC
sbddextended_MyDictNode* sbddextended_MyDictNode_balance(
                                        sbddextended_MyDictNode* node)
{
    int balance;

    sbddextended_MyDictNode_updateHeight(node);
    balance = sbddextended_MyDictNode_height(node->left)
                - sbddextended_MyDictNode_height(node->right);
    if (balance > 1) {
        if (sbddextended_MyDictNode_height(node->left->left)
                < sbddextended_MyDictNode_height(node->left->right)) {
            node->left = sbddextended_MyDictNode_rotateLeft(node->left);
        }
        return sbddextended_MyDictNode_rotateRight(node);
    } else if (balance < -1) {
        if (sbddextended_MyDictNode_height(node->right->right)
                < sbddextended_MyDictNode_height(node->right->left)) {
            node->right = sbddextended_MyDictNode_rotateRight(node->right);
        }
        return sbddextended_MyDictNode_rotateLeft(node);
    }
    return node;
}

#endif

/* Internal type. It is not part of the public API. */
/* Raw struct assignment (e.g. "u = v;") must not be used because it   */
/* copies only the owning pointer ("dict" or "root"), which leads to   */
/* use-after-free and double free. Use sbddextended_MyDict_copy        */
/* instead.                                                            */
/* On memory exhaustion, the C version prints an error and calls        */
/* exit(1), while the C++ version propagates std::bad_alloc and leaves  */
/* the dictionary consistent (count == dict->size()) and usable; C++    */
/* callers that own a MyDict must deinitialize it when an exception     */
/* propagates through them.                                             */
typedef struct tagsbddextended_MyDict {
#ifdef __cplusplus
    std::map<llint, llint>* dict;
#endif
    size_t count;
#ifndef __cplusplus
    sbddextended_MyDictNode* root;
#endif
} sbddextended_MyDict;

/* "d" must be uninitialized or deinitialized. Calling this on an      */
/* already initialized dictionary leaks the previously owned tree or   */
/* container (this function cannot distinguish garbage from an owned   */
/* pointer, so it never frees the previous content).                   */
sbddextended_INLINE_FUNC
void sbddextended_MyDict_initialize(sbddextended_MyDict* d)
{
#ifdef __cplusplus
    /* Put "d" into the deinitialized state first so that if "new"
       throws, the caller can safely call
       sbddextended_MyDict_deinitialize on "d". */
    d->dict = NULL;
    d->count = 0;
    d->dict = new std::map<llint, llint>();
#else
    d->count = 0;
    d->root = NULL;
#endif
}

sbddextended_INLINE_FUNC
void sbddextended_MyDict_deinitialize(sbddextended_MyDict* d)
{
#ifdef __cplusplus
    delete d->dict;
    d->dict = NULL;
    d->count = 0;
#else
    /* The traversal stack depth is bounded by the tree height, which */
    /* is less than sbddextended_MYDICT_MAXHEIGHT (see the comment on  */
    /* that macro). */
    sbddextended_MyDictNode* node_stack[sbddextended_MYDICT_MAXHEIGHT];
    char op_stack[sbddextended_MYDICT_MAXHEIGHT];
    char op;
    int sp;
    sbddextended_MyDictNode* node;
    sbddextended_MyDictNode* child;
#ifndef NDEBUG
    /* used only by the asserts below, which a build with NDEBUG
       removes together with every use of this variable */
    size_t debug_count;
#endif

    if (d->root == NULL) {
        assert(d->count == 0);
        return;
    }

    assert((debug_count = 0) || 1);

    sp = 0;
    node_stack[sp] = d->root;
    op_stack[sp] = 0;

    /* free each node (not using a recursive function) */
    while (sp >= 0) {
        node = node_stack[sp];
        op = op_stack[sp];

        if (node == NULL) {
            op = 2;
        }

        while (op <= 1) {
            if (op == 0) {
                child = node->left;
            } else { /* op == 1 */
                child = node->right;
            }
            if (child == NULL) {
                ++op;
                ++op_stack[sp];
            } else {
                break;
            }
        }
        if (op <= 1) {
            ++sp;
            assert(sp < sbddextended_MYDICT_MAXHEIGHT);
            node_stack[sp] = child;
            op_stack[sp] = 0;
        } else {
            assert((++debug_count) || 1);
            free(node_stack[sp]);
            --sp;
            if (sp < 0) {
                break;
            }
            ++op_stack[sp];
        }
    }
    assert(debug_count == d->count);
    d->count = 0;
    d->root = NULL;
#endif
}


sbddextended_INLINE_FUNC
void sbddextended_MyDict_add(sbddextended_MyDict* d, llint key, llint value)
{
#ifdef __cplusplus
    /* Insert before updating count so that count stays equal to */
    /* dict->size() when the node allocation throws std::bad_alloc. */
    std::pair<std::map<llint, llint>::iterator, bool> result =
        d->dict->insert(std::make_pair(key, value));
    if (result.second) { /* newly inserted */
        ++d->count;
    } else { /* the key already exists */
        result.first->second = value;
    }
    assert(d->dict->size() == static_cast<size_t>(d->count));
#else
    sbddextended_MyDictNode* path[sbddextended_MYDICT_MAXHEIGHT];
    char dir[sbddextended_MYDICT_MAXHEIGHT];
    sbddextended_MyDictNode* node;
    int sp;
    int i;

    /* search for the key, remembering the path from the root */
    sp = 0;
    node = d->root;
    while (node != NULL) {
        if (node->key == key) { /* found */
            node->value = value;
            return;
        }
        assert(sp < sbddextended_MYDICT_MAXHEIGHT);
        path[sp] = node;
        if (key < node->key) {
            dir[sp] = 0;
            node = node->left;
        } else { /* key > node->key */
            dir[sp] = 1;
            node = node->right;
        }
        ++sp;
    }

    node = sbddextended_MyDictNode_makeNewNode(key, value);
    ++d->count;

    /* hang the new node and rebalance the path back to the root */
    for (i = sp - 1; i >= 0; --i) {
        if (dir[i] == 0) {
            path[i]->left = node;
        } else {
            path[i]->right = node;
        }
        node = sbddextended_MyDictNode_balance(path[i]);
    }
    d->root = node;
#endif
}

/* returned value: 1 -> found, 0 -> not found */
/* The found value is stored into "value" argument. */
sbddextended_INLINE_FUNC
int sbddextended_MyDict_find(const sbddextended_MyDict* d, llint key, llint* value)
{
#ifdef __cplusplus
    std::map<llint, llint>::const_iterator itor = d->dict->find(key);
    if (itor != d->dict->end()) {
        if (value != NULL) {
            *value = itor->second;
        }
        return 1;
    } else {
        return 0;
    }
#else
    sbddextended_MyDictNode* node;
    node = d->root;
    while (node != NULL) {
        if (node->key == key) {
            if (value != NULL) {
                *value = node->value;
            }
            return 1;
        } else if (key < node->key) {
            node = node->left;
        } else {/* key > node->key */
            node = node->right;
        }
    }
    return 0;
#endif
}

/* "dest" must be initialized before calling this function. */
/* The current content of "dest" is discarded. */
sbddextended_INLINE_FUNC
void sbddextended_MyDict_copy(sbddextended_MyDict* dest,
                                const sbddextended_MyDict* src)
{
#ifndef __cplusplus
    /* The traversal stack depth is bounded by the tree height, which */
    /* is less than sbddextended_MYDICT_MAXHEIGHT (see the comment on  */
    /* that macro). */
    sbddextended_MyDictNode* node_stack[sbddextended_MYDICT_MAXHEIGHT];
    sbddextended_MyDictNode* dest_node_stack[sbddextended_MYDICT_MAXHEIGHT];
    char op_stack[sbddextended_MYDICT_MAXHEIGHT];
    char op;
    int sp;
    sbddextended_MyDictNode* node;
    sbddextended_MyDictNode* child;
    sbddextended_MyDictNode* dest_node;
#ifndef NDEBUG
    /* see the comment in sbddextended_MyDict_deinitialize */
    size_t debug_count;
#endif
#endif

    if (dest == src) {
        return;
    }
#ifdef __cplusplus
    /* Copy into a temporary and swap so that dest keeps its original, */
    /* consistent content (count == dict->size()) when the copy throws */
    /* std::bad_alloc. */
    std::map<llint, llint> tmp(*src->dict);
    dest->dict->swap(tmp);
    dest->count = src->count;
#else
    /* discard the current content of dest */
    sbddextended_MyDict_deinitialize(dest);

    if (src->root == NULL) {
        assert(src->count == 0);
        dest->count = 0;
        dest->root = NULL;
        return;
    }

    assert((debug_count = 0) || 1);

    dest->root = sbddextended_MyDictNode_makeNewNode(src->root->key,
                                                        src->root->value);
    dest->root->key = src->root->key;
    dest->root->value = src->root->value;
    dest->root->height = src->root->height;

    sp = 0;
    node_stack[sp] = src->root;
    dest_node_stack[sp] = dest->root;
    op_stack[sp] = 0;

    /* copy each node (not using a recursive function) */
    while (sp >= 0) {
        node = node_stack[sp];
        dest_node = dest_node_stack[sp];
        op = op_stack[sp];

        if (node == NULL) {
            op = 2;
        }

        while (op <= 1) {
            if (op == 0) {
                child = node->left;
            } else { /* op == 1 */
                child = node->right;
            }
            if (child == NULL) {
                ++op;
                ++op_stack[sp];
            } else {
                break;
            }
        }
        if (op <= 1) {
            ++sp;
            assert(sp < sbddextended_MYDICT_MAXHEIGHT);
            node_stack[sp] = child;
            dest_node_stack[sp] =
                sbddextended_MyDictNode_makeNewNode(child->key, child->value);
            dest_node_stack[sp]->height = child->height;
            op_stack[sp] = 0;

            if (op == 0) {
                dest_node->left = dest_node_stack[sp];
            } else { /* op == 1 */
                dest_node->right = dest_node_stack[sp];
            }
        } else {
            assert((++debug_count) || 1);
            --sp;
            if (sp < 0) {
                break;
            }
            ++op_stack[sp];
        }
    }
    assert(debug_count == src->count);
    dest->count = src->count;
#endif
}

/* Internal type. It is not part of the public API.                    */
/* Raw struct assignment (e.g. "u = v;") must not be used because it   */
/* copies only the owning pointer ("se") or the owning tree ("dict"),  */
/* which leads to use-after-free and double free. Use                  */
/* sbddextended_MySet_copy instead.                                    */
/* On memory exhaustion, the C version prints an error and calls       */
/* exit(1) (via sbddextended_MyDict), while the C++ version            */
/* propagates std::bad_alloc from initialize/add/copy and leaves the   */
/* set consistent and usable; C++ callers that own a MySet must        */
/* deinitialize it when an exception propagates through them.          */
typedef struct tagsbddextended_MySet {
#ifdef __cplusplus
    std::set<llint>* se;
#else
    sbddextended_MyDict dict;
#endif
} sbddextended_MySet;

/* "d" must be uninitialized or deinitialized. Calling this on an      */
/* already initialized set leaks the previously owned container        */
/* (this function cannot distinguish garbage from an owned pointer,    */
/* so it never frees the previous content).                            */
sbddextended_INLINE_FUNC
void sbddextended_MySet_initialize(sbddextended_MySet* d)
{
#ifdef __cplusplus
    /* Put "d" into the deinitialized state first so that if "new"
       throws, the caller can safely call
       sbddextended_MySet_deinitialize on "d". */
    d->se = NULL;
    d->se = new std::set<llint>();
#else
    sbddextended_MyDict_initialize(&d->dict);
#endif
}

/* After this function returns, the only functions that may be called  */
/* on "d" are sbddextended_MySet_initialize (to reuse "d") and         */
/* sbddextended_MySet_deinitialize (that is, deinitializing twice is   */
/* safe).                                                              */
sbddextended_INLINE_FUNC
void sbddextended_MySet_deinitialize(sbddextended_MySet* d)
{
#ifdef __cplusplus
    delete d->se;
    d->se = NULL;
#else
    sbddextended_MyDict_deinitialize(&d->dict);
#endif
}


sbddextended_INLINE_FUNC
void sbddextended_MySet_add(sbddextended_MySet* d, llint key)
{
#ifdef __cplusplus
    d->se->insert(key);
#else
    /* value is unused */
    sbddextended_MyDict_add(&d->dict, key, 0ll);
#endif
}

/* returned value: 1 -> found, 0 -> not found */
sbddextended_INLINE_FUNC
int sbddextended_MySet_exists(const sbddextended_MySet* d, llint key)
{
#ifdef __cplusplus
    return (d->se->count(key) > 0 ? 1 : 0);
#else
    return sbddextended_MyDict_find(&d->dict, key, NULL);
#endif
}

/* "dest" must be initialized before calling this function. */
/* The current content of "dest" is discarded. */
sbddextended_INLINE_FUNC
void sbddextended_MySet_copy(sbddextended_MySet* dest,
                                const sbddextended_MySet* src)
{
    if (dest == src) {
        return;
    }
#ifdef __cplusplus
    /* Copy into a temporary and swap so that dest keeps its original */
    /* content when the copy throws std::bad_alloc (the same strong   */
    /* guarantee as sbddextended_MyDict_copy and                      */
    /* sbddextended_MyVector_copy). */
    std::set<llint> tmp(*src->se);
    dest->se->swap(tmp);
#else
    sbddextended_MyDict_copy(&dest->dict, &src->dict);
#endif
}

/* The returned count always fits in llint because each element */
/* occupies far more than one byte of memory. */
sbddextended_INLINE_FUNC
llint sbddextended_MySet_count(const sbddextended_MySet* d)
{
#ifdef __cplusplus
    return (llint)d->se->size();
#else
    return (llint)d->dict.count;
#endif
}

/* The following report functions only print the message; the reader
   returns an error value and the importers reject the input by
   returning bddnull, like they do for the other format errors, instead
   of terminating the process of the library user. */
sbddextended_INLINE_FUNC
void sbddextended_readLine_lineTooLong(void)
{
    fprintf(stderr, "Each line must not exceed %d characters\n",
            sbddextended_BUFSIZE - 1);
}

sbddextended_INLINE_FUNC
void sbddextended_readLine_nullInLine(void)
{
    fprintf(stderr, "A line must not contain a null character\n");
}

sbddextended_INLINE_FUNC
void sbddextended_read_ioError(void)
{
    fprintf(stderr, "An I/O error occurred while reading the input.\n");
}

/* Returns the read character (a non-negative value), -1 at the clean
   end of the input, or -2 on an I/O error (after printing a message).
   Without the ferror check a read error would look like the end of the
   input and a partially read file would be imported silently. */
sbddextended_INLINE_FUNC
int sbddextended_readChar_inner(FILE* fp)
{
    int c = fgetc(fp);
    if (c == EOF) {
        if (ferror(fp)) {
            sbddextended_read_ioError();
            return -2;
        }
        return -1;
    }
    return c;
}

/* The three ways of reading a line (FILE*, std::istream and std::string) */
/* share the following contract so that the same input is accepted or */
/* rejected regardless of the source: a line holds at most */
/* sbddextended_BUFSIZE - 1 characters, the terminating newline is not */
/* stored in buf, and a null character in the middle of a line is a format */
/* error (otherwise the part after it would be silently dropped, because */
/* the callers treat buf as a C string). */

/* The size of buf should be sbddextended_BUFSIZE. Returns 1 when a
   line was read into buf, 0 at the clean end of the input, and -1 on
   an error (an I/O error, a too long line or a null character in a
   line; a message has been printed). The callers must compare with 1
   rather than test for non-zero, because -1 is also non-zero. */
sbddextended_INLINE_FUNC
int sbddextended_readLine_inner(char* buf, FILE* fp)
{
    int c;
    size_t n = 0;

    /* The characters are read one by one instead of with fgets because */
    /* fgets gives no way to tell how many bytes it wrote when the line */
    /* contains a null character. */
    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') {
            buf[n] = '\0';
            return 1;
        }
        if (c == '\0') {
            sbddextended_readLine_nullInLine();
            return -1;
        }
        if (n >= (size_t)sbddextended_BUFSIZE - 1) {
            sbddextended_readLine_lineTooLong();
            return -1;
        }
        buf[n] = (char)c;
        ++n;
    }
    if (ferror(fp)) {
        /* A partial line must not be delivered as a valid one; see */
        /* sbddextended_readChar_inner. */
        sbddextended_read_ioError();
        return -1;
    }
    if (n == 0) { /* the end of the file */
        return 0;
    }
    /* The last line of a file may have no newline character at the end. */
    buf[n] = '\0';
    return 1;
}

/* The binary formats treated in this library store each multi-byte value */
/* in little endian and in a fixed width (16/32/64 bits), which do not */
/* depend on the byte order nor the type sizes of the machine. The */
/* following functions convert a byte sequence read from a binary into */
/* a value. They are written so that no shift wider than the type occurs */
/* even if the type is narrower than the value in the binary. */
sbddextended_INLINE_FUNC
unsigned short sbddextended_bytesToUint16(const unsigned char* buf)
{
    int i;
    unsigned short v = 0;
    for (i = 1; i >= 0; --i) {
        v = (unsigned short)(((unsigned int)v << 8) | (unsigned int)buf[i]);
    }
    return v;
}

/* The 32-bit field helpers hold the value in unsigned int, whose
   guaranteed minimum width is only 16 bits. Require the full 32 bits at
   compile time so that an implementation with a narrower unsigned int
   cannot silently lose the upper bytes of a 32-bit field (a negative
   array size is the compile-time condition that both C99 and C++98
   accept; see also sbddextended_charMustBe8Bits in writeLine.h). */
typedef char sbddextended_uintMustHold32Bits[
    (UINT_MAX >= 0xffffffffu) ? 1 : -1];

sbddextended_INLINE_FUNC
unsigned int sbddextended_bytesToUint32(const unsigned char* buf)
{
    int i;
    unsigned int v = 0;
    for (i = 3; i >= 0; --i) {
        v = (v << 8) | (unsigned int)buf[i];
    }
    return v;
}

sbddextended_INLINE_FUNC
ullint sbddextended_bytesToUint64(const unsigned char* buf)
{
    int i;
    ullint v = 0;
    for (i = 7; i >= 0; --i) {
        v = (v << 8) | (ullint)buf[i];
    }
    return v;
}

/* The readUint*_inner functions return 0 both at the end of the input
   and on an I/O error (the binary importers reject the input either
   way), but an I/O error additionally prints a message so that a
   device failure is not misreported as a truncated file only. */
sbddextended_INLINE_FUNC
int sbddextended_readUint8_inner(unsigned char* v, FILE* fp)
{
    assert(fp != NULL);
    if (fread(v, sizeof(unsigned char), (size_t)1, fp) != (size_t)1) {
        if (ferror(fp)) {
            sbddextended_read_ioError();
        }
        return 0;
    }
    return 1;
}

sbddextended_INLINE_FUNC
int sbddextended_readUint16_inner(unsigned short* v, FILE* fp)
{
    unsigned char buf[2];
    assert(fp != NULL);
    if (fread(buf, sizeof(unsigned char), (size_t)2, fp) != (size_t)2) {
        if (ferror(fp)) {
            sbddextended_read_ioError();
        }
        return 0;
    }
    *v = sbddextended_bytesToUint16(buf);
    return 1;
}

sbddextended_INLINE_FUNC
int sbddextended_readUint32_inner(unsigned int* v, FILE* fp)
{
    unsigned char buf[4];
    assert(fp != NULL);
    if (fread(buf, sizeof(unsigned char), (size_t)4, fp) != (size_t)4) {
        if (ferror(fp)) {
            sbddextended_read_ioError();
        }
        return 0;
    }
    *v = sbddextended_bytesToUint32(buf);
    return 1;
}

sbddextended_INLINE_FUNC
int sbddextended_readUint64_inner(ullint* v, FILE* fp)
{
    unsigned char buf[8];
    assert(fp != NULL);
    if (fread(buf, sizeof(unsigned char), (size_t)8, fp) != (size_t)8) {
        if (ferror(fp)) {
            sbddextended_read_ioError();
        }
        return 0;
    }
    *v = sbddextended_bytesToUint64(buf);
    return 1;
}


#ifdef __cplusplus

class ReadCharObject {
protected:
    static const int STREAM = 0;
    static const int FP = 1;
    static const int STRING = 2;
    typedef int Mode;

    Mode mode_;
    std::istream* ist_;
    /* In the STRING mode, the object owns a copy of the source string */
    /* so that it remains valid even after the caller's string is */
    /* destroyed or modified. */
    const std::string st_;
    llint stpos_;
    const llint stlen_;

public:
    ReadCharObject()
        : mode_(FP), ist_(NULL), st_(), stpos_(0), stlen_(0) { }

    ReadCharObject(std::istream* ist)
        : mode_(STREAM), ist_(ist), st_(), stpos_(0), stlen_(0) { }

    ReadCharObject(const std::string& st)
        : mode_(STRING), ist_(NULL), st_(st), stpos_(0),
          stlen_(static_cast<llint>(st.length())) { }

    ReadCharObject(const char* st)
        : mode_(STRING), ist_(NULL), st_((st != NULL) ? st : ""), stpos_(0),
          stlen_((st != NULL) ? static_cast<llint>(strlen(st)) : 0) { }

    /* Returns the read character (a non-negative value), -1 at the
       clean end of the input, or -2 on an I/O error (after printing a
       message). */
    int operator()(FILE* fp) {
        switch (mode_) {
        case STREAM: {
            const int c = ist_->get();
            if (ist_->bad()) {
                sbddextended_read_ioError();
                return -2;
            }
            if (ist_->fail() && !ist_->eof()) {
                /* The stream was already in a failed state before this
                   read (a failed extraction has no other way to set
                   failbit without eofbit here). Report it as an error
                   instead of folding it into the clean end of the
                   input, which would turn the previous failure into a
                   successfully parsed empty input. */
                sbddextended_read_ioError();
                return -2;
            }
            if (!*ist_) {
                return -1;
            }
            return c;
        }
        case FP:
            if (fp == NULL) { /* the caller passes NULL in the other modes */
                return -1;
            }
            return sbddextended_readChar_inner(fp);
        case STRING:
            if (stpos_ >= stlen_) {
                return -1;
            } else {
                ++stpos_;
                /* Convert through unsigned char so that a byte whose value */
                /* is 0x80 or larger is not sign-extended into a negative */
                /* number, which the callers would confuse with the -1 that */
                /* means the end of the input. fgetc and istream::get also */
                /* return a byte as a non-negative value. */
                return static_cast<unsigned char>(st_[stpos_ - 1]);
            }
        }
        return -1; /* never come here */
    }

    /* Like the readUint*_inner functions, the following readers return
       false both at the end of the input and on an I/O error, but an
       I/O error (badbit) additionally prints a message. */
    bool operator()(unsigned char* v, FILE* fp) const {
        switch (mode_) {
        case STREAM:
            ist_->read(reinterpret_cast<char*>(v), 1);
            /* Check the state of the stream, not the pointer to it, so */
            /* that failbit and badbit are detected as well as eof. */
            if (!*ist_ || ist_->gcount() != 1) {
                if (ist_->bad()) {
                    sbddextended_read_ioError();
                }
                return false;
            }
            break;
        case FP:
            return sbddextended_readUint8_inner(v, fp) != 0;
        case STRING:
            std::cerr << "not implemented" << std::endl;
            return false;
        }
        return true;
    }

    bool operator()(unsigned short* v, FILE* fp) const {
        unsigned char buf[2];
        switch (mode_) {
        case STREAM:
            ist_->read(reinterpret_cast<char*>(buf), 2);
            if (!*ist_ || ist_->gcount() != 2) {
                if (ist_->bad()) {
                    sbddextended_read_ioError();
                }
                return false;
            }
            *v = sbddextended_bytesToUint16(buf);
            return true;
        case FP:
            return sbddextended_readUint16_inner(v, fp) != 0;
        case STRING:
            std::cerr << "not implemented" << std::endl;
            return false;
        }
        return false; /* never come here */
    }

    bool operator()(unsigned int* v, FILE* fp) const {
        unsigned char buf[4];
        switch (mode_) {
        case STREAM:
            ist_->read(reinterpret_cast<char*>(buf), 4);
            if (!*ist_ || ist_->gcount() != 4) {
                if (ist_->bad()) {
                    sbddextended_read_ioError();
                }
                return false;
            }
            *v = sbddextended_bytesToUint32(buf);
            return true;
        case FP:
            return sbddextended_readUint32_inner(v, fp) != 0;
        case STRING:
            std::cerr << "not implemented" << std::endl;
            return false;
        }
        return false; /* never come here */
    }

    bool operator()(ullint* v, FILE* fp) const {
        unsigned char buf[8];
        switch (mode_) {
        case STREAM:
            ist_->read(reinterpret_cast<char*>(buf), 8);
            if (!*ist_ || ist_->gcount() != 8) {
                if (ist_->bad()) {
                    sbddextended_read_ioError();
                }
                return false;
            }
            *v = sbddextended_bytesToUint64(buf);
            return true;
        case FP:
            return sbddextended_readUint64_inner(v, fp) != 0;
        case STRING:
            std::cerr << "not implemented" << std::endl;
            return false;
        }
        return false; /* never come here */
    }
};

class ReadLineObject : public ReadCharObject {
public:
    ReadLineObject()
        : ReadCharObject() { }

    ReadLineObject(std::istream* ist)
        : ReadCharObject(ist) { }

    ReadLineObject(const std::string& st)
        : ReadCharObject(st) { }

    ReadLineObject(const char* st)
        : ReadCharObject(st) { }

    /* Returns 1 when a line was read into buf, 0 at the clean end of
       the input, and -1 on an error (an I/O error, a too long line or
       a null character in a line; a message has been printed). The
       callers must compare with 1 rather than test for non-zero,
       because -1 is also non-zero. */
    int operator()(char* buf, FILE* fp) {
        size_t len;
        switch (mode_) {
        case STREAM:
            /* istream::getline stores at most sbddextended_BUFSIZE - 1 */
            /* characters and then reports the failure, so a huge line does */
            /* not make this function allocate memory proportional to it, */
            /* as reading the line into a std::string first would. */
            ist_->getline(buf, sbddextended_BUFSIZE);
            if (ist_->bad()) {
                sbddextended_read_ioError();
                return -1;
            }
            if (ist_->fail() && !ist_->eof()) {
                if (ist_->gcount() == sbddextended_BUFSIZE - 1) {
                    /* getline stored the maximum number of characters
                       and then failed: the line does not fit in buf. */
                    sbddextended_readLine_lineTooLong();
                } else {
                    /* The stream was already in a failed state before
                       this read; do not misreport it as a long line. */
                    sbddextended_read_ioError();
                }
                return -1;
            }
            if (ist_->gcount() <= 0) { /* the end of the stream */
                return 0;
            }
            /* gcount() counts the newline when it was extracted, that is, */
            /* when the stream did not end before it. */
            len = static_cast<size_t>(ist_->gcount());
            if (!ist_->eof()) {
                --len;
            }
            if (strlen(buf) != len) {
                sbddextended_readLine_nullInLine();
                return -1;
            }
            return 1;
        case FP:
            if (fp == NULL) { /* the caller passes NULL in the other modes */
                return 0;
            }
            return sbddextended_readLine_inner(buf, fp);
        case STRING:
            if (stpos_ >= stlen_) {
                return 0;
            } else {
                llint start = stpos_;
                while (stpos_ < stlen_ && st_[stpos_] != '\n') {
                    ++stpos_;
                    if (stpos_ - start > sbddextended_BUFSIZE - 1) {
                        sbddextended_readLine_lineTooLong();
                        return -1;
                    }
                }
                len = static_cast<size_t>(stpos_ - start);
                /* The line length is at most sbddextended_BUFSIZE - 1 */
                /* because of the check above, and thus the null character */
                /* always fits in buf. */
                memcpy(buf, st_.c_str() + start, len);
                buf[len] = '\0';
                if (strlen(buf) != len) {
                    sbddextended_readLine_nullInLine();
                    return -1;
                }
                ++stpos_;
                return 1;
            }
        }
        return 0; /* never come here */
    }
};

#else

sbddextended_INLINE_FUNC
int sbddextended_readChar(FILE* fp)
{
    return sbddextended_readChar_inner(fp);
}

sbddextended_INLINE_FUNC
int sbddextended_readLine(char* buf, FILE* fp)
{
    return sbddextended_readLine_inner(buf, fp);
}

sbddextended_INLINE_FUNC
int sbddextended_readUint8(unsigned char* v, FILE* fp)
{
    return sbddextended_readUint8_inner(v, fp);
}

sbddextended_INLINE_FUNC
int sbddextended_readUint16(unsigned short* v, FILE* fp)
{
    return sbddextended_readUint16_inner(v, fp);
}

sbddextended_INLINE_FUNC
int sbddextended_readUint32(unsigned int* v, FILE* fp)
{
    return sbddextended_readUint32_inner(v, fp);
}

sbddextended_INLINE_FUNC
int sbddextended_readUint64(ullint* v, FILE* fp)
{
    return sbddextended_readUint64_inner(v, fp);
}



#endif

/* The exporters report a failed write with this message and stop.
   A write fails when the disk is full, when the given stream is
   already in a failed state, and so on; the file is then incomplete,
   which the exporters must not leave behind silently (they return
   void, so the message is the only way to tell the user). */
sbddextended_INLINE_FUNC
void sbddextended_printWriteError(void)
{
    fprintf(stderr, "Failed to write the output. "
            "The output is incomplete.\n");
}

sbddextended_INLINE_FUNC
int sbddextended_write_inner(const char* buf, FILE* fp)
{
    if (fputs(buf, fp) == EOF) {
        return 0;
    }
    return 1;
}

sbddextended_INLINE_FUNC
int sbddextended_writeLine_inner(const char* buf, FILE* fp)
{
    if (fputs(buf, fp) == EOF || fputc('\n', fp) == EOF) {
        return 0;
    }
    return 1;
}

/* The conversions below split a value into groups of 8 bits and write */
/* one char per group, so the width of a field of the external format is */
/* the promised 16/32/64 bits only where a char is 8 bits wide. That is */
/* the case on every supported platform, and a machine where it is not */
/* could not exchange files with the others anyway, so require it here */
/* rather than produce a file of a different physical length. A negative */
/* array size is the way to state a compile-time condition that both C99 */
/* and C++98 accept. */
typedef char sbddextended_charMustBe8Bits[(CHAR_BIT == 8) ? 1 : -1];

/* Converts a value into a byte sequence of a fixed width in little endian */
/* so that the produced binary does not depend on the byte order nor the */
/* type sizes of the machine. See also the comment of */
/* sbddextended_bytesToUint16 in readLine.h. */
sbddextended_INLINE_FUNC
void sbddextended_uint16ToBytes(unsigned short v, unsigned char* buf)
{
    int i;
    for (i = 0; i < 2; ++i) {
        buf[i] = (unsigned char)(v & 0xffu);
        v = (unsigned short)(v >> 8);
    }
}

sbddextended_INLINE_FUNC
void sbddextended_uint32ToBytes(unsigned int v, unsigned char* buf)
{
    int i;
    for (i = 0; i < 4; ++i) {
        buf[i] = (unsigned char)(v & 0xffu);
        v >>= 8;
    }
}

sbddextended_INLINE_FUNC
void sbddextended_uint64ToBytes(ullint v, unsigned char* buf)
{
    int i;
    for (i = 0; i < 8; ++i) {
        buf[i] = (unsigned char)(v & 0xffu);
        v >>= 8;
    }
}

sbddextended_INLINE_FUNC
int sbddextended_writeUint8_inner(unsigned char v, FILE* fp)
{
    assert(fp != NULL);
    return fwrite(&v, sizeof(unsigned char), (size_t)1, fp) != 0;
}

sbddextended_INLINE_FUNC
int sbddextended_writeUint16_inner(unsigned short v, FILE* fp)
{
    unsigned char buf[2];
    assert(fp != NULL);
    sbddextended_uint16ToBytes(v, buf);
    return fwrite(buf, sizeof(unsigned char), (size_t)2, fp) == (size_t)2;
}

sbddextended_INLINE_FUNC
int sbddextended_writeUint32_inner(unsigned int v, FILE* fp)
{
    unsigned char buf[4];
    assert(fp != NULL);
    sbddextended_uint32ToBytes(v, buf);
    return fwrite(buf, sizeof(unsigned char), (size_t)4, fp) == (size_t)4;
}

sbddextended_INLINE_FUNC
int sbddextended_writeUint64_inner(ullint v, FILE* fp)
{
    unsigned char buf[8];
    assert(fp != NULL);
    sbddextended_uint64ToBytes(v, buf);
    return fwrite(buf, sizeof(unsigned char), (size_t)8, fp) == (size_t)8;
}

#ifdef __cplusplus

class WriteObject {
private:
    const bool is_fstream_;
    const bool is_ln_;
    std::ostream* ost_;

public:
    WriteObject(bool is_fstream, bool is_ln, std::ostream* ost)
        : is_fstream_(is_fstream), is_ln_(is_ln), ost_(ost) {}

    bool operator()(const char* buf, FILE* fp) const {
        if (is_fstream_) {
            if (!*ost_ || !(*ost_ << buf)) {
                return false;
            }
            if (is_ln_) {
                if (!*ost_ || !(*ost_ << '\n')) {
                    return false;
                }
            }
        } else {
            assert(fp != NULL);
            if (is_ln_) {
                return sbddextended_writeLine_inner(buf, fp) != 0;
            } else {
                return sbddextended_write_inner(buf, fp) != 0;
            }
        }
        return true;
    }

    bool operator()(unsigned char v, FILE* fp) const {
        /*std::cerr << "uint8 " << (ullint)v << std::endl; */
        if (is_fstream_) {
            if (!*ost_) {
                return false;
            }
            if (!ost_->write(reinterpret_cast<char*>(&v),
                             sizeof(unsigned char))) {
                return false;
            }
        } else {
            assert(fp != NULL);
            return sbddextended_writeUint8_inner(v, fp) != 0;
        }
        return true;
    }

    bool operator()(unsigned short v, FILE* fp) const {
        /*std::cerr << "uint16 " << (ullint)v << std::endl; */
        unsigned char buf[2];
        if (is_fstream_) {
            if (!*ost_) {
                return false;
            }
            sbddextended_uint16ToBytes(v, buf);
            if (!ost_->write(reinterpret_cast<char*>(buf), 2)) {
                return false;
            }
        } else {
            assert(fp != NULL);
            return sbddextended_writeUint16_inner(v, fp) != 0;
        }
        return true;
    }

    bool operator()(unsigned int v, FILE* fp) const {
        /*std::cerr << "uint32 " << (ullint)v << std::endl; */
        unsigned char buf[4];
        if (is_fstream_) {
            if (!*ost_) {
                return false;
            }
            sbddextended_uint32ToBytes(v, buf);
            if (!ost_->write(reinterpret_cast<char*>(buf), 4)) {
                return false;
            }
        } else {
            assert(fp != NULL);
            return sbddextended_writeUint32_inner(v, fp) != 0;
        }
        return true;
    }

    bool operator()(ullint v, FILE* fp) const {
        /*std::cerr << "uint64 " << (ullint)v << std::endl; */
        unsigned char buf[8];
        if (is_fstream_) {
            if (!*ost_) {
                return false;
            }
            sbddextended_uint64ToBytes(v, buf);
            if (!ost_->write(reinterpret_cast<char*>(buf), 8)) {
                return false;
            }
        } else {
            assert(fp != NULL);
            return sbddextended_writeUint64_inner(v, fp) != 0;
        }
        return true;
    }
};

#else

sbddextended_INLINE_FUNC
int sbddextended_write(const char* buf, FILE* fp)
{
    return sbddextended_write_inner(buf, fp);
}

sbddextended_INLINE_FUNC
int sbddextended_writeLine(const char* buf, FILE* fp)
{
    return sbddextended_writeLine_inner(buf, fp);
}

sbddextended_INLINE_FUNC
int sbddextended_writeUint8(unsigned char v, FILE* fp)
{
    return sbddextended_writeUint8_inner(v, fp);
}

sbddextended_INLINE_FUNC
int sbddextended_writeUint16(unsigned short v, FILE* fp)
{
    return sbddextended_writeUint16_inner(v, fp);
}

sbddextended_INLINE_FUNC
int sbddextended_writeUint32(unsigned int v, FILE* fp)
{
    return sbddextended_writeUint32_inner(v, fp);
}

sbddextended_INLINE_FUNC
int sbddextended_writeUint64(ullint v, FILE* fp)
{
    return sbddextended_writeUint64_inner(v, fp);
}


#endif

sbddextended_INLINE_FUNC int bddisnegative(bddp f)
{
    return (f & B_INV_MASK) ? 1 : 0;
}

sbddextended_INLINE_FUNC int bddisconstant(bddp f)
{
    return (f & B_CST_MASK) ? 1 : 0;
}

sbddextended_INLINE_FUNC int bddisterminal(bddp f)
{
    return (f == bddempty || f == bddsingle || f == bddfalse || f == bddtrue) ? 1 : 0;
}

/* This function only flips the inversion bit of f and does not check
   whether f is bddnull. Since bddnull is an error sentinel whose
   inversion bit is set, flipping the bit turns it into a different,
   invalid node ID. Not passing bddnull is the caller's responsibility. */
sbddextended_INLINE_FUNC bddp bddtakenot(bddp f)
{
    return f ^ B_INV_MASK;
}

/* This function only sets the inversion bit of f and does not check
   whether f is bddnull. Not passing bddnull is the caller's
   responsibility; see the comment of bddtakenot. */
sbddextended_INLINE_FUNC bddp bddaddnot(bddp f)
{
    return f | B_INV_MASK;
}

/* This function only clears the inversion bit of f and does not check
   whether f is bddnull. Not passing bddnull is the caller's
   responsibility; see the comment of bddtakenot. */
sbddextended_INLINE_FUNC bddp bdderasenot(bddp f)
{
    return f & ~B_INV_MASK;
}

sbddextended_INLINE_FUNC int bddis64bitversion(void)
{
#ifdef SAPPOROBDD_PLUS_PLUS
#ifdef B_32
    return 0;
#else
    return 1;
#endif
#else
#ifdef B_64
    return 1;
#else
    return 0;
#endif
#endif
}

sbddextended_INLINE_FUNC void bddnewvarn(unsigned int n)
{
    unsigned int i;

    /* Compare by subtraction because bddvarused() + n, which is computed
       in unsigned int, can wrap around and pass this check. */
    if (n > bddvarmax - bddvarused()) {
        fprintf(stderr, "The number of variables cannot exceed bddvarmax.\n");
        exit(1);
    }
    for (i = 0; i < n; ++i) {
        bddnewvar();
    }
}

sbddextended_INLINE_FUNC void bddnewvarrev(unsigned int n)
{
    unsigned int i;

    /* Compare by subtraction because bddvarused() + n, which is computed
       in unsigned int, can wrap around and pass this check. */
    if (n > bddvarmax - bddvarused()) {
        fprintf(stderr, "The number of variables cannot exceed bddvarmax.\n");
        exit(1);
    }
    for (i = 0; i < n; ++i) {
        bddnewvaroflev(1);
    }
}

/* Not implemented. Deciding whether a bddp is valid requires access to
   the internal data of SAPPOROBDD, which is not available to the one
   header library. This function is kept only for the compatibility of
   the API and always terminates the program with an error message. */
sbddextended_INLINE_FUNC int bddisvalid(bddp f)
{
    sbddextended_unused(f);
    fprintf(stderr, "not supported in the one header library\n");
    exit(1);
}

sbddextended_INLINE_FUNC bddp bddgetterminal(int terminal, int is_zbdd)
{
    assert(terminal == 0 || terminal == 1);
    if (is_zbdd != 0) {
        return (terminal == 0 ? bddempty : bddsingle);
    } else {
        return (terminal == 0 ? bddfalse : bddtrue);
    }
}

/* assume that f is ZBDD */
sbddextended_INLINE_FUNC int bddisemptymember(bddp f)
{
    /* bddnull has the inversion bit set, but it is an error sentinel,
       not a ZBDD containing the empty set. */
    if (f == bddnull) {
        return 0;
    }
    return bddisnegative(f);
}

sbddextended_INLINE_FUNC bddvar bddgetvar(bddp f)
{
    return bddtop(f);
}

sbddextended_INLINE_FUNC bddvar bddgetlev(bddp f)
{
    return bddlevofvar(bddtop(f));
}

/* All the bddgetchild* functions return a weak reference: the reference
   counter of the returned bddp is not incremented, so the caller must not
   release it with bddfree. The returned pointer is valid only while f
   holds the child; after f is released, a garbage collection can free the
   child and leave the pointer dangling. Use bddcopy to keep a child longer
   than f. In C++, write ZBDD_ID(bddcopy(bddgetchild0z(f))) rather than
   ZBDD_ID(bddgetchild0z(f)), which would make the ZBDD destructor release
   a reference that was never taken. */
/* bddat0 and bddat1, on which the B-kind accessors are built, do not
   check the kind of the node (bddoffset and bddonset0 do), so applied
   to a ZBDD node they return a value silently, and a wrong one when
   the node has a negative edge: bddat1 flips the inversion bit of the
   1-child, which a ZBDD node does not have. This check makes the
   B-kind accessors stop the process with a message instead, which is
   what SAPPOROBDD does when a Z-kind function is given a BDD node.
   bddisbdd is 0 for bddnull as well; bddat0 would refuse it too (its
   bddtop is 0, an invalid variable). A constant is accepted here and
   refused by bddat0 / bddat1 in the same way, as before. */
sbddextended_INLINE_FUNC void sbddextended_checkBDDNode(bddp f,
                                                        const char* name)
{
    if (!bddisbdd(f)) {
        fprintf(stderr, "%s: f must be a BDD node "
                "(a ZBDD node or bddnull is given)\n", name);
        exit(1);
    }
}

sbddextended_INLINE_FUNC bddp bddgetchild0b(bddp f)
{
    bddp g;
    sbddextended_checkBDDNode(f, "bddgetchild0b");
    g = bddat0(f, bddtop(f));
    bddfree(g);
    return g;
}

sbddextended_INLINE_FUNC bddp bddgetchild0z(bddp f)
{
    bddp g;
    g = bddoffset(f, bddtop(f));
    bddfree(g);
    return g;
}

sbddextended_INLINE_FUNC bddp bddgetchild0(bddp f)
{
    if (bddisbdd(f)) {
        return bddgetchild0b(f);
    } else {
        return bddgetchild0z(f);
    }
}

sbddextended_INLINE_FUNC bddp bddgetchild0braw(bddp f)
{
    bddp child;
    child = bddgetchild0b(f);
    /* bddtakenot must not be applied to the error sentinel bddnull */
    /* (see the comment of bddtakenot); propagate it instead. */
    if (child != bddnull && bddisnegative(f)) {
        return bddtakenot(child);
    } else {
        return child;
    }
}

sbddextended_INLINE_FUNC bddp bddgetchild0zraw(bddp f)
{
    bddp child;
    child = bddgetchild0z(f);
    /* see the comment of bddgetchild0braw */
    if (child != bddnull && bddisnegative(f)) {
        return bddtakenot(child);
    } else {
        return child;
    }
}

sbddextended_INLINE_FUNC bddp bddgetchild0raw(bddp f)
{
    if (bddisbdd(f)) {
        return bddgetchild0braw(f);
    } else {
        return bddgetchild0zraw(f);
    }
}

sbddextended_INLINE_FUNC bddp bddgetchild1b(bddp f)
{
    bddp g;
    sbddextended_checkBDDNode(f, "bddgetchild1b");
    g = bddat1(f, bddtop(f));
    bddfree(g);
    return g;
}

sbddextended_INLINE_FUNC bddp bddgetchild1z(bddp f)
{
    bddp g;
    g = bddonset0(f, bddtop(f));
    bddfree(g);
    return g;
}

sbddextended_INLINE_FUNC bddp bddgetchild1(bddp f)
{
    if (bddisbdd(f)) {
        return bddgetchild1b(f);
    } else {
        return bddgetchild1z(f);
    }
}

sbddextended_INLINE_FUNC bddp bddgetchild1braw(bddp f)
{
    bddp child;
    child = bddgetchild1b(f);
    /* see the comment of bddgetchild0braw */
    if (child != bddnull && bddisnegative(f)) {
        return bddtakenot(child);
    } else {
        return child;
    }
}

sbddextended_INLINE_FUNC bddp bddgetchild1zraw(bddp f)
{
    return bddgetchild1z(f);
}

sbddextended_INLINE_FUNC bddp bddgetchild1raw(bddp f)
{
    if (bddisbdd(f)) {
        return bddgetchild1braw(f);
    } else {
        return bddgetchild1zraw(f);
    }
}

sbddextended_INLINE_FUNC bddp bddgetchildb(bddp f, int child)
{
    return (child != 0 ? bddgetchild1b(f) : bddgetchild0b(f));
}

sbddextended_INLINE_FUNC bddp bddgetchildz(bddp f, int child)
{
    return (child != 0 ? bddgetchild1z(f) : bddgetchild0z(f));
}

sbddextended_INLINE_FUNC bddp bddgetchild(bddp f, int child)
{
    return (child != 0 ? bddgetchild1(f) : bddgetchild0(f));
}

sbddextended_INLINE_FUNC bddp bddgetchildbraw(bddp f, int child)
{
    return (child != 0 ? bddgetchild1braw(f) : bddgetchild0braw(f));
}

sbddextended_INLINE_FUNC bddp bddgetchildzraw(bddp f, int child)
{
    return (child != 0 ? bddgetchild1zraw(f) : bddgetchild0zraw(f));
}

sbddextended_INLINE_FUNC bddp bddgetchildraw(bddp f, int child)
{
    return (child != 0 ? bddgetchild1raw(f) : bddgetchild0raw(f));
}

sbddextended_INLINE_FUNC bddp bddgetchild0g(bddp f, int is_zbdd, int is_raw)
{
    if (is_zbdd != 0 && is_zbdd != 1) {
        fprintf(stderr, "bddgetchild0g: is_zbdd must be 0 or 1.\n");
        exit(1);
    }
    if (is_zbdd) {
        if (is_raw) {
            return bddgetchild0zraw(f);
        } else {
            return bddgetchild0z(f);
        }
    } else {
        if (is_raw) {
            return bddgetchild0braw(f);
        } else {
            return bddgetchild0b(f);
        }
    }
}

sbddextended_INLINE_FUNC bddp bddgetchild1g(bddp f, int is_zbdd, int is_raw)
{
    if (is_zbdd != 0 && is_zbdd != 1) {
        fprintf(stderr, "bddgetchild1g: is_zbdd must be 0 or 1.\n");
        exit(1);
    }
    if (is_zbdd) {
        if (is_raw) {
            return bddgetchild1zraw(f);
        } else {
            return bddgetchild1z(f);
        }
    } else {
        if (is_raw) {
            return bddgetchild1braw(f);
        } else {
            return bddgetchild1b(f);
        }
    }
}

sbddextended_INLINE_FUNC bddp bddgetchildg(bddp f, int child,
                                            int is_zbdd, int is_raw)
{
    return (child != 0 ? bddgetchild1g(f, is_zbdd, is_raw) : bddgetchild0g(f, is_zbdd, is_raw));
}

sbddextended_INLINE_FUNC
bddp bddmakenodeb(bddvar v, bddp f0, bddp f1)
{
    bddp p, pn, g0, g1, g;

    /* VarID 0 is not a variable either: without this check the
       level comparison below would reject it with a message about the
       level, and bddprime/bddchange would stop the process. */
    if (v == 0 || v > bddvarused()) {
        fprintf(stderr, "bddmakenodeb: Invalid VarID %u\n", v);
        exit(1);
    }
    if (bddlevofvar(v) <= bddgetlev(f0)) {
        fprintf(stderr, "bddmakenodeb: The level of VarID %u "
            "must be larger than the level of f0\n", v);
        exit(1);
    }
    if (bddlevofvar(v) <= bddgetlev(f1)) {
        fprintf(stderr, "bddmakenodeb: The level of VarID %u "
            "must be larger than the level of f1\n", v);
        exit(1);
    }
    /* When compiled as C++ (in particular with SAPPOROBDD++), the DD
       operations below can throw. Track the intermediate results and
       release them in that case; bddfree(bddnull) is a no-op. */
    p = pn = g0 = g1 = bddnull;
#ifdef __cplusplus
    try {
#endif
    p = bddprime(v);
    pn = bddnot(p);
    g0 = bddand(f0, pn);
    bddfree(pn);
    pn = bddnull;
    g1 = bddand(f1, p);
    bddfree(p);
    p = bddnull;
    g = bddor(g0, g1);
    bddfree(g0);
    bddfree(g1);
    return g;
#ifdef __cplusplus
    } catch (...) {
        bddfree(g1);
        bddfree(g0);
        bddfree(pn);
        bddfree(p);
        throw;
    }
#endif
}

sbddextended_INLINE_FUNC
bddp bddmakenodez(bddvar v, bddp f0, bddp f1)
{
    bddp g1, g;

    /* VarID 0 is not a variable either: without this check the
       level comparison below would reject it with a message about the
       level, and bddprime/bddchange would stop the process. */
    if (v == 0 || v > bddvarused()) {
        fprintf(stderr, "bddmakenodez: Invalid VarID %u\n", v);
        exit(1);
    }
    if (bddlevofvar(v) <= bddgetlev(f0)) {
        fprintf(stderr, "bddmakenodez: The level of VarID %u "
            "must be larger than the level of f0\n", v);
        exit(1);
    }
    if (bddlevofvar(v) <= bddgetlev(f1)) {
        fprintf(stderr, "bddmakenodez: The level of VarID %u "
            "must be larger than the level of f1\n", v);
        exit(1);
    }

    g1 = bddchange(f1, v);
#ifdef __cplusplus
    /* release g1 even when bddunion throws (e.g. with SAPPOROBDD++) */
    try {
#endif
    g = bddunion(f0, g1);
    bddfree(g1);
    return g;
#ifdef __cplusplus
    } catch (...) {
        bddfree(g1);
        throw;
    }
#endif
}

sbddextended_INLINE_FUNC
bddp bddprimenot(bddvar v)
{
    bddp f;

    /* VarID 0 is not a variable either; without this check bddprime
       would stop the process with its own message. */
    if (v == 0 || v > bddvarused()) {
        fprintf(stderr, "bddprimenot: Invalid VarID %u\n", v);
        exit(1);
    }
    f = bddprime(v);
    return bddaddnot(f);
}

sbddextended_INLINE_FUNC
bddp bddgetsingleton(bddvar v)
{
    /* VarID 0 is not a variable either; without this check bddchange
       would stop the process with its own message. */
    if (v == 0 || v > bddvarused()) {
        fprintf(stderr, "bddgetsingleton: Invalid VarID %u\n", v);
        exit(1);
    }
    return bddchange(bddsingle, v);
}

sbddextended_INLINE_FUNC
int sbddextended_sort_compare(const void* p1, const void* p2)
{
    const bddvar v1 = *(const bddvar*)p1;
    const bddvar v2 = *(const bddvar*)p2;
    /* not v1 - v2, which would overflow for large unsigned values */
    return (v1 > v2) - (v1 < v2);
}

sbddextended_INLINE_FUNC
void sbddextended_sort_array(bddvar* arr, int n)
{
    assert(n >= 0);
    /* The callers reject a negative n before they reach this function,
       but check it here as well: (size_t)n of a negative n is a huge
       value, with which qsort would read and write far outside the
       array in a build where the assert above is disabled. */
    if (n <= 1) {
        return;
    }
    qsort(arr, (size_t)n, sizeof(bddvar), sbddextended_sort_compare);
}

/* must free the returned pointer.
   This function never returns NULL; it calls exit(1) when it fails to
   allocate memory. */
sbddextended_INLINE_FUNC
bddvar* sbddextended_getsortedarraybylevel_inner(const bddvar* vararr, int n)
{
    int i;
    bddvar* ar;

    assert(n >= 0);
    /* Allocate at least one element even when n == 0 because malloc(0)
       may legitimately return NULL, which would be mistaken for an
       out-of-memory condition. */
    ar = (bddvar*)malloc((size_t)(n > 0 ? n : 1) * sizeof(bddvar));
    if (ar == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    /* translate varIDs to levels */
    for (i = 0; i < n; ++i) {
        ar[i] = bddlevofvar(vararr[i]);
    }

    sbddextended_sort_array(ar, n);

    return ar;
}

/* Returns 1 if v is a variable number that SAPPOROBDD knows. The
   functions that take variable numbers check them with this function
   and report an error instead of letting bddlevofvar or bddchange stop
   the process of the library user. */
sbddextended_INLINE_FUNC
int sbddextended_isValidVar(bddvar v)
{
    return (1 <= v && v <= bddvarused()) ? 1 : 0;
}

/* Builds {{vararr[0], ..., vararr[n - 1]}}. The variables are applied
   with bddchange in the ascending order of their levels, whatever the
   order they are given in: bddchange(f, v) makes a single node when the
   level of v is above the top of f, but walks down f when it is below,
   so applying the variables as they come would cost O(n^2) time and n
   levels of recursion for a set listed from the root downwards (the
   order in which bddprintzbddelements writes a set), and SAPPOROBDD
   stops the process once its recursion limit (8192) is reached.
   Sorting also makes the duplicates adjacent, which is how they are
   dropped. */
sbddextended_INLINE_FUNC
bddp bddgetsingleset(const bddvar* vararr, int n)
{
    int i;
    bddp f, g;
    bddvar* levarr;

    /* Without these checks, a negative n would make the sort below run
       over a huge number of elements, and a variable number outside
       {1,...,bddvarused()} would make bddlevofvar stop the process. */
    if (n < 0) {
        fprintf(stderr, "bddgetsingleset: n must not be negative\n");
        return bddnull;
    }
    for (i = 0; i < n; ++i) {
        if (!sbddextended_isValidVar(vararr[i])) {
            fprintf(stderr, "bddgetsingleset: the variable number %u is "
                    "out of range {1,...,%u}\n",
                    vararr[i], bddvarused());
            return bddnull;
        }
    }
    if (n == 0) {
        return bddsingle;
    }

    /* the levels of the variables in the ascending order */
    levarr = sbddextended_getsortedarraybylevel_inner(vararr, n);

    f = bddsingle;
#ifdef __cplusplus
    /* release levarr and f even when bddchange throws (e.g. with
       SAPPOROBDD++) */
    try {
#endif
    for (i = 0; i < n; ++i) {
        if (i > 0 && levarr[i] == levarr[i - 1]) { /* duplicate */
            continue;
        }
        g = bddchange(f, bddvaroflev(levarr[i]));
        bddfree(f);
        f = g;
    }
    free(levarr);
    return f;
#ifdef __cplusplus
    } catch (...) {
        free(levarr);
        bddfree(f);
        throw;
    }
#endif
}

sbddextended_INLINE_FUNC
bddp bddgetsinglesetv(int n, ...)
{
    int i;
    bddp f;
    va_list ap;
    bddvar* vararr;

    if (n < 0) {
        fprintf(stderr, "bddgetsinglesetv: n must not be negative\n");
        return bddnull;
    }
    if (n == 0) {
        return bddsingle;
    }

    /* Collect the arguments and let bddgetsingleset do the rest (the
       checks, the ordering by level and the removal of duplicates). */
    vararr = (bddvar*)malloc((size_t)n * sizeof(bddvar));
    if (vararr == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    va_start(ap, n);
    for (i = 0; i < n; ++i) {
        vararr[i] = va_arg(ap, bddvar);
    }
    va_end(ap);

#ifdef __cplusplus
    /* release vararr even when a DD operation throws (e.g. with
       SAPPOROBDD++) */
    try {
#endif
    f = bddgetsingleset(vararr, n);
#ifdef __cplusplus
    } catch (...) {
        free(vararr);
        throw;
    }
#endif
    free(vararr);
    return f;
}


sbddextended_INLINE_FUNC
bddp bddgetpowerset(const bddvar* vararr, int n)
{
    int i;
    bddp f, g, h;
    bddvar* ar;
    bddvar v;

    /* Without these checks, a negative n would make the sort of the */
    /* function below run over a huge number of elements, and a */
    /* variable number outside {1,...,bddvarused()} would make */
    /* bddlevofvar stop the process. bddgetpowersetn reports an */
    /* invalid argument in the same way. */
    if (n < 0) {
        fprintf(stderr, "bddgetpowerset: n must not be negative\n");
        return bddnull;
    }
    for (i = 0; i < n; ++i) {
        if (!sbddextended_isValidVar(vararr[i])) {
            fprintf(stderr, "bddgetpowerset: the variable number %u is "
                    "out of range {1,...,%u}\n",
                    vararr[i], bddvarused());
            return bddnull;
        }
    }

    ar = sbddextended_getsortedarraybylevel_inner(vararr, n);

    f = bddsingle;
    g = bddnull;
#ifdef __cplusplus
    /* release the temporaries even when a DD operation throws
       (e.g. with SAPPOROBDD++); bddfree(bddnull) is a no-op */
    try {
#endif
    for (i = 0; i < n; ++i) {
        v = bddvaroflev(ar[i]);
        assert(1 <= v && v <= bddvarused());
        g = bddchange(f, v);
        h = bddunion(f, g);
        bddfree(g);
        g = bddnull;
        bddfree(f);
        f = h;
    }
    free(ar);
    return f;
#ifdef __cplusplus
    } catch (...) {
        bddfree(g);
        bddfree(f);
        free(ar);
        throw;
    }
#endif
}

sbddextended_INLINE_FUNC
bddp bddgetpowersetn(int n)
{
    bddp f, g, h;
    bddvar v;

    /* Without this check, a negative n cast to bddvar below would be a */
    /* huge unsigned value, and the function would build the power set */
    /* of all the variables and then exit(1) inside bddchange. */
    if (n < 0 || (bddvar)n > bddvarused()) {
        fprintf(stderr, "bddgetpowersetn: n must be between 0 and "
                "bddvarused()\n");
        return bddnull;
    }

    f = bddsingle;
    g = bddnull;
#ifdef __cplusplus
    /* release the temporaries even when a DD operation throws
       (e.g. with SAPPOROBDD++); bddfree(bddnull) is a no-op */
    try {
#endif
    for (v = 1; v <= (bddvar)n; ++v) {
        assert(1 <= v && v <= bddvarused());
        g = bddchange(f, v);
        h = bddunion(f, g);
        bddfree(g);
        g = bddnull;
        bddfree(f);
        f = h;
    }
    return f;
#ifdef __cplusplus
    } catch (...) {
        bddfree(g);
        bddfree(f);
        throw;
    }
#endif
}

sbddextended_INLINE_FUNC
int bddismemberz_inner(bddp f, const bddvar* levarr, int n)
{
    bddp h;
    int sp;

    h = f;
    sp = n - 1;
    while (h != bddempty && h != bddsingle) {
        /* SAPPOROBDD returns bddnull when it cannot compute the child
           (out of memory). Without this check the walk would silently
           answer "not a member" for it. */
        if (h == bddnull) {
            fprintf(stderr, "bddismemberz: cannot obtain a child "
                    "(out of memory?)\n");
            return 0;
        }
        if (sp < 0 || bddgetlev(h) > levarr[sp]) {
            h = bddgetchild0z(h);
        } else if (bddgetlev(h) < levarr[sp]) { /* return false */
            break;
        } else {
            h = bddgetchild1z(h);
            --sp;
        }
    }
    return ((sp < 0 && h == bddsingle) ? 1 : 0);
}

sbddextended_INLINE_FUNC
int bddismemberz(bddp f, const bddvar* vararr, int n)
{
    int i, c, num_unique;
    bddvar* ar;

    /* A negative n is a caller error; without this check the sort in
       the function below would run over a huge number of elements. */
    if (n < 0) {
        fprintf(stderr, "bddismemberz: n must not be negative\n");
        return 0;
    }

    if (n == 0) {
        return bddisemptymember(f);
    }

    /* bddnull is not a family and contains nothing. Without this
       check the walk below would report it as a child that could not
       be obtained (out of memory), which is misleading. */
    if (f == bddnull) {
        return 0;
    }

    /* A set containing a variable that does not exist is not a member
       of any family. */
    for (i = 0; i < n; ++i) {
        if (!sbddextended_isValidVar(vararr[i])) {
            return 0;
        }
    }

    ar = sbddextended_getsortedarraybylevel_inner(vararr, n);

    /* remove the duplicated variables to treat the array as a set */
    num_unique = 0;
    for (i = 0; i < n; ++i) {
        if (i == 0 || ar[i] != ar[i - 1]) {
            ar[num_unique] = ar[i];
            ++num_unique;
        }
    }

#ifdef __cplusplus
    /* release ar even when a child access throws (e.g. with
       SAPPOROBDD++) */
    try {
#endif
    c = bddismemberz_inner(f, ar, num_unique);
#ifdef __cplusplus
    } catch (...) {
        free(ar);
        throw;
    }
#endif

    free(ar);

    return c;
}

sbddextended_INLINE_FUNC
llint bddcountnodes_inner(bddp* dds, int n, int is_zbdd, int is_raw)
{
    int i, k, error = 0;
    llint count = 0;
    bddp f, child;
    sbddextended_MyVector next_p;
    sbddextended_MySet visited;

    if (n == 0) {
        return 0;
    }
    for (i = 0; i < n; ++i) {
        if (dds[i] == bddnull) {
            return 0;
        }
    }
    sbddextended_MyVector_initialize(&next_p);
#ifdef __cplusplus
    /* Deinitialize the containers even when a container operation or a
       child access throws (e.g. std::bad_alloc, or a DD operation with
       SAPPOROBDD++). If sbddextended_MySet_initialize itself throws,
       "visited" is left in the deinitialized state, so deinitializing
       it again in the catch block is safe. */
    try {
#endif
    sbddextended_MySet_initialize(&visited);

    /* With negative arcs (is_raw != 0), a node reached through both a
       positive and a negative reference is the same physical node, so
       strip the negation bit before looking it up or storing it. */
    for (i = n - 1; i >= 0; --i) {
        f = dds[i];
        if (is_raw != 0) {
            f = bdderasenot(f);
        }
        if (!bddisconstant(f)
                && !sbddextended_MySet_exists(&visited, (llint)f)) {
            sbddextended_MyVector_add(&next_p, (llint)f);
            sbddextended_MySet_add(&visited, (llint)f);
            ++count;
        }
    }

    while (next_p.count > 0 && error == 0) {
        f = (bddp)sbddextended_MyVector_get(&next_p, (llint)next_p.count - 1);
        sbddextended_MyVector_pop_back(&next_p);
        for (k = 0; k < sbddextended_NUMBER_OF_CHILDREN; ++k) {
            child = bddgetchildg(f, k, is_zbdd, is_raw);
            /* SAPPOROBDD returns bddnull when it cannot compute the
               child (out of memory). bdderasenot must not be applied
               to it (see the comment of bddtakenot), and the nodes
               below f cannot be counted, so stop with an error
               instead of walking an invalid node id. */
            if (child == bddnull) {
                fprintf(stderr, "bddcountnodes: cannot obtain a child "
                        "(out of memory?)\n");
                error = 1;
                break;
            }
            if (is_raw != 0) {
                child = bdderasenot(child);
            }
            if (!bddisconstant(child)
                    && !sbddextended_MySet_exists(&visited, (llint)child)) {
                sbddextended_MyVector_add(&next_p, (llint)child);
                sbddextended_MySet_add(&visited, (llint)child);
                ++count;
            }
        }
    }
    sbddextended_MySet_deinitialize(&visited);
    sbddextended_MyVector_deinitialize(&next_p);
    /* 0 is the value this function returns for an input it cannot
       count, as it does for a bddnull among the given DDs. */
    return (error == 0 ? count : 0);
#ifdef __cplusplus
    } catch (...) {
        sbddextended_MySet_deinitialize(&visited);
        sbddextended_MyVector_deinitialize(&next_p);
        throw;
    }
#endif
}

sbddextended_INLINE_FUNC
llint bddcountnodes(bddp* dds, int n, int is_raw)
{
    int i, is_zbdd = -1, error = 0;
    /* Check for bddnull before the BDD/ZBDD classification because
       bddisbdd(bddnull) is 0, which would misclassify bddnull as a
       ZBDD and report a spurious BDD/ZBDD mixture error. */
    for (i = 0; i < n; ++i) {
        if (dds[i] == bddnull) {
            return 0;
        }
    }
    for (i = 0; i < n; ++i) {
        if (!bddisconstant(dds[i])) {
            if (bddisbdd(dds[i])) {
                if (is_zbdd == 1) {
                    error = 1;
                    break;
                } else {
                    is_zbdd = 0;
                }
            } else { /* zbdd */
                if (is_zbdd == 0) {
                    error = 1;
                    break;
                } else {
                    is_zbdd = 1;
                }
            }
        }
    }
    if (error != 0) {
        fprintf(stderr, "bddcountnodes: both BDD and ZBDD exist.\n");
        exit(1);
    }
    return bddcountnodes_inner(dds, n, is_zbdd, is_raw);
}

/* *************************** C++ version start ***************************** */

#ifdef __cplusplus

sbddextended_INLINE_FUNC bool isNegative(const BDD& f)
{
    return bddisnegative(f.GetID()) != 0;
}

sbddextended_INLINE_FUNC bool isNegative(const ZBDD& f)
{
    return bddisnegative(f.GetID()) != 0;
}

sbddextended_INLINE_FUNC bool isConstant(const BDD& f)
{
    return bddisconstant(f.GetID()) != 0;
}

sbddextended_INLINE_FUNC bool isConstant(const ZBDD& f)
{
    return bddisconstant(f.GetID()) != 0;
}

sbddextended_INLINE_FUNC bool isTerminal(const BDD& f)
{
    return bddisterminal(f.GetID()) != 0;
}

sbddextended_INLINE_FUNC bool isTerminal(const ZBDD& f)
{
    return bddisterminal(f.GetID()) != 0;
}

sbddextended_INLINE_FUNC BDD takeNot(const BDD& f)
{
    return BDD_ID(bddcopy(bddtakenot(f.GetID())));
}

sbddextended_INLINE_FUNC ZBDD takeNot(const ZBDD& f)
{
    return ZBDD_ID(bddcopy(bddtakenot(f.GetID())));
}

sbddextended_INLINE_FUNC BDD addNot(const BDD& f)
{
    return BDD_ID(bddcopy(bddaddnot(f.GetID())));
}

sbddextended_INLINE_FUNC ZBDD addNot(const ZBDD& f)
{
    return ZBDD_ID(bddcopy(bddaddnot(f.GetID())));
}

sbddextended_INLINE_FUNC BDD eraseNot(const BDD& f)
{
    return BDD_ID(bddcopy(bdderasenot(f.GetID())));
}

sbddextended_INLINE_FUNC ZBDD eraseNot(const ZBDD& f)
{
    return ZBDD_ID(bddcopy(bdderasenot(f.GetID())));
}

sbddextended_INLINE_FUNC bool is64BitVersion()
{
    return bddis64bitversion() != 0;
}

sbddextended_INLINE_FUNC void SBDDH_NewVar(unsigned int n)
{
    unsigned int i;

    /* Compare by subtraction because bddvarused() + n, which is computed
       in unsigned int, can wrap around and pass this check. */
    if (n > bddvarmax - bddvarused()) {
        fprintf(stderr, "The number of variables cannot exceed bddvarmax.\n");
        exit(1);
    }
    /* Call BDD_NewVar, not bddnewvar: when the BDDV system variables
       are active, BDD_NewVar inserts the variable just below them
       instead of at the very top, which keeps the variable order BDDV
       relies on (and is what the reference documents). */
    for (i = 0; i < n; ++i) {
        BDD_NewVar();
    }
}

sbddextended_INLINE_FUNC void SBDDH_NewVarRev(unsigned int n)
{
    bddnewvarrev(n);
}

/* Not implemented. See the comment of bddisvalid; both overloads always
   terminate the program with an error message. */
sbddextended_INLINE_FUNC bool isValid(const BDD& f)
{
    return bddisvalid(f.GetID()) != 0;
}

sbddextended_INLINE_FUNC bool isValid(const ZBDD& f)
{
    return bddisvalid(f.GetID()) != 0;
}

sbddextended_INLINE_FUNC bool isEmptyMember(const ZBDD& f)
{
    return bddisemptymember(f.GetID()) != 0;
}

sbddextended_INLINE_FUNC bddvar getVar(const BDD& f)
{
    return bddgetvar(f.GetID());
}

sbddextended_INLINE_FUNC bddvar getVar(const ZBDD& f)
{
    return bddgetvar(f.GetID());
}

sbddextended_INLINE_FUNC bddvar getLev(const BDD& f)
{
    return bddgetlev(f.GetID());
}

sbddextended_INLINE_FUNC bddvar getLev(const ZBDD& f)
{
    return bddgetlev(f.GetID());
}

sbddextended_INLINE_FUNC BDD getChild0(const BDD& f)
{
    bddp g;
    sbddextended_checkBDDNode(f.GetID(), "getChild0");
    g = bddat0(f.GetID(), f.Top());
    return BDD_ID(g);
}

sbddextended_INLINE_FUNC ZBDD getChild0(const ZBDD& f)
{
    bddp g;
    g = bddoffset(f.GetID(), f.Top());
    return ZBDD_ID(g);
}

sbddextended_INLINE_FUNC BDD getChild0Raw(const BDD& f)
{
    bddp g;
    sbddextended_checkBDDNode(f.GetID(), "getChild0Raw");
    g = bddat0(f.GetID(), f.Top());
    /* bddtakenot must not be applied to the error sentinel bddnull; */
    /* propagate it instead. */
    if (g != bddnull && isNegative(f)) {
        g = bddtakenot(g);
    }
    return BDD_ID(g);
}

sbddextended_INLINE_FUNC ZBDD getChild0Raw(const ZBDD& f)
{
    bddp g;
    g = bddoffset(f.GetID(), f.Top());
    /* see the comment of the BDD overload above */
    if (g != bddnull && isNegative(f)) {
        g = bddtakenot(g);
    }
    return ZBDD_ID(g);
}

sbddextended_INLINE_FUNC BDD getChild1(const BDD& f)
{
    bddp g;
    sbddextended_checkBDDNode(f.GetID(), "getChild1");
    g = bddat1(f.GetID(), f.Top());
    return BDD_ID(g);
}

sbddextended_INLINE_FUNC ZBDD getChild1(const ZBDD& f)
{
    bddp g;
    g = bddonset0(f.GetID(), f.Top());
    return ZBDD_ID(g);
}

sbddextended_INLINE_FUNC BDD getChild1Raw(const BDD& f)
{
    bddp g;
    sbddextended_checkBDDNode(f.GetID(), "getChild1Raw");
    g = bddat1(f.GetID(), f.Top());
    /* see the comment of getChild0Raw */
    if (g != bddnull && isNegative(f)) {
        g = bddtakenot(g);
    }
    return BDD_ID(g);
}

sbddextended_INLINE_FUNC ZBDD getChild1Raw(const ZBDD& f)
{
    return getChild1(f);
}

sbddextended_INLINE_FUNC BDD getChild(const BDD& f, int child)
{
    if (child == 1) {
        return getChild1(f);
    } else {
        return getChild0(f);
    }
}

sbddextended_INLINE_FUNC ZBDD getChild(const ZBDD& f, int child)
{
    if (child == 1) {
        return getChild1(f);
    } else {
        return getChild0(f);
    }
}

sbddextended_INLINE_FUNC BDD getChildRaw(const BDD& f, int child)
{
    if (child == 1) {
        return getChild1Raw(f);
    } else {
        return getChild0Raw(f);
    }
}

sbddextended_INLINE_FUNC ZBDD getChildRaw(const ZBDD& f, int child)
{
    if (child == 1) {
        return getChild1Raw(f);
    } else {
        return getChild0Raw(f);
    }
}

sbddextended_INLINE_FUNC
BDD makeNode(bddvar v, const BDD& f0, const BDD& f1)
{
    return BDD_ID(bddmakenodeb(v, f0.GetID(), f1.GetID()));
}

sbddextended_INLINE_FUNC
ZBDD makeNode(bddvar v, const ZBDD& f0, const ZBDD& f1)
{
    return ZBDD_ID(bddmakenodez(v, f0.GetID(), f1.GetID()));
}

sbddextended_INLINE_FUNC BDD getPrimeNot(bddvar v)
{
    return BDD_ID(bddprimenot(v));
}

sbddextended_INLINE_FUNC ZBDD getSingleton(bddvar v)
{
    return ZBDD_ID(bddgetsingleton(v));
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getSingleSet(const T& variables)
{
    /* use std::vector so that the array is released even when a DD
       operation throws (e.g. with SAPPOROBDD++); bddgetsingleset
       checks the variables, orders them by level and drops the
       duplicates */
    std::vector<bddvar> ar(variables.begin(), variables.end());
    if (ar.empty()) {
        return ZBDD(1);
    }
    if (ar.size() > static_cast<size_t>(INT_MAX)) {
        fprintf(stderr, "getSingleSet: the number of variables "
            "must be at most INT_MAX\n");
        exit(1);
    }
    return ZBDD_ID(bddgetsingleset(&ar[0], static_cast<int>(ar.size())));
}

sbddextended_INLINE_FUNC ZBDD getSingleSet(int n, ...)
{
    int i;
    va_list ap;

    if (n < 0) {
        fprintf(stderr, "getSingleSet: n must not be negative\n");
        return ZBDD(-1);
    }
    if (n == 0) {
        return ZBDD(1);
    }

    /* collect the arguments (a std::vector cannot be passed through
       the variable arguments) and build the set as for a container */
    std::vector<bddvar> ar;
    ar.reserve(static_cast<size_t>(n));
    va_start(ap, n);
    for (i = 0; i < n; ++i) {
        ar.push_back(va_arg(ap, bddvar));
    }
    va_end(ap);
    return ZBDD_ID(bddgetsingleset(&ar[0], n));
}

/* need to delete[] the returned pointer */
template<typename T>
sbddextended_INLINE_FUNC bddvar* containerToArray(const T& variables,
                                                  int* n)
{
    /* count in size_t to avoid signed overflow on huge containers */
    size_t count = 0;

    for (typename T::const_iterator itor = variables.begin();
         itor != variables.end(); ++itor) {
        ++count;
    }

    if (count > static_cast<size_t>(INT_MAX)) {
        fprintf(stderr, "containerToArray: the number of elements "
            "must be at most INT_MAX\n");
        exit(1);
    }
    *n = static_cast<int>(count);

    if (count == 0) {
        return NULL;
    }

    /* new(std::nothrow) returns NULL on failure instead of throwing, */
    /* so the check below is reachable. */
    bddvar* ar = new(std::nothrow) bddvar[count];
    if (ar == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    size_t c = 0;
    for (typename T::const_iterator itor = variables.begin();
         itor != variables.end(); ++itor) {
        ar[c] = *itor;
        ++c;
    }
    return ar;
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getPowerSet(const T& variables)
{
    /* use std::vector so that the array is released even when a DD
       operation throws (e.g. with SAPPOROBDD++) */
    std::vector<bddvar> ar(variables.begin(), variables.end());
    if (ar.empty()) {
        /* The power set of the empty set is {{}}, which is the
           unit set as a ZBDD. */
        return ZBDD(1);
    }
    if (ar.size() > static_cast<size_t>(INT_MAX)) {
        fprintf(stderr, "getPowerSet: the number of variables "
            "must be at most INT_MAX\n");
        exit(1);
    }

    bddp f = bddgetpowerset(&ar[0], static_cast<int>(ar.size()));

    return ZBDD_ID(f);
}

sbddextended_INLINE_FUNC ZBDD getPowerSet(int n)
{
    bddp f = bddgetpowersetn(n);
    return ZBDD_ID(f);
}

template<typename T1, typename T2>
sbddextended_INLINE_FUNC ZBDD getPowerSetIncluding_inner(const T1& base_variables,
                                                        const T2& target_variables)
{
    ZBDD f = getPowerSet(base_variables);
    if (f == ZBDD(-1)) { /* the base variables are invalid */
        return f;
    }

    /* Report an invalid target variable the same way, instead of
       letting OnSet/Change stop the process. */
    for (typename T2::const_iterator itor = target_variables.begin();
         itor != target_variables.end(); ++itor) {
        if (!sbddextended_isValidVar(*itor)) {
            fprintf(stderr, "getPowerSetIncluding: the variable number "
                    "%u is out of range {1,...,%u}\n",
                    static_cast<bddvar>(*itor), bddvarused());
            return ZBDD(-1);
        }
    }

    /* Deduplicate the target variables. Applying Change twice for the
       same variable not in base_variables would cancel out and drop
       the variable from the result. */
    std::set<bddvar> unique_targets(target_variables.begin(),
                                    target_variables.end());

    bddvar c = 0;
    for (std::set<bddvar>::const_iterator itor = unique_targets.begin();
         itor != unique_targets.end(); ++itor) {
        c = *itor;
        if (std::find(base_variables.begin(), base_variables.end(), c) !=
            base_variables.end()) { /* c found */
            f = f.OnSet(c);
        } else {
            f = f.Change(c);
        }
    }
    return f;
}


template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getPowerSetIncluding(const T& base_variables,
                                                  const std::vector<bddvar>& target_variables)
{
    return getPowerSetIncluding_inner(base_variables, target_variables);
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getPowerSetIncluding(const T& base_variables,
                                                  const std::set<bddvar>& target_variables)
{
    return getPowerSetIncluding_inner(base_variables, target_variables);
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getPowerSetIncluding(const T& base_variables,
                                                  bddvar v)
{
    std::vector<bddvar> target_variables;
    target_variables.push_back(v);

    return getPowerSetIncluding(base_variables, target_variables);
}

/* The overloads that take the number of variables use {1,...,n} as the
   base, so n must be the number of existing variables at most, as it
   must be for getPowerSet(int). */
sbddextended_INLINE_FUNC bool sbddextended_isValidNumberOfVars(
                                            const char* name, int n)
{
    if (n < 0 || static_cast<bddvar>(n) > bddvarused()) {
        fprintf(stderr, "%s: n must be between 0 and bddvarused()\n", name);
        return false;
    }
    return true;
}

sbddextended_INLINE_FUNC ZBDD getPowerSetIncluding(int n,
                                                       const std::vector<bddvar>& target_variables)
{
    if (!sbddextended_isValidNumberOfVars("getPowerSetIncluding", n)) {
        return ZBDD(-1);
    }

    std::vector<bddvar> base_variables;
    for (int v = 1; v <= n; ++v) {
        base_variables.push_back(v);
    }

    return getPowerSetIncluding(base_variables, target_variables);
}

sbddextended_INLINE_FUNC ZBDD getPowerSetIncluding(int n,
                                                       const std::set<bddvar>& target_variables)
{
    if (!sbddextended_isValidNumberOfVars("getPowerSetIncluding", n)) {
        return ZBDD(-1);
    }

    std::vector<bddvar> base_variables;
    for (int v = 1; v <= n; ++v) {
        base_variables.push_back(v);
    }

    return getPowerSetIncluding(base_variables, target_variables);
}

sbddextended_INLINE_FUNC ZBDD getPowerSetIncluding(int n, int v)
{
    std::vector<bddvar> target_variables;
    target_variables.push_back(v);

    return getPowerSetIncluding(n, target_variables);
}

template<typename T>
sbddextended_INLINE_FUNC ZBDD getPowerSetNotIncluding_inner(int n,
                                                                const T& target_variables)
{
    ZBDD f = getPowerSet(n);
    if (f == ZBDD(-1)) { /* n is invalid; getPowerSet has reported it */
        return f;
    }

    /* Report an invalid target variable the same way, instead of
       letting OffSet stop the process. */
    for (typename T::const_iterator itor = target_variables.begin();
         itor != target_variables.end(); ++itor) {
        if (!sbddextended_isValidVar(*itor)) {
            fprintf(stderr, "getPowerSetNotIncluding: the variable "
                    "number %u is out of range {1,...,%u}\n",
                    static_cast<bddvar>(*itor), bddvarused());
            return ZBDD(-1);
        }
    }

    for (typename T::const_iterator itor = target_variables.begin();
         itor != target_variables.end(); ++itor) {
        f = f.OffSet(*itor);
    }
    return f;
}

sbddextended_INLINE_FUNC ZBDD getPowerSetNotIncluding(int n,
                                                          const std::vector<bddvar>& target_variables)
{
    return getPowerSetNotIncluding_inner(n, target_variables);
}

sbddextended_INLINE_FUNC ZBDD getPowerSetNotIncluding(int n,
                                                          const std::set<bddvar>& target_variables)
{
    return getPowerSetNotIncluding_inner(n, target_variables);
}

sbddextended_INLINE_FUNC ZBDD getPowerSetNotIncluding(int n, int v)
{
    /* delegate so that n and v are checked as in the overloads above */
    std::vector<bddvar> target_variables;
    target_variables.push_back(static_cast<bddvar>(v));

    return getPowerSetNotIncluding(n, target_variables);
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getPowerSetWithCard(const T& variables, int k)
{
    /* use std::vector so that the array is released even when a DD
       operation throws (e.g. with SAPPOROBDD++) */
    std::vector<bddvar> ar(variables.begin(), variables.end());
    /* Check the variables before anything else, so that an invalid
       argument is reported whatever k is. */
    for (size_t i = 0; i < ar.size(); ++i) {
        if (!sbddextended_isValidVar(ar[i])) {
            fprintf(stderr, "getPowerSetWithCard: the variable number "
                    "%u is out of range {1,...,%u}\n",
                    ar[i], bddvarused());
            return ZBDD(-1);
        }
    }

    if (k < 0) { /* no set has a negative cardinality */
        return ZBDD(0);
    }
    if (ar.size() < static_cast<size_t>(k)) {
        return ZBDD(0);
    }
    /* translate varIDs to levels */
    for (size_t i = 0; i < ar.size(); ++i) {
        ar[i] = bddlevofvar(ar[i]);
    }

    std::sort(ar.begin(), ar.end());

    /* remove the duplicated variables to treat the container as a set */
    ar.erase(std::unique(ar.begin(), ar.end()), ar.end());
    if (ar.size() < static_cast<size_t>(k)) {
        return ZBDD(0);
    }
    /* the number of unique levels is at most the number of variables,
       which fits in int */
    assert(ar.size() <= static_cast<size_t>(INT_MAX));
    const int n = static_cast<int>(ar.size());

    std::vector<ZBDD> current;
    std::vector<ZBDD> next(static_cast<size_t>(k) + 1);

    for (int i = 0; i < k; ++i) {
        current.push_back(ZBDD(0));
    }
    current.push_back(ZBDD(1));

    for (int i = 0; i < n; ++i) {
        int v = bddvaroflev(ar[static_cast<size_t>(i)]);
        for (int j = 0; j <= std::min(n - i - 1, k); ++j) {
            if (j < k) {
                next[static_cast<size_t>(j)] = current[static_cast<size_t>(j)]
                    + current[static_cast<size_t>(j) + 1].Change(v);
            } else {
                next[static_cast<size_t>(j)] = current[static_cast<size_t>(j)];
            }
        }
        current = next;
    }
    return current[0];
}

sbddextended_INLINE_FUNC ZBDD getPowerSetWithCard(int n, int k)
{
    if (!sbddextended_isValidNumberOfVars("getPowerSetWithCard", n)) {
        return ZBDD(-1);
    }

    std::vector<bddvar> variables;
    for (int v = 1; v <= n; ++v) {
        variables.push_back(v);
    }
    return getPowerSetWithCard(variables, k);
}


template<typename T>
sbddextended_INLINE_FUNC ZBDD makeDontCare(const ZBDD& f, const T& variables)
{
    /* Check the variables before anything else, so that an invalid
       argument is reported (as the getPowerSet family does) whatever
       f is. */
    for (typename T::const_iterator itor = variables.begin();
         itor != variables.end(); ++itor) {
        if (!sbddextended_isValidVar(*itor)) {
            fprintf(stderr, "makeDontCare: the variable number %u is "
                    "out of range {1,...,%u}\n",
                    static_cast<unsigned int>(*itor), bddvarused());
            return ZBDD(-1);
        }
    }
    /* bddnull is not a family; the DD operations below would only
       propagate it, so return it at once. */
    if (f.GetID() == bddnull) {
        return ZBDD(-1);
    }
    ZBDD g = f;
    for (typename T::const_iterator itor = variables.begin();
         itor != variables.end(); ++itor) {
        g = g + g.Change(*itor);
    }
    return g;
}

template<typename T>
sbddextended_INLINE_FUNC bool isMemberZ(const ZBDD& f, const T& variables)
{
    /* use std::vector so that the array is released even when a child
       access throws (e.g. with SAPPOROBDD++); translate varIDs to
       levels while counting the elements in size_t */
    std::vector<bddvar> ar;

    for (typename T::const_iterator itor = variables.begin();
         itor != variables.end(); ++itor) {
        if (!(1 <= *itor && *itor <= bddvarused())) {
            return false;
        }
        ar.push_back(bddlevofvar(*itor));
    }

    if (ar.empty()) {
        return bddisemptymember(f.GetID()) != 0;
    }

    /* see the comment in bddismemberz */
    if (f.GetID() == bddnull) {
        return false;
    }

    std::sort(ar.begin(), ar.end());

    /* remove the duplicated variables to treat the container as a set */
    ar.erase(std::unique(ar.begin(), ar.end()), ar.end());

    /* the number of unique levels is at most the number of variables,
       which fits in int */
    assert(ar.size() <= static_cast<size_t>(INT_MAX));

    int b = bddismemberz_inner(f.GetID(), &ar[0],
                               static_cast<int>(ar.size()));

    return b != 0;
}

template<typename T>
sbddextended_INLINE_FUNC bool isMember(const ZBDD& f, const T& variables)
{
    return isMemberZ(f, variables);
}

/* BDD and ZBDD do not have operator<, so std::set<BDD> and std::set<ZBDD>
   cannot be instantiated. This comparator can be used instead, as in
   std::set<BDD, DDComparator<BDD> >. */
template<typename T>
struct DDComparator {
    bool operator()(const T& f, const T& g) const
    {
        return f.GetID() < g.GetID();
    }
};

/* The countNodes overloads copy the DDs into a std::vector so that the
   buffer is released even when the counting throws (e.g.
   std::bad_alloc, or a DD operation with SAPPOROBDD++). The element
   count is validated against INT_MAX before it is narrowed to the int
   expected by the C API. */
sbddextended_INLINE_FUNC
void sbddextended_countNodes_checkSize(size_t size)
{
    if (size > static_cast<size_t>(INT_MAX)) {
        fprintf(stderr, "countNodes: the number of DDs must be "
            "at most INT_MAX\n");
        exit(1);
    }
}

sbddextended_INLINE_FUNC
llint countNodes(const std::vector<bddp>& dds, bool is_raw = false)
{
    if (dds.empty()) {
        return 0;
    }
    sbddextended_countNodes_checkSize(dds.size());
    std::vector<bddp> bps(dds.begin(), dds.end());
    return bddcountnodes(&bps[0], static_cast<int>(bps.size()),
                            (is_raw ? 1 : 0));
}

sbddextended_INLINE_FUNC
llint countNodes(const std::set<bddp>& dds, bool is_raw = false)
{
    if (dds.empty()) {
        return 0;
    }
    sbddextended_countNodes_checkSize(dds.size());
    std::vector<bddp> bps(dds.begin(), dds.end());
    return bddcountnodes(&bps[0], static_cast<int>(bps.size()),
                            (is_raw ? 1 : 0));
}

sbddextended_INLINE_FUNC
llint countNodes(const std::vector<BDD>& dds, bool is_raw = false)
{
    if (dds.empty()) {
        return 0;
    }
    sbddextended_countNodes_checkSize(dds.size());
    std::vector<bddp> bps;
    bps.reserve(dds.size());
    for (std::vector<BDD>::const_iterator itor = dds.begin();
            itor != dds.end(); ++itor) {
        bps.push_back(itor->GetID());
    }
    return bddcountnodes_inner(&bps[0], static_cast<int>(bps.size()),
                                        0, (is_raw ? 1 : 0));
}

template<typename Cmp, typename Alloc>
sbddextended_INLINE_FUNC
llint countNodes(const std::set<BDD, Cmp, Alloc>& dds, bool is_raw = false)
{
    if (dds.empty()) {
        return 0;
    }
    sbddextended_countNodes_checkSize(dds.size());
    std::vector<bddp> bps;
    bps.reserve(dds.size());
    for (typename std::set<BDD, Cmp, Alloc>::const_iterator itor = dds.begin();
            itor != dds.end(); ++itor) {
        bps.push_back(itor->GetID());
    }
    return bddcountnodes_inner(&bps[0], static_cast<int>(bps.size()),
                                        0, (is_raw ? 1 : 0));
}

sbddextended_INLINE_FUNC
llint countNodes(const std::vector<ZBDD>& dds, bool is_raw = false)
{
    if (dds.empty()) {
        return 0;
    }
    sbddextended_countNodes_checkSize(dds.size());
    std::vector<bddp> bps;
    bps.reserve(dds.size());
    for (std::vector<ZBDD>::const_iterator itor = dds.begin();
            itor != dds.end(); ++itor) {
        bps.push_back(itor->GetID());
    }
    return bddcountnodes_inner(&bps[0], static_cast<int>(bps.size()),
                                        1, (is_raw ? 1 : 0));
}

template<typename Cmp, typename Alloc>
sbddextended_INLINE_FUNC
llint countNodes(const std::set<ZBDD, Cmp, Alloc>& dds, bool is_raw = false)
{
    if (dds.empty()) {
        return 0;
    }
    sbddextended_countNodes_checkSize(dds.size());
    std::vector<bddp> bps;
    bps.reserve(dds.size());
    for (typename std::set<ZBDD, Cmp, Alloc>::const_iterator itor = dds.begin();
            itor != dds.end(); ++itor) {
        bps.push_back(itor->GetID());
    }
    return bddcountnodes_inner(&bps[0], static_cast<int>(bps.size()),
                                        1, (is_raw ? 1 : 0));
}

/* Returns true if no ZBDD whose node levels are at most 'level' can have
   exactly 'card' elements, that is, card < 0 or card > 2^level. */
sbddextended_INLINE_FUNC bool sbddh_isCardOutOfRange(int level, llint card)
{
    if (card < 0) {
        return true;
    }
    if (level < 0) {
        level = 0;
    }
    if (level >= 63) { /* 2^level is larger than any llint value */
        return false;
    }
    return card > ((llint)1 << level);
}

#if __cplusplus >= 201103L

class DDUtilityCpp11 {
public:
    template <typename T>
    static ZBDD getUniformlyRandomZBDD(int level, T& random_engine)
    {
        if (level > 0) {
            /* Generate the 0-child before the 1-child in separate
               statements. If both calls were written as arguments of
               makeNode, the evaluation order, and thus which subtree
               consumes the random sequence first, would depend on the
               compiler. */
            ZBDD f0 = getUniformlyRandomZBDD(level - 1, random_engine);
            ZBDD f1 = getUniformlyRandomZBDD(level - 1, random_engine);
            return makeNode(bddvaroflev(level), f0, f1);
        } else {
            std::uniform_int_distribution<int> dist(0, 1);
            if (dist(random_engine) != 0) {
                return ZBDD(1);
            } else {
                return ZBDD(0);
            }
        }
    }

    template <typename T>
    static ZBDD getRandomZBDDWithCard(int level, llint card, T& random_engine)
    {
        if (sbddh_isCardOutOfRange(level, card)) {
            return ZBDD(-1);
        }

        ZBDD f(0);
        /* Count the sets in f ourselves instead of using f.Card(),
           which saturates at bddnull when the cardinality reaches the
           representation limit and would make this loop non-
           terminating for larger values of card. */
        llint num_sets = 0;
        std::set<bddvar> s;
        std::uniform_int_distribution<int> dist(0, 1);

        while (num_sets < card) {
            for (int lev = 1; lev <= level; ++lev) {
                if (dist(random_engine) != 0) {
                    s.insert(bddvaroflev(lev));
                }
            }
            ZBDD g = f + getSingleSet(s);
            if (g != f) { /* the drawn set was new */
                f = g;
                ++num_sets;
            }
            s.clear();
        }

        return f;
    }
};

template <typename T>
sbddextended_INLINE_FUNC
ZBDD getUniformlyRandomZBDD(int level, T& random_engine)
{
    return DDUtilityCpp11::getUniformlyRandomZBDD(level, random_engine);
}

template <typename T>
sbddextended_INLINE_FUNC
ZBDD getRandomZBDDWithCard(int level, llint card, T& random_engine)
{
    return DDUtilityCpp11::getRandomZBDDWithCard(level, card, random_engine);
}

#endif /* __cplusplus >= 201103L */

class DDUtility {
public:
    static BDD getUniformlyRandomBDDX(int level, ullint* rand_state)
    {
        if (level > 0) {
            /* Generate the 0-child before the 1-child in separate
               statements. If both calls were written as arguments of
               makeNode, the evaluation order, and thus which subtree
               consumes the random sequence first, would depend on the
               compiler, although this function must be
               environment-independent. */
            BDD f0 = getUniformlyRandomBDDX(level - 1, rand_state);
            BDD f1 = getUniformlyRandomBDDX(level - 1, rand_state);
            return makeNode(bddvaroflev(level), f0, f1);
        } else {
            ullint v = sbddextended_getXRand(rand_state);
            if (((v >> 19) & 1u) != 0) {
                return BDD(1);
            } else {
                return BDD(0);
            }
        }
    }

    static ZBDD getUniformlyRandomZBDDX(int level, ullint* rand_state)
    {
        if (level > 0) {
            /* Generate the 0-child before the 1-child in separate
               statements. If both calls were written as arguments of
               makeNode, the evaluation order, and thus which subtree
               consumes the random sequence first, would depend on the
               compiler, although this function must be
               environment-independent. */
            ZBDD f0 = getUniformlyRandomZBDDX(level - 1, rand_state);
            ZBDD f1 = getUniformlyRandomZBDDX(level - 1, rand_state);
            return makeNode(bddvaroflev(level), f0, f1);
        } else {
            ullint v = sbddextended_getXRand(rand_state);
            if (((v >> 19) & 1u) != 0) {
                return ZBDD(1);
            } else {
                return ZBDD(0);
            }
        }
    }

    static ZBDD getRandomZBDDWithCardX(int level, llint card, ullint* rand_state)
    {
        if (sbddh_isCardOutOfRange(level, card)) {
            return ZBDD(-1);
        }

        ZBDD f(0);
        /* Count the sets in f ourselves instead of using f.Card(),
           which saturates at bddnull when the cardinality reaches the
           representation limit and would make this loop non-
           terminating for larger values of card. */
        llint num_sets = 0;
        std::set<bddvar> s;

        while (num_sets < card) {
            for (int lev = 1; lev <= level; ++lev) {
                ullint v = sbddextended_getXRand(rand_state);
                if (((v >> 19) & 1u) != 0) {
                    s.insert(bddvaroflev(lev));
                }
            }
            ZBDD g = f + getSingleSet(s);
            if (g != f) { /* the drawn set was new */
                f = g;
                ++num_sets;
            }
            s.clear();
        }

        return f;
    }
};

sbddextended_INLINE_FUNC
BDD getUniformlyRandomBDDX(int level, ullint* rand_state)
{
    return DDUtility::getUniformlyRandomBDDX(level, rand_state);
}

sbddextended_INLINE_FUNC
ZBDD getUniformlyRandomZBDDX(int level, ullint* rand_state)
{
    return DDUtility::getUniformlyRandomZBDDX(level, rand_state);
}

sbddextended_INLINE_FUNC
ZBDD getRandomZBDDWithCardX(int level, llint card, ullint* rand_state)
{
    return DDUtility::getRandomZBDDWithCardX(level, card, rand_state);
}

sbddextended_INLINE_FUNC
BDD exampleBdd(ullint kind = 0ull)
{
    /* kind == 0 cannot be used for rand_state */
    kind += 1;
    ullint rand_state = kind;
    ullint v = sbddextended_getXRand(&rand_state);
    int size = static_cast<int>((v % 6) + 3);
    while (BDD_VarUsed() < size) {
        BDD_NewVar();
    }
    return DDUtility::getUniformlyRandomBDDX(size, &rand_state);
}

sbddextended_INLINE_FUNC
ZBDD exampleZbdd(ullint kind = 0ull)
{
    /* kind == 0 cannot be used for rand_state */
    kind += 1;
    ullint rand_state = kind;
    ullint v = sbddextended_getXRand(&rand_state);
    int size = static_cast<int>((v % 6) + 3);
    while (BDD_VarUsed() < size) {
        BDD_NewVar();
    }
    return DDUtility::getUniformlyRandomZBDDX(size, &rand_state);
}

#ifdef SBDDH_BDDCT

sbddextended_INLINE_FUNC
llint sbddextended_bddcost_min(void)
{
    return -static_cast<llint>(bddcost_null) + 1;
}

sbddextended_INLINE_FUNC
llint sbddextended_bddcost_max(void)
{
    return static_cast<llint>(bddcost_null) - 1;
}

sbddextended_INLINE_FUNC
bool sbddextended_is_valid_bddcost(llint v)
{
    return sbddextended_bddcost_min() <= v
        && v <= sbddextended_bddcost_max();
}

sbddextended_INLINE_FUNC
void sbddextended_print_bddcost_range_error(const char* name)
{
    std::cerr << name << " should be between "
        << sbddextended_bddcost_min() << " and "
        << sbddextended_bddcost_max() << std::endl;
}

/* On success, *sum_pos and *sum_neg hold the sum of the positive
   weights and the sum of the absolute values of the negative weights
   among the weights that are actually used (those of the variables
   appearing in f). BDDCT computes costs in int, so the callers must
   check that every bound they pass to ZBDD_CostLE keeps the
   intermediate values (bound minus any partial sum of the used
   weights, and any partial sum itself) inside the bddcost range. */
sbddextended_INLINE_FUNC
bool weightRange_initialize(BDDCT* bddct, bddvar lev,
    const ZBDD& f, const std::vector<llint>& weights,
    llint* sum_pos, llint* sum_neg)
{
    *sum_pos = 0;
    *sum_neg = 0;
    /* Alloc returns 1 on failure. Without this check, CostOfLev would */
    /* silently return 1 for every level and the result would be wrong. */
    if (bddct->Alloc(lev) != 0) {
        std::cerr << "BDDCT::Alloc failed (out of memory?)." << std::endl;
        return false;
    }
    /* Validate only the weights that are actually used. In particular,
       weights[0] is documented to be unused and may hold any value, and
       a weight is not required for a variable that does not appear in f
       (a level between 1 and lev can hold such a variable when the
       variable order differs from the variable numbering), because no
       set of f contains it and its weight never contributes. */
    std::vector<bool> var_used(static_cast<size_t>(lev) + 1, false);
    {
        /* bddsupport returns the set of the variables of f as the
           family of their singletons, so the chain continues in the
           0-child of each node. The result is held in a ZBDD so that
           the reference is released even when a child access throws
           (e.g. with SAPPOROBDD++). */
        const ZBDD support = ZBDD_ID(bddsupport(f.GetID()));
        /* bddsupport returns bddnull when SAPPOROBDD fails (out of
           memory); walking it would read invalid node ids. */
        if (support == ZBDD(-1)) {
            std::cerr << "Cannot compute the variables of the family "
                         "(out of memory?)." << std::endl;
            return false;
        }
        bddp s = support.GetID();
        while (!(s == bddempty || s == bddsingle)) {
            const bddvar sle = bddlevofvar(bddgetvar(s));
            assert(1 <= sle && sle <= lev);
            var_used[sle] = true;
            s = bddgetchild0z(s);
        }
    }
    for (bddvar le = 1; le <= lev; ++le) {
        if (!var_used[le]) {
            /* BDDCT still needs a cost for the level */
            if (bddct->SetCostOfLev(le, 0) != 0) {
                std::cerr << "BDDCT::SetCostOfLev failed." << std::endl;
                return false;
            }
            continue;
        }
        const int var = bddvaroflev(le);
        if (static_cast<int>(weights.size()) <= var) {
            std::cerr << "The size of weights should be larger than "
                "the maximum variable number in f." << std::endl;
            return false;
        }
        if (!sbddextended_is_valid_bddcost(weights[var])) {
            sbddextended_print_bddcost_range_error("Each weight");
            return false;
        }
        if (weights[var] >= 0) {
            *sum_pos += weights[var];
        } else {
            *sum_neg -= weights[var];
        }
        if (bddct->SetCostOfLev(le, static_cast<int>(weights[var])) != 0) {
            std::cerr << "BDDCT::SetCostOfLev failed." << std::endl;
            return false;
        }
    }
    return true;
}

sbddextended_INLINE_FUNC
ZBDD weightRange(const ZBDD& f, llint lower_bound, llint upper_bound, const std::vector<llint>& weights)
{
    /* bddsupport(bddnull) returns bddnull, on which
       weightRange_initialize would loop into invalid child accesses,
       so propagate the error sentinel here. */
    if (f == ZBDD(-1)) {
        return ZBDD(-1);
    }
    if (lower_bound > upper_bound) {
        return ZBDD(0);
    }
    if (upper_bound < sbddextended_bddcost_min()) {
        return ZBDD(0);
    }
    /* The wrappers (weightLE etc.) call this function with bounds
       derived from their own "bound" argument, so name no specific
       parameter in the message. */
    if (!sbddextended_is_valid_bddcost(upper_bound)) {
        sbddextended_print_bddcost_range_error("The bound");
        return ZBDD(-1);
    }
    if (lower_bound > sbddextended_bddcost_min()
            && !sbddextended_is_valid_bddcost(lower_bound - 1)) {
        sbddextended_print_bddcost_range_error("The bound");
        return ZBDD(-1);
    }
    BDDCT bddct;
    const int lev = getLev(f);
    llint sum_pos, sum_neg;
    if (!weightRange_initialize(&bddct, lev, f, weights,
                                &sum_pos, &sum_neg)) {
        return ZBDD(-1);
    }

    /* BDDCT::ZBDD_CostLE computes in int the running bound (the given
       bound minus a partial sum of the used weights) and the cost of a
       lightest rejected / heaviest accepted set (a partial sum of the
       used weights). Refuse the call unless all of these provably fit
       in the bddcost range; otherwise the computation would silently
       overflow and return a wrong ZBDD. The lowest bound passed to
       ZBDD_CostLE is lower_bound - 1 when the lower bound is used, and
       upper_bound otherwise. */
    {
        const llint lowest_bound =
            (lower_bound > sbddextended_bddcost_min())
                ? lower_bound - 1 : upper_bound;
        if (sum_pos > sbddextended_bddcost_max()
                || sum_neg > sbddextended_bddcost_max()
                || upper_bound + sum_neg > sbddextended_bddcost_max()
                || lowest_bound - sum_pos < sbddextended_bddcost_min()) {
            std::cerr << "The sum of the weights is too large: every "
                "partial sum of the used weights, and the bounds "
                "combined with such a sum, must be between "
                << sbddextended_bddcost_min() << " and "
                << sbddextended_bddcost_max() << std::endl;
            return ZBDD(-1);
        }
    }

    ZBDD z = bddct.ZBDD_CostLE(f, static_cast<int>(upper_bound));
    if (lower_bound > sbddextended_bddcost_min()) {
        z -= bddct.ZBDD_CostLE(f, static_cast<int>(lower_bound - 1));
    }
    return z;
}

sbddextended_INLINE_FUNC
ZBDD weightLE(const ZBDD& f, llint bound, const std::vector<llint>& weights)
{
    return weightRange(f, LLONG_MIN, bound, weights);
}

sbddextended_INLINE_FUNC
ZBDD weightLT(const ZBDD& f, llint bound, const std::vector<llint>& weights)
{
    if (bound == LLONG_MIN) {
        return ZBDD(0);
    }
    return weightLE(f, bound - 1, weights);
}

sbddextended_INLINE_FUNC
ZBDD weightGE(const ZBDD& f, llint bound, const std::vector<llint>& weights)
{
    ZBDD z = weightLT(f, bound, weights);
    if (z == ZBDD(-1)) {
        return ZBDD(-1);
    }
    return f - z;
}

sbddextended_INLINE_FUNC
ZBDD weightGT(const ZBDD& f, llint bound, const std::vector<llint>& weights)
{
    if (bound == LLONG_MAX) {
        return ZBDD(0);
    }
    ZBDD z = weightLE(f, bound, weights);
    if (z == ZBDD(-1)) {
        return ZBDD(-1);
    }
    return f - z;
}

sbddextended_INLINE_FUNC
ZBDD weightEQ(const ZBDD& f, llint bound, const std::vector<llint>& weights)
{
    return weightRange(f, bound, bound, weights);
}

sbddextended_INLINE_FUNC
ZBDD weightNE(const ZBDD& f, llint bound, const std::vector<llint>& weights)
{
    ZBDD z = weightEQ(f, bound, weights);
    if (z == ZBDD(-1)) {
        return ZBDD(-1);
    }
    return f - z;
}

#endif /* SBDDH_BDDCT */

#endif

typedef struct tagbddNodeIndex {
    int is_raw;
    int is_zbdd;
    /* All of the following four pointers are NULL if f is a terminal or bddnull. */
    sbddextended_MyDict* node_dict_arr;
    sbddextended_MyVector* level_vec_arr; /* stores all nodes at level i */
    llint* offset_arr;
    ullint* count_arr; /* array representing the number of solutions for node i */
    int height;
    /* The index owns a reference to f: the make and copy functions call
       bddcopy and bddNodeIndex_destruct releases it, so the nodes of f
       stay alive (are not garbage collected) while the index exists,
       even when the caller releases every other reference to f. */
    bddp f;
} bddNodeIndex;

/* defined below; declared here because a make function releases a
   complete index with it when a DD operation throws */
sbddextended_INLINE_FUNC
void bddNodeIndex_destruct(bddNodeIndex* node_index);

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeIndexWithoutCount_inner(bddp f, int is_raw, int is_zbdd)
{
    int i, k, level;
    size_t j;
    bddp node, child;
    bddNodeIndex* node_index;
#ifdef __cplusplus
    int num_initialized_dicts = 0;
    int num_initialized_vecs = 0;
#endif

    node_index = (bddNodeIndex*)malloc(sizeof(bddNodeIndex));
    if (node_index == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    node_index->is_raw = (is_raw != 0 ? 1 : 0);
    /* take a reference of our own so that the nodes of f stay alive
       while the index exists (bddNodeIndex_destruct releases it) */
    node_index->f = bddcopy(f);
    node_index->height = (int)bddgetlev(f);
    node_index->is_zbdd = (is_zbdd != 0 ? 1 : 0);

    /* bddisconstant, not a comparison with the four terminals: a */
    /* multi-valued terminal bddconst(c) (c >= 2) has no node either, */
    /* and treating it as a node would make the code below write into */
    /* node_dict_arr[0], which is never initialized. */
    if (f == bddnull || bddisconstant(f)) {
        node_index->node_dict_arr = NULL;
        node_index->level_vec_arr = NULL;
        node_index->offset_arr = NULL;
        node_index->count_arr = NULL;
        return node_index;
    }

    if (is_raw) {
        f = bdderasenot(f);
    }

    node_index->node_dict_arr = (sbddextended_MyDict*)malloc(
                            (size_t)(node_index->height + 1) * sizeof(sbddextended_MyDict));
    if (node_index->node_dict_arr == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    node_index->level_vec_arr = (sbddextended_MyVector*)malloc(
                            (size_t)(node_index->height + 1) * sizeof(sbddextended_MyVector));
    if (node_index->level_vec_arr == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
#ifdef __cplusplus
    /* The initialize and add calls below can throw (e.g.
       std::bad_alloc). Release the elements initialized so far and the
       structure itself then, so that nothing leaks when the exception
       propagates to the caller (bddNodeIndex_copy protects itself the
       same way). An element whose initialize call itself threw needs
       no release because initialize puts the element into the
       deinitialized state before allocating. */
    try {
#endif
    for (i = 1; i <= node_index->height; ++i) {
        sbddextended_MyDict_initialize(&node_index->node_dict_arr[i]);
#ifdef __cplusplus
        num_initialized_dicts = i;
#endif
        sbddextended_MyVector_initialize(&node_index->level_vec_arr[i]);
#ifdef __cplusplus
        num_initialized_vecs = i;
#endif
    }

    sbddextended_MyDict_add(&node_index->node_dict_arr[node_index->height],
                            (llint)f,
                            0ll); /* 0 means the first node in the level */
    sbddextended_MyVector_add(&node_index->level_vec_arr[node_index->height], (llint)f);

    for (i = node_index->height; i >= 1; --i) {
        for (j = 0; j < node_index->level_vec_arr[i].count; ++j) {
            node = (bddp)sbddextended_MyVector_get(&node_index->level_vec_arr[i], (llint)j);
            for (k = 0; k < sbddextended_NUMBER_OF_CHILDREN; ++k) {
                if (is_raw) {
                    if (is_zbdd) {
                        child = bddgetchildzraw(node, k);
                    } else {
                        child = bddgetchildbraw(node, k);
                    }
                    child = bdderasenot(child);
                } else {
                    if (is_zbdd) {
                        child = bddgetchildz(node, k);
                    } else {
                        child = bddgetchildb(node, k);
                    }
                }

                /* bddisconstant, not bddisterminal: a multi-valued
                   terminal bddconst(c) (c >= 2), which bddchange on
                   such a constant makes a child, has level 0, and the
                   index has no place for it (node_dict_arr[0] is never
                   initialized). It is rejected below, as it is when it
                   is the root. */
                if (!bddisconstant(child)) {
                    level = (int)bddgetlev(child);
                    if (sbddextended_MyDict_find(&node_index->node_dict_arr[level],
                                                 (llint)child, NULL) == 0) {
                        sbddextended_MyDict_add(&node_index->node_dict_arr[level],
                                                (llint)child,
                                                (llint)node_index->node_dict_arr[level].count);
                        sbddextended_MyVector_add(&node_index->level_vec_arr[level], (llint)child);
                    }
                } else if (!bddisterminal(child)) {
                    fprintf(stderr, "bddNodeIndex: a multi-valued terminal "
                            "(bddconst) is not supported as a child.\n");
                    exit(1);
                }
            }
        }
    }
#ifdef __cplusplus
    } catch (...) {
        for (i = 1; i <= num_initialized_dicts; ++i) {
            sbddextended_MyDict_deinitialize(&node_index->node_dict_arr[i]);
        }
        for (i = 1; i <= num_initialized_vecs; ++i) {
            sbddextended_MyVector_deinitialize(&node_index->level_vec_arr[i]);
        }
        free(node_index->node_dict_arr);
        free(node_index->level_vec_arr);
        bddfree(node_index->f); /* the reference taken above */
        free(node_index);
        throw;
    }
#endif

    node_index->offset_arr = (llint*)malloc((size_t)(node_index->height + 1) * sizeof(llint));
    if (node_index->offset_arr == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    node_index->offset_arr[node_index->height] = sbddextended_BDDNODE_START;

    for (i = node_index->height; i >= 1; --i) {
        node_index->offset_arr[i - 1] = node_index->offset_arr[i] + (llint)node_index->level_vec_arr[i].count;
    }

    node_index->count_arr = NULL;
    return node_index;
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeIndexBWithoutCount(bddp f)
{
    return bddNodeIndex_makeIndexWithoutCount_inner(f, 0, 0);
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeIndexZWithoutCount(bddp f)
{
    return bddNodeIndex_makeIndexWithoutCount_inner(f, 0, 1);
}

/* In this and the other auto-detecting make functions below, a terminal
   f is shared between the BDD and ZBDD representations (bddiszbdd is
   true for it), so the auto-detection records a terminal as a ZBDD.
   Use the B/Z-suffixed functions when the intended kind of a terminal
   matters. */
sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeIndexWithoutCount(bddp f)
{
    return bddNodeIndex_makeIndexWithoutCount_inner(f, 0, (bddiszbdd(f) ? 1 : 0));
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeRawIndexBWithoutCount(bddp f)
{
    return bddNodeIndex_makeIndexWithoutCount_inner(f, 1, 0);
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeRawIndexZWithoutCount(bddp f)
{
    return bddNodeIndex_makeIndexWithoutCount_inner(f, 1, 1);
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeRawIndexWithoutCount(bddp f)
{
    return bddNodeIndex_makeIndexWithoutCount_inner(f, 1, (bddiszbdd(f) ? 1 : 0));
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeIndex_inner(bddp f, int is_raw, int is_zbdd)
{
    int i, clevel, raw_flag;
    llint j, id0, id1;
    bddp node, n0, n1;
    bddNodeIndex* node_index;

    node_index = bddNodeIndex_makeIndexWithoutCount_inner(f, is_raw, is_zbdd);

    /* see the comment of the same condition in the function above */
    if (f == bddnull || bddisconstant(f)) {
        return node_index;
    }

    if (is_zbdd) {
        node_index->count_arr = (ullint*)malloc((size_t)node_index->offset_arr[0] * sizeof(ullint));
        if (node_index->count_arr == NULL) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        node_index->count_arr[0] = 0;
        node_index->count_arr[1] = 1;

#ifdef __cplusplus
        /* A child access can throw (e.g. with SAPPOROBDD++). Release
           the index built so far then, so that nothing leaks when the
           exception propagates to the caller. */
        try {
#endif
        for (i = 1; i <= node_index->height; ++i) {
            for (j = node_index->offset_arr[i]; j < node_index->offset_arr[i - 1]; ++j) {
                node = (bddp)sbddextended_MyVector_get(&node_index->level_vec_arr[i],
                                                       j - node_index->offset_arr[i]);
                if (is_raw) {
                    n0 = bddgetchild0zraw(node);
                } else {
                    n0 = bddgetchild0z(node);
                }
                if (n0 == bddempty) {
                    id0 = 0;
                } else if (n0 == bddsingle) {
                    id0 = 1;
                } else {
                    clevel = (int)bddgetlev(n0);
                    if (sbddextended_MyDict_find(&node_index->node_dict_arr[clevel],
                                                     (llint)n0, &id0) == 0) {
                        fprintf(stderr, "node not found!\n");
                        exit(1);
                    }
                    id0 += node_index->offset_arr[clevel];
                }
                raw_flag = 0;
                if (is_raw) {
                    n1 = bddgetchild1zraw(node);
                    if (bddisnegative(n1)) {
                        raw_flag = 1;
                        n1 = bdderasenot(n1);
                    }
                } else {
                    n1 = bddgetchild1z(node);
                }
                if (n1 == bddempty) {
                    id1 = 0;
                } else if (n1 == bddsingle) {
                    id1 = 1;
                } else {
                    clevel = (int)bddgetlev(n1);
                    if (sbddextended_MyDict_find(&node_index->node_dict_arr[clevel],
                                                     (llint)n1, &id1) == 0) {
                        fprintf(stderr, "node not found!\n");
                        exit(1);
                    }
                    id1 += node_index->offset_arr[clevel];
                }
                /* We do not check the overflow. */
                node_index->count_arr[j] = node_index->count_arr[id0] + node_index->count_arr[id1];
                if (is_raw && raw_flag) {
                    node_index->count_arr[j] += 1;
                }
            }
        }
#ifdef __cplusplus
        } catch (...) {
            bddNodeIndex_destruct(node_index);
            free(node_index);
            throw;
        }
#endif
    } else {
        /* not implemented yet. */
    }
    return node_index;
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeIndexB(bddp f)
{
    return bddNodeIndex_makeIndex_inner(f, 0, 0);
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeIndexZ(bddp f)
{
    return bddNodeIndex_makeIndex_inner(f, 0, 1);
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeIndex(bddp f)
{
    return bddNodeIndex_makeIndex_inner(f, 0, (bddiszbdd(f) ? 1 : 0));
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeRawIndexB(bddp f)
{
    return bddNodeIndex_makeIndex_inner(f, 1, 0);
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeRawIndexZ(bddp f)
{
    return bddNodeIndex_makeIndex_inner(f, 1, 1);
}

sbddextended_INLINE_FUNC
bddNodeIndex* bddNodeIndex_makeRawIndex(bddp f)
{
    return bddNodeIndex_makeIndex_inner(f, 1, (bddiszbdd(f) ? 1 : 0));
}

/* Checks that node_index was built from f and for the same kind of DD. */
/* The export functions accept a prepared index and then walk its nodes, */
/* so an index of another DD would make them write that DD instead of f, */
/* and an index of the other kind would make the searches in its node */
/* dictionaries fail. is_zbdd may be negative, which means that the */
/* caller lets the kind of f decide and thus accepts either. A terminal */
/* (or bddnull) is shared between the BDD and ZBDD representations */
/* (bddisbdd and bddiszbdd are both true for it), so the kind stored by */
/* the auto-detecting make functions is arbitrary for it; accept either */
/* kind then, because the index holds no node dictionary to disagree */
/* with anyway. */
sbddextended_INLINE_FUNC
int bddNodeIndex_checkIndexOf(const bddNodeIndex* node_index, bddp f,
                                int is_zbdd)
{
    if (node_index->f != f) {
        fprintf(stderr, "The node index must be constructed from the "
                "BDD/ZBDD to be written.\n");
        return 0;
    }
    if (is_zbdd >= 0 && !(f == bddnull || bddisterminal(f))
            && (node_index->is_zbdd != 0) != (is_zbdd != 0)) {
        fprintf(stderr, "The node index must be constructed for the same "
                "kind of DD as the one to be written.\n");
        return 0;
    }
    return 1;
}

sbddextended_INLINE_FUNC
ullint bddNodeIndex_size(const bddNodeIndex* node_index)
{
    /* an index of bddnull or of a constant (a terminal or a
       multi-valued terminal) holds no node and none of the arrays */
    if (node_index->f == bddnull || bddisconstant(node_index->f)) {
        return 0ull;
    }
    return (ullint)node_index->offset_arr[0] - sbddextended_BDDNODE_START;
}

sbddextended_INLINE_FUNC
ullint bddNodeIndex_sizeAtLevel(const bddNodeIndex* node_index, int level)
{
    if (node_index->f == bddnull || bddisconstant(node_index->f)) {
        return 0ull;
    } else if (level <= 0 || node_index->height < level) {
        return 0ull;
    }
    assert(node_index->offset_arr[level - 1] >= node_index->offset_arr[level]);
    return (ullint)(node_index->offset_arr[level - 1] - node_index->offset_arr[level]);
}

/* arr must have at least node_index->height + 1 elements. All of them
   are overwritten on every call (arr[0] is always 0, and for a terminal
   or bddnull the height is 0, so only arr[0] is written), like the C++
   sizeEachLevel; leaving a part of the output untouched would make
   whatever a reused buffer held before look like the result. The number
   of nodes is stored as ullint because a single level can hold more
   nodes than the variable number type can represent. */
sbddextended_INLINE_FUNC
void bddNodeIndex_sizeEachLevel(const bddNodeIndex* node_index, ullint* arr)
{
    int i;
    for (i = 0; i <= node_index->height; ++i) {
        arr[i] = 0;
    }
    if (!(node_index->f == bddnull || bddisconstant(node_index->f))) {
        for (i = 1; i <= node_index->height; ++i) {
            assert(node_index->offset_arr[i - 1] >= node_index->offset_arr[i]);
            arr[i] = (ullint)(node_index->offset_arr[i - 1] - node_index->offset_arr[i]);
        }
    }
}

sbddextended_INLINE_FUNC
ullint bddNodeIndex_count(const bddNodeIndex* node_index)
{
    /* We assume that the two values are the same. */
    assert(bddtrue == bddsingle);
    if (node_index->f == bddnull || node_index->f == bddfalse
            || node_index->f == bddempty) {
        return 0ull;
    } else if (node_index->f == bddtrue) {
        return 1ull;
    } else if (bddisconstant(node_index->f)) {
        /* a multi-valued terminal bddconst(c) with c >= 2, which is
           neither a BDD nor a ZDD and represents no family */
        fprintf(stderr, "bddNodeIndex_count: the index is made for a "
                "multi-valued terminal, which has no number of "
                "elements.\n");
        exit(1);
    }
    if (node_index->count_arr == NULL) {
        /* count_arr is made only for a ZDD. It is NULL when the index is */
        /* made for a BDD, or made by the "WithoutCount" functions. */
        fprintf(stderr, "bddNodeIndex_count: the number of elements is not "
                "stored in the index. It is stored only when the index is "
                "made for a ZDD with counting (bddNodeIndex_makeIndexZ, "
                "DDNodeIndex(const ZBDD&), etc.).\n");
        exit(1);
    }
    if (node_index->is_raw) {
        if (bddisnegative(node_index->f)) {
            return node_index->count_arr[sbddextended_BDDNODE_START] + 1;
        } else {
            return node_index->count_arr[sbddextended_BDDNODE_START];
        }
    } else {
        return node_index->count_arr[sbddextended_BDDNODE_START];
    }
}

/* Releases everything the index holds and leaves it in the state of an
   index made for bddnull (every array member NULL, f bddnull and
   height 0). Calling this function twice for the same index is
   therefore safe, as it is for the deinitialize functions of MyVector
   and MyDict. node_index may be NULL, in which case this function does
   nothing. */
sbddextended_INLINE_FUNC
void bddNodeIndex_destruct(bddNodeIndex* node_index)
{
    int i;

    if (node_index != NULL) {
        if (node_index->level_vec_arr != NULL) {
            for (i = 1; i <= node_index->height; ++i) {
                sbddextended_MyVector_deinitialize(&node_index->level_vec_arr[i]);
            }
            free(node_index->level_vec_arr);
            node_index->level_vec_arr = NULL;
        }
        if (node_index->node_dict_arr != NULL) {
            for (i = 1; i <= node_index->height; ++i) {
                sbddextended_MyDict_deinitialize(&node_index->node_dict_arr[i]);
            }
            free(node_index->node_dict_arr);
            node_index->node_dict_arr = NULL;
        }
        if (node_index->offset_arr != NULL) {
            free(node_index->offset_arr);
            node_index->offset_arr = NULL;
        }
        if (node_index->count_arr != NULL) {
            free(node_index->count_arr);
            node_index->count_arr = NULL;
        }
        /* release the reference taken by the make or copy function
           (bddfree accepts bddnull and terminals) */
        bddfree(node_index->f);
        node_index->f = bddnull;
        node_index->height = 0;
    }
}

/* "dest" is assumed to be uninitialized. All of its members are overwritten. */
/* If "dest" already holds an index, call bddNodeIndex_destruct for it */
/* before calling this function; otherwise its memory leaks. */
/* In the C++ version, when copying an element throws (e.g. */
/* std::bad_alloc), the elements copied so far are released and all the */
/* array members of "dest" are set to NULL before the exception */
/* propagates, so calling bddNodeIndex_destruct on "dest" afterwards is */
/* safe (and needed: it releases the reference to the root DD that this */
/* function has already taken for "dest"). */
sbddextended_INLINE_FUNC
void bddNodeIndex_copy(bddNodeIndex* dest,
                       const bddNodeIndex* src)
{
    int i;
#ifdef __cplusplus
    int num_initialized_dicts = 0;
    int num_initialized_vecs = 0;
#endif

    dest->is_raw = src->is_raw;
    dest->is_zbdd = src->is_zbdd;
    dest->height = src->height;
    /* the copy owns a reference of its own (see the struct comment) */
    dest->f = bddcopy(src->f);
    dest->node_dict_arr = NULL;
    dest->level_vec_arr = NULL;
    dest->offset_arr = NULL;
    dest->count_arr = NULL;

    /* The arrays hold the elements of index 1, 2, ..., height. */
    /* The element of index 0 is unused (see */
    /* bddNodeIndex_makeIndexWithoutCount_inner). */
#ifdef __cplusplus
    /* The initialize and copy calls below can throw (e.g.
       std::bad_alloc). Release the elements initialized so far and
       reset the array members then; without that, the elements after
       the failed one are left uninitialized, and passing them to
       bddNodeIndex_destruct frees indeterminate pointers. An element
       whose initialize call itself threw needs no release because
       initialize puts the element into the deinitialized state before
       allocating. */
    try {
#endif
    if (src->node_dict_arr != NULL) {
        dest->node_dict_arr = (sbddextended_MyDict*)malloc(
                            (size_t)(src->height + 1) * sizeof(sbddextended_MyDict));
        if (dest->node_dict_arr == NULL) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        for (i = 1; i <= src->height; ++i) {
            sbddextended_MyDict_initialize(&dest->node_dict_arr[i]);
#ifdef __cplusplus
            num_initialized_dicts = i;
#endif
            sbddextended_MyDict_copy(&dest->node_dict_arr[i],
                                     &src->node_dict_arr[i]);
        }
    }
    if (src->level_vec_arr != NULL) {
        dest->level_vec_arr = (sbddextended_MyVector*)malloc(
                            (size_t)(src->height + 1) * sizeof(sbddextended_MyVector));
        if (dest->level_vec_arr == NULL) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        for (i = 1; i <= src->height; ++i) {
            sbddextended_MyVector_initialize(&dest->level_vec_arr[i]);
#ifdef __cplusplus
            num_initialized_vecs = i;
#endif
            sbddextended_MyVector_copy(&dest->level_vec_arr[i],
                                       &src->level_vec_arr[i]);
        }
    }
#ifdef __cplusplus
    } catch (...) {
        for (i = 1; i <= num_initialized_dicts; ++i) {
            sbddextended_MyDict_deinitialize(&dest->node_dict_arr[i]);
        }
        for (i = 1; i <= num_initialized_vecs; ++i) {
            sbddextended_MyVector_deinitialize(&dest->level_vec_arr[i]);
        }
        free(dest->node_dict_arr);
        free(dest->level_vec_arr);
        dest->node_dict_arr = NULL;
        dest->level_vec_arr = NULL;
        throw;
    }
#endif
    if (src->offset_arr != NULL) {
        dest->offset_arr = (llint*)malloc((size_t)(src->height + 1) * sizeof(llint));
        if (dest->offset_arr == NULL) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        memcpy(dest->offset_arr, src->offset_arr,
               (size_t)(src->height + 1) * sizeof(llint));
    }
    if (src->count_arr != NULL) {
        /* count_arr has offset_arr[0] elements. */
        assert(src->offset_arr != NULL);
        dest->count_arr = (ullint*)malloc((size_t)src->offset_arr[0] * sizeof(ullint));
        if (dest->count_arr == NULL) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        memcpy(dest->count_arr, src->count_arr,
               (size_t)src->offset_arr[0] * sizeof(ullint));
    }
}


/* *************************** C++ version start ***************************** */

#ifdef __cplusplus

/* This class is obsolate. */
class DDNodeIndex {
private:
    bddNodeIndex* node_index_;

    /* Non-copyable; owns node_index_. */
    DDNodeIndex(const DDNodeIndex&);
    DDNodeIndex& operator=(const DDNodeIndex&);

public:
    /* The index takes a reference of its own to f (the underlying
       bddNodeIndex calls bddcopy), so the nodes stay alive while the
       index exists, and building an index from a temporary, as in
       DDNodeIndex index(getPowerSet(10)), is safe. */
    DDNodeIndex(const BDD& f, bool is_raw = true)
    {
        node_index_ = bddNodeIndex_makeIndex_inner(f.GetID(), (is_raw ? 1 : 0), 0);
    }

    DDNodeIndex(const ZBDD& f, bool is_raw = true)
    {
        node_index_ = bddNodeIndex_makeIndex_inner(f.GetID(), (is_raw ? 1 : 0), 1);
    }

    bddNodeIndex* getRawPointer()
    {
        return node_index_;
    }

    ullint size()
    {
        return bddNodeIndex_size(node_index_);
    }

    ullint sizeAtLevel(int level)
    {
        return bddNodeIndex_sizeAtLevel(node_index_, level);
    }

    void sizeEachLevel(std::vector<ullint>& arr)
    {
        /* See the comment of DDIndex::sizeEachLevel. */
        arr.assign(static_cast<size_t>(node_index_->height) + 1, 0ull);
        if (!(node_index_->f == bddnull || bddisconstant(node_index_->f))) {
            for (int i = 1; i <= node_index_->height; ++i) {
                arr[static_cast<size_t>(i)] = (ullint)(node_index_->offset_arr[i - 1]
                                                        - node_index_->offset_arr[i]);
            }
        }
    }

    /* for compatibility. The number of nodes of a level is truncated if it
       exceeds the range of bddvar. Use the std::vector<ullint> version. */
    void sizeEachLevel(std::vector<bddvar>& arr)
    {
        std::vector<ullint> arr_ull;
        sizeEachLevel(arr_ull);
        arr.resize(arr_ull.size());
        for (size_t i = 0; i < arr_ull.size(); ++i) {
            arr[i] = static_cast<bddvar>(arr_ull[i]);
        }
    }

    ullint count()
    {
        return bddNodeIndex_count(node_index_);
    }

    ~DDNodeIndex()
    {
        bddNodeIndex_destruct(node_index_);
        free(node_index_);
    }

#if __cplusplus >= 201703L
    class DDNodeIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = bddp;
        using difference_type = std::ptrdiff_t;
        /* operator* returns a value, not a reference to an element. */
        using pointer = const bddp*;
        using reference = bddp;
    private:
#else
    class DDNodeIterator : public std::iterator<std::input_iterator_tag, bddp,
                                                std::ptrdiff_t, const bddp*, bddp>
    {
    private:
#endif
        DDNodeIndex* node_index_;
        size_t pos_;
        size_t level_;

    public:
        DDNodeIterator(DDNodeIndex* node_index, bool is_end) : node_index_(node_index), pos_(0)
        {
            if (is_end) {
                level_ = 0; /* This means pointing at end; */
            } else {
                level_ = node_index->node_index_->height;
            }
        }

        bddp operator*() const
        {
            if (level_ <= 0) {
                return bddfalse;
            }
            return (bddp)sbddextended_MyVector_get(&node_index_->node_index_->
                                                    level_vec_arr[level_],
                                                    (llint)pos_);
        }

        DDNodeIterator& operator++()
        {
            if (level_ > 0) {
                ++pos_;
                while (level_ > 0 &&
                       pos_ >= node_index_->node_index_->level_vec_arr[level_].count) {
                    pos_ = 0;
                    --level_;
                }
            }
            return *this;
        }

        DDNodeIterator operator++(int)
        {
            DDNodeIterator it(*this);
            operator++();
            return it;
        }

        bool operator==(const DDNodeIterator& it) const
        {
            if (level_ <= 0) {
                return it.level_ <= 0;
            } else {
                /* also compare the index, so that iterators of */
                /* different indexes never compare equal */
                return node_index_ == it.node_index_
                        && pos_ == it.pos_ && level_ == it.level_;
            }
        }

        bool operator!=(const DDNodeIterator& it) const
        {
            return !(operator==(it));
        }
    };

    DDNodeIterator begin()
    {
        return DDNodeIterator(this, false);
    }

    DDNodeIterator end()
    {
        return DDNodeIterator(this, true);
    }
};

template <typename T> class DDIndex;

template <typename T>
class DDNode {
private:
    const bddp f_;
    DDIndex<T>& node_index_;
public:
    /* A reference to the storage of node_index_ for f_. DDIndex::clear()
       destroys the storage, so the value of a DDNode obtained before the
       call is a dangling reference afterwards. */
    T& value;

public:
    DDNode(bddp f, DDIndex<T>& node_index) : f_(f), node_index_(node_index), value(node_index.getStorageRef(f)) { }

    bddp getBddp() const
    {
        return f_;
    }

    DDNode<T> child(int c) const
    {
        bddp g;
        if (isTerminal()) {
            g = f_;
        } else if (bddisbdd(f_)) {
            g = bddgetchildb(f_, c);
        } else {
            g = bddgetchildz(f_, c);
        }
        DDNode<T> node(g, node_index_);
        return node;
    }

    bool isTerminal() const
    {
        return (!!bddisterminal(f_));
    }

    /* t is interpreted as a truth value: nonzero means the 1-terminal
       and 0 means the 0-terminal, consistently with child(c) and
       DDIndex::terminal(t). */
    bool isTerminal(int t) const
    {
        return f_ == (t != 0 ? bddtrue : bddfalse);
    }
};

template <typename T>
bool operator==(const DDNode<T>& node1, const DDNode<T>& node2)
{
    return node1.getBddp() == node2.getBddp();
}

template <typename T>
bool operator!=(const DDNode<T>& node1, const DDNode<T>& node2)
{
    return node1.getBddp() != node2.getBddp();
}

/* T must be default-constructible: the value attached to a node is
   created by storage_[f] on first access, so a T without a default
   constructor fails to compile at the first use of root(), terminal(),
   getNode() or DDNode<T>::child(). */
template <typename T>
class DDIndex {
private:
    bddNodeIndex* node_index_;
    std::map<bddp, T> storage_;
    bool is_count_made;
    /* Whether an addition of the count table wrapped around, that is,
       whether the cardinality of the family is outside the range of
       count_t. It stays false when count_t is mpz_class. */
    bool is_count_overflow;

#ifdef SBDDH_GMP
    typedef mpz_class count_t;
#else
    typedef ullint count_t;
#endif
    typedef std::map<bddp, count_t> map_t;
    map_t count_storage_;

    /* Non-copyable; owns node_index_. */
    DDIndex(const DDIndex&);
    DDIndex& operator=(const DDIndex&);

    void initialize(bddp f, bool is_raw, int is_zbdd)
    {
        /* currently, we do not support raw mode. We reject it here instead
           of silently constructing an index of the non-raw mode. */
        if (is_raw) {
            std::cerr << "DDIndex currently does not support raw mode."
                      << std::endl;
            exit(1);
        }
        if (f == bddnull) {
            /* No index can be made for bddnull. We leave the index invalid
               so that isValid() returns false and the member functions
               behave as if the index were cleared. */
            node_index_ = NULL;
            return;
        }
        /* A multi-valued terminal bddconst(c) with c >= 2 is neither a
           BDD nor a ZDD, so no member of this class can treat it.
           Reject it here instead of letting a later member fail with an
           obscure message. */
        if (bddisconstant(f) && !bddisterminal(f)) {
            std::cerr << "DDIndex does not support a multi-valued terminal."
                      << std::endl;
            exit(1);
        }
        node_index_ = bddNodeIndex_makeIndexWithoutCount_inner(f, 0, is_zbdd);
    }

    /* The functions that count/enumerate the elements of the family and
       the functions that deal with weights apply ZDD operations to the
       nodes. Such operations are invalid for a BDD, so we reject them
       here instead of letting SAPPOROBDD abort with an obscure message. */
    void checkZBDD() const
    {
        if (node_index_ != NULL && !node_index_->is_zbdd) {
            std::cerr << "DDIndex currently supports this function only for "
                         "ZDD, but the index is constructed from a BDD."
                      << std::endl;
            exit(1);
        }
    }

    /* The weights are indexed by the variable number, so the vector must
       have an element for every variable of f. std::vector does not check
       the index, so check the size here, as weightRange does.
       A level with no node holds a variable that does not appear in f,
       whose weight is never read, and the variable order may put such a
       variable above the ones f uses, so require a weight only for the
       levels that do have a node. */
    void checkWeights(const std::vector<llint>& weights) const
    {
        for (int level = 1; level <= height(); ++level) {
            if (size(level) == 0) {
                continue;
            }
            if (weights.size() <= static_cast<size_t>(bddvaroflev(level))) {
                std::cerr << "The size of weights should be larger than "
                             "the maximum variable number in f."
                          << std::endl;
                exit(1);
            }
        }
    }

    /* count() is documented to return the cardinality modulo 2^64, but
       the members that walk the family in dictionary order, take the k
       first sets or pick one at random need the exact value. Without GMP
       the count table holds the remainder, and a family of exactly 2^64
       sets would look empty, so refuse rather than answer with it.
       makeCountIndex must have been called. */
    void checkCountExact() const
    {
        if (is_count_overflow) {
            std::cerr << "The number of the elements of the family exceeds "
                         "2^64 - 1, for which this function needs the exact "
                         "number. Define the SBDDH_GMP macro and use the "
                         "GMP version." << std::endl;
            exit(1);
        }
    }

    llint optimize(const std::vector<llint>& weights, bool is_max,
                    std::set<bddvar>& s) const
    {
        checkZBDD();
        checkWeights(weights);
        if (node_index_->is_raw) {
            std::cerr << "DDIndex currently does not support raw mode." << std::endl;
            exit(1);
        }

        /* llint -> max/min value, bool -> 1-arc if true, 0-arc if false */
        std::map<bddp, std::pair<llint, bool> > sto;

        sto[bddsingle].first = 0;

        /* By the zero-suppress rule, the 1-child of a ZDD node is never
           bddempty, so every node except bddempty represents a non-empty
           family and has a value in sto. We detect the empty 0-child by
           comparing the node with bddempty itself instead of assigning a
           sentinel value (LLONG_MIN/LLONG_MAX) to it, because a
           legitimate set can also have such a weight. */
        for (int level = 1; level <= height(); ++level) {
            for (ullint pos = 0; pos < size(level); ++pos) {
                int var = bddvaroflev(level);
                bddp f = getBddp(level, pos);
                bddp child0 = bddgetchild0z(f);
                bddp child1 = bddgetchild1z(f);
                llint value1 = sbddh_checkedAdd(sto[child1].first,
                                                weights[var]);
                bool takes_1_arc;
                if (child0 == bddempty) {
                    /* the 1-arc side is the only choice */
                    takes_1_arc = true;
                } else if (is_max) {
                    takes_1_arc = !(sto[child0].first > value1);
                } else {
                    takes_1_arc = (sto[child0].first > value1);
                }
                if (takes_1_arc) {
                    sto[f].first = value1;
                    sto[f].second = true; /* 1-arc side */
                } else {
                    sto[f].first = sto[child0].first;
                    sto[f].second = false; /* 0-arc side */
                }
            }
        }
        bddp g = node_index_->f;
        while (!bddisterminal(g)) {
            if (sto[g].second) { /* 1-arc */
                s.insert(bddgetvar(g));
                g = bddgetchild1z(g);
            } else { /* 0-arc */
                g = bddgetchild0z(g);
            }
        }
        assert(g == bddsingle);
        return sto[node_index_->f].first;
    }

    /* std::map::at is a C++11 addition, which the oldest supported
       compilers (GCC 4.1.2 era libstdc++) do not provide, so the count
       lookups below use find instead. */
#ifdef SBDDH_GMP
    ullint getStorageValue(bddp f) const
    {
        std::map<bddp, mpz_class>::const_iterator itor = count_storage_.find(f);
        if (itor != count_storage_.end()) {
            return sbddh_mpz_to_ullint(itor->second);
        } else {
            bddp fn = bddtakenot(f);
            itor = count_storage_.find(fn);
            if (itor == count_storage_.end()) {
                std::cerr << "key f not found" << std::endl;
                exit(1);
            } else {
                return sbddh_mpz_to_ullint(itor->second) + (bddisnegative(f) ? 1 : -1);
            }
        }
    }

#else

    ullint getStorageValue(bddp f) const
    {
        std::map<bddp, ullint>::const_iterator itor0 = count_storage_.find(f);
        if (itor0 != count_storage_.end()) {
            return itor0->second;
        } else {
            bddp fn = bddtakenot(f);
            std::map<bddp, ullint>::const_iterator itor = count_storage_.find(fn);
            if (itor == count_storage_.end()) {
                std::cerr << "key f not found" << std::endl;
                exit(1);
            } else {
                if (bddisnegative(f)) {
                    return itor->second + 1;
                } else {
                    assert(itor->second > 0);
                    return itor->second - 1;
                }
            }
        }
    }

#endif

    template<typename value_t>
    value_t getStorageValue2(bddp f) const
    {
        map_t::const_iterator itor = count_storage_.find(f);
        if (itor != count_storage_.end()) {
            return sbddh_getValueFromMpz<value_t>(itor->second);
        } else {
            bddp fn = bddtakenot(f);
            itor = count_storage_.find(fn);
            if (itor == count_storage_.end()) {
                std::cerr << "key f not found" << std::endl;
                exit(1);
            } else {
                if (bddisnegative(f)) {
                    return sbddh_getValueFromMpz<value_t>(itor->second)
                        + sbddh_getOne<value_t>();
                } else {
                    assert(itor->second > 0);
                    return sbddh_getValueFromMpz<value_t>(itor->second)
                        - sbddh_getOne<value_t>();
                }
            }
        }
    }

#ifndef SBDDH_GMP
    /* Returns the position of s in the dictionary order of the
       sub-family rooted at f, or the maximum ullint value as the
       "s is not in the family" sentinel. makeCountIndex and
       checkCountExact must have been called: the counts are then
       exact, every partial value is at most the order of s in the
       whole family, which is less than the exact number of the
       elements (at most 2^64 - 1), so the additions cannot wrap and
       the sentinel cannot collide with a valid order. */
    /* Iterative, not recursive: the DD can be as high as the maximum
       level (65535) and beyond what the process stack can hold as
       recursion frames. The walk descends a single path, accumulating
       the orders of the branches taken on the 0-side. */
    ullint getOrderNumber(bddp f, std::set<bddvar>& s) const
    {
        const ullint not_found = ~static_cast<ullint>(0);
        ullint value = 0;

        for (;;) {
            if (f == bddempty) {
                return not_found;
            } else if (f == bddsingle) {
                if (s.size() > 0) {
                    return not_found;
                } else {
                    return value;
                }
            }

            if (s.size() == 0) {
                if (bddisemptymember(f)) {
                    return value;
                } else {
                    return not_found;
                }
            }

            bddp f0 = f;
            if (bddisemptymember(f)) {
                value += 1;
                f0 = bdderasenot(f);
            }

            bddvar var = bddgetvar(f);
            if (s.count(var) >= 1) {
                s.erase(var);
                f = bddgetchild1z(f);
            } else {
                value += getStorageValue(bddgetchild1z(f));
                f = bddgetchild0z(f0);
            }
        }
    }
#endif

#ifdef SBDDH_GMP
    /* Iterative for the same reason as the non-GMP getOrderNumber. */
    mpz_class getOrderNumberMP(bddp f, std::set<bddvar>& s) const
    {
        mpz_class value(0);

        for (;;) {
            if (f == bddempty) {
                return mpz_class(-1);
            } else if (f == bddsingle) {
                if (s.size() > 0) {
                    return mpz_class(-1);
                } else {
                    return value;
                }
            }

            if (s.size() == 0) {
                if (bddisemptymember(f)) {
                    return value;
                } else {
                    return mpz_class(-1);
                }
            }

            bddp f0 = f;
            if (bddisemptymember(f)) {
                value += mpz_class(1);
                f0 = bdderasenot(f);
            }

            bddvar var = bddgetvar(f);
            if (s.count(var) >= 1) {
                s.erase(var);
                f = bddgetchild1z(f);
            } else {
                map_t::const_iterator itor =
                    count_storage_.find(bddgetchild1z(f));
                assert(itor != count_storage_.end());
                value += itor->second;
                f = bddgetchild0z(f0);
            }
        }
    }
#endif

    /* Iterative, not recursive: the DD can be higher than what the
       process stack can hold as recursion frames. */
    void getSet(bddp f, ullint order, std::set<bddvar>& s)
    {
        while (!(f == bddempty || f == bddsingle)) {
            bddp f0 = f;

            if (bddisemptymember(f)) {
                if (order == 0) {
                    return;
                } else {
                    order -= 1;
                    f0 = bdderasenot(f);
                }
            }

            ullint card1 = getStorageValue(bddgetchild1z(f));
            if (order < card1) {
                s.insert(bddgetvar(f));
                f = bddgetchild1z(f);
            } else {
                order -= card1;
                f = bddgetchild0z(f0);
            }
        }
    }

#ifdef SBDDH_GMP
    /* Iterative for the same reason as getSet. */
    void getSetMP(bddp f, mpz_class order, std::set<bddvar>& s)
    {
        while (!(f == bddempty || f == bddsingle)) {
            bddp f0 = f;

            if (bddisemptymember(f)) {
                if (order == mpz_class(0)) {
                    return;
                } else {
                    order -= mpz_class(1);
                    f0 = bdderasenot(f);
                }
            }

            mpz_class card1 = count_storage_[bddgetchild1z(f)];
            if (order < card1) {
                s.insert(bddgetvar(f));
                f = bddgetchild1z(f);
            } else {
                order -= card1;
                f = bddgetchild0z(f0);
            }
        }
    }
#endif

#ifndef SBDDH_GMP
    std::set<bddvar> getSetByOrder(ullint order)
    {
        if (node_index_ == NULL) {
            return std::set<bddvar>();
        }
        makeCountIndex();
        checkCountExact();
        if (order >= count()) { /* out of range */
            return std::set<bddvar>();
        }
        std::set<bddvar> s;
        getSet(node_index_->f, order, s);
        return s;
    }

#else
    std::set<bddvar> getSetByOrder(const mpz_class& order)
    {
        if (node_index_ == NULL) {
            return std::set<bddvar>();
        }
        makeCountIndex();
        if (order < mpz_class(0) || order >= countMP()) { /* out of range */
            return std::set<bddvar>();
        }
        std::set<bddvar> s;
        getSetMP(node_index_->f, order, s);
        return s;
    }
#endif /* SBDDH_GMP */

    /* A frame of the iterative getKSetsZBDD below: what the walk chose
       at one node of the descent path, which is all that rebuilding the
       result on the way back up needs. f1 is a weak reference (valid
       because the DD of the index stays alive during the call). */
    struct KSetsFrame {
        bddvar var;
        bddp f1;
        bool took0;
        bool emptymem;
        KSetsFrame(bddvar va, bddp f1a, bool took0a, bool emptymema)
            : var(va), f1(f1a), took0(took0a), emptymem(emptymema) { }
    };

    /* Iterative, not recursive: the DD can be higher than what the
       process stack can hold as recursion frames, so the walk descends
       a single path recording a KSetsFrame per node and then rebuilds
       the result bottom-up. */
    template<typename value_t>
    bddp getKSetsZBDD(bddp f, value_t k)
    {
        std::vector<KSetsFrame> frames;
#ifndef NDEBUG
        const value_t k_entry = k;
#endif
        bddp g;

        for (;;) {
            if (k <= sbddh_getZero<value_t>() || f == bddempty) {
                g = bddempty;
                break;
            } else if (f == bddsingle) {
                g = bddsingle;
                break;
            } else if (k >= getStorageValue2<value_t>(f)) {
                g = bddcopy(f);
                break;
            } else {
                bddp fn = f;
                bddp f1 = bddgetchild1z(f);
                value_t card1 = getStorageValue2<value_t>(f1);
                bool emptymem = (bddisemptymember(f) != 0);
                if (emptymem) {
                    if (k == sbddh_getOne<value_t>()) {
                        g = bddsingle;
                        break;
                    }
                    card1 += sbddh_getOne<value_t>();
                    fn = bdderasenot(f);
                }
                if (k > card1) {
                    frames.push_back(KSetsFrame(bddgetvar(f), f1,
                                                true, emptymem));
                    f = bddgetchild0z(fn);
                    k -= card1;
                } else {
                    frames.push_back(KSetsFrame(bddgetvar(f), f1,
                                                false, emptymem));
                    f = f1;
                    if (emptymem) {
                        k -= sbddh_getOne<value_t>();
                    }
                }
            }
        }

#ifndef NDEBUG
        const bool built = !frames.empty();
#endif
        while (!frames.empty()) {
            const KSetsFrame frame = frames.back();
            frames.pop_back();
            bddp child = g; /* the owned reference built so far */
            /* Release that reference even when the node construction
               throws (e.g. with SAPPOROBDD++); the references held by
               the remaining frames are weak and need no release. */
            try {
                if (frame.took0) {
                    if (frame.emptymem) {
                        assert(!bddisnegative(child));
                        child = bddtakenot(child);
                    }
                    g = bddmakenodez(frame.var, child, frame.f1);
                } else {
                    g = bddmakenodez(frame.var,
                                     (frame.emptymem ? bddsingle : bddempty),
                                     child);
                }
            } catch (...) {
                bddfree(child);
                throw;
            }
            bddfree(child);
        }
#ifndef NDEBUG
        /* Check the built family only once at the top: a per-node check
           would cost O(height^2) on a deep DD. bddcard saturates at
           bddnull, so it must not be used to check a family that this
           function can build but that bddcard cannot count;
           sbddh_getCard is exact for both value_t. */
        if (built) {
            assert(sbddh_getCard<value_t>(ZBDD_ID(bddcopy(g))) == k_entry);
        }
#endif
        return g;
    }

#ifdef SBDDH_BDDCT
    template<typename value_t>
    ZBDD getKLightestZBDD(const ZBDD& f, const value_t& k,
        const std::vector<llint>& weights, int strict)
    {
        if (k <= sbddh_getZero<value_t>() || f == ZBDD(0)) {
            return ZBDD(0);
        }
        makeCountIndex();
        /* k is more than or equal to the card of f */
        if (k >= getStorageValue2<value_t>(f.GetID())) {
            return f;
        }
        BDDCT bddct;
        const int lev = getLev(f);
        llint sum_pos, sum_neg;
        if (!weightRange_initialize(&bddct, lev, f, weights,
                                    &sum_pos, &sum_neg)) {
            return ZBDD(-1);
        }
        /* The binary search passes bounds between MinCost(f) - 1 and
           MaxCost(f) to ZBDD_CostLE, which computes in int the running
           bound (the given bound minus a partial sum of the used
           weights) and costs that are partial sums of the used weights.
           All of these lie within [-(sum_pos + sum_neg) - 1,
           sum_pos + sum_neg], so requiring the total spread to fit in
           the bddcost range keeps every intermediate value (and the
           right_bound - left_bound subtraction below) free of int
           overflow. */
        if (sum_pos + sum_neg > sbddextended_bddcost_max()) {
            std::cerr << "The sum of the absolute values of the used "
                "weights must be at most " << sbddextended_bddcost_max()
                << std::endl;
            return ZBDD(-1);
        }

        /* binary search: k is in (left_bound, right_bound) */
        int left_bound = bddct.MinCost(f) - 1;
        int right_bound = bddct.MaxCost(f);
        assert(left_bound < right_bound);
        ZBDD c_zbdd;
        ZBDD left_zbdd(0);
        ZBDD right_zbdd = f;
        value_t c_card;
        value_t left_card = 0;
        while (right_bound - left_bound > 1) {
            int c_bound = left_bound + (right_bound - left_bound) / 2;
            assert(c_bound >= left_bound + 1);
            c_zbdd = bddct.ZBDD_CostLE(f, c_bound);
            /* The ZBDD operations inside ZBDD_CostLE return the null
               ZBDD when SAPPOROBDD runs out of memory. Without this
               check sbddh_getCard would report 0 sets for it and the
               binary search would go on with a wrong bound. */
            if (c_zbdd == ZBDD(-1)) {
                std::cerr << "BDDCT::ZBDD_CostLE failed (out of memory?)."
                          << std::endl;
                return ZBDD(-1);
            }
            c_card = sbddh_getCard<value_t>(c_zbdd);
            if (c_card == k) {
                return c_zbdd;
            } else if (c_card < k) {
                left_bound = c_bound;
                left_zbdd = c_zbdd;
                left_card = c_card;
            } else {
                right_bound = c_bound;
                right_zbdd = c_zbdd;
            }
            assert(left_bound < right_bound);
        }
        assert(left_bound + 1 == right_bound);
        /* assert(bddct.ZBDD_CostLE(f, left_bound) == left_zbdd); */
        /* assert(bddct.ZBDD_CostLE(f, right_bound) == right_zbdd); */
        if (strict < 0) {
            return left_zbdd;
        } else if (strict > 0) {
            return right_zbdd;
        } else {
            ZBDD delta = right_zbdd - left_zbdd;
            DDIndex<int> delta_index(delta);
            return left_zbdd + delta_index.getKSetsZBDD(k - left_card);
        }
    }
#endif /* SBDDH_BDDCT */

    template<typename value_t>
    void sampleRandomlyA(ullint* rand_state, std::set<bddvar>& s)
    {
        bddp f = node_index_->f;
        while (!bddisconstant(f)) {
            bddp f0 = bddgetchild0(f);
            bddp f1 = bddgetchild1(f);
            value_t card0 = getStorageValue2<value_t>(f0);
            value_t card1 = getStorageValue2<value_t>(f1);
            double r = static_cast<double>(sbddextended_getXRand(rand_state) - 1)
                    /* / 0xffffffffffffffffull; */
                    / 1.8446744073709552e+19; /* avoid warning */
            /* not sbddh_divide(card0, card0 + card1): with value_t ==
               ullint the sum wraps around for a family of exactly 2^64
               sets (see sbddh_divideBySum) */
            if (r < sbddh_divideBySum<value_t>(card0, card1)) {
                f = f0;
            } else {
                s.insert(bddgetvar(f));
                f = f1;
            }
        }
        assert(f == bddsingle);
    }

    bddp getBddp(int level, ullint pos) const
    {
        return static_cast<bddp>(sbddextended_MyVector_get(&node_index_->
                                                    level_vec_arr[level],
                                                    static_cast<llint>(pos)));
    }

public:
    /* The index takes a reference of its own to f (the underlying
       bddNodeIndex calls bddcopy), so the nodes stay alive while the
       index exists, and building an index from a temporary, as in
       DDIndex<int> index(getPowerSet(10)), is safe. */
    DDIndex(const BDD& f, bool is_raw = false)
        : is_count_made(false), is_count_overflow(false)
    {
        initialize(f.GetID(), is_raw, 0);
    }

    DDIndex(const ZBDD& f, bool is_raw = false)
        : is_count_made(false), is_count_overflow(false)
    {
        initialize(f.GetID(), is_raw, 1);
    }

    /* A terminal f is shared between the BDD and ZBDD representations
       (bddiszbdd is true for it), so this constructor records a
       terminal as a ZBDD. Use the BDD/ZBDD overloads when the intended
       kind of a terminal matters. */
    DDIndex(bddp f, bool is_raw = false)
        : is_count_made(false), is_count_overflow(false)
    {
        initialize(f, is_raw, (bddiszbdd(f) ? 1 : 0));
    }

    ~DDIndex()
    {
        clear();
    }

    /* Releases the memory held by the index. The data attached to the
       nodes is destroyed as well, so the references returned by
       getStorageRef() and the value member of the DDNodes obtained
       before the call must not be used afterwards. */
    void clear()
    {
        if (node_index_ != NULL) {
            bddNodeIndex_destruct(node_index_);
            free(node_index_);
            node_index_ = NULL;
        }
        storage_.clear();
        count_storage_.clear();
        is_count_made = false;
        is_count_overflow = false;
    }

    bool isValid() const
    {
        return node_index_ != NULL;
    }

    /* for compatibility */
    bool is_valid() const
    {
        return isValid();
    }

    bddNodeIndex* getRawPointer()
    {
        return node_index_;
    }

    T& getStorageRef(bddp f)
    {
        return storage_[f];
    }

    ZBDD getZBDD() const
    {
        if (node_index_ == NULL) {
            return ZBDD(0);
        }
        /* Wrapping the raw id of a BDD node in a ZBDD would produce an
           object whose static type promises a ZDD and whose node makes
           SAPPOROBDD stop with "applying non-ZBDD node". */
        checkZBDD();
        return ZBDD_ID(bddcopy(node_index_->f));
    }

    int height() const
    {
        if (node_index_ == NULL) {
            return 0;
        }
        return bddgetlev(node_index_->f);
    }

    ullint size() const
    {
        if (node_index_ == NULL) {
            return 0ull;
        }
        return bddNodeIndex_size(node_index_);
    }

    ullint size(int level) const
    {
        if (node_index_ == NULL) {
            return 0ull;
        }
        return bddNodeIndex_sizeAtLevel(node_index_, level);
    }

    void sizeEachLevel(std::vector<ullint>& arr) const
    {
        /* Overwrite the whole output on every call. Leaving the argument
           untouched for a terminal or a cleared index would make whatever
           it held before look like the result. */
        if (node_index_ == NULL) {
            arr.clear();
            return;
        }
        arr.assign(static_cast<size_t>(node_index_->height) + 1, 0ull);
        if (!(node_index_->f == bddnull || bddisconstant(node_index_->f))) {
            for (int i = 1; i <= node_index_->height; ++i) {
                arr[static_cast<size_t>(i)] = (ullint)(node_index_->offset_arr[i - 1]
                                                        - node_index_->offset_arr[i]);
            }
        }
    }

    /* for compatibility. The number of nodes of a level is truncated if it
       exceeds the range of bddvar. Use the std::vector<ullint> version. */
    void sizeEachLevel(std::vector<bddvar>& arr) const
    {
        std::vector<ullint> arr_ull;
        sizeEachLevel(arr_ull);
        arr.resize(arr_ull.size());
        for (size_t i = 0; i < arr_ull.size(); ++i) {
            arr[i] = static_cast<bddvar>(arr_ull[i]);
        }
    }

    std::set<bddvar> usedVar() const
    {
        std::set<bddvar> result;
        if (node_index_ == NULL) {
            return result;
        }
        std::vector<ullint> size_arr;
        sizeEachLevel(size_arr);
        for (int lev = 1; lev <= node_index_->height; ++lev) {
            if (size_arr[lev] > 0) {
                result.insert(bddvaroflev(lev));
            }
        }
        return result;
    }

    ullint count()
    {
        if (node_index_ == NULL) {
            return 0ull;
        }
        makeCountIndex();
        return getStorageValue2<ullint>(node_index_->f);
    }

#ifdef SBDDH_GMP
    mpz_class countMP()
    {
        if (node_index_ == NULL) {
            return mpz_class(0);
        }
        makeCountIndex();
        return getStorageValue2<mpz_class>(node_index_->f);
    }

    mpz_class count_v()
    {
        return countMP();
    }
#else
    ullint count_v()
    {
        return count();
    }
#endif

    /* In getMaximum/getMinimum and the getK* functions below, the
       ZDD-only check comes before every argument- or terminal-dependent
       short circuit, so that calling them on an index built from a BDD
       is always diagnosed instead of depending on the value of the DD
       or of the arguments (only an invalid/cleared index skips it). */
    llint getMaximum(const std::vector<llint>& weights, std::set<bddvar>& s) const
    {
        if (node_index_ == NULL) {
            return 0ll;
        }
        checkZBDD();
        if (node_index_->f == bddempty) {
            return 0ll;
        }
        return optimize(weights, true, s);
    }

    llint getMaximum(const std::vector<llint>& weights) const
    {
        std::set<bddvar> dummy;
        return getMaximum(weights, dummy);
    }

    llint getMinimum(const std::vector<llint>& weights, std::set<bddvar>& s) const
    {
        if (node_index_ == NULL) {
            return 0ll;
        }
        checkZBDD();
        if (node_index_->f == bddempty) {
            return 0ll;
        }
        return optimize(weights, false, s);
    }

    llint getMinimum(const std::vector<llint>& weights) const
    {
        std::set<bddvar> dummy;
        return getMinimum(weights, dummy);
    }

    llint getSum(const std::vector<llint>& weights)
    {
#ifdef SBDDH_GMP
        /* getStorageValue reads the counts through a mpz -> ullint
           truncation, with which the checked arithmetic below could
           not detect a wrong sum, so compute exactly with getSumMP
           and convert at the end. */
        const mpz_class v = getSumMP(weights);
        const mpz_class llmax =
            sbddh_ullint_to_mpz(static_cast<ullint>(LLONG_MAX));
        if (v > llmax || v < -llmax - 1) {
            std::cerr << "The weight computation causes an overflow of "
                         "long long int. Use getSumMP." << std::endl;
            exit(1);
        }
        if (v >= 0) {
            return static_cast<llint>(sbddh_mpz_to_ullint(v));
        }
        {
            const ullint uv = sbddh_mpz_to_ullint(mpz_class(-v));
            if (uv == static_cast<ullint>(LLONG_MAX) + 1) {
                return LLONG_MIN;
            }
            return -static_cast<llint>(uv);
        }
#else
        checkWeights(weights);
        if (node_index_ == NULL) {
            return 0ll;
        }
        if (node_index_->is_raw) {
            std::cerr << "DDIndex currently does not support raw mode." << std::endl;
            exit(1);
        }

        makeCountIndex();
        /* Without exact counts getStorageValue returns the cardinality
           modulo 2^64, which would slip through the checked arithmetic
           below and produce a silently wrong sum. */
        checkCountExact();

        std::map<bddp, llint> sto;

        sto[bddempty] = 0;
        sto[bddsingle] = 0;

        for (int level = 1; level <= height(); ++level) {
            for (ullint pos = 0; pos < size(level); ++pos) {
                int var = bddvaroflev(level);
                bddp f = getBddp(level, pos);
                bddp child0 = bddgetchild0z(f);
                bddp child1 = bddgetchild1z(f);
                sto[f] = sbddh_checkedAdd(
                    sbddh_checkedAdd(sto[child0], sto[child1]),
                    sbddh_checkedMul(weights[var],
                                     getStorageValue(child1)));
            }
        }
        return sto[node_index_->f];
#endif
    }

#ifdef SBDDH_GMP
    mpz_class getSumMP(const std::vector<llint>& weights)
    {
        checkWeights(weights);
        if (node_index_ == NULL) {
            return mpz_class(0);
        }
        if (node_index_->is_raw) {
            std::cerr << "DDIndex currently does not support raw mode." << std::endl;
            exit(1);
        }

        makeCountIndex();

        std::map<bddp, mpz_class> sto;

        sto[bddempty] = mpz_class(0);
        sto[bddsingle] = mpz_class(0);

        for (int level = 1; level <= height(); ++level) {
            for (ullint pos = 0; pos < size(level); ++pos) {
                int var = bddvaroflev(level);
                bddp f = getBddp(level, pos);
                bddp child0 = bddgetchild0z(f);
                bddp child1 = bddgetchild1z(f);
                mpz_class w_mp = sbddh_llint_to_mpz(weights[var]);
                map_t::const_iterator itor = count_storage_.find(child1);
                assert(itor != count_storage_.end());
                sto[f] = sto[child0] + sto[child1] + w_mp * itor->second;
            }
        }
        return sto[node_index_->f];
    }
#endif

    llint getOrderNumber(const std::set<bddvar>& s)
    {
        if (node_index_ == NULL) {
            return -1;
        }
        makeCountIndex();
        std::set<bddvar> ss(s);
#ifdef SBDDH_GMP
        /* The non-GMP recursion reads the counts through a
           mpz -> ullint truncation, so compute exactly with mpz and
           convert at the end. */
        mpz_class v = getOrderNumberMP(node_index_->f, ss);
        if (v < 0) {
            return -1;
        }
        if (v > sbddh_ullint_to_mpz(static_cast<ullint>(LLONG_MAX))) {
            std::cerr << "The order number of the set does not fit in "
                         "long long int. Use getOrderNumberMP."
                      << std::endl;
            exit(1);
        }
        return static_cast<llint>(sbddh_mpz_to_ullint(v));
#else
        /* Without GMP the count table holds the cardinality modulo
           2^64, with which the order number would be wrong, so require
           the exact value like getSet does. */
        checkCountExact();
        ullint v = getOrderNumber(node_index_->f, ss);
        if (v == ~static_cast<ullint>(0)) {
            return -1;
        }
        if (v > static_cast<ullint>(LLONG_MAX)) {
            std::cerr << "The order number of the set does not fit in "
                         "long long int. Define the SBDDH_GMP macro "
                         "and use getOrderNumberMP." << std::endl;
            exit(1);
        }
        return static_cast<llint>(v);
#endif
    }

#ifdef SBDDH_GMP
    mpz_class getOrderNumberMP(const std::set<bddvar>& s)
    {
        if (node_index_ == NULL) {
            return mpz_class(-1);
        }
        makeCountIndex();
        std::set<bddvar> ss(s);
        return getOrderNumberMP(node_index_->f, ss);
    }
#endif

    std::set<bddvar> getSet(llint order)
    {
        if (order < 0) {
            return std::set<bddvar>();
        }
#ifdef SBDDH_GMP
        return getSetByOrder(sbddh_llint_to_mpz(order));
#else
        return getSetByOrder(static_cast<ullint>(order));
#endif
    }

#ifdef SBDDH_GMP
    std::set<bddvar> getSet(mpz_class order)
    {
        return getSetByOrder(order);
    }
#endif

    ZBDD getKSetsZBDD(ullint k)
    {
#ifdef SBDDH_GMP
        /* Compute with mpz_class.  The card of the family or of a
           subfamily may not fit in ullint even when k does, and the
           truncated card would make the result wrong. */
        return getKSetsZBDD(sbddh_ullint_to_mpz(k));
#else
        if (node_index_ == NULL) {
            return ZBDD(0);
        }
        checkZBDD();
        if (k <= 0) {
            return ZBDD(0);
        }
        makeCountIndex();
        checkCountExact();
        return ZBDD_ID(getKSetsZBDD<ullint>(node_index_->f, k));
#endif
    }

#ifdef SBDDH_GMP
    ZBDD getKSetsZBDD(const mpz_class& k)
    {
        if (node_index_ == NULL) {
            return ZBDD(0);
        }
        checkZBDD();
        if (k <= 0) {
            return ZBDD(0);
        }
        makeCountIndex();
        return ZBDD_ID(getKSetsZBDD<mpz_class>(node_index_->f, k));
    }
#endif

#ifdef SBDDH_BDDCT
    ZBDD getKLightestZBDD(ullint k,
        const std::vector<llint>& weights, int strict)
    {
#ifdef SBDDH_GMP
        /* Compute with mpz_class (see getKSetsZBDD(ullint)). */
        return getKLightestZBDD(sbddh_ullint_to_mpz(k), weights, strict);
#else
        if (node_index_ == NULL) {
            return ZBDD(0);
        }
        checkZBDD();
        makeCountIndex();
        checkCountExact();
        ZBDD f = ZBDD_ID(bddcopy(node_index_->f));
        return getKLightestZBDD<ullint>(f, k, weights, strict);
#endif
    }

#ifdef SBDDH_GMP
    ZBDD getKLightestZBDD(const mpz_class& k,
        const std::vector<llint>& weights, int strict)
    {
        if (node_index_ == NULL) {
            return ZBDD(0);
        }
        checkZBDD();
        ZBDD f = ZBDD_ID(bddcopy(node_index_->f));
        return getKLightestZBDD<mpz_class>(f, k, weights, strict);
    }
#endif /* SBDDH_GMP */

    ZBDD getKHeaviestZBDD(ullint k,
        const std::vector<llint>& weights, int strict)
    {
#ifdef SBDDH_GMP
        /* Compute with mpz_class (see getKSetsZBDD(ullint)).  In
           particular, count() is the card modulo 2^64, so it must not
           be used to decide whether k covers the whole family. */
        return getKHeaviestZBDD(sbddh_ullint_to_mpz(k), weights, strict);
#else
        if (node_index_ == NULL) {
            return ZBDD(0);
        }
        checkZBDD();
        if (k == 0) {
            return ZBDD(0);
        }
        ullint card = count();
        checkCountExact();
        ZBDD f = ZBDD_ID(bddcopy(node_index_->f));
        if (k >= card) {
            return f;
        }
        return f - getKLightestZBDD<ullint>(f, card - k, weights,
                                            sbddh_invertStrict(strict));
#endif
    }

#ifdef SBDDH_GMP
    ZBDD getKHeaviestZBDD(const mpz_class& k,
        const std::vector<llint>& weights, int strict)
    {
        if (node_index_ == NULL) {
            return ZBDD(0);
        }
        checkZBDD();
        if (k <= 0) {
            return ZBDD(0);
        }
        mpz_class card = countMP();
        ZBDD f = ZBDD_ID(bddcopy(node_index_->f));
        if (k >= card) {
            return f;
        }
        return f - getKLightestZBDD<mpz_class>(f, card - k, weights,
                                               sbddh_invertStrict(strict));
    }
#endif /* SBDDH_GMP */

#endif /* SBDDH_BDDCT */

#ifdef SBDDH_GMP /* use GMP random */
    std::set<bddvar> sampleRandomly(gmp_randclass& random)
    {
        if (node_index_ == NULL) {
            return std::set<bddvar>();
        }
        makeCountIndex();
        const mpz_class card = countMP();
        if (card <= 0) { /* get_z_range needs a positive range */
            return std::set<bddvar>();
        }
        return getSet(random.get_z_range(card));
    }
#else /* SBDDH_GMP */

#if __cplusplus >= 201103L /* use C++ random class */

    template <typename U>
    std::set<bddvar> sampleRandomly(U& random_engine)
    {
        if (node_index_ == NULL) {
            return std::set<bddvar>();
        }
        makeCountIndex();
        checkCountExact();
        const ullint card = count();
        if (card < 1) { /* the distribution needs a non-empty range */
            return std::set<bddvar>();
        }
        std::uniform_int_distribution<ullint> dist(0, card - 1);
        return getSetByOrder(dist(random_engine));
    }

#else /* __cplusplus >= 201103L // use rand() function */

    std::set<bddvar> sampleRandomly()
    {
        if (node_index_ == NULL) {
            return std::set<bddvar>();
        }
        makeCountIndex();
        checkCountExact();
        const ullint card = count();
        if (card < 1) {
            return std::set<bddvar>();
        }
        /* Not rand() % card: rand() cannot return an order number above
           RAND_MAX, so the sets past it would never be chosen, and the
           remainder is biased towards the small order numbers. */
        return getSetByOrder(sbddh_randBelow(card));
    }

#endif /* __cplusplus >= 201103L */

#endif /* SBDDH_GMP */

    std::set<bddvar> sampleRandomlyA(ullint* rand_state)
    {
        /* Detect the error here: the internal random generator would
           otherwise dereference the pointer without a check. */
        if (rand_state == NULL) {
            std::cerr << "sampleRandomlyA: rand_state must not be NULL."
                      << std::endl;
            exit(1);
        }
        if (node_index_ == NULL) {
            return std::set<bddvar>();
        }
        makeCountIndex();
        std::set<bddvar> s;
#ifdef SBDDH_GMP
        /* count() is the card modulo 2^64, so it must not be used to */
        /* decide whether the family is empty. */
        const mpz_class card = countMP();
        if (card <= 0) {
            return s;
        }
        /* card is larger than or equal to 2^64 */
        if (card >= mpz_class("18446744073709551616")) {
            sampleRandomlyA<mpz_class>(rand_state, s);
            return s;
        }
#else
        /* count() is the cardinality modulo 2^64, so a family of exactly
           2^64 sets would look empty. The family is empty exactly when
           the DD is the 0-terminal, which this function does support. */
        if (node_index_->f == bddempty) {
            return s;
        }
#endif
        sampleRandomlyA<ullint>(rand_state, s);
        return s;
    }

    DDNode<T> root()
    {
        if (node_index_ == NULL) {
            return DDNode<T>(bddfalse, *this);
        }
        return DDNode<T>(node_index_->f, *this);
    }

    DDNode<T> terminal(int t)
    {
        return DDNode<T>((t != 0 ? bddtrue : bddfalse), *this);
    }

    DDNode<T> getNode(int level, ullint pos)
    {
        if (node_index_ == NULL) {
            return DDNode<T>(bddfalse, *this);
        }
        /* Check the public arguments unconditionally. Without this,
           level 0 reads an array element that is never initialized and
           an out-of-range pos reads out of the bounds of the level
           vector (the assert inside disappears in NDEBUG builds). */
        if (level < 1 || level > height() || pos >= size(level)) {
            std::cerr << "getNode: level must be in [1, height()] and "
                         "pos must be less than size(level)." << std::endl;
            exit(1);
        }
        return DDNode<T>(getBddp(level, pos), *this);
    }

    void makeCountIndex() /* currently support only for ZDD */
    {
        if (node_index_ == NULL) {
            return;
        }
        checkZBDD();
        if (!is_count_made) {
            if (node_index_->is_raw) {
                std::cerr << "DDIndex currently does not support raw mode." << std::endl;
                exit(1);
            }
            /* Record that the table is made only after it is complete.
               An exception from an allocation of the map, or from a child
               accessor, would otherwise leave a partial table that the
               later calls take for a finished one, which makes the index
               unusable for good even though the caller recovered. */
            try {
                count_storage_[bddempty] = sbddextended_VALUE_ZERO;
                count_storage_[bddsingle] = sbddextended_VALUE_ONE;
                for (int level = 1; level <= height(); ++level) {
                    for (ullint pos = 0; pos < size(level); ++pos) {
                        bddp f = getBddp(level, pos);
                        bddp child0 = bddgetchild0z(f);
                        bddp child1 = bddgetchild1z(f);
                        const count_t c0 = count_storage_[child0];
                        const count_t c1 = count_storage_[child1];
                        const count_t sum = c0 + c1;
#ifndef SBDDH_GMP
                        /* count_t is ullint, whose addition wraps around
                           2^64 instead of overflowing. Every node of the
                           index contributes to the count of the root, so
                           one wrap anywhere means that the cardinality of
                           the family does not fit in count_t. */
                        if (sum < c0) {
                            is_count_overflow = true;
                        }
#endif
                        count_storage_[f] = sum;
                    }
                }
            } catch (...) {
                count_storage_.clear();
                /* the flag possibly set by the aborted pass above */
                is_count_overflow = false;
                throw;
            }
            is_count_made = true;
        }
    }

#if __cplusplus >= 201703L
    class DDNodeIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = bddp;
        using difference_type = std::ptrdiff_t;
        /* operator* returns a value, not a reference to an element. */
        using pointer = const bddp*;
        using reference = bddp;
    private:
#else
    class DDNodeIterator : public std::iterator<std::input_iterator_tag, bddp,
                                                std::ptrdiff_t, const bddp*, bddp>
    {
    private:
#endif
        DDIndex* node_index_;
        size_t pos_;
        size_t level_;

    public:
        DDNodeIterator(DDIndex* node_index, bool is_end) : node_index_(node_index), pos_(0)
        {
            if (is_end || node_index->node_index_ == NULL) {
                level_ = 0; /* This means pointing at end; */
            } else {
                level_ = node_index->node_index_->height;
            }
        }

        bddp operator*() const
        {
            if (level_ <= 0 || node_index_->node_index_ == NULL) {
                return bddfalse;
            }
            return sbddextended_MyVector_get(&node_index_->node_index_->
                                                level_vec_arr[level_],
                                                (llint)pos_);
        }

        DDNodeIterator& operator++()
        {
            if (node_index_->node_index_ == NULL) {
                /* The index has been cleared after this iterator was
                   made. Degrade into the end iterator, so that a loop
                   advancing this iterator until end() terminates. */
                pos_ = 0;
                level_ = 0;
            } else if (level_ > 0) {
                ++pos_;
                while (level_ > 0 &&
                        pos_ >= node_index_->node_index_->level_vec_arr[level_].count) {
                    pos_ = 0;
                    --level_;
                }
            }
            return *this;
        }

        DDNodeIterator operator++(int)
        {
            DDNodeIterator it(*this);
            operator++();
            return it;
        }

        bool operator==(const DDNodeIterator& it) const
        {
            if (level_ <= 0) {
                return it.level_ <= 0;
            } else {
                /* also compare the index, so that iterators of */
                /* different indexes never compare equal */
                return node_index_ == it.node_index_
                        && pos_ == it.pos_ && level_ == it.level_;
            }
        }

        bool operator!=(const DDNodeIterator& it) const
        {
            return !(operator==(it));
        }
    };

#if __cplusplus >= 201703L
    class WeightIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::set<bddvar>;
        using difference_type = std::ptrdiff_t;
        /* operator* returns a value, not a reference to an element. */
        using pointer = const std::set<bddvar>*;
        using reference = std::set<bddvar>;
    private:
#else
    class WeightIterator : public std::iterator<std::input_iterator_tag, std::set<bddvar>,
                                    std::ptrdiff_t, const std::set<bddvar>*,
                                    std::set<bddvar> >
    {
    private:
#endif
        ZBDD f_;
        std::set<bddvar> current_;
        std::vector<llint> weights_;
        bool is_min_;

        void setCurrent()
        {
            current_.clear();
            DDIndex<int> current_index(f_);
            if (is_min_) {
                current_index.getMinimum(weights_, current_);
            } else {
                current_index.getMaximum(weights_, current_);
            }
        }

    public:
        WeightIterator() : f_(ZBDD(0)), is_min_(false) { }

        WeightIterator(const ZBDD& f,
                const std::vector<llint>& weights,
                bool is_min) :
                    f_(f), weights_(weights), is_min_(is_min)
        {
            setCurrent();
        }

        std::set<bddvar> operator*() const
        {
            return current_;
        }

        /* An input iterator must support it->m as well as (*it).m. The
           pointer stays valid until the iterator is advanced. */
        const std::set<bddvar>* operator->() const
        {
            return &current_;
        }

        WeightIterator& operator++()
        {
            f_ -= getSingleSet(current_);
            setCurrent();
            return *this;
        }

        WeightIterator operator++(int)
        {
            WeightIterator it(*this);
            operator++();
            return it;
        }

        bool operator==(const WeightIterator& it) const
        {
            /* The end iterator holds the empty family, and an iterator
               that has enumerated every set becomes equal to it, so
               comparing f_ alone decides once either side is the end.
               Two other iterators enumerate the same sets in the same
               order only when the weights and the direction agree as
               well; without them weight_min_begin(w) and
               weight_max_begin(w) of the same index compared equal. */
            if (f_ == ZBDD(0) || it.f_ == ZBDD(0)) {
                return f_ == it.f_;
            }
            return f_ == it.f_ && is_min_ == it.is_min_
                    && weights_ == it.weights_;
        }

        bool operator!=(const WeightIterator& it) const
        {
            return !(operator==(it));
        }
    };

#if __cplusplus >= 201703L
    class RandomIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::set<bddvar>;
        using difference_type = std::ptrdiff_t;
        /* operator* returns a value, not a reference to an element. */
        using pointer = const std::set<bddvar>*;
        using reference = std::set<bddvar>;
    private:
#else
    class RandomIterator : public std::iterator<std::input_iterator_tag, std::set<bddvar>,
                                    std::ptrdiff_t, const std::set<bddvar>*,
                                    std::set<bddvar> >
    {
    private:
#endif
        ZBDD f_;
        std::set<bddvar> current_;
        ullint rand_seed_;

        void setCurrent()
        {
            DDIndex<int> current_index(f_);
            current_ = current_index.sampleRandomlyA(&rand_seed_);
        }

    public:
        RandomIterator() : f_(ZBDD(0)),
                rand_seed_(0) { }

        RandomIterator(const ZBDD& f,
                ullint rand_seed) : f_(f),
                rand_seed_(rand_seed)
        {
            setCurrent();
        }

        std::set<bddvar> operator*() const
        {
            return current_;
        }

        /* see WeightIterator::operator-> */
        const std::set<bddvar>* operator->() const
        {
            return &current_;
        }

        RandomIterator& operator++()
        {
            f_ -= getSingleSet(current_);
            setCurrent();
            return *this;
        }

        RandomIterator operator++(int)
        {
            RandomIterator it(*this);
            operator++();
            return it;
        }

        bool operator==(const RandomIterator& it) const
        {
            /* see the comment of WeightIterator::operator==; the state
               of the random generator takes the place of the weights,
               so that two iterators of the same family that go on with
               different sequences do not compare equal */
            if (f_ == ZBDD(0) || it.f_ == ZBDD(0)) {
                return f_ == it.f_;
            }
            return f_ == it.f_ && rand_seed_ == it.rand_seed_;
        }

        bool operator!=(const RandomIterator& it) const
        {
            return !(operator==(it));
        }
    };

#if __cplusplus >= 201703L
    class DictIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::set<bddvar>;
        using difference_type = std::ptrdiff_t;
        /* operator* returns a value, not a reference to an element. */
        using pointer = const std::set<bddvar>*;
        using reference = std::set<bddvar>;
    private:
#else
    class DictIterator : public std::iterator<std::input_iterator_tag, std::set<bddvar>,
                                    std::ptrdiff_t, const std::set<bddvar>*,
                                    std::set<bddvar> >
    {
    private:
#endif
        DDIndex<T>* dd_index_;
        count_t card_;
        bool reverse_;
        count_t current_;
        /* operator* computes the set on each call; operator-> needs an
           object to point to, so it keeps the last one here. */
        mutable std::set<bddvar> arrow_set_;

    public:
        /* The end iterators are made with this constructor, which
           records the direction as well so that the end of the forward
           order and the end of the reverse order are distinguishable
           (dict_begin() and dict_rend() both hold the order number 0).
           The default value keeps the calls that pass only an order
           number valid. */
        DictIterator(count_t current, bool reverse = false) :
            dd_index_(NULL),
            card_(0),
            reverse_(reverse),
            current_(current)
        { }

        DictIterator(DDIndex<T>* dd_index, bool reverse) :
            dd_index_(dd_index),
            card_(dd_index->count_v()),
            reverse_(reverse),
            current_(reverse ? card_ : 0)
        { }

        std::set<bddvar> operator*() const
        {
            count_t order = (reverse_ ? current_ - sbddh_getOne<count_t>() : current_);
            return dd_index_->getSetByOrder(order);
        }

        /* see WeightIterator::operator-> */
        const std::set<bddvar>* operator->() const
        {
            arrow_set_ = operator*();
            return &arrow_set_;
        }

        DictIterator& operator++()
        {
            if (reverse_) {
                --current_;
            } else {
                ++current_;
            }
            return *this;
        }

        DictIterator operator++(int)
        {
            DictIterator it(*this);
            operator++();
            return it;
        }

        bool operator==(const DictIterator& it) const
        {
            return current_ == it.current_ && reverse_ == it.reverse_;
        }

        bool operator!=(const DictIterator& it) const
        {
            return !(operator==(it));
        }
    };

    DDNodeIterator begin() const
    {
        return DDNodeIterator(const_cast<DDIndex*>(this), false);
    }

    DDNodeIterator end() const
    {
        return DDNodeIterator(const_cast<DDIndex*>(this), true);
    }

    WeightIterator weight_min_begin(const std::vector<llint>& weights) const
    {
        if (node_index_ == NULL) {
            return WeightIterator();
        }
        checkZBDD();
        return WeightIterator(ZBDD_ID(bddcopy(node_index_->f)),
            weights, true);
    }

    WeightIterator weight_min_end() const
    {
        return WeightIterator();
    }

    WeightIterator weight_max_begin(const std::vector<llint>& weights) const
    {
        if (node_index_ == NULL) {
            return WeightIterator();
        }
        checkZBDD();
        return WeightIterator(ZBDD_ID(bddcopy(node_index_->f)),
            weights, false);
    }

    WeightIterator weight_max_end() const
    {
        return WeightIterator();
    }

    RandomIterator random_begin(ullint rand_seed = 1) const
    {
        if (node_index_ == NULL) {
            return RandomIterator();
        }
        checkZBDD();
        return RandomIterator(ZBDD_ID(bddcopy(node_index_->f)), rand_seed);
    }

    RandomIterator random_end() const
    {
        return RandomIterator();
    }

    DictIterator dict_begin()
    {
        if (node_index_ == NULL) {
            return DictIterator(count_t(0));
        }
        makeCountIndex();
        checkCountExact();
        return DictIterator(this, false);
    }

    DictIterator dict_end()
    {
        if (node_index_ == NULL) {
            return DictIterator(count_t(0));
        }
        makeCountIndex();
        checkCountExact();
        return DictIterator(count_v());
    }

    DictIterator dict_rbegin()
    {
        if (node_index_ == NULL) {
            return DictIterator(count_t(0), true);
        }
        makeCountIndex();
        checkCountExact();
        return DictIterator(this, true);
    }

    DictIterator dict_rend()
    {
        return DictIterator(count_t(0), true);
    }
};

#ifdef SBDDH_GMP

template<>
inline
mpz_class sbddh_getCard<mpz_class>(const ZBDD& f)
{
    DDIndex<int> dd_index(f);
    return dd_index.countMP();
}

#endif /* SBDDH_GMP */

template<>
inline
ullint sbddh_getCard<ullint>(const ZBDD& f)
{
    /* ZBDD::Card, that is bddcard, saturates at bddnull, which is
       2^39 - 1 in the 64-bit build, so it must not be used where the
       exact cardinality matters. The count table of DDIndex is exact
       over the whole range of ullint. */
    DDIndex<int> dd_index(f);
    return dd_index.count();
}

#endif /* __cplusplus */


typedef struct tagbddNodeIterator {
    bddNodeIndex* node_index;
    size_t pos;
    llint level;
} bddNodeIterator;

sbddextended_INLINE_FUNC
bddNodeIterator* bddNodeIterator_make(bddNodeIndex* node_index)
{
    bddNodeIterator* itor;

    itor = (bddNodeIterator*)malloc(sizeof(bddNodeIterator));
    if (itor == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    itor->node_index = node_index;
    itor->pos = 0;
    itor->level = itor->node_index->height;
    return itor;
}

/* Frees the iterator made by bddNodeIterator_make. The node index the */
/* iterator was made from is not owned by the iterator and is not freed. */
sbddextended_INLINE_FUNC
void bddNodeIterator_destruct(bddNodeIterator* itor)
{
    free(itor);
}

sbddextended_INLINE_FUNC
int bddNodeIterator_hasNext(const bddNodeIterator* itor)
{
    return (itor->level > 0 ? 1 : 0);
}

/* The returned bddp is a borrowed reference stored in the node index: */
/* its reference counter is not incremented, so the caller must not */
/* release it with bddfree, and must bddcopy it to keep it longer than */
/* the iterator, the index and the DD the index was built from (like */
/* the bddgetchild* functions). */
sbddextended_INLINE_FUNC
bddp bddNodeIterator_next(bddNodeIterator* itor)
{
    bddp f;

    if (itor->level <= 0) {
        return bddfalse;
    }

    f = (bddp)sbddextended_MyVector_get(&itor->node_index->level_vec_arr[itor->level],
                                        (llint)itor->pos);
    ++itor->pos;

    while (itor->level > 0 && itor->pos >= itor->node_index->level_vec_arr[itor->level].count) {
        itor->pos = 0;
        --itor->level;
    }
    return f;
}

/* *************************** C++ version start ***************************** */

#ifdef __cplusplus


#endif /* __cplusplus */

typedef struct tagbddElementIterator {
    int sp;
    bddp* bddnode_stack;
    char* op_stack;
} bddElementIterator;

/* itor may be NULL, in which case this function does nothing (like */
/* free and bddNodeIterator_destruct). */
sbddextended_INLINE_FUNC
void bddElementIterator_destruct(bddElementIterator* itor)
{
    if (itor == NULL) {
        return;
    }
    free(itor->op_stack);
    free(itor->bddnode_stack);
    free(itor);
}

sbddextended_INLINE_FUNC
int bddElementIterator_hasNext(const bddElementIterator* itor)
{
    return (itor->sp >= 0 ? 1 : 0);
}

sbddextended_INLINE_FUNC
void bddElementIterator_proceed(bddElementIterator* itor)
{
    char op;
    bddp node, child;

#ifdef __cplusplus
    /* A child access can throw (e.g. with SAPPOROBDD++) in the middle
       of a step of the walk, from where there is no state to go on
       with. End the iteration then, so that the iterator is left in a
       defined state instead of returning wrong elements to a caller
       that catches the exception. */
    try {
#endif
    while (itor->sp >= 0) {
        node = itor->bddnode_stack[itor->sp];
        op = itor->op_stack[itor->sp];
        if (node == bddempty || op == 2) {
            --(itor->sp);
            if (itor->sp >= 0) {
                ++itor->op_stack[itor->sp];
            }
        } else if (node == bddsingle) {
            break;
        } else {
            if (op == 0) {
                child = bddgetchild1z(node);
            } else {
                child = bddgetchild0z(node);
            }
            /* SAPPOROBDD returns bddnull when it cannot compute the
               child (out of memory). Pushing it would make the next
               round take the children of an invalid node id, so end
               the iteration instead. */
            if (child == bddnull) {
                fprintf(stderr, "The element iterator cannot obtain a "
                        "child (out of memory?).\n");
                itor->sp = -1;
                return;
            }
            ++(itor->sp);
            itor->bddnode_stack[itor->sp] = child;
            itor->op_stack[itor->sp] = 0;
        }
    }
#ifdef __cplusplus
    } catch (...) {
        itor->sp = -1;
        throw;
    }
#endif
}

sbddextended_INLINE_FUNC
bddElementIterator* bddElementIterator_make(bddp f)
{
    int height;
    bddElementIterator* itor;

    itor = (bddElementIterator*)malloc(sizeof(bddElementIterator));
    if (itor == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    if (f == bddnull || f == bddempty) {
        itor->sp = -1;
        itor->bddnode_stack = NULL;
        itor->op_stack = NULL;
        return itor;
    }

    /* The iterator takes the children with the ZDD rule, which is invalid
       for a BDD. Return an iterator pointing at the end instead of
       letting SAPPOROBDD abort with an obscure message. */
    if (!bddiszbdd(f)) {
        fprintf(stderr, "The element iterator supports only ZDD, "
                "but a BDD is given.\n");
        itor->sp = -1;
        itor->bddnode_stack = NULL;
        itor->op_stack = NULL;
        return itor;
    }

    height = (int)bddgetlev(f) + 1;
    itor->bddnode_stack = (bddp*)malloc((size_t)height * sizeof(bddp));
    if (itor->bddnode_stack == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    itor->op_stack = (char*)malloc((size_t)height * sizeof(char));
    if (itor->op_stack == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    itor->sp = 0;
    itor->bddnode_stack[itor->sp] = f;
    itor->op_stack[itor->sp] = 0;

#ifdef __cplusplus
    /* Release the iterator when a child access throws (e.g. with
       SAPPOROBDD++); the caller never receives it and could not free
       it itself. */
    try {
#endif
    bddElementIterator_proceed(itor);
#ifdef __cplusplus
    } catch (...) {
        bddElementIterator_destruct(itor);
        throw;
    }
#endif

    return itor;
}

/* Stores the element the iterator currently points at into arr as a */
/* sequence of variable numbers terminated by (bddvar)-1. Since an */
/* element consists of at most bddgetlev(f) variables, where f is the */
/* ZBDD passed to bddElementIterator_make, arr must have room for at */
/* least bddgetlev(f) + 1 elements (the variables and the terminator). */
sbddextended_INLINE_FUNC
void bddElementIterator_getValue(bddElementIterator* itor, bddvar* arr)
{
    int i, c;

    c = 0;
    for (i = 0; i < itor->sp; ++i) {
        if (itor->op_stack[i] == 0) {
            arr[c] = bddgetvar(itor->bddnode_stack[i]);
            ++c;
        }
    }
    arr[c] = (bddvar)-1;
}

/* Stores the current element into arr and proceeds to the next element. */
/* arr may be NULL, in which case the current element is discarded. */
/* Otherwise arr must be as large as bddElementIterator_getValue requires, */
/* that is, at least bddgetlev(f) + 1 elements. */
sbddextended_INLINE_FUNC
void bddElementIterator_next(bddElementIterator* itor, bddvar* arr)
{
    if (itor->sp < 0) { /* The iterator has already reached the end. */
        return; /* This is a complete no-op: arr is not written either. */
    }
    if (arr != NULL) {
        bddElementIterator_getValue(itor, arr);
    }
    --itor->sp;
    if (itor->sp >= 0) {
        ++itor->op_stack[itor->sp];
        bddElementIterator_proceed(itor);
    }
}

#ifdef __cplusplus

#if __cplusplus >= 201703L
class ElementIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::set<bddvar>;
    using difference_type = std::ptrdiff_t;
    /* operator* returns a value, not a reference to an element. */
    using pointer = const std::set<bddvar>*;
    using reference = std::set<bddvar>;
private:
#else
class ElementIterator
    : public std::iterator<std::input_iterator_tag, std::set<bddvar>,
                           std::ptrdiff_t, const std::set<bddvar>*,
                           std::set<bddvar> > {
private:
#endif
    int sp_;
    std::vector<bddp> bddnode_stack_;
    std::vector<char> op_stack_;
    mutable std::set<bddvar> buffSet_;

    /* This function is written by copy-paste of void */
    /* bddElementIterator_proceed(bddElementIterator* itor). */
    void proceed()
    {
        char op;
        bddp node, child;

        while (sp_ >= 0) {
            node = bddnode_stack_[sp_];
            op = op_stack_[sp_];
            if (node == bddempty || op == 2) {
                --sp_;
                if (sp_ >= 0) {
                    ++op_stack_[sp_];
                }
            } else if (node == bddsingle) {
                break;
            } else {
                if (op == 0) {
                    child = bddgetchild1z(node);
                } else {
                    child = bddgetchild0z(node);
                }
                /* see the comment of bddElementIterator_proceed */
                if (child == bddnull) {
                    std::cerr << "The element iterator cannot obtain a "
                                 "child (out of memory?)." << std::endl;
                    sp_ = -1;
                    return;
                }
                ++sp_;
                bddnode_stack_[sp_] = child;
                op_stack_[sp_] = 0;
            }
        }
    }

    void setToBuff()
    {
        buffSet_.clear();
        if (sp_ >= 0) {
            for (int i = 0; i < sp_; ++i) {
                if (op_stack_[i] == 0) {
                    buffSet_.insert(bddgetvar(bddnode_stack_[i]));
                }
            }
        }
    }

public:
    /* f is stored without taking a reference of its own (bddcopy is not
       called), so the ZBDD it comes from must be kept alive while the
       iterator is used. */
    ElementIterator(bddp f, bool is_end)
    {
        if (is_end || f == bddnull || f == bddempty) {
            sp_ = -1;
        } else if (!bddiszbdd(f)) {
            /* The iterator takes the children with the ZDD rule, which is
               invalid for a BDD. Point at the end instead of letting
               SAPPOROBDD abort with an obscure message. */
            std::cerr << "The element iterator supports only ZDD, "
                         "but a BDD is given." << std::endl;
            sp_ = -1;
        } else {
            sp_ = 0;
            int height = (int)bddgetlev(f) + 1;
            bddnode_stack_.resize(height);
            op_stack_.resize(height);

            bddnode_stack_[sp_] = f;
            op_stack_[sp_] = 0;

            proceed();

            setToBuff();
        }
    }

    std::set<bddvar> operator*() const
    {
        return buffSet_;
    }

    const std::set<bddvar>* operator->() const
    {
        return &buffSet_;
    }

    ElementIterator& operator++()
    {
        if (sp_ < 0) { /* The iterator has already reached the end. */
            return *this;
        }
        /* A child access can throw (e.g. with SAPPOROBDD++) in the
           middle of the step, from where there is no state to go on
           with. End the iteration then, so that a caller that catches
           the exception does not go on with an iterator that would
           skip or repeat elements. */
        try {
            --sp_;
            if (sp_ >= 0) {
                ++op_stack_[sp_];
                proceed();
            }
        } catch (...) {
            sp_ = -1;
            setToBuff();
            throw;
        }
        /* setToBuff clears the buffer when sp_ becomes negative, that is, */
        /* when the iterator reaches the end by this increment. */
        setToBuff();
        return *this;
    }

    ElementIterator operator++(int)
    {
        ElementIterator it(*this);
        operator++();
        return it;
    }

    bool operator==(const ElementIterator& it) const
    {
        if (sp_ >= 0) {
            if (sp_ != it.sp_) {
                return false;
            }
            for (int i = 0; i < sp_; ++i) {
                if (bddnode_stack_[i] != it.bddnode_stack_[i]) {
                    return false;
                }
                if (op_stack_[i] != it.op_stack_[i]) {
                    return false;
                }
            }
            return true;
        } else {
            return it.sp_ < 0;
        }
    }

    bool operator!=(const ElementIterator& it) const
    {
        return !(operator==(it));
    }
};

/* The holder owns a reference to the ZBDD (both constructors take a
   reference of their own), so passing a temporary ZBDD is safe: the
   holder keeps the underlying nodes alive while it exists. The
   iterators returned by begin()/end() still borrow the nodes, so they
   must not be used after the holder is destroyed. */
class ElementIteratorHolder {
private:
    ZBDD f_;
public:
    ElementIteratorHolder(bddp f) : f_(ZBDD_ID(bddcopy(f))) { }

    ElementIteratorHolder(const ZBDD& f) : f_(f) { }

    ElementIterator begin() const {
        return ElementIterator(f_.GetID(), false);
    }

    ElementIterator end() const {
        return ElementIterator(f_.GetID(), true);
    }
};

#endif

/* *************** import functions */

sbddextended_INLINE_FUNC
bddp bddconstructzbddfromelements_inner(FILE* fp
#ifdef __cplusplus
                    , ReadCharObject& sbddextended_readChar
#endif
                                        )
{
    bddp p, q, r;
    bddvar* vararr;
    bddvar* new_vararr;
    /* size_t, not int: a single line may hold more than INT_MAX */
    /* numbers, and doubling an int size would be signed overflow */
    size_t vararr_pos, vararr_size;
    int c, prev_c, v, is_v_too_large;
    int mode, first;

    vararr_size = sbddextended_BUFSIZE;
    vararr = (bddvar*)malloc(vararr_size * sizeof(bddvar));
    if (vararr == NULL) {
        /* Report the failure as an error of this import instead of
           ending the process of the library user: the array grows with
           the numbers on a line of the input, so a line of a hostile
           input can ask for a buffer that no memory can hold. */
        fprintf(stderr, "Cannot allocate memory for the numbers in the "
                "elements format.\n");
        return bddnull;
    }
    vararr_pos = 0;

    p = bddfalse;
    q = bddnull;
    first = 1;

    mode = 0; /* 0: skip ws, 1: reading nums */
    v = 0;
    is_v_too_large = 0;
    c = 0;
#ifdef __cplusplus
    /* The reader throws when the caller gave an istream with exceptions
       enabled, and the DD operations can throw with SAPPOROBDD++, so
       release the working array and the references owned at that moment
       before letting an exception propagate. */
    try {
#endif
    while (c != -1) {
        prev_c = c;
        c = sbddextended_readChar(fp);
        if (c == -2) { /* a read error; a message has been printed */
            free(vararr);
            bddfree(p);
            return bddnull;
        }
        if (first == 1 && c != -1) {
            if (c == '\n') {
                /* An empty first line denotes the empty set, so it is */
                /* not whitespace before a terminal marker. */
                first = 0;
            } else if (isspace(c)) {
                /* Leading spaces are skipped like the spaces between */
                /* the numbers below, so a marker after them is still */
                /* the first character of the input. */
            } else {
                first = 0;
                if (c == 'T' || c == 'B' || c == 'E' || c == 'F'
                        || c == 'N') {
                    if (c == 'T') {
                        p = bddtrue;
                    } else if (c == 'N') {
                        /* the exporter writes "N" for bddnull, so */
                        /* accept it for the round trip */
                        p = bddnull;
                    }
                    /* The marker stands for the whole input, so only */
                    /* whitespace may follow it. Otherwise a mistyped */
                    /* input would be taken for a terminal and the rest */
                    /* of it would be dropped silently. */
                    for (;;) {
                        c = sbddextended_readChar(fp);
                        if (c == -2) { /* a read error */
                            free(vararr);
                            bddfree(p);
                            return bddnull;
                        }
                        if (c == -1) {
                            break;
                        }
                        if (!isspace(c)) {
                            fprintf(stderr, "The character 0x%02x is not "
                                    "allowed after a terminal symbol in "
                                    "the elements format.\n",
                                    (unsigned int)c);
                            free(vararr);
                            bddfree(p);
                            return bddnull;
                        }
                    }
                    break;
                }
            }
        }
        if (c != -1) {
            if (!isdigit(c) && c != '\n' && !isspace(c)) {
                /* A character that the format does not allow is a format */
                /* error like an out-of-range variable number below, so */
                /* report it by returning bddnull instead of terminating */
                /* the process of the library user. */
                fprintf(stderr, "The character 0x%02x is not allowed in "
                        "the elements format.\n", (unsigned int)c);
                free(vararr);
                bddfree(p);
                return bddnull;
            }
            if (mode == 0) {
                if (isdigit(c)) {
                    v = c - '0';
                    is_v_too_large = 0;
                    mode = 1;
                }
            } else if (mode == 1) {
                if (isdigit(c)) {
                    /* Since the overflow of a signed integer is undefined */
                    /* behavior, we stop increasing v and remember that the */
                    /* number in the input does not fit in v. */
                    if (v > (INT_MAX - (c - '0')) / 10) {
                        is_v_too_large = 1;
                    } else {
                        v = 10 * v + (c - '0');
                    }
                }
            }
        }
        if ((c == -1 || c == '\n' || isspace(c)) && mode == 1) {
            /* The variable number must be in {1,...,bddvarused()}. */
            /* Otherwise bddgetsingleset below causes an error. */
            if (is_v_too_large != 0 || !(1 <= v && v <= (int)bddvarused())) {
                if (is_v_too_large != 0) { /* v does not hold the number */
                    fprintf(stderr, "The variable number is out of range "
                            "{1,...,%d}.\n", (int)bddvarused());
                } else {
                    fprintf(stderr, "The variable number %d is out of range "
                            "{1,...,%d}.\n", v, (int)bddvarused());
                }
                free(vararr);
                bddfree(p);
                return bddnull;
            }
            if (vararr_pos >= vararr_size) {
                /* bddgetsingleset below takes the count as an int */
                if (vararr_size > (size_t)INT_MAX / 2) {
                    fprintf(stderr, "A line must not hold more than "
                            "%d numbers.\n", INT_MAX);
                    free(vararr);
                    bddfree(p);
                    return bddnull;
                }
                vararr_size *= 2;
                /* keep the old pointer so that it can be released when
                   realloc fails (see the comment of the first
                   allocation above) */
                new_vararr = (bddvar*)realloc(vararr,
                                        vararr_size * sizeof(bddvar));
                if (new_vararr == NULL) {
                    fprintf(stderr, "Cannot allocate memory for the "
                            "numbers in the elements format.\n");
                    free(vararr);
                    bddfree(p);
                    return bddnull;
                }
                vararr = new_vararr;
            }
            vararr[vararr_pos] = (bddvar)v;
            ++vararr_pos;
            mode = 0;
        }
        if ((c == -1 && prev_c != '\n') || c == '\n') {
            q = bddgetsingleset(vararr, (int)vararr_pos);
            r = bddunion(p, q);
            bddfree(p);
            bddfree(q);
            q = bddnull; /* so that the catch below frees q only while
                            this function owns it */
            p = r;
            vararr_pos = 0;
        }
    }
#ifdef __cplusplus
    } catch (...) {
        free(vararr);
        bddfree(q);
        bddfree(p);
        throw;
    }
#endif
    free(vararr);
    return p;
}

#ifdef __cplusplus

sbddextended_INLINE_FUNC
ZBDD constructZBDDFromElements(FILE* fp)
{
    ReadLineObject glo;
    bddp p;
    p = bddconstructzbddfromelements_inner(fp, glo);
    return ZBDD_ID(p);
}

sbddextended_INLINE_FUNC
ZBDD constructZBDDFromElements(std::istream& ist)
{
    ReadLineObject glo(&ist);
    bddp p;
    p = bddconstructzbddfromelements_inner(NULL, glo);
    return ZBDD_ID(p);
}

sbddextended_INLINE_FUNC
bddp bddconstructzbddfromelements(FILE* fp)
{
    ReadLineObject glo;
    return bddconstructzbddfromelements_inner(fp, glo);
}

sbddextended_INLINE_FUNC
ZBDD constructZBDDFromElements(FILE* /*fp*/, const char* /*large_sep*/,
                                const char* /*small_sep*/)
{
    std::cerr << "Arguments large_sep and small_sep are no longer supported.";
    exit(1);
}

sbddextended_INLINE_FUNC
ZBDD constructZBDDFromElements(std::istream& /*ist*/, const char* /*large_sep*/,
                                const char* /*small_sep*/)
{
    std::cerr << "Arguments large_sep and small_sep are no longer supported.";
    exit(1);
}

sbddextended_INLINE_FUNC
bddp bddconstructzbddfromelements(FILE* /*fp*/, const char* /*large_sep*/,
                                    const char* /*small_sep*/)
{
    std::cerr << "Arguments large_sep and small_sep are no longer supported.";
    exit(1);
}

#else

/* The following function is no longer supported.
sbddextended_INLINE_FUNC
bddp bddconstructzbddfromelements(FILE* fp, const char* large_sep,
                                    const char* small_sep)
{
    return bddconstructzbddfromelements_inner(fp, large_sep, small_sep);
}*/

sbddextended_INLINE_FUNC
bddp bddconstructzbddfromelements(FILE* fp)
{
    return bddconstructzbddfromelements_inner(fp);
}

#endif


/* *************** export functions */


/* num_of_variables: 0 -> elements format, non 0 -> value list format */
/* var_name_map_size: the number of elements of var_name_map, or a */
/* negative value when the caller does not know it (the C API takes a */
/* bare array, whose size the caller is responsible for) */
/* Writes one string and returns from bddprintzbddelements_inner when
   the write fails, releasing the working arrays (they are NULL until
   they are allocated). Used only there and undefined after it. */
#define sbddextended_writeOrReturn(str) \
    do { \
        if (!sbddextended_write(str, fp)) { \
            sbddextended_printWriteError(); \
            free(value_list); \
            free(op_stack); \
            free(bddnode_stack); \
            return; \
        } \
    } while (0)

sbddextended_INLINE_FUNC
void bddprintzbddelements_inner(FILE* fp, bddp f, const char* delim1,
                                const char* delim2, const char* var_name_map[],
                                llint var_name_map_size,
                                int num_of_variables
#ifdef __cplusplus
                       , const WriteObject& sbddextended_write
#endif
                          )
{
    /* NULL until they are allocated below, so that the macro above
       can release them from anywhere in this function */
    bddp* bddnode_stack = NULL;
    bddp g;
    char* op_stack = NULL;
    char* value_list = NULL;
    char op;
    int i, height, sp, is_first_delim1, is_first_delim2;
    bddvar v;
    bddp node, child;
    char buf[sbddextended_BUFSIZE];

    if (f == bddnull) {
        sbddextended_writeOrReturn("N");
        return;
    } else if (f == bddempty) {
        sbddextended_writeOrReturn("E");
        return;
    } else if (f == bddsingle) {
        if (num_of_variables != 0) {
            for (i = 1; i <= num_of_variables; ++i) {
                sbddextended_writeOrReturn("0");
                if (i < num_of_variables) {
                    sbddextended_writeOrReturn(delim2);
                }
            }
        }
        return;
    }

    is_first_delim1 = 1;
    if (bddisnegative(f)) {
        /* Output bddsingle first */
        if (num_of_variables != 0) {
            for (i = 1; i <= num_of_variables; ++i) {
                sbddextended_writeOrReturn("0");
                if (i < num_of_variables) {
                    sbddextended_writeOrReturn(delim2);
                }
            }
        }
        is_first_delim1 = 0;
        g = bddtakenot(f);
    } else {
        g = f;
    }

    height = (int)bddgetlev(g);
    bddnode_stack = (bddp*)malloc((size_t)(height + 1) * sizeof(bddp));
    if (bddnode_stack == NULL) {
        fprintf(stderr, "out of memory\n");
        return;
    }
    op_stack = (char*)malloc((size_t)(height + 1) * sizeof(char));
    if (op_stack == NULL) {
        fprintf(stderr, "out of memory\n");
        free(bddnode_stack);
        return;
    }
    if (num_of_variables != 0) {
        value_list = (char*)malloc((size_t)(num_of_variables + 1) * sizeof(char));
        if (value_list == NULL) {
            fprintf(stderr, "out of memory\n");
            free(op_stack);
            free(bddnode_stack);
            return;
        }
    }

#ifdef __cplusplus
    /* The write callback throws instead of returning false when the
       ostream has exceptions enabled, so release the working arrays
       before letting it propagate. */
    try {
#endif
    sp = 0;
    bddnode_stack[sp] = g;
    op_stack[sp] = 0;

    while (sp >= 0) {
        node = bddnode_stack[sp];
        op = op_stack[sp];
        if (node == bddempty) {
            --sp;
            if (sp < 0) {
                break;
            }
            ++op_stack[sp];
        } else if (node == bddsingle) {
            if (is_first_delim1 != 0) {
                is_first_delim1 = 0;
            } else {
                sbddextended_writeOrReturn(delim1);
            }
            is_first_delim2 = 1;
            if (num_of_variables != 0) {
                for (i = 1; i <= num_of_variables; ++i) {
                    value_list[i] = 0;
                }
                for (i = 0; i < sp; ++i) {
                    if (op_stack[i] == 0) {
                        v = bddgetvar(bddnode_stack[i]);
                        if (! (1 <= v && v <= (bddvar)num_of_variables)) {
                            fprintf(stderr, "variable number is "
                                "not in {1,...,num_of_variables} \n");
                            free(value_list);
                            free(op_stack);
                            free(bddnode_stack);
                            return;
                        }
                        value_list[v] = 1;
                    }
                }
                for (i = 1; i <= num_of_variables; ++i) {
                    sbddextended_snprintf1(buf, sbddextended_BUFSIZE, "%d",
                                           (int)value_list[i]);
                    sbddextended_writeOrReturn(buf);
                    if (i < num_of_variables) {
                        sbddextended_writeOrReturn(delim2);
                    }
                }
            } else {
                for (i = 0; i < sp; ++i) {
                    if (op_stack[i] == 0) {
                        if (is_first_delim2 != 0) {
                            is_first_delim2 = 0;
                        } else {
                            sbddextended_writeOrReturn(delim2);
                        }
                        v = bddgetvar(bddnode_stack[i]);
                        if (var_name_map != NULL) {
                            /* var_name_map is indexed by the variable */
                            /* number, so it must have an entry for every */
                            /* variable of f. */
                            if (var_name_map_size >= 0
                                    && (llint)v >= var_name_map_size) {
                                fprintf(stderr, "var_name_map has no name "
                                        "for the variable %u\n", v);
                                free(op_stack);
                                free(bddnode_stack);
                                return;
                            }
                            sbddextended_writeOrReturn(var_name_map[v]);
                        } else {
                            sbddextended_snprintf1(buf, sbddextended_BUFSIZE,
                                "%u", v);
                            sbddextended_writeOrReturn(buf);
                        }
                    }
                }
            }

            --sp;
            if (sp < 0) {
                break;
            }
            ++op_stack[sp];
        } else {
            if (op <= 1) {
                if (op == 0) {
                    child = bddgetchild1z(node);
                } else {
                    child = bddgetchild0z(node);
                }
                ++sp;
                bddnode_stack[sp] = child;
                op_stack[sp] = 0;
            } else {
                --sp;
                if (sp < 0) {
                    break;
                }
                ++op_stack[sp];
            }
        }
    }
    if (num_of_variables != 0) {
        free(value_list);
    }
    free(op_stack);
    free(bddnode_stack);
#ifdef __cplusplus
    } catch (...) {
        free(value_list);
        free(op_stack);
        free(bddnode_stack);
        throw;
    }
#endif
}

#undef sbddextended_writeOrReturn

#ifdef __cplusplus

sbddextended_INLINE_FUNC
void printZBDDElements(FILE* fp, const ZBDD& zbdd,
    const std::string& delim1, const std::string& delim2)
{
    WriteObject wo(false, false, NULL);
    bddprintzbddelements_inner(fp, zbdd.GetID(), delim1.c_str(),
        delim2.c_str(), NULL, -1, 0, wo);
}

sbddextended_INLINE_FUNC
void printZBDDElements(FILE* fp, const ZBDD& zbdd, const std::string& delim1,
    const std::string& delim2, const std::vector<std::string>& var_name_map)
{
    WriteObject wo(false, false, NULL);
    std::vector<const char*> arr = sbddextended_strVectorToArray(var_name_map);
    bddprintzbddelements_inner(fp, zbdd.GetID(), delim1.c_str(),
        delim2.c_str(), &arr[0], (llint)var_name_map.size(), 0, wo);
}

sbddextended_INLINE_FUNC
void printZBDDElements(std::ostream& ost, const ZBDD& zbdd,
    const std::string& delim1, const std::string& delim2)
{
    WriteObject wo(true, false, &ost);
    bddprintzbddelements_inner(NULL, zbdd.GetID(), delim1.c_str(),
    delim2.c_str(), NULL, -1, 0, wo);
}

sbddextended_INLINE_FUNC
void printZBDDElements(std::ostream& ost, const ZBDD& zbdd,
    const std::string& delim1, const std::string& delim2,
    const std::vector<std::string>& var_name_map)
{
    WriteObject wo(true, false, &ost);
    std::vector<const char*> arr = sbddextended_strVectorToArray(var_name_map);
    bddprintzbddelements_inner(NULL, zbdd.GetID(), delim1.c_str(),
        delim2.c_str(), &arr[0], (llint)var_name_map.size(), 0, wo);
}

/* The overloads without delimiters wrap the sets in braces. bddnull
   and the empty family are written as the marker "N" and "E" instead
   of as a list of sets, and wrapping those in braces would make them
   look like a family that holds one set whose only element is named N
   or E, so print them alone, as ZStr does. */
sbddextended_INLINE_FUNC
void printZBDDElements(FILE* fp, const ZBDD& zbdd)
{
    if (zbdd == ZBDD(-1) || zbdd == ZBDD(0)) {
        printZBDDElements(fp, zbdd, "}, {", ", ");
        return;
    }
    if (fprintf(fp, "{") < 0) {
        sbddextended_printWriteError();
        return;
    }
    printZBDDElements(fp, zbdd, "}, {", ", ");
    if (fprintf(fp, "}") < 0) {
        sbddextended_printWriteError();
    }
}

sbddextended_INLINE_FUNC
void printZBDDElements(std::ostream& ost, const ZBDD& zbdd)
{
    if (zbdd == ZBDD(-1) || zbdd == ZBDD(0)) {
        printZBDDElements(ost, zbdd, "}, {", ", ");
        return;
    }
    if (!(ost << "{")) {
        sbddextended_printWriteError();
        return;
    }
    printZBDDElements(ost, zbdd, "}, {", ", ");
    if (!(ost << "}")) {
        sbddextended_printWriteError();
    }
}

sbddextended_INLINE_FUNC
void bddprintzbddelements(FILE* fp, bddp f, const char* delim1,
    const char* delim2)
{
    WriteObject wo(false, false, NULL);
    bddprintzbddelements_inner(fp, f, delim1, delim2, NULL, -1, 0, wo);
}

sbddextended_INLINE_FUNC
void bddprintzbddelementswithmap(FILE* fp, bddp f, const char* delim1,
    const char* delim2, const char* var_name_map[])
{
    WriteObject wo(false, false, NULL);
    bddprintzbddelements_inner(fp, f, delim1, delim2, var_name_map, -1, 0,
                               wo);
}

#else

sbddextended_INLINE_FUNC
void bddprintzbddelements(FILE* fp, bddp f, const char* delim1,
    const char* delim2)
{
    bddprintzbddelements_inner(fp, f, delim1, delim2, NULL, -1, 0);
}

sbddextended_INLINE_FUNC
void bddprintzbddelementswithmap(FILE* fp, bddp f, const char* delim1,
    const char* delim2, const char* var_name_map[])
{
    bddprintzbddelements_inner(fp, f, delim1, delim2, var_name_map, -1, 0);
}

#endif


#ifdef __cplusplus

sbddextended_INLINE_FUNC
void printZBDDElementsAsValueList(FILE* fp, const ZBDD& zbdd, const std::string& delim1, const std::string& delim2, int num_of_variables)
{
    /* num_of_variables == 0 makes the inner function use the elements */
    /* format instead of the value list format, and a value larger than */
    /* the largest level of SAPPOROBDD cannot be the number of variables */
    /* of a DD and would make num_of_variables + 1 overflow below. */
    if (!(1 <= num_of_variables
            && num_of_variables <= sbddextended_varMaxAsInt())) {
        fprintf(stderr, "num_of_variables must be in {1,...,%d}\n",
                sbddextended_varMaxAsInt());
        return;
    }

    WriteObject wo(false, false, NULL);
    bddprintzbddelements_inner(fp, zbdd.GetID(), delim1.c_str(),
                               delim2.c_str(), NULL, -1, num_of_variables, wo);
}

sbddextended_INLINE_FUNC
void printZBDDElementsAsValueList(std::ostream& ost, const ZBDD& zbdd, const std::string& delim1, const std::string& delim2, int num_of_variables)
{
    /* See the comment of the FILE* overload above. */
    if (!(1 <= num_of_variables
            && num_of_variables <= sbddextended_varMaxAsInt())) {
        std::cerr << "num_of_variables must be in {1,...,"
                  << sbddextended_varMaxAsInt() << "}" << std::endl;
        return;
    }

    WriteObject wo(true, false, &ost);
    bddprintzbddelements_inner(NULL, zbdd.GetID(), delim1.c_str(),
                               delim2.c_str(), NULL, -1, num_of_variables, wo);
}

sbddextended_INLINE_FUNC
std::string ZStr(const ZBDD& zbdd)
{
    /* printZBDDElements writes "N" for bddnull and "E" for the empty
       family, both without the braces (see its comment). */
    std::ostringstream ost;
    printZBDDElements(ost, zbdd);
    return ost.str();
}

sbddextended_INLINE_FUNC
std::string zstr(const ZBDD& zbdd)
{
    return ZStr(zbdd);
}

#endif

/* *************** import functions */

/* Note on the byte order: each multi-byte value of the BDD binary format */
/* is read and written in little endian with a fixed width (16/32/64 bits) */
/* regardless of the byte order and the type sizes of the machine. */
/* See sbddextended_bytesToUint16 in readLine.h and */
/* sbddextended_uint16ToBytes in writeLine.h. */

/* Reads a value of the BDD binary format and returns bddnull from the */
/* caller when the binary ends before the value is read. */
/* Used only in bddimportbddasbinary_inner and undefined after it. */
#define sbddextended_readOrReturnNull(read_func, value_ptr) \
    do { \
        if (!read_func(value_ptr, fp)) { \
            fprintf(stderr, "Unexpected end of the BDD binary.\n"); \
            return bddnull; \
        } \
    } while (0)

/* Frees the working buffers of the validation pass of */
/* bddimportbddasbinary_inner and returns bddnull from it. */
/* Used only there and undefined after it. */
#define sbddextended_freeBuffersAndReturnNull() \
    do { \
        free(child_buf); \
        free(bddnode_buf); \
        sbddextended_MyVector_deinitialize(&level_vec); \
        return bddnull; \
    } while (0)

sbddextended_INLINE_FUNC
bddp bddimportbddasbinary_inner(FILE* fp, int root_level, int is_zbdd
#ifdef __cplusplus
                                , ReadCharObject& sbddextended_readUint8
                                , ReadCharObject& sbddextended_readUint16
                                , ReadCharObject& sbddextended_readUint32
                                , ReadCharObject& sbddextended_readUint64
#endif
                                )
{
    ullint i, level, max_level, root_id, number_of_nodes;
    ullint node_count, node_sum, max_number_of_nodes, level_start_id;
    unsigned int number_of_terminals;
    bddvar var;
    bddp f, f0, f1;
    bddp* bddnode_buf = NULL;
    ullint* child_buf = NULL;
    sbddextended_MyVector level_vec;
    unsigned char use_negative_arcs;
    unsigned char v8;
    unsigned short v16;
    ullint v64;
#ifdef __cplusplus
    /* the ids below this value in bddnode_buf hold a created node that
       the catch at the end must release */
    ullint built_count = 0;
#endif

    /* read head 'B' 'D' 'D' */
    for (i = 0; i < 3; ++i) {
        sbddextended_readOrReturnNull(sbddextended_readUint8, &v8);
        if ((i == 0 && v8 != 'B') || (i >= 1 && v8 != 'D')) {
            fprintf(stderr, "This binary is not in the BDD binary format.\n");
            return bddnull;
        }
    }
    sbddextended_readOrReturnNull(sbddextended_readUint8, &v8); /* version */
    if (v8 != 1) {
        fprintf(stderr, "This function supports only version 1.\n");
        return bddnull;
    }

    sbddextended_readOrReturnNull(sbddextended_readUint8, &v8); /* type */
    /* 1: either BDD or ZDD, 2: BDD, 3: ZDD */
    if (v8 < 1 || v8 > 3) {
        fprintf(stderr, "The DD type %u in the binary is not supported.\n",
                (unsigned int)v8);
        return bddnull;
    }
    if (is_zbdd < 0 && v8 == 1) {
        fprintf(stderr, "Need to specify BDD or ZBDD.\n");
        return bddnull;
    } else if (is_zbdd > 0 && v8 == 2) {
        fprintf(stderr, "The binary indicates that it is a BDD, but we interpret it as a ZBDD.\n");
    } else if (is_zbdd == 0 && v8 == 3) {
        fprintf(stderr, "The binary indicates that it is a ZDD, but we interpret it as a BDD.\n");
    }

    sbddextended_readOrReturnNull(sbddextended_readUint16, &v16); /* number_of_arcs */
    if (v16 != 2) {
        fprintf(stderr, "Currently, this function supports only 2 branches.\n");
        return bddnull;
    }

    /* number_of_terminals */
    sbddextended_readOrReturnNull(sbddextended_readUint32, &number_of_terminals);
    if (number_of_terminals != 2) {
        fprintf(stderr, "Currently, this function supports only 2 terminals.\n");
        return bddnull;
    }

    /* number_of_bits_for_level */
    sbddextended_readOrReturnNull(sbddextended_readUint8, &v8);
    if (v8 != 16) {
        fprintf(stderr, "Currently, this function supports only the case of number_of_bits_for_level == 16.\n");
        return bddnull;
    }

    /* number_of_bits_for_id */
    sbddextended_readOrReturnNull(sbddextended_readUint8, &v8);
    if (v8 != 64) {
        fprintf(stderr, "Currently, this function supports only the case of number_of_bits_for_id == 64.\n");
        return bddnull;
    }

    /* use_negative_arcs */
    sbddextended_readOrReturnNull(sbddextended_readUint8, &use_negative_arcs);
    if (use_negative_arcs > 1) {
        fprintf(stderr, "The value %u of \"use_negative_arcs\" in the binary "
                "is not supported.\n", (unsigned int)use_negative_arcs);
        return bddnull;
    }
    /* max_level */
    sbddextended_readOrReturnNull(sbddextended_readUint64, &max_level);

    if (max_level > (ullint)INT_MAX) {
        fprintf(stderr, "The value of \"max_level\" in the binary is too large.\n");
        return bddnull;
    }

    if (root_level < 0) {
        root_level = (int)max_level;
    } else if (root_level < (int)max_level) {
        /* This importer is shared by the BDD and the ZBDD versions, */
        /* so the message says "DD". */
        fprintf(stderr, "The argument \"root_level\" must be "
                "at least the height of the DD.\n");
        return bddnull;
    }
    /* The levels are turned into variables with bddnewvar below, which */
    /* terminates the process once the variable index range of SAPPOROBDD */
    /* is full, so reject a level that it cannot represent before any */
    /* variable is added. */
    if (root_level > sbddextended_varMaxAsInt()) {
        fprintf(stderr, "The level of the root must be at most %d.\n",
                sbddextended_varMaxAsInt());
        return bddnull;
    }

    sbddextended_readOrReturnNull(sbddextended_readUint64, &v64); /* number_of_roots */
    if (v64 != 1) {
        fprintf(stderr, "Currently, this function supports only 1 root.\n");
        return bddnull;
    }
    /* reserved */
    for (i = 0; i < 8; ++i) {
        sbddextended_readOrReturnNull(sbddextended_readUint64, &v64);
    }

    if (max_level == 0) { /* case of a constant function (0/1-terminal) */
        sbddextended_readOrReturnNull(sbddextended_readUint64, &v64);
        if (v64 == 0) {
            return bddempty;
        } else if (v64 == 1) {
            return bddsingle;
        } else {
            fprintf(stderr, "Currently, this function supports only 0/1-terminal.\n");
            return bddnull;
        }
    }

    sbddextended_MyVector_initialize(&level_vec);

#ifdef __cplusplus
    /* MyVector_add can throw std::bad_alloc, the readers throw when the
       caller gave an istream with exceptions enabled, and the node
       construction can throw with SAPPOROBDD++. Release the working
       buffers, the vector and the nodes created so far before letting
       an exception propagate (an exception thrown by the initialize
       call above needs no cleanup because nothing is owned then). */
    try {
#endif

    /* level 0, unused (dummy) */
    sbddextended_MyVector_add(&level_vec, 0ll);

    /* the number of nodes that bddnode_buf (one bddp per node) and */
    /* child_buf (two ullint per node) can hold at the same time */
    max_number_of_nodes = (ullint)(((size_t)-1)
                                   / (sizeof(bddp) + 2 * sizeof(ullint)));

    number_of_nodes = number_of_terminals;
    for (level = 1; level <= max_level; ++level) {
        if (!sbddextended_readUint64(&v64, fp)) {
            fprintf(stderr, "Unexpected end of the BDD binary.\n");
            sbddextended_MyVector_deinitialize(&level_vec);
            return bddnull;
        }
        /* The check is written in this form so that */
        /* number_of_nodes never overflows. It comes before the value */
        /* is stored, so that the implementation-defined conversion of */
        /* a huge v64 to llint never happens. */
        if (v64 > max_number_of_nodes - number_of_nodes) {
            fprintf(stderr, "The number of nodes in the BDD binary is too large.\n");
            sbddextended_MyVector_deinitialize(&level_vec);
            return bddnull;
        }
        sbddextended_MyVector_add(&level_vec, (llint)v64);
        number_of_nodes += v64;
    }
    if (!sbddextended_readUint64(&root_id, fp)) {
        fprintf(stderr, "Unexpected end of the BDD binary.\n");
        sbddextended_MyVector_deinitialize(&level_vec);
        return bddnull;
    }

    assert(number_of_nodes >= number_of_terminals);
    assert(number_of_nodes >= 2);
    bddnode_buf = (bddp*)malloc((size_t)number_of_nodes * sizeof(bddp));
    if (bddnode_buf == NULL) {
        /* number_of_nodes is the sum of the per-level node counts read */
        /* from the input, so a corrupted binary of a few hundred bytes */
        /* can claim far more nodes than any memory can hold. Failing */
        /* to allocate the buffer is therefore an input error of this */
        /* import, not a reason to end the caller's process. */
        fprintf(stderr, "Cannot allocate memory for the nodes in the BDD binary.\n");
        sbddextended_MyVector_deinitialize(&level_vec);
        return bddnull;
    }
    /* The child ids are buffered so that the whole structure can be */
    /* validated before any variable is added below. SAPPOROBDD has no */
    /* way to remove an added variable, so adding the variables first */
    /* would let a broken binary change the global state permanently */
    /* even though this function reports the error with bddnull. */
    /* The buffer holds two ids per node; the two slots of the */
    /* terminals are unused so that a node id can index the buffer */
    /* directly. */
    child_buf = (ullint*)malloc((size_t)(2 * number_of_nodes)
                                * sizeof(ullint));
    if (child_buf == NULL) {
        fprintf(stderr, "Cannot allocate memory for the nodes in the BDD binary.\n");
        free(bddnode_buf);
        sbddextended_MyVector_deinitialize(&level_vec);
        return bddnull;
    }
    bddnode_buf[0] = bddempty;
    bddnode_buf[1] = bddsingle;

    /* The first pass reads and validates all the node records without */
    /* creating any node or variable. */
    /* The ids are assigned to the nodes in the ascending order of the */
    /* levels, so the level of the current node and the id of the first */
    /* node at that level only advance as node_count grows. They are */
    /* updated incrementally below instead of being searched from level */
    /* 1 for each node, which would take time quadratic in the number */
    /* of levels. */
    level = 0;
    node_sum = number_of_terminals;
    level_start_id = number_of_terminals;
    for (node_count = number_of_terminals;
            node_count < number_of_nodes; ++node_count) {
        /* obtain the node's level and the id of the first node at it */
        while (node_sum <= node_count) {
            ++level;
            /* the level of every node exists because the sum of the */
            /* per-level numbers of nodes is number_of_nodes */
            assert(level <= max_level);
            level_start_id = node_sum;
            /* add the number of nodes at the level */
            node_sum += (ullint)sbddextended_MyVector_get(&level_vec, (llint)level);
        }
        assert(1 <= level && level <= max_level);
        assert(level_start_id <= node_count);

        /* read 0-child */
        if (!sbddextended_readUint64(&v64, fp)) {
            fprintf(stderr, "Unexpected end of the BDD binary.\n");
            sbddextended_freeBuffersAndReturnNull();
        }
        child_buf[2 * node_count] = v64;
        if (v64 > 1) {
            if (use_negative_arcs != 0) {
                if(v64 % 2 == 1) {
                    fprintf(stderr, "0-child must not be negative.\n");
                    sbddextended_freeBuffersAndReturnNull();
                }
                v64 >>= 1;
            }
            /* A child must be a node at a level lower than that of the */
            /* current node, that is, one whose id is at least */
            /* number_of_terminals (the terminals are written as 0 and 1 */
            /* even in the negative arc mode, in which the reference to */
            /* a node is twice its id) and smaller than the id of the */
            /* first node at the current level. Comparing it with */
            /* node_count alone would let a node point at another node of */
            /* the same level, which makes bddmakenodeb/bddmakenodez stop */
            /* the process instead of returning an error, and without the */
            /* lower bound the references 2 and 3 of the negative arc */
            /* mode would silently be read as the 1-terminal. */
            if (v64 < number_of_terminals || v64 >= level_start_id) {
                fprintf(stderr, "The node id of a 0-child is out of range.\n");
                sbddextended_freeBuffersAndReturnNull();
            }
        }

        /* read 1-child */
        if (!sbddextended_readUint64(&v64, fp)) {
            fprintf(stderr, "Unexpected end of the BDD binary.\n");
            sbddextended_freeBuffersAndReturnNull();
        }
        child_buf[2 * node_count + 1] = v64;
        if (v64 > 1) {
            if (use_negative_arcs != 0) {
                v64 >>= 1;
            }
            /* see the comment of the 0-child above */
            if (v64 < number_of_terminals || v64 >= level_start_id) {
                fprintf(stderr, "The node id of a 1-child is out of range.\n");
                sbddextended_freeBuffersAndReturnNull();
            }
        }
    }
    /* the index of the root node in bddnode_buf */
    v64 = (use_negative_arcs != 0 ? (root_id >> 1) : root_id);
    /* max_level is not 0 here, so the root is a node, not a terminal
       (see the comment of the 0-child above for the lower bound). */
    if (v64 < number_of_terminals || v64 >= number_of_nodes) {
        fprintf(stderr, "The node id of the root is out of range.\n");
        sbddextended_freeBuffersAndReturnNull();
    }

    /* All the structure checks are done. The global state is changed */
    /* and the nodes are created only below this point. */
    while (bddvarused() < (bddvar)root_level) {
        bddnewvar();
    }

    /* The second pass builds the nodes from the buffered child ids, */
    /* which the first pass has already validated. */
    level = 0;
    node_sum = number_of_terminals;
    for (node_count = number_of_terminals;
            node_count < number_of_nodes; ++node_count) {
        /* obtain the node's level */
        while (node_sum <= node_count) {
            ++level;
            node_sum += (ullint)sbddextended_MyVector_get(&level_vec, (llint)level);
        }

        v64 = child_buf[2 * node_count]; /* 0-child */
        if (v64 <= 1) {
            f0 = bddgetterminal((int)v64, is_zbdd);
        } else {
            if (use_negative_arcs != 0) {
                v64 >>= 1;
            }
            f0 = bddnode_buf[v64];
        }

        v64 = child_buf[2 * node_count + 1]; /* 1-child */
        if (v64 <= 1) {
            f1 = bddgetterminal((int)v64, is_zbdd);
        } else if (use_negative_arcs != 0) {
            f1 = bddnode_buf[v64 >> 1];
            if (v64 % 2 == 1) {
                f1 = bddtakenot(f1);
            }
        } else {
            f1 = bddnode_buf[v64];
        }

        var = bddvaroflev((bddvar)((int)level + root_level - (int)max_level));
        assert(node_count < number_of_nodes);
        if (is_zbdd != 0) {
            bddnode_buf[node_count] = bddmakenodez(var, f0, f1);
        } else {
            bddnode_buf[node_count] = bddmakenodeb(var, f0, f1);
        }
#ifdef __cplusplus
        built_count = node_count + 1;
#endif
        /* SAPPOROBDD returns bddnull when it cannot create the node
           (out of memory). The loop must not go on with it: the
           negative reference of a later node would be built as
           bddtakenot(bddnull), an invalid node id that is not the
           error sentinel any more. */
        if (bddnode_buf[node_count] == bddnull) {
            fprintf(stderr, "Cannot create a node of the BDD binary "
                    "(out of memory?).\n");
            for (i = number_of_terminals; i < node_count; ++i) {
                bddfree(bddnode_buf[i]);
            }
            free(child_buf);
            free(bddnode_buf);
            sbddextended_MyVector_deinitialize(&level_vec);
            return bddnull;
        }
    }
    v64 = (use_negative_arcs != 0 ? (root_id >> 1) : root_id);
    assert(v64 < number_of_nodes);
    if (use_negative_arcs != 0 && root_id % 2 == 1) { /* negative arc */
        f = bddtakenot(bddcopy(bddnode_buf[v64]));
    } else {
        f = bddcopy(bddnode_buf[v64]);
    }
    for (node_count = number_of_terminals;
            node_count < number_of_nodes; ++node_count) {
        bddfree(bddnode_buf[node_count]);
    }
    free(child_buf);
    free(bddnode_buf);
    sbddextended_MyVector_deinitialize(&level_vec);
    return f;

#ifdef __cplusplus
    } catch (...) {
        for (node_count = number_of_terminals;
                node_count < built_count; ++node_count) {
            bddfree(bddnode_buf[node_count]);
        }
        free(child_buf);
        free(bddnode_buf);
        sbddextended_MyVector_deinitialize(&level_vec);
        throw;
    }
#endif
}

#undef sbddextended_readOrReturnNull
#undef sbddextended_freeBuffersAndReturnNull

#ifdef __cplusplus

sbddextended_INLINE_FUNC
BDD importBDDAsBinary(FILE* fp, int root_level = -1)
{
    ReadLineObject glo;
    bddp p;
    p = bddimportbddasbinary_inner(fp,
                                    root_level, 0,
                                    glo, glo, glo, glo);
    return BDD_ID(p);
}

sbddextended_INLINE_FUNC
BDD importBDDAsBinary(std::istream& ist, int root_level = -1)
{
    ReadLineObject glo(&ist);
    bddp p;
    p = bddimportbddasbinary_inner(NULL,
                                    root_level, 0,
                                    glo, glo, glo, glo);
    return BDD_ID(p);
}

sbddextended_INLINE_FUNC
ZBDD importZBDDAsBinary(FILE* fp, int root_level = -1)
{
    ReadLineObject glo;
    bddp p;
    p = bddimportbddasbinary_inner(fp,
                                    root_level, 1,
                                    glo, glo, glo, glo);
    return ZBDD_ID(p);
}

sbddextended_INLINE_FUNC
ZBDD importZBDDAsBinary(std::istream& ist, int root_level = -1)
{
    ReadLineObject glo(&ist);
    bddp p;
    p = bddimportbddasbinary_inner(NULL,
                                    root_level, 1,
                                    glo, glo, glo, glo);
    return ZBDD_ID(p);
}

sbddextended_INLINE_FUNC
bddp bddimportbddasbinary(FILE* fp, int root_level = -1)
{
    ReadLineObject glo;
    return bddimportbddasbinary_inner(fp,
                                        root_level, 0,
                                        glo, glo, glo, glo);
}

sbddextended_INLINE_FUNC
bddp bddimportzbddasbinary(FILE* fp, int root_level = -1)
{
    ReadLineObject glo;
    return bddimportbddasbinary_inner(fp,
                                        root_level, 1,
                                        glo, glo, glo, glo);
}

#else

sbddextended_INLINE_FUNC
bddp bddimportbddasbinary(FILE* fp, int root_level)
{
    return bddimportbddasbinary_inner(fp, root_level, 0);
}

sbddextended_INLINE_FUNC
bddp bddimportzbddasbinary(FILE* fp, int root_level)
{
    return bddimportbddasbinary_inner(fp, root_level, 1);
}

#endif


/* *************** export functions */


/* Writes one value of the BDD binary format and returns from
   bddexportbddasbinary_inner when the write fails, releasing what the
   function owns at that point (id_prefix is NULL and is_making_index
   is 0 until they are set, so the releases are no-ops before that).
   Used only there and undefined after it. */
#define sbddextended_writeOrReturn(write_func, value) \
    do { \
        if (!write_func(value, fp)) { \
            sbddextended_printWriteError(); \
            free(id_prefix); \
            if (is_making_index) { \
                bddNodeIndex_destruct(node_index); \
                free(node_index); \
            } \
            return; \
        } \
    } while (0)

sbddextended_INLINE_FUNC
void bddexportbddasbinary_inner(FILE* fp, bddp f,
                                bddNodeIndex* node_index,
                                int is_zbdd,
                                int use_negative_arcs
#ifdef __cplusplus
                                , const WriteObject& sbddextended_writeUint8
                                , const WriteObject& sbddextended_writeUint16
                                , const WriteObject& sbddextended_writeUint32
                                , const WriteObject& sbddextended_writeUint64
#endif
                                )
{
    /* Since only BDD/ZDD is treated in the current version, */
    /* the number of terminals is fixed to be 2. */
    const unsigned int number_of_terminals = 2;
    ullint i, j;
    int k, is_making_index = 0;
    ullint max_level;
    ullint number_of_nodes = 0;
    ullint root_id;
    llint id;
    /* NULL until it is allocated below, so that the macro above can
       release it from anywhere in this function */
    ullint* id_prefix = NULL;
    bddp node, child, rchild;

    /* The BDD binary format has no representation for bddnull. Without */
    /* this check the level 0 branch below would reach its assert(0) in a */
    /* debug build and would write a header with no root id in a release */
    /* build, that is, a file that looks like a valid constant DD. */
    if (f == bddnull) {
        fprintf(stderr, "The BDD binary format cannot represent bddnull.\n");
        return;
    }
    /* Nor for a multi-valued terminal bddconst(c) (c >= 2): the level */
    /* 0 branch below knows only the two terminals, and would reach its */
    /* assert(0) or write a header without a root id in the same way. */
    if (bddisconstant(f) && !bddisterminal(f)) {
        fprintf(stderr, "The BDD binary format cannot represent "
                "a multi-valued terminal.\n");
        return;
    }

    if (node_index != NULL) {
        if (!bddNodeIndex_checkIndexOf(node_index, f, is_zbdd)) {
            return;
        }
        if (node_index->is_raw != 0 && use_negative_arcs == 0) {
            fprintf(stderr, "The node index must not be constructed in the raw mode "
                            "when not using negative arcs.\n");
            return;
        } else if (node_index->is_raw == 0 && use_negative_arcs != 0) {
            fprintf(stderr, "The node index must be constructed in the raw mode "
                            "when using negative arcs.\n");
            return;
        }
    }

    max_level = (ullint)bddgetlev(f);

    /* start header */

    sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)'B');
    sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)'D');
    sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)'D');

    sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)1u); /* version */
    /* DD type */
    if (is_zbdd < 0) { /* can be interpreted as BDD/ZBDD */
        sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)1u);
    } else if (is_zbdd == 0) { /* BDD */
        sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)2u);
    } else { /* ZBDD */
        sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)3u);
    }
    /* number_of_arcs */
    sbddextended_writeOrReturn(sbddextended_writeUint16, (unsigned short)2u);
    /* number_of_terminals */
    sbddextended_writeOrReturn(sbddextended_writeUint32, number_of_terminals);
    /* number_of_bits_for_level */
    sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)16u);
    /* number_of_bits_for_id */
    sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)64u);
    /* use_negative_arcs */
    if (use_negative_arcs != 0) {
        sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)1u);
    } else {
        sbddextended_writeOrReturn(sbddextended_writeUint8, (unsigned char)0u);
    }
    /* max_level */
    sbddextended_writeOrReturn(sbddextended_writeUint64, max_level);
    /* number_of_roots */
    sbddextended_writeOrReturn(sbddextended_writeUint64, (ullint)1u);

    /* reserved */
    for (i = 0; i < 8; ++i) {
        sbddextended_writeOrReturn(sbddextended_writeUint64, (ullint)0u);
    }

    /* end header */

    if (is_zbdd < 0) {
        is_zbdd = (!bddisbdd(f) ? 1 : 0);
    }

    if (max_level == 0) { /* case of a constant function (0/1-terminal) */
        if (f == bddempty) {
            sbddextended_writeOrReturn(sbddextended_writeUint64, (ullint)0ull);
        } else if (f == bddsingle) {
            sbddextended_writeOrReturn(sbddextended_writeUint64, (ullint)1ull);
        } else {
            assert(0);
        }
        return;
    }

    if (node_index == NULL) {
        is_making_index = 1;
        if (is_zbdd != 0) {
            if (use_negative_arcs != 0) {
                node_index = bddNodeIndex_makeRawIndexZWithoutCount(f);
            } else {
                node_index = bddNodeIndex_makeIndexZWithoutCount(f);
            }
        } else {
            if (use_negative_arcs != 0) {
                node_index = bddNodeIndex_makeRawIndexBWithoutCount(f);
            } else {
                node_index = bddNodeIndex_makeIndexBWithoutCount(f);
            }
        }
    }

#ifdef __cplusplus
    /* The write callback throws instead of returning false when the
       ostream has exceptions enabled, so release the index this function
       made and the id array before letting it propagate. */
    try {
#endif
    assert((ullint)node_index->height == max_level);

    /* write the number of nodes in level i and compute the number of nodes */
    for (i = 1; i <= max_level; ++i) {
        sbddextended_writeOrReturn(sbddextended_writeUint64, (ullint)node_index->level_vec_arr[i].count);
        number_of_nodes += (ullint)node_index->level_vec_arr[i].count;
    }

    id_prefix = (ullint*)malloc((size_t)(max_level + 1) * sizeof(ullint));
    if (id_prefix == NULL) {
        fprintf(stderr, "out of memory\n");
        if (is_making_index) {
            bddNodeIndex_destruct(node_index);
            free(node_index);
        }
        return;
    }

    id_prefix[1] = number_of_terminals;
    for (i = 1; i < max_level; ++i) {
        id_prefix[i + 1] = id_prefix[i] + (ullint)node_index->level_vec_arr[i].count;
    }

    /* write the number of the root id */
    /* (In the current version, assume that the number of roots is 1.) */
    root_id = (ullint)number_of_terminals + number_of_nodes - 1;
    assert(root_id == id_prefix[max_level]);

    if (use_negative_arcs != 0) {
        root_id *= 2;
        if (bddisnegative(f)) {
            ++root_id;
        }
    }

    sbddextended_writeOrReturn(sbddextended_writeUint64, root_id);

    for (i = 1; i <= max_level; ++i) {
        for (j = 0; j < node_index->level_vec_arr[i].count; ++j) {
            node = (bddp)sbddextended_MyVector_get(&node_index->level_vec_arr[i], (llint)j);
            for (k = 0; k < sbddextended_NUMBER_OF_CHILDREN; ++k) {
                child = bddgetchildg(node, k, node_index->is_zbdd, node_index->is_raw);
                if (child == bddempty) {
                    sbddextended_writeOrReturn(sbddextended_writeUint64, 0llu);
                } else if (child == bddsingle) {
                    sbddextended_writeOrReturn(sbddextended_writeUint64, 1llu);
                } else {
                    rchild = (use_negative_arcs != 0 ? bdderasenot(child) : child);
                    if (sbddextended_MyDict_find(&node_index->node_dict_arr[bddgetlev(child)],
                                                    (llint)rchild, &id) == 0) {
                        fprintf(stderr, "node not found\n");
                        exit(1);
                    }
                    id += (llint)id_prefix[bddgetlev(child)];
                    if (use_negative_arcs != 0) {
                        id *= 2;
                        if (bddisnegative(child)) {
                            id += 1;
                        }
                    }
                    sbddextended_writeOrReturn(sbddextended_writeUint64, (ullint)id);
                }
            }
        }
    }

    if (is_making_index) {
        bddNodeIndex_destruct(node_index);
        free(node_index);
    }
    free(id_prefix);
#ifdef __cplusplus
    } catch (...) {
        free(id_prefix);
        if (is_making_index) {
            bddNodeIndex_destruct(node_index);
            free(node_index);
        }
        throw;
    }
#endif
}

#undef sbddextended_writeOrReturn

#ifdef __cplusplus

template <typename T>
sbddextended_INLINE_FUNC
void exportBDDAsBinary(FILE* fp, const BDD& bdd, bool use_negative_arcs, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
        /* The negative arc mode needs an index of the raw mode, which
           DDIndex does not support, so this combination can never
           work. Say so instead of reporting the raw mode of the index
           below, which the caller cannot change. */
        if (use_negative_arcs) {
            fprintf(stderr, "A DDIndex cannot be used with "
                    "use_negative_arcs == true, because it does not "
                    "support the raw mode. Pass false, or pass no "
                    "index.\n");
            return;
        }
    }
    WriteObject wo(false, true, NULL);
    bddexportbddasbinary_inner(fp, bdd.GetID(), bnode_index,
                                0, (use_negative_arcs ? 1 : 0),
                                wo, wo, wo, wo);
}

sbddextended_INLINE_FUNC
void exportBDDAsBinary(FILE* fp, const BDD& bdd, bool use_negative_arcs = true)
{
    exportBDDAsBinary<int>(fp, bdd, use_negative_arcs, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportBDDAsBinary(std::ostream& ost, const BDD& bdd, bool use_negative_arcs, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
        /* The negative arc mode needs an index of the raw mode, which
           DDIndex does not support, so this combination can never
           work. Say so instead of reporting the raw mode of the index
           below, which the caller cannot change. */
        if (use_negative_arcs) {
            fprintf(stderr, "A DDIndex cannot be used with "
                    "use_negative_arcs == true, because it does not "
                    "support the raw mode. Pass false, or pass no "
                    "index.\n");
            return;
        }
    }
    WriteObject wo(true, true, &ost);
    bddexportbddasbinary_inner(NULL, bdd.GetID(), bnode_index,
                                0, (use_negative_arcs ? 1 : 0),
                                wo, wo, wo, wo);
}

sbddextended_INLINE_FUNC
void exportBDDAsBinary(std::ostream& ost, const BDD& bdd, bool use_negative_arcs = true)
{
    exportBDDAsBinary<int>(ost, bdd, use_negative_arcs, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportZBDDAsBinary(FILE* fp, const ZBDD& zbdd, bool use_negative_arcs, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
        /* The negative arc mode needs an index of the raw mode, which
           DDIndex does not support, so this combination can never
           work. Say so instead of reporting the raw mode of the index
           below, which the caller cannot change. */
        if (use_negative_arcs) {
            fprintf(stderr, "A DDIndex cannot be used with "
                    "use_negative_arcs == true, because it does not "
                    "support the raw mode. Pass false, or pass no "
                    "index.\n");
            return;
        }
    }
    WriteObject wo(false, true, NULL);
    bddexportbddasbinary_inner(fp, zbdd.GetID(), bnode_index,
                                1, (use_negative_arcs ? 1 : 0),
                                wo, wo, wo, wo);
}

sbddextended_INLINE_FUNC
void exportZBDDAsBinary(FILE* fp, const ZBDD& zbdd, bool use_negative_arcs = true)
{
    exportZBDDAsBinary<int>(fp, zbdd, use_negative_arcs, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportZBDDAsBinary(std::ostream& ost, const ZBDD& zbdd, bool use_negative_arcs, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
        /* The negative arc mode needs an index of the raw mode, which
           DDIndex does not support, so this combination can never
           work. Say so instead of reporting the raw mode of the index
           below, which the caller cannot change. */
        if (use_negative_arcs) {
            fprintf(stderr, "A DDIndex cannot be used with "
                    "use_negative_arcs == true, because it does not "
                    "support the raw mode. Pass false, or pass no "
                    "index.\n");
            return;
        }
    }
    WriteObject wo(true, true, &ost);
    bddexportbddasbinary_inner(NULL, zbdd.GetID(), bnode_index,
                                1, (use_negative_arcs ? 1 : 0),
                                wo, wo, wo, wo);
}

sbddextended_INLINE_FUNC
void exportZBDDAsBinary(std::ostream& ost, const ZBDD& zbdd, bool use_negative_arcs = true)
{
    exportZBDDAsBinary<int>(ost, zbdd, use_negative_arcs, NULL);
}

sbddextended_INLINE_FUNC
void bddexportbddasbinary(FILE* fp, bddp f, int use_negative_arcs, bddNodeIndex* node_index)
{
    WriteObject wo(false, true, NULL);
    bddexportbddasbinary_inner(fp, f, node_index,
                                0, use_negative_arcs,
                                wo, wo, wo, wo);
}

sbddextended_INLINE_FUNC
void bddexportzbddasbinary(FILE* fp, bddp f, int use_negative_arcs, bddNodeIndex* node_index)
{
    WriteObject wo(false, true, NULL);
    bddexportbddasbinary_inner(fp, f, node_index,
                                1, use_negative_arcs,
                                wo, wo, wo, wo);
}

#else

sbddextended_INLINE_FUNC
void bddexportbddasbinary(FILE* fp, bddp f, int use_negative_arcs, bddNodeIndex* node_index)
{
    bddexportbddasbinary_inner(fp, f, node_index, 0, use_negative_arcs);
}

sbddextended_INLINE_FUNC
void bddexportzbddasbinary(FILE* fp, bddp f, int use_negative_arcs, bddNodeIndex* node_index)
{
    bddexportbddasbinary_inner(fp, f, node_index, 1, use_negative_arcs);
}

#endif

/* *************** import functions */

/* Frees the working vectors of bddimportbddasgraphillion_inner and */
/* returns bddnull from it. Used only there and undefined after it. */
#define sbddextended_freeVectorsAndReturnNull() \
    do { \
        sbddextended_MyVector_deinitialize(&hi_vec); \
        sbddextended_MyVector_deinitialize(&lo_vec); \
        sbddextended_MyVector_deinitialize(&level_vec); \
        sbddextended_MyVector_deinitialize(&node_vec); \
        return bddnull; \
    } while (0)

/* Frees the working data of the node construction loop of */
/* bddimportbddasgraphillion_inner and returns bddnull from it. */
/* Used only there and undefined after it. */
#define sbddextended_freeNodesAndReturnNull() \
    do { \
        for (j = 0; j < i; ++j) { \
            if (sbddextended_MyDict_find(&node_dict, \
                    sbddextended_MyVector_get(&node_vec, j), &value) != 0) { \
                bddfree((bddp)value); \
            } \
        } \
        sbddextended_MyDict_deinitialize(&node_dict); \
        sbddextended_freeVectorsAndReturnNull(); \
    } while (0)

/* Parses the whole token as a decimal node ID in [0, LLONG_MAX - 2] and
   stores the internal ID (the parsed value plus 2). Returns 0 if the token
   contains extra characters, is out of range, or the addition of 2 would
   overflow. */
sbddextended_INLINE_FUNC
int bddimportbddasgraphillion_readid(const char* token, llint* id)
{
    char* end;
    llint v;
    /* strtoll skips leading whitespace and accepts a sign, neither of
       which the format allows, so require a digit first (the reader of
       the Knuth format does the same). */
    if (!isdigit((int)(unsigned char)token[0])) {
        return 0;
    }
    errno = 0;
    end = NULL;
    v = strtoll(token, &end, 10);
    if (end == token || *end != '\0' || errno == ERANGE
            || v < 0 || v > LLONG_MAX - 2) {
        return 0;
    }
    *id = v + 2;
    return 1;
}

/* Parses a child token of the graphillion format: "B" (0-terminal),
   "T" (1-terminal), or a node ID. Stores the internal ID (0, 1, or the
   parsed ID plus 2). Returns 0 on failure. */
sbddextended_INLINE_FUNC
int bddimportbddasgraphillion_readchild(const char* token, llint* id)
{
    if (token[0] == 'B' && token[1] == '\0') {
        *id = 0;
        return 1;
    }
    if (token[0] == 'T' && token[1] == '\0') {
        *id = 1;
        return 1;
    }
    return bddimportbddasgraphillion_readid(token, id);
}

/* Parses the whole token as a decimal level in [1, INT_MAX].
   Returns 0 on failure. */
sbddextended_INLINE_FUNC
int bddimportbddasgraphillion_readlevel(const char* token, int* level)
{
    char* end;
    llint v;
    /* see the comment of bddimportbddasgraphillion_readid */
    if (!isdigit((int)(unsigned char)token[0])) {
        return 0;
    }
    errno = 0;
    end = NULL;
    v = strtoll(token, &end, 10);
    if (end == token || *end != '\0' || errno == ERANGE
            || v < 1 || v > INT_MAX) {
        return 0;
    }
    *level = (int)v;
    return 1;
}

sbddextended_INLINE_FUNC
bddp bddimportbddasgraphillion_inner(FILE* fp, int root_level, int is_zdd
#ifdef __cplusplus
                                            , ReadLineObject& sbddextended_readLine
#endif
                                            )
{
    int c, level, max_level = 0, line_status;
    llint i, j, id, lo, hi, value, line_count = 0;
    llint lo_value, hi_value, child_level;
    llint root_node_id = 0;
    bddvar var;
    char buf[sbddextended_BUFSIZE];
    char buf1[sbddextended_BUFSIZE];
    char buf2[sbddextended_BUFSIZE];
    char buf3[sbddextended_BUFSIZE];
    char buf4[sbddextended_BUFSIZE];
    char buf5[sbddextended_BUFSIZE];
    bddp p, p0, p1, pf, pfn, pr;
    sbddextended_MyVector node_vec, level_vec, lo_vec, hi_vec;
    sbddextended_MyDict node_dict;
    sbddextended_MyDict defined_levels;
#ifdef __cplusplus
    int num_initialized_vecs = 0;
    int defined_levels_initialized = 0;
    int node_dict_initialized = 0;
#endif

    p0 = p1 = pf = pfn = pr = bddnull;

    line_status = sbddextended_readLine(buf, fp); /* read first line */
    if (line_status != 1) {
        if (line_status == 0) {
            fprintf(stderr, "Unexpected end of the input.\n");
        } /* on a read error a message has been printed */
        return bddnull;
    }
    if (buf[0] == '.' && sbddextended_isBlankString(buf + 1)) {
        /* end of file */
        return bddnull;
    } else if ((buf[0] == 'B' || buf[0] == 'T')
            && sbddextended_isBlankString(buf + 1)) {
        c = buf[0];
        /* The exporter writes the terminator line after the terminal, so */
        /* consume it here. Otherwise the code that keeps reading from the */
        /* same stream would take it for the next input. The terminator is */
        /* optional, as it is for a DD with nodes below. */
        line_status = sbddextended_readLine(buf, fp);
        if (line_status < 0) { /* a read error; a message has been printed */
            return bddnull;
        }
        if (line_status == 1
                && !(buf[0] == '.'
                     && sbddextended_isBlankString(buf + 1))) {
            fprintf(stderr, "Format error in line 2\n");
            return bddnull;
        }
        if (c == 'B') {
            return (is_zdd == 0 ? bddfalse : bddempty);
        }
        return (is_zdd == 0 ? bddtrue : bddsingle);
    }

#ifdef __cplusplus
    /* MyVector/MyDict operations can throw std::bad_alloc, the reader
       throws when the caller gave an istream with exceptions enabled,
       and the DD operations can throw with SAPPOROBDD++. Release the
       initialized containers and the DD references owned at that moment
       before letting an exception propagate (an element whose
       initialize call itself threw needs no release because initialize
       puts the element into the deinitialized state before
       allocating). */
    try {
#endif
    sbddextended_MyVector_initialize(&node_vec);
#ifdef __cplusplus
    num_initialized_vecs = 1;
#endif
    sbddextended_MyVector_initialize(&level_vec);
#ifdef __cplusplus
    num_initialized_vecs = 2;
#endif
    sbddextended_MyVector_initialize(&lo_vec);
#ifdef __cplusplus
    num_initialized_vecs = 3;
#endif
    sbddextended_MyVector_initialize(&hi_vec);
#ifdef __cplusplus
    num_initialized_vecs = 4;
#endif

    do {
        ++line_count;
        if (buf[0] == '.' && sbddextended_isBlankString(buf + 1)) {
            /* end of file */
            break;
        }
        /* Each line must consist of exactly four tokens: the node ID,
           the level, the 0-child, and the 1-child. */
        c = sscanf(buf, "%s %s %s %s %s", buf1, buf2, buf3, buf4, buf5);
        if (c != 4
                || bddimportbddasgraphillion_readid(buf1, &id) == 0
                || bddimportbddasgraphillion_readlevel(buf2, &level) == 0
                || bddimportbddasgraphillion_readchild(buf3, &lo) == 0
                || bddimportbddasgraphillion_readchild(buf4, &hi) == 0) {
            fprintf(stderr, "Format error in line %lld\n", line_count);
            sbddextended_freeVectorsAndReturnNull();
        }
        sbddextended_MyVector_add(&node_vec, id);
        sbddextended_MyVector_add(&level_vec, (llint)level);
        sbddextended_MyVector_add(&lo_vec, lo);
        sbddextended_MyVector_add(&hi_vec, hi);
        if (max_level < level) {
            max_level = level;
        }
    } while ((line_status = sbddextended_readLine(buf, fp)) == 1);
    if (line_status < 0) { /* a read error; a message has been printed */
        sbddextended_freeVectorsAndReturnNull();
    }

    if (root_level < 0) {
        root_level = max_level;
    } else if (root_level < max_level) {
        fprintf(stderr, "The argument \"root_level\" must be "
                "larger than or equal to the height of the DD.\n");
        sbddextended_freeVectorsAndReturnNull();
    }
    /* The levels are turned into variables with bddnewvar below, which */
    /* terminates the process once the variable index range of SAPPOROBDD */
    /* is full, so reject a level that it cannot represent before any */
    /* variable is added. */
    if (root_level > sbddextended_varMaxAsInt()) {
        fprintf(stderr, "The level of the root must be at most %d.\n",
                sbddextended_varMaxAsInt());
        sbddextended_freeVectorsAndReturnNull();
    }

    /* Check the structure before any variable is added below. */
    /* SAPPOROBDD has no way to remove a variable, so a file that turns */
    /* out to be malformed would otherwise leave behind the variables */
    /* that were added for it. The ids 0 and 1 are the terminals, and a */
    /* node must be defined once and only after both of its children. */
    /* The levels count from the root in this format, so a child must */
    /* also have a level strictly larger than that of the node. The */
    /* construction below builds a violating input without failing and */
    /* would return a wrong DD instead of rejecting it. The terminals */
    /* are below every node, which LLONG_MAX represents here. */
    sbddextended_MyDict_initialize(&defined_levels);
#ifdef __cplusplus
    defined_levels_initialized = 1;
#endif
    sbddextended_MyDict_add(&defined_levels, 0ll, LLONG_MAX);
    sbddextended_MyDict_add(&defined_levels, 1ll, LLONG_MAX);
    for (i = 0; i < (llint)node_vec.count; ++i) {
        id = sbddextended_MyVector_get(&node_vec, i);
        level = (int)sbddextended_MyVector_get(&level_vec, i);
        lo = sbddextended_MyVector_get(&lo_vec, i);
        hi = sbddextended_MyVector_get(&hi_vec, i);
        if (sbddextended_MyDict_find(&defined_levels, id, NULL) != 0) {
            fprintf(stderr, "The node %lld is defined more than once.\n",
                    id - 2);
            sbddextended_MyDict_deinitialize(&defined_levels);
            sbddextended_freeVectorsAndReturnNull();
        }
        if (sbddextended_MyDict_find(&defined_levels, lo, &child_level) == 0) {
            fprintf(stderr, "The 0-child (%lld) of the node %lld is not found.\n",
                    lo - 2, id - 2);
            sbddextended_MyDict_deinitialize(&defined_levels);
            sbddextended_freeVectorsAndReturnNull();
        }
        if (child_level <= (llint)level) {
            fprintf(stderr, "The 0-child (%lld) of the node %lld must have "
                    "a larger level.\n", lo - 2, id - 2);
            sbddextended_MyDict_deinitialize(&defined_levels);
            sbddextended_freeVectorsAndReturnNull();
        }
        if (sbddextended_MyDict_find(&defined_levels, hi, &child_level) == 0) {
            fprintf(stderr, "The 1-child (%lld) of the node %lld is not found.\n",
                    hi - 2, id - 2);
            sbddextended_MyDict_deinitialize(&defined_levels);
            sbddextended_freeVectorsAndReturnNull();
        }
        if (child_level <= (llint)level) {
            fprintf(stderr, "The 1-child (%lld) of the node %lld must have "
                    "a larger level.\n", hi - 2, id - 2);
            sbddextended_MyDict_deinitialize(&defined_levels);
            sbddextended_freeVectorsAndReturnNull();
        }
        sbddextended_MyDict_add(&defined_levels, id, (llint)level);
    }
    sbddextended_MyDict_deinitialize(&defined_levels);
#ifdef __cplusplus
    defined_levels_initialized = 0;
#endif

    while (bddvarused() < (bddvar)root_level) {
        bddnewvar();
    }

    sbddextended_MyDict_initialize(&node_dict);
#ifdef __cplusplus
    node_dict_initialized = 1;
#endif
    sbddextended_MyDict_add(&node_dict, 0ll, (is_zdd == 0 ? bddfalse : bddempty));
    sbddextended_MyDict_add(&node_dict, 1ll, (is_zdd == 0 ? bddtrue : bddsingle));

    for (i = 0; i < (llint)node_vec.count; ++i) {
        id = sbddextended_MyVector_get(&node_vec, i);
        level = (int)sbddextended_MyVector_get(&level_vec, i);
        lo = sbddextended_MyVector_get(&lo_vec, i);
        hi = sbddextended_MyVector_get(&hi_vec, i);
        /* Each id is defined once and both children are defined before */
        /* it, which the pass above has checked, so the following are */
        /* invariants rather than checks of the input. */
        assert(sbddextended_MyDict_find(&node_dict, id, NULL) == 0);
        lo_value = hi_value = 0;
        if (sbddextended_MyDict_find(&node_dict, lo, &lo_value) == 0
                || sbddextended_MyDict_find(&node_dict, hi, &hi_value) == 0) {
            fprintf(stderr, "The children of the node %lld are not found.\n",
                    id - 2);
            sbddextended_freeNodesAndReturnNull();
        }
        var = bddvaroflev((bddvar)(root_level - level + 1));
        /* The owned intermediate references are held in pf/pfn/p0/p1/pr
           and each is reset to bddnull as soon as it is released or its
           ownership is transferred, so that the catch below releases
           exactly the ones owned at the moment an operation throws. */
        if (is_zdd == 0) { /* BDD */
            pf = bddprime(var);
            pfn = bddnot(pf);
            p0 = bddand((bddp)lo_value, pfn);
            p1 = bddand((bddp)hi_value, pf);
            pr = bddor(p0, p1);
            bddfree(pf);
            pf = bddnull;
            bddfree(pfn);
            pfn = bddnull;
            bddfree(p0);
            p0 = bddnull;
            bddfree(p1);
            p1 = bddnull;
        } else { /* ZDD */
            p1 = bddchange((bddp)hi_value, var);
            pr = bddunion((bddp)lo_value, p1);
            bddfree(p1);
            p1 = bddnull;
        }
        sbddextended_MyDict_add(&node_dict, id, (llint)pr);
        pr = bddnull; /* now owned by node_dict */
        root_node_id = id; /* The root node is the last node. */
    }
    /* The root node is always registered because node_vec is not empty. */
    if (sbddextended_MyDict_find(&node_dict, root_node_id, &value) == 0) {
        fprintf(stderr, "The root node %lld is not found.\n", root_node_id - 2);
        sbddextended_freeNodesAndReturnNull();
    }
    p = (bddp)value;

    for (i = 0; i < (llint)node_vec.count; ++i) {
        id = sbddextended_MyVector_get(&node_vec, i);
        if (id != root_node_id &&
                sbddextended_MyDict_find(&node_dict, id, &value) != 0) {
            bddfree((bddp)value);
        }
    }

    sbddextended_MyDict_deinitialize(&node_dict);
    sbddextended_MyVector_deinitialize(&hi_vec);
    sbddextended_MyVector_deinitialize(&lo_vec);
    sbddextended_MyVector_deinitialize(&level_vec);
    sbddextended_MyVector_deinitialize(&node_vec);

    return p;

#ifdef __cplusplus
    } catch (...) {
        if (node_dict_initialized) {
            /* release the references registered by the construction
               loop; i is its index, and an iteration registers its
               reference (as the last throwing step) at the id
               node_vec[i], so the registered ids are node_vec[0..i-1] */
            for (j = 0; j < i && j < (llint)node_vec.count; ++j) {
                if (sbddextended_MyDict_find(&node_dict,
                        sbddextended_MyVector_get(&node_vec, j),
                        &value) != 0) {
                    bddfree((bddp)value);
                }
            }
            sbddextended_MyDict_deinitialize(&node_dict);
        }
        bddfree(pr);
        bddfree(p1);
        bddfree(p0);
        bddfree(pfn);
        bddfree(pf);
        if (defined_levels_initialized) {
            sbddextended_MyDict_deinitialize(&defined_levels);
        }
        if (num_initialized_vecs >= 4) {
            sbddextended_MyVector_deinitialize(&hi_vec);
        }
        if (num_initialized_vecs >= 3) {
            sbddextended_MyVector_deinitialize(&lo_vec);
        }
        if (num_initialized_vecs >= 2) {
            sbddextended_MyVector_deinitialize(&level_vec);
        }
        if (num_initialized_vecs >= 1) {
            sbddextended_MyVector_deinitialize(&node_vec);
        }
        throw;
    }
#endif
}

#undef sbddextended_freeNodesAndReturnNull
#undef sbddextended_freeVectorsAndReturnNull

#ifdef __cplusplus

sbddextended_INLINE_FUNC
BDD importBDDAsGraphillion(FILE* fp, int root_level = -1)
{
    ReadLineObject glo;
    bddp p;
    p = bddimportbddasgraphillion_inner(fp, root_level, 0, glo);
    return BDD_ID(p);
}

sbddextended_INLINE_FUNC
BDD importBDDAsGraphillion(std::istream& ist, int root_level = -1)
{
    ReadLineObject glo(&ist);
    bddp p;
    p = bddimportbddasgraphillion_inner(NULL, root_level, 0, glo);
    return BDD_ID(p);
}

sbddextended_INLINE_FUNC
ZBDD importZBDDAsGraphillion(FILE* fp, int root_level = -1)
{
    ReadLineObject glo;
    bddp p;
    p = bddimportbddasgraphillion_inner(fp, root_level, 1, glo);
    return ZBDD_ID(p);
}

sbddextended_INLINE_FUNC
ZBDD importZBDDAsGraphillion(std::istream& ist, int root_level = -1)
{
    ReadLineObject glo(&ist);
    bddp p;
    p = bddimportbddasgraphillion_inner(NULL, root_level, 1, glo);
    return ZBDD_ID(p);
}

sbddextended_INLINE_FUNC
bddp bddimportbddasgraphillion(FILE* fp, int root_level = -1)
{
    ReadLineObject glo;
    return bddimportbddasgraphillion_inner(fp, root_level, 0, glo);
}

sbddextended_INLINE_FUNC
bddp bddimportzbddasgraphillion(FILE* fp, int root_level = -1)
{
    ReadLineObject glo;
    return bddimportbddasgraphillion_inner(fp, root_level, 1, glo);
}

#else

sbddextended_INLINE_FUNC
bddp bddimportbddasgraphillion(FILE* fp, int root_level)
{
    return bddimportbddasgraphillion_inner(fp, root_level, 0);
}

sbddextended_INLINE_FUNC
bddp bddimportzbddasgraphillion(FILE* fp, int root_level)
{
    return bddimportbddasgraphillion_inner(fp, root_level, 1);
}

#endif

/* *************** export functions */


/* Writes one line and returns from bddexportbddasgraphillion_inner when the
   write fails, releasing the node index that the function made
   (is_making_index is 0 until it makes one). Used only there and
   undefined after it. */
#define sbddextended_writeLineOrReturn(str) \
    do { \
        if (!sbddextended_writeLine(str, fp)) { \
            sbddextended_printWriteError(); \
            if (is_making_index) { \
                bddNodeIndex_destruct(node_index); \
                free(node_index); \
            } \
            return; \
        } \
    } while (0)

sbddextended_INLINE_FUNC
void bddexportbddasgraphillion_inner(FILE* fp, bddp f,
                                        bddNodeIndex* node_index, int is_zbdd,
                                        int root_level
#ifdef __cplusplus
                        , const WriteObject& sbddextended_writeLine
#endif
                                    )
{
    int i, k, n;
    size_t j;
    bddp node, child;
    int is_making_index = 0;
    char ss[sbddextended_BUFSIZE];

    /* bddnull is the value the import functions return on an error, not a */
    /* DD. Without this check it would produce an empty output silently, */
    /* which could be taken for a valid result. */
    if (f == bddnull) {
        fprintf(stderr, "The graphillion format cannot represent bddnull.\n");
        return;
    }

    if (is_zbdd < 0 && !(f == bddempty || f == bddsingle)) {
        if (bddiszbdd(f) != 0) {
            is_zbdd = 1;
        } else {
            is_zbdd = 0;
        }
    }

    if (node_index != NULL) {
        if (!bddNodeIndex_checkIndexOf(node_index, f, is_zbdd)) {
            return;
        }
        if (node_index->is_raw != 0) {
            fprintf(stderr, "The node index must not be constructed "
                    "in the raw mode.\n");
            return;
        }
    }

    if (node_index == NULL && !(f == bddempty || f == bddsingle)) {
        is_making_index = 1;
        if (is_zbdd != 0) {
            node_index = bddNodeIndex_makeIndexZWithoutCount(f);
        } else {
            node_index = bddNodeIndex_makeIndexBWithoutCount(f);
        }
    }

#ifdef __cplusplus
    /* The write callback throws instead of returning false when the
       ostream has exceptions enabled, so release the index this function
       made before letting it propagate. */
    try {
#endif
    if (f == bddempty) {
        sbddextended_writeLineOrReturn("B");
        sbddextended_writeLineOrReturn(".");
        return;
    } else if (f == bddsingle) {
        sbddextended_writeLineOrReturn("T");
        sbddextended_writeLineOrReturn(".");
        return;
    }
    if (root_level < 0) {
        root_level = (int)bddlevofvar(bddtop(f));
    } else if (root_level < node_index->height) {
        /* The graphillion level of a node at index level i is
           (root_level - i + 1), so a root_level smaller than the height
           would produce invalid levels smaller than 1. */
        fprintf(stderr, "The argument \"root_level\" must be "
                "larger than or equal to the height of the DD.\n");
        if (is_making_index) {
            bddNodeIndex_destruct(node_index);
            free(node_index);
        }
        return;
    }

    for (i = 1; i <= node_index->height; ++i) {
        for (j = 0; j < node_index->level_vec_arr[i].count; ++j) {
            node = (bddp)sbddextended_MyVector_get(&node_index->level_vec_arr[i], (llint)j);
            n = sbddextended_snprintf2(ss, sbddextended_BUFSIZE, "%lld %d", (llint)node, (root_level - i + 1));
            for (k = 0; k < sbddextended_NUMBER_OF_CHILDREN; ++k) {
                child = bddgetchildg(node, k, is_zbdd, 0);
                if (!bddisterminal(child)) {
                    n += sbddextended_snprintf1(ss + sbddextended_bufPos(n),
                        sbddextended_bufRest(n), " %lld", (llint)child);
                } else if (child == bddempty) {
                    n += sbddextended_snprintf0(ss + sbddextended_bufPos(n),
                        sbddextended_bufRest(n), " B");
                } else if (child == bddsingle) {
                    n += sbddextended_snprintf0(ss + sbddextended_bufPos(n),
                        sbddextended_bufRest(n), " T");
                }
            }
            sbddextended_writeLineOrReturn(ss);
        }
    }

    sbddextended_writeLineOrReturn(".");

    if (is_making_index) {
        bddNodeIndex_destruct(node_index);
        free(node_index);
    }
#ifdef __cplusplus
    } catch (...) {
        if (is_making_index) {
            bddNodeIndex_destruct(node_index);
            free(node_index);
        }
        throw;
    }
#endif
}

#undef sbddextended_writeLineOrReturn

#ifdef __cplusplus

template <typename T>
sbddextended_INLINE_FUNC
void exportBDDAsGraphillion(FILE* fp, const BDD& bdd, int root_level, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(false, true, NULL);
    bddexportbddasgraphillion_inner(fp, bdd.GetID(), bnode_index, 0, root_level, wo);
}

sbddextended_INLINE_FUNC
void exportBDDAsGraphillion(FILE* fp, const BDD& bdd, int root_level = -1)
{
    exportBDDAsGraphillion<int>(fp, bdd, root_level, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportBDDAsGraphillion(std::ostream& ost, const BDD& bdd, int root_level, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(true, true, &ost);
    bddexportbddasgraphillion_inner(NULL, bdd.GetID(), bnode_index, 0, root_level, wo);
}

sbddextended_INLINE_FUNC
void exportBDDAsGraphillion(std::ostream& ost, const BDD& bdd, int root_level = -1)
{
    exportBDDAsGraphillion<int>(ost, bdd, root_level, NULL);
}

template<typename T>
sbddextended_INLINE_FUNC
void exportZBDDAsGraphillion(FILE* fp, const ZBDD& zbdd, int root_level, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(false, true, NULL);
    bddexportbddasgraphillion_inner(fp, zbdd.GetID(), bnode_index, 1, root_level, wo);
}

sbddextended_INLINE_FUNC
void exportZBDDAsGraphillion(FILE* fp, const ZBDD& zbdd, int root_level = -1)
{
    exportZBDDAsGraphillion<int>(fp, zbdd, root_level, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportZBDDAsGraphillion(std::ostream& ost, const ZBDD& zbdd, int root_level, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(true, true, &ost);
    bddexportbddasgraphillion_inner(NULL, zbdd.GetID(), bnode_index, 1, root_level, wo);
}

sbddextended_INLINE_FUNC
void exportZBDDAsGraphillion(std::ostream& ost, const ZBDD& zbdd, int root_level = -1)
{
    exportZBDDAsGraphillion<int>(ost, zbdd, root_level, NULL);
}

sbddextended_INLINE_FUNC
void bddexportbddasgraphillion(FILE* fp, bddp f,
                                bddNodeIndex* node_index, int root_level)
{
    WriteObject wo(false, true, NULL);
    /* can be used for BDD/ZBDD */
    bddexportbddasgraphillion_inner(fp, f, node_index, -1, root_level, wo);
}

sbddextended_INLINE_FUNC
void bddexportzbddasgraphillion(FILE* fp, bddp f,
                                bddNodeIndex* node_index, int root_level)
{
    WriteObject wo(false, true, NULL);
    /* only for ZBDD */
    bddexportbddasgraphillion_inner(fp, f, node_index, 1, root_level, wo);
}

#else

sbddextended_INLINE_FUNC
void bddexportbddasgraphillion(FILE* fp, bddp f,
                                bddNodeIndex* node_index, int root_level)
{
    /* can be used for BDD/ZBDD */
    bddexportbddasgraphillion_inner(fp, f, node_index, -1, root_level);
}

sbddextended_INLINE_FUNC
void bddexportzbddasgraphillion(FILE* fp, bddp f,
                                bddNodeIndex* node_index, int root_level)
{
    /* only for ZBDD */
    bddexportbddasgraphillion_inner(fp, f, node_index, 1, root_level);
}

#endif

/* *************** import functions */

/* Frees the working vectors of bddimportbddasknuth_inner and returns */
/* bddnull from it. Used only there (after all the three vectors are */
/* initialized) and undefined after it. */
#define sbddextended_freeVectorsAndReturnNull() \
    do { \
        sbddextended_MyVector_deinitialize(&hi_vec); \
        sbddextended_MyVector_deinitialize(&lo_vec); \
        sbddextended_MyVector_deinitialize(&level_vec); \
        return bddnull; \
    } while (0)

/* Parses a non-negative integer at *p into *v and advances *p past it.
   Returns 0 unless the number is there and fits in llint. The scanf
   conversions this replaces have undefined behavior when the value does
   not fit in the type of the argument. */
sbddextended_INLINE_FUNC
int bddimportbddasknuth_readnum(const char** p, int is_hex, llint* v)
{
    char* end;
    ullint u;
    const char* s = *p;

    /* strtoull skips leading whitespace and accepts a sign, neither of
       which is allowed here, so require a digit first. */
    if (is_hex != 0) {
        if (!isxdigit((int)(unsigned char)*s)) {
            return 0;
        }
    } else {
        if (!isdigit((int)(unsigned char)*s)) {
            return 0;
        }
    }
    /* strtoull with base 16 also accepts a "0x" prefix, which the */
    /* format does not contain, so parse such a token as the single */
    /* digit 0 followed by the letter x instead. */
    if (is_hex != 0 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        *p = s + 1;
        *v = 0;
        return 1;
    }
    errno = 0;
    end = NULL;
    u = strtoull(s, &end, (is_hex != 0 ? 16 : 10));
    if (end == s || errno == ERANGE || u > (ullint)LLONG_MAX) {
        return 0;
    }
    *p = end;
    *v = (llint)u;
    return 1;
}

/* Parses a node line "<id>:<lo>,<hi>". Returns 0 unless the whole line
   consists of it, so that a trailing token is not silently ignored. */
sbddextended_INLINE_FUNC
int bddimportbddasknuth_readnode(const char* buf, int is_hex,
                                    llint* id, llint* lo, llint* hi)
{
    const char* p = buf;
    if (!bddimportbddasknuth_readnum(&p, is_hex, id) || *p != ':') {
        return 0;
    }
    ++p;
    if (!bddimportbddasknuth_readnum(&p, is_hex, lo) || *p != ',') {
        return 0;
    }
    ++p;
    if (!bddimportbddasknuth_readnum(&p, is_hex, hi)) {
        return 0;
    }
    return sbddextended_isBlankString(p);
}

/* Parses a level header line "#<level>" into *level, which must be in
   [1, INT_MAX]. Returns 0 unless the whole line consists of it. */
sbddextended_INLINE_FUNC
int bddimportbddasknuth_readlevelline(const char* buf, int* level)
{
    const char* p = buf + 1; /* buf[0] is '#' */
    llint v;
    if (!bddimportbddasknuth_readnum(&p, 0, &v)) {
        return 0;
    }
    if (v < 1 || v > (llint)INT_MAX || !sbddextended_isBlankString(p)) {
        return 0;
    }
    *level = (int)v;
    return 1;
}

sbddextended_INLINE_FUNC
bddp bddimportbddasknuth_inner(FILE* fp, int is_hex, int root_level,
                                int is_zbdd
#ifdef __cplusplus
                                , ReadLineObject& sbddextended_readLine
#endif
                                )
{
    int level, level_count = 1, line_status;
    llint i, id, lo, hi, level_end, line_count = 0;
    bddvar var;
    char buf[sbddextended_BUFSIZE];
    bddp p, p0, p1, pf, pfn;
    bddp* bddnode_buf = NULL;
    sbddextended_MyVector level_vec, lo_vec, hi_vec;
#ifdef __cplusplus
    int num_initialized_vecs = 0;
    /* the ids at and above this value in bddnode_buf hold a created
       node that the catch at the end must release; the initial value
       makes the release loop empty until the first node is created */
    llint first_built_id = LLONG_MAX;
#endif

    p0 = p1 = pf = pfn = bddnull;

#ifdef __cplusplus
    /* MyVector_initialize/_add can throw std::bad_alloc, readLine */
    /* throws when the caller gave an istream with exceptions enabled, */
    /* and the DD operations can throw with SAPPOROBDD++, so release */
    /* the working vectors, the node buffer and the DD references owned */
    /* at that moment before letting an exception propagate (an element */
    /* whose initialize call itself threw needs no release because */
    /* initialize puts the element into the deinitialized state before */
    /* allocating). */
    try {
#endif

    sbddextended_MyVector_initialize(&level_vec);
#ifdef __cplusplus
    num_initialized_vecs = 1;
#endif
    sbddextended_MyVector_initialize(&lo_vec);
#ifdef __cplusplus
    num_initialized_vecs = 2;
#endif
    sbddextended_MyVector_initialize(&hi_vec);
#ifdef __cplusplus
    num_initialized_vecs = 3;
#endif

    while ((line_status = sbddextended_readLine(buf, fp)) == 1) {
        ++line_count;
        if (buf[0] == '0' && sbddextended_isBlankString(buf + 1)) {
            sbddextended_MyVector_deinitialize(&hi_vec);
            sbddextended_MyVector_deinitialize(&lo_vec);
            sbddextended_MyVector_deinitialize(&level_vec);
            return bddgetterminal(0, is_zbdd);
        } else if (buf[0] == '1' && sbddextended_isBlankString(buf + 1)) {
            sbddextended_MyVector_deinitialize(&hi_vec);
            sbddextended_MyVector_deinitialize(&lo_vec);
            sbddextended_MyVector_deinitialize(&level_vec);
            return bddgetterminal(1, is_zbdd);
        } else if (buf[0] == '#') {
            if (!bddimportbddasknuth_readlevelline(buf, &level)) {
                fprintf(stderr, "Format error in line %lld\n", line_count);
                sbddextended_freeVectorsAndReturnNull();
            }
            if (level != level_count) {
                fprintf(stderr, "Format error in line %lld: the level "
                        "header must be #%d\n", line_count, level_count);
                sbddextended_freeVectorsAndReturnNull();
            }
            ++level_count;
            sbddextended_MyVector_add(&level_vec, (llint)2);
            break;
        } else {
            /* Everything before the first level header would otherwise */
            /* be skipped silently, so a broken head of the input would */
            /* not be detected. */
            fprintf(stderr, "Format error in line %lld\n", line_count);
            sbddextended_freeVectorsAndReturnNull();
        }
    }
    if (line_status < 0) { /* a read error; a message has been printed */
        sbddextended_freeVectorsAndReturnNull();
    }

    sbddextended_MyVector_add(&lo_vec, 0ll);
    sbddextended_MyVector_add(&lo_vec, 1ll);
    sbddextended_MyVector_add(&hi_vec, 0ll);
    sbddextended_MyVector_add(&hi_vec, 1ll);

    while ((line_status = sbddextended_readLine(buf, fp)) == 1) {
        ++line_count;
        if (buf[0] == '#') {
            if (!bddimportbddasknuth_readlevelline(buf, &level)) {
                fprintf(stderr, "Format error in line %lld\n", line_count);
                sbddextended_freeVectorsAndReturnNull();
            }
            if (level != level_count) {
                fprintf(stderr, "Format error in line %lld: the level "
                        "header must be #%d\n", line_count, level_count);
                sbddextended_freeVectorsAndReturnNull();
            }
            /* The height of the DD (level_count - 1 after the */
            /* increment below) can never exceed the largest level of */
            /* SAPPOROBDD (root_level, at least the height, is checked */
            /* against it below), so reject a larger header as soon as */
            /* it is read. Checking before the increment also keeps */
            /* level_count away from the int overflow on a stream of */
            /* billions of header lines, and stops reading such an */
            /* input early. */
            if (level_count > sbddextended_varMaxAsInt()) {
                fprintf(stderr, "The number of levels must be at most "
                        "%d.\n", sbddextended_varMaxAsInt());
                sbddextended_freeVectorsAndReturnNull();
            }
            ++level_count;
            sbddextended_MyVector_add(&level_vec, (llint)lo_vec.count);
        } else {
            if (!bddimportbddasknuth_readnode(buf, is_hex, &id, &lo, &hi)) {
                fprintf(stderr, "Format error in line %lld\n", line_count);
                sbddextended_freeVectorsAndReturnNull();
            }
            if (id != (llint)lo_vec.count) {
                fprintf(stderr, "Format error in line %lld\n",
                        line_count);
                sbddextended_freeVectorsAndReturnNull();
            }
            sbddextended_MyVector_add(&lo_vec, lo);
            sbddextended_MyVector_add(&hi_vec, hi);
        }
    }
    if (line_status < 0) { /* a read error; a message has been printed */
        sbddextended_freeVectorsAndReturnNull();
    }
    sbddextended_MyVector_add(&level_vec, (llint)lo_vec.count);
    /* level_vec holds one entry per level header line and one more added */
    /* just above, and the checks of the header lines guarantee that the */
    /* headers are #1, #2, ..., so the following is an invariant of this */
    /* function rather than a check of the input. */
    assert(level_count == (int)level_vec.count);

    /* The input contains no node (the file is empty or consists only of */
    /* level header lines). Note that the root node must exist at the */
    /* index sbddextended_BDDNODE_START of bddnode_buf. */
    if ((llint)lo_vec.count <= sbddextended_BDDNODE_START) {
        fprintf(stderr, "Unexpected end of the input.\n");
        sbddextended_freeVectorsAndReturnNull();
    }

    /* Check the structure before any variable is added below. */
    /* SAPPOROBDD has no way to remove a variable, so a file that turns */
    /* out to be malformed would otherwise leave behind the variables */
    /* that were added for it. A child must be a terminal or a node in a */
    /* later level block, that is, one at a strictly lower level. */
    /* Requiring only an id larger than that of the current node would */
    /* let a node point at another node of its own level, which the */
    /* construction below cannot represent, so the import would return */
    /* a wrong DD instead of rejecting the input. */
    for (level = 1; level < (int)level_vec.count; ++level) {
        level_end = sbddextended_MyVector_get(&level_vec, (llint)level);
        for (i = sbddextended_MyVector_get(&level_vec, (llint)level - 1);
                i < level_end; ++i) {
            lo = sbddextended_MyVector_get(&lo_vec, i);
            hi = sbddextended_MyVector_get(&hi_vec, i);
            if (!(0 <= lo && lo < sbddextended_BDDNODE_START) &&
                    !(level_end <= lo && lo < (llint)lo_vec.count)) {
                fprintf(stderr, "The 0-child (%lld) of the node %lld must "
                        "be a terminal or a node at a lower level.\n",
                        lo, i);
                sbddextended_freeVectorsAndReturnNull();
            }
            if (!(0 <= hi && hi < sbddextended_BDDNODE_START) &&
                    !(level_end <= hi && hi < (llint)lo_vec.count)) {
                fprintf(stderr, "The 1-child (%lld) of the node %lld must "
                        "be a terminal or a node at a lower level.\n",
                        hi, i);
                sbddextended_freeVectorsAndReturnNull();
            }
        }
    }

    if (root_level < 0) {
        root_level = level_count - 1;
    } else if (root_level < level_count - 1) {
        /* This importer is shared by the BDD and the ZBDD versions, */
        /* so the message says "DD". */
        fprintf(stderr, "The argument \"root_level\" must be "
                "larger than or equal to the height of the DD.\n");
        sbddextended_freeVectorsAndReturnNull();
    }
    /* The levels are turned into variables with bddnewvar below, which */
    /* terminates the process once the variable index range of SAPPOROBDD */
    /* is full, so reject a level that it cannot represent before any */
    /* variable is added. */
    if (root_level > sbddextended_varMaxAsInt()) {
        fprintf(stderr, "The level of the root must be at most %d.\n",
                sbddextended_varMaxAsInt());
        sbddextended_freeVectorsAndReturnNull();
    }

    /* Allocate before the variables are added below: the size comes
       from the number of node lines of the input, so a hostile input
       can ask for a buffer that no memory can hold, and that is an
       error of this import rather than a reason to end the process of
       the library user (or to leave the added variables behind, which
       SAPPOROBDD cannot remove). */
    bddnode_buf = (bddp*)malloc(lo_vec.count * sizeof(bddp));
    if (bddnode_buf == NULL) {
        fprintf(stderr, "Cannot allocate memory for the nodes of the "
                "input.\n");
        sbddextended_freeVectorsAndReturnNull();
    }

    while (bddvarused() < (bddvar)root_level) {
        bddnewvar();
    }

    bddnode_buf[0] = bddgetterminal(0, is_zbdd);
    bddnode_buf[1] = bddgetterminal(1, is_zbdd);

    /* The entries of level_vec are non-decreasing and partition the */
    /* node ids into the level blocks, so as i decreases below, the */
    /* level of i can only decrease. Walking it down instead of */
    /* searching from the head for every node makes the loop */
    /* O(nodes + levels) instead of O(nodes * levels). */
    level = (int)level_vec.count - 1;
    for (i = (llint)lo_vec.count - 1; i >= sbddextended_BDDNODE_START; --i) {
        while (level > 1 &&
                i < sbddextended_MyVector_get(&level_vec, (llint)level - 1)) {
            --level;
        }
        /* The last entry of level_vec is lo_vec.count, so the level of */
        /* i is always found, and root_level >= level_count - 1 >= level */
        /* has been checked, so the following are invariants rather */
        /* than checks of the input. */
        assert(sbddextended_MyVector_get(&level_vec, (llint)level - 1) <= i
                && i < sbddextended_MyVector_get(&level_vec, (llint)level));
        assert((1 <= root_level - level + 1) && ((root_level - level + 1) <= (int)bddvarused()));
        lo = sbddextended_MyVector_get(&lo_vec, i);
        hi = sbddextended_MyVector_get(&hi_vec, i);
        /* The ranges of lo and hi have been checked above. */
        assert((0 <= lo && lo < sbddextended_BDDNODE_START)
                || (i < lo && lo < (llint)lo_vec.count));
        assert((0 <= hi && hi < sbddextended_BDDNODE_START)
                || (i < hi && hi < (llint)lo_vec.count));
        var = bddvaroflev((bddvar)(root_level - level + 1));
        /* The owned intermediate references are held in pf/pfn/p0/p1
           and each is reset to bddnull as soon as it is released, so
           that the catch below releases exactly the ones owned at the
           moment an operation throws. */
        if (is_zbdd == 0) { /* BDD */
            pf = bddprime(var);
            pfn = bddnot(pf);
            p0 = bddand(bddnode_buf[lo], pfn);
            p1 = bddand(bddnode_buf[hi], pf);
            bddnode_buf[i] = bddor(p0, p1);
#ifdef __cplusplus
            first_built_id = i;
#endif
            bddfree(pf);
            pf = bddnull;
            bddfree(pfn);
            pfn = bddnull;
            bddfree(p0);
            p0 = bddnull;
            bddfree(p1);
            p1 = bddnull;
        } else { /* ZDD */
            p1 = bddchange(bddnode_buf[hi], var);
            bddnode_buf[i] = bddunion(bddnode_buf[lo], p1);
#ifdef __cplusplus
            first_built_id = i;
#endif
            bddfree(p1);
            p1 = bddnull;
        }
    }
    for (i = (llint)lo_vec.count - 1;
            i >= sbddextended_BDDNODE_START + 1; --i) {
        bddfree(bddnode_buf[i]);
    }

    p = bddnode_buf[sbddextended_BDDNODE_START];

    free(bddnode_buf);

    sbddextended_MyVector_deinitialize(&hi_vec);
    sbddextended_MyVector_deinitialize(&lo_vec);
    sbddextended_MyVector_deinitialize(&level_vec);

    return p;

#ifdef __cplusplus
    } catch (...) {
        if (bddnode_buf != NULL) {
            for (i = first_built_id; i < (llint)lo_vec.count; ++i) {
                bddfree(bddnode_buf[i]);
            }
            free(bddnode_buf);
        }
        bddfree(p1);
        bddfree(p0);
        bddfree(pfn);
        bddfree(pf);
        if (num_initialized_vecs >= 3) {
            sbddextended_MyVector_deinitialize(&hi_vec);
        }
        if (num_initialized_vecs >= 2) {
            sbddextended_MyVector_deinitialize(&lo_vec);
        }
        if (num_initialized_vecs >= 1) {
            sbddextended_MyVector_deinitialize(&level_vec);
        }
        throw;
    }
#endif
}

#undef sbddextended_freeVectorsAndReturnNull

#ifdef __cplusplus

sbddextended_INLINE_FUNC
BDD importBDDAsKnuth(FILE* fp, bool is_hex, int root_level = -1)
{
    ReadLineObject glo;
    bddp p;
    p = bddimportbddasknuth_inner(fp, (is_hex ? 1 : 0),
                                            root_level, 0, glo);
    return BDD_ID(p);
}

sbddextended_INLINE_FUNC
BDD importBDDAsKnuth(std::istream& ist, bool is_hex, int root_level = -1)
{
    ReadLineObject glo(&ist);
    bddp p;
    p = bddimportbddasknuth_inner(NULL, (is_hex ? 1 : 0),
                                            root_level, 0, glo);
    return BDD_ID(p);
}

sbddextended_INLINE_FUNC
ZBDD importZBDDAsKnuth(FILE* fp, bool is_hex, int root_level = -1)
{
    ReadLineObject glo;
    bddp p;
    p = bddimportbddasknuth_inner(fp, (is_hex ? 1 : 0),
                                            root_level, 1, glo);
    return ZBDD_ID(p);
}

sbddextended_INLINE_FUNC
ZBDD importZBDDAsKnuth(std::istream& ist, bool is_hex, int root_level = -1)
{
    ReadLineObject glo(&ist);
    bddp p;
    p = bddimportbddasknuth_inner(NULL, (is_hex ? 1 : 0),
                                            root_level, 1, glo);
    return ZBDD_ID(p);
}

sbddextended_INLINE_FUNC
bddp bddimportbddasknuth(FILE* fp, int is_hex, int root_level = -1)
{
    ReadLineObject glo;
    return bddimportbddasknuth_inner(fp, is_hex, root_level, 0, glo);
}

sbddextended_INLINE_FUNC
bddp bddimportzbddasknuth(FILE* fp, int is_hex, int root_level = -1)
{
    ReadLineObject glo;
    return bddimportbddasknuth_inner(fp, is_hex, root_level, 1, glo);
}

#else

sbddextended_INLINE_FUNC
bddp bddimportbddasknuth(FILE* fp, int is_hex, int root_level)
{
    return bddimportbddasknuth_inner(fp, is_hex, root_level, 0);
}

sbddextended_INLINE_FUNC
bddp bddimportzbddasknuth(FILE* fp, int is_hex, int root_level)
{
    return bddimportbddasknuth_inner(fp, is_hex, root_level, 1);
}

#endif


/* *************** export functions */


/* Writes one line and returns from bddexportbddasknuth_inner when the
   write fails, releasing the node index that the function made
   (is_making_index is 0 until it makes one). Used only there and
   undefined after it. */
#define sbddextended_writeLineOrReturn(str) \
    do { \
        if (!sbddextended_writeLine(str, fp)) { \
            sbddextended_printWriteError(); \
            if (is_making_index) { \
                bddNodeIndex_destruct(node_index); \
                free(node_index); \
            } \
            return; \
        } \
    } while (0)

sbddextended_INLINE_FUNC
void bddexportbddasknuth_inner(FILE* fp, bddp f, int is_hex,
                                bddNodeIndex* node_index,
                                int is_zbdd
#ifdef __cplusplus
                                    , const WriteObject& sbddextended_writeLine
#endif
                                    )
{
    int clevel, i, is_making_index = 0;
    bddp node, n0, n1;
    llint id0, id1, k;
    char ss[sbddextended_BUFSIZE];

    /* bddnull is the value the import functions return on an error, not a */
    /* DD. Without this check it would produce an empty output silently, */
    /* which could be taken for a valid result. */
    if (f == bddnull) {
        fprintf(stderr, "The Knuth format cannot represent bddnull.\n");
        return;
    }

    if (f == bddempty) {
        sbddextended_writeLineOrReturn("0");
        return;
    } else if (f == bddsingle) {
        sbddextended_writeLineOrReturn("1");
        return;
    }

    if (is_zbdd < 0 && !(f == bddempty || f == bddsingle)) {
        if (bddiszbdd(f) != 0) {
            is_zbdd = 1;
        } else {
            is_zbdd = 0;
        }
    }

    if (node_index != NULL) {
        if (!bddNodeIndex_checkIndexOf(node_index, f, is_zbdd)) {
            return;
        }
        if (node_index->is_raw != 0) {
            fprintf(stderr, "The node index must not be constructed "
                    "in the raw mode.\n");
            return;
        }
    }

    if (node_index == NULL && !(f == bddempty || f == bddsingle)) {
        is_making_index = 1;
        if (is_zbdd != 0) {
            node_index = bddNodeIndex_makeIndexZWithoutCount(f);
        } else {
            node_index = bddNodeIndex_makeIndexBWithoutCount(f);
        }
    }

#ifdef __cplusplus
    /* The write callback throws instead of returning false when the
       ostream has exceptions enabled, so release the index this function
       made before letting it propagate. */
    try {
#endif
    for (i = node_index->height; i >= 1; --i) {
        sbddextended_snprintf1(ss, sbddextended_BUFSIZE, "#%d", node_index->height - i + 1);
        sbddextended_writeLineOrReturn(ss);
        for (k = 0; k < (llint)node_index->level_vec_arr[i].count; ++k) {
            node = (bddp)sbddextended_MyVector_get(&node_index->level_vec_arr[i], k);
            n0 = bddgetchild0g(node, is_zbdd, 0);
            if (n0 == bddempty) {
                id0 = 0;
            } else if (n0 == bddsingle) {
                id0 = 1;
            } else {
                clevel = (int)bddgetlev(n0);
                if (sbddextended_MyDict_find(&node_index->node_dict_arr[clevel],
                                                (llint)n0, &id0) == 0) {
                    fprintf(stderr, "node not found!\n");
                    exit(1);
                }
                id0 += node_index->offset_arr[clevel];
            }
            n1 = bddgetchild1g(node, is_zbdd, 0);
            if (n1 == bddempty) {
                id1 = 0;
            } else if (n1 == bddsingle) {
                id1 = 1;
            } else {
                clevel = (int)bddgetlev(n1);
                if (sbddextended_MyDict_find(&node_index->node_dict_arr[clevel],
                                                (llint)n1, &id1) == 0) {
                    fprintf(stderr, "node not found!\n");
                    exit(1);
                }
                id1 += node_index->offset_arr[clevel];
            }
            if (is_hex) {
                sbddextended_snprintf3(ss, sbddextended_BUFSIZE,
                    "%llx:%llx,%llx",
                    (ullint)(node_index->offset_arr[i] + k),
                    (ullint)id0, (ullint)id1);
            } else {
                sbddextended_snprintf3(ss, sbddextended_BUFSIZE,
                    "%lld:%lld,%lld", node_index->offset_arr[i] + k,
                    id0, id1);
            }
            sbddextended_writeLineOrReturn(ss);
        }
    }
    if (is_making_index) {
        bddNodeIndex_destruct(node_index);
        free(node_index);
    }
#ifdef __cplusplus
    } catch (...) {
        if (is_making_index) {
            bddNodeIndex_destruct(node_index);
            free(node_index);
        }
        throw;
    }
#endif
}

#undef sbddextended_writeLineOrReturn

#ifdef __cplusplus

template <typename T>
sbddextended_INLINE_FUNC
void exportBDDAsKnuth(FILE* fp, const BDD& bdd, bool is_hex, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(false, true, NULL);
    bddexportbddasknuth_inner(fp, bdd.GetID(), (is_hex ? 1 : 0), bnode_index, 0, wo);
}

sbddextended_INLINE_FUNC
void exportBDDAsKnuth(FILE* fp, const BDD& bdd, bool is_hex = false)
{
    exportBDDAsKnuth<int>(fp, bdd, is_hex, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportBDDAsKnuth(std::ostream& ost, const BDD& bdd, bool is_hex, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(true, true, &ost);
    bddexportbddasknuth_inner(NULL, bdd.GetID(), (is_hex ? 1 : 0), bnode_index, 0, wo);
}

sbddextended_INLINE_FUNC
void exportBDDAsKnuth(std::ostream& ost, const BDD& bdd, bool is_hex = false)
{
    exportBDDAsKnuth<int>(ost, bdd, is_hex, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportZBDDAsKnuth(FILE* fp, const ZBDD& zbdd, bool is_hex, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(false, true, NULL);
    bddexportbddasknuth_inner(fp, zbdd.GetID(), (is_hex ? 1 : 0), bnode_index, 1, wo);
}

sbddextended_INLINE_FUNC
void exportZBDDAsKnuth(FILE* fp, const ZBDD& zbdd, bool is_hex = false)
{
    exportZBDDAsKnuth<int>(fp, zbdd, is_hex, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportZBDDAsKnuth(std::ostream& ost, const ZBDD& zbdd, bool is_hex, DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(true, true, &ost);
    bddexportbddasknuth_inner(NULL, zbdd.GetID(), (is_hex ? 1 : 0), bnode_index, 1, wo);
}

sbddextended_INLINE_FUNC
void exportZBDDAsKnuth(std::ostream& ost, const ZBDD& zbdd, bool is_hex = false)
{
    exportZBDDAsKnuth<int>(ost, zbdd, is_hex, NULL);
}

sbddextended_INLINE_FUNC
void bddexportbddasknuth(FILE* fp, bddp f, int is_hex, bddNodeIndex* node_index)
{
    WriteObject wo(false, true, NULL);
    bddexportbddasknuth_inner(fp, f, (is_hex ? 1 : 0), node_index, 0, wo);
}

sbddextended_INLINE_FUNC
void bddexportzbddasknuth(FILE* fp, bddp f, int is_hex, bddNodeIndex* node_index)
{
    WriteObject wo(false, true, NULL);
    bddexportbddasknuth_inner(fp, f, (is_hex ? 1 : 0), node_index, 1, wo);
}

#else

sbddextended_INLINE_FUNC
void bddexportbddasknuth(FILE* fp, bddp f, int is_hex, bddNodeIndex* node_index)
{
    bddexportbddasknuth_inner(fp, f, is_hex, node_index, 0);
}

sbddextended_INLINE_FUNC
void bddexportzbddasknuth(FILE* fp, bddp f, int is_hex, bddNodeIndex* node_index)
{
    bddexportbddasknuth_inner(fp, f, is_hex, node_index, 1);
}

#endif


/* Writes one line and returns from bddexportbddasgraphviz_inner when
   the write fails, releasing the node index that the function made
   (is_making_index is 0 until it makes one). Used only there and
   undefined after it. */
#define sbddextended_writeLineOrReturn(str) \
    do { \
        if (!sbddextended_writeLine(str, fp)) { \
            sbddextended_printWriteError(); \
            if (is_making_index) { \
                bddNodeIndex_destruct(node_index); \
                free(node_index); \
            } \
            return; \
        } \
    } while (0)

sbddextended_INLINE_FUNC
void bddexportbddasgraphviz_inner(FILE* fp, bddp f,
                                    bddNodeIndex* node_index, int is_zbdd
#ifdef __cplusplus
                        , const WriteObject& sbddextended_writeLine
#endif
                                    )
{
    int i, k, c, clevel, n;
    llint cvalue;
    size_t j;
    bddp node, child;
    int is_making_index = 0;
    int zero_reachable, one_reachable;
    char ss[sbddextended_BUFSIZE];

    /* bddnull is the value the import functions return on an error, not a */
    /* DD. Without this check it would produce an empty output silently, */
    /* which could be taken for a valid result. */
    if (f == bddnull) {
        fprintf(stderr, "The Graphviz output cannot represent bddnull.\n");
        return;
    }

    if (is_zbdd < 0 && !(f == bddempty || f == bddsingle)) {
        if (bddiszbdd(f) != 0) {
            is_zbdd = 1;
        } else {
            is_zbdd = 0;
        }
    }

    /* The node dictionary of a raw index holds the nodes with their */
    /* negative arcs, while the children are taken below without them. */
    if (node_index != NULL) {
        if (!bddNodeIndex_checkIndexOf(node_index, f, is_zbdd)) {
            return;
        }
        if (node_index->is_raw != 0) {
            fprintf(stderr, "The node index must not be constructed "
                    "in the raw mode.\n");
            return;
        }
    }

    if (node_index == NULL && !(f == bddempty || f == bddsingle)) {
        is_making_index = 1;
        if (is_zbdd != 0) {
            node_index = bddNodeIndex_makeIndexZWithoutCount(f);
        } else {
            node_index = bddNodeIndex_makeIndexBWithoutCount(f);
        }
    }

#ifdef __cplusplus
    /* The child accesses below and the write callback throw instead of
       returning an error when the caller gave an istream with
       exceptions enabled or uses SAPPOROBDD++, so release the index
       this function made before letting the exception propagate. */
    try {
#endif

    /* Whether an arc of the DD reaches each terminal. A terminal that */
    /* no arc reaches (e.g. the 0-terminal of the ZBDD {{},{1},{2}}) */
    /* is not drawn; its box would be an isolated object in the figure. */
    zero_reachable = 0;
    one_reachable = 0;
    if (!(f == bddempty || f == bddsingle)) {
        for (i = node_index->height;
                i >= 1 && !(zero_reachable && one_reachable); --i) {
            for (j = 0; j < node_index->level_vec_arr[i].count; ++j) {
                node = (bddp)sbddextended_MyVector_get(
                    &node_index->level_vec_arr[i], (llint)j);
                for (k = 0; k < sbddextended_NUMBER_OF_CHILDREN; ++k) {
                    if (is_zbdd != 0) {
                        child = bddgetchildz(node, k);
                    } else {
                        child = bddgetchildb(node, k);
                    }
                    if (child == bddfalse) {
                        zero_reachable = 1;
                    } else if (child == bddtrue) {
                        one_reachable = 1;
                    }
                }
            }
        }
    }

    sbddextended_writeLineOrReturn("digraph {");
    /* print terminals (only those that the DD actually reaches) */
    if (f == bddempty || zero_reachable) {
        sbddextended_writeLineOrReturn("\tt0 [label = \"0\", shape = box, "
            "style = filled, color = \"#81B65D\", "
            "fillcolor = \"#F6FAF4\", penwidth = 2.5, "
            "width = 0.4, height = 0.6, fontsize = 24];");
    }
    if (f == bddsingle || one_reachable) {
        sbddextended_writeLineOrReturn("\tt1 [label = \"1\", shape = box, "
            "style = filled, color = \"#81B65D\", "
            "fillcolor = \"#F6FAF4\", penwidth = 2.5, width = 0.4, "
            "height = 0.6, fontsize = 24];");
    }
    if (f == bddempty || f == bddsingle) {
        sbddextended_writeLineOrReturn("}");
        return;
    }

    /* print vars and levels */
    sbddextended_snprintf1(ss, sbddextended_BUFSIZE, 
        "\tr%d [shape = plaintext, label = \"var level\"]",
        node_index->height + 1);
    sbddextended_writeLineOrReturn(ss);
    sbddextended_snprintf3(ss, sbddextended_BUFSIZE,
        "\tr%d [shape = plaintext, label = \"%4u%7d\"]",
        node_index->height, bddvaroflev((bddvar)node_index->height),
        node_index->height);
    sbddextended_writeLineOrReturn(ss);
    sbddextended_snprintf2(ss, sbddextended_BUFSIZE,
        "\tr%d -> r%d [style = invis];", node_index->height + 1,
        node_index->height);
    sbddextended_writeLineOrReturn(ss);
    for (i = node_index->height; i >= 1; --i) {
        if (i > 1) {
            sbddextended_snprintf3(ss, sbddextended_BUFSIZE,
                "\tr%d [shape = plaintext, label = \"%4u%7d\"];",
                i - 1, bddvaroflev((bddvar)(i - 1)), i - 1);
            sbddextended_writeLineOrReturn(ss);
        } else {
            sbddextended_writeLineOrReturn("\tr0 [style = invis];");
        }
        sbddextended_snprintf2(ss, sbddextended_BUFSIZE,
            "\tr%d -> r%d [style = invis];", i, i - 1);
        sbddextended_writeLineOrReturn(ss);
    }

    for (i = node_index->height; i >= 1; --i) {
        for (j = 0; j < node_index->level_vec_arr[i].count; ++j) {
            node = (bddp)sbddextended_MyVector_get(
                &node_index->level_vec_arr[i], (llint)j);
            sbddextended_snprintf2(ss, sbddextended_BUFSIZE,
                "\tv%d_%lld [shape = circle, style = filled, "
                "color = \"#81B65D\", fillcolor = \"#F6FAF4\", "
                "penwidth = 2.5, label = \"\"];", i, (llint)j);
            sbddextended_writeLineOrReturn(ss);
            for (k = 0; k < sbddextended_NUMBER_OF_CHILDREN; ++k) {
                if (is_zbdd != 0) {
                    child = bddgetchildz(node, k);
                } else {
                    child = bddgetchildb(node, k);
                }
                if (!bddisterminal(child)) {
                    clevel = (int)bddgetlev(child);
                    c = sbddextended_MyDict_find(&node_index->node_dict_arr[clevel],
                                                    (llint)child, &cvalue);
                    /* Not an assert: without the check, a build with
                       NDEBUG would print the uninitialized cvalue.
                       The other exporters report a node that the index
                       does not hold the same way. */
                    if (c == 0) {
                        fprintf(stderr, "node not found!\n");
                        exit(1);
                    }

                    n = sbddextended_snprintf4(ss, sbddextended_BUFSIZE,
                        "\tv%d_%lld -> v%d_%lld", i, (llint)j,
                        clevel, cvalue);
                    n += sbddextended_snprintf0(ss + sbddextended_bufPos(n),
                        sbddextended_bufRest(n),
                    " [color = \"#81B65D\", penwidth = 2.5");
                    if (k == 0) {
                        n += sbddextended_snprintf0(
                            ss + sbddextended_bufPos(n),
                            sbddextended_bufRest(n), ", style = dotted");
                    }
                    sbddextended_snprintf0(ss + sbddextended_bufPos(n),
                        sbddextended_bufRest(n), "];");
                    sbddextended_writeLineOrReturn(ss);
                } else {
                    n = sbddextended_snprintf3(ss, sbddextended_BUFSIZE,
                        "\tv%d_%lld -> t%d", i, (llint)j,
                        (child == bddfalse ? 0 : 1));
                    n += sbddextended_snprintf0(ss + sbddextended_bufPos(n),
                        sbddextended_bufRest(n),
                        " [color = \"#81B65D\", penwidth = 2.5");
                    if (k == 0) {
                        n += sbddextended_snprintf0(
                            ss + sbddextended_bufPos(n),
                            sbddextended_bufRest(n), ", style = dotted");
                    }
                    sbddextended_snprintf0(ss + sbddextended_bufPos(n),
                        sbddextended_bufRest(n), "];");
                    sbddextended_writeLineOrReturn(ss);
                }
            }
        }
        n = sbddextended_snprintf1(ss, sbddextended_BUFSIZE,
            "\t{rank = same; r%d; ", i);
        for (j = 0; j < node_index->level_vec_arr[i].count; ++j) {
            n += sbddextended_snprintf2(ss + sbddextended_bufPos(n),
                sbddextended_bufRest(n), "v%d_%lld; ", i, (llint)j);
            if (j % 10 == 9 && j < node_index->level_vec_arr[i].count - 1) {
                sbddextended_writeLineOrReturn(ss);
                n = sbddextended_snprintf0(ss, sbddextended_BUFSIZE, "\t\t");
            }
        }
        n += sbddextended_snprintf0(ss + sbddextended_bufPos(n),
            sbddextended_bufRest(n), "}");
        sbddextended_writeLineOrReturn(ss);
    }

    n = sbddextended_snprintf0(ss, sbddextended_BUFSIZE,
        "\t{rank = same; r0; ");
    if (zero_reachable) {
        n += sbddextended_snprintf0(ss + sbddextended_bufPos(n),
            sbddextended_bufRest(n), "t0; ");
    }
    if (one_reachable) {
        n += sbddextended_snprintf0(ss + sbddextended_bufPos(n),
            sbddextended_bufRest(n), "t1; ");
    }
    sbddextended_snprintf0(ss + sbddextended_bufPos(n),
        sbddextended_bufRest(n), "}");
    sbddextended_writeLineOrReturn(ss);
    sbddextended_writeLineOrReturn("}");

    if (is_making_index) {
        bddNodeIndex_destruct(node_index);
        free(node_index);
    }
#ifdef __cplusplus
    } catch (...) {
        if (is_making_index) {
            bddNodeIndex_destruct(node_index);
            free(node_index);
        }
        throw;
    }
#endif
}

#undef sbddextended_writeLineOrReturn

#ifdef __cplusplus

template <typename T>
sbddextended_INLINE_FUNC
void exportBDDAsGraphviz(FILE* fp, const BDD& bdd,
                            std::map<std::string, std::string>* /*option*/,
                            DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(false, true, NULL);
    bddexportbddasgraphviz_inner(fp, bdd.GetID(), bnode_index, 0, wo);
}

sbddextended_INLINE_FUNC
void exportBDDAsGraphviz(FILE* fp, const BDD& bdd,
                            std::map<std::string, std::string>* option = NULL)
{
    exportBDDAsGraphviz<int>(fp, bdd, option, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportBDDAsGraphviz(std::ostream& ost, const BDD& bdd,
                            std::map<std::string, std::string>* /*option*/,
                            DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(true, true, &ost);
    bddexportbddasgraphviz_inner(NULL, bdd.GetID(), bnode_index, 0, wo);
}

sbddextended_INLINE_FUNC
void exportBDDAsGraphviz(std::ostream& ost, const BDD& bdd,
                            std::map<std::string, std::string>* option = NULL)
{
    exportBDDAsGraphviz<int>(ost, bdd, option, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportZBDDAsGraphviz(FILE* fp, const ZBDD& zbdd,
                            std::map<std::string, std::string>* /*option*/,
                            DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(false, true, NULL);
    bddexportbddasgraphviz_inner(fp, zbdd.GetID(), bnode_index, 1, wo);
}

sbddextended_INLINE_FUNC
void exportZBDDAsGraphviz(FILE* fp, const ZBDD& zbdd,
                            std::map<std::string, std::string>* option = NULL)
{
    exportZBDDAsGraphviz<int>(fp, zbdd, option, NULL);
}

template <typename T>
sbddextended_INLINE_FUNC
void exportZBDDAsGraphviz(std::ostream& ost, const ZBDD& zbdd,
                            std::map<std::string, std::string>* /*option*/,
                            DDIndex<T>* node_index)
{
    bddNodeIndex* bnode_index = NULL;
    if (node_index != NULL) {
        bnode_index = node_index->getRawPointer();
        if (bnode_index == NULL) {
            /* an index built from bddnull or one that has been */
            /* cleared; the SVG exporter reports this the same way */
            fprintf(stderr, "The given index does not hold a DD.\n");
            return;
        }
    }
    WriteObject wo(true, true, &ost);
    bddexportbddasgraphviz_inner(NULL, zbdd.GetID(), bnode_index, 1, wo);
}

sbddextended_INLINE_FUNC
void exportZBDDAsGraphviz(std::ostream& ost, const ZBDD& zbdd,
                            std::map<std::string, std::string>* option = NULL)
{
    exportZBDDAsGraphviz<int>(ost, zbdd, option, NULL);
}

sbddextended_INLINE_FUNC
void bddexportbddasgraphviz(FILE* fp, bddp f,
                            bddNodeIndex* node_index)
{
    WriteObject wo(false, true, NULL);
    /* can be used for BDD/ZBDD */
    bddexportbddasgraphviz_inner(fp, f, node_index, -1, wo);
}

sbddextended_INLINE_FUNC
void bddexportzbddasgraphviz(FILE* fp, bddp f,
                                bddNodeIndex* node_index)
{
    WriteObject wo(false, true, NULL);
    /* only for ZBDD */
    bddexportbddasgraphviz_inner(fp, f, node_index, 1, wo);
}

#else

sbddextended_INLINE_FUNC
void bddexportbddasgraphviz(FILE* fp, bddp f,
                            bddNodeIndex* node_index)
{
    /* can be used for BDD/ZBDD */
    bddexportbddasgraphviz_inner(fp, f, node_index, -1);
}

sbddextended_INLINE_FUNC
void bddexportzbddasgraphviz(FILE* fp, bddp f,
                            bddNodeIndex* node_index)
{
    /* only for ZBDD */
    bddexportbddasgraphviz_inner(fp, f, node_index, 1);
}

#endif


#ifdef __cplusplus /* currently svg is supported only in C++ version */

struct ExportAsSvg_arcinfo {
public:
    bddp f;
    int arc; /* 0 or 1-arc */
    int fposx;
    int fposy;

    ExportAsSvg_arcinfo(bddp ff, int a, int fpx, int fpy)
        : f(ff), arc(a), fposx(fpx), fposy(fpy) { }
};

sbddextended_INLINE_FUNC
bool ExportAsSvg_arcinfo_compare(const ExportAsSvg_arcinfo& a1,
                                    const ExportAsSvg_arcinfo& a2)
{
    return a1.fposx < a2.fposx;
}

sbddextended_INLINE_FUNC
int ExportAsSvg_getCirclePosX(int x, int r, double rad)
{
    return x + static_cast<int>(r * cos(rad));
}

sbddextended_INLINE_FUNC
int ExportAsSvg_getCirclePosY(int y, int r, double rad)
{
    return y - static_cast<int>(r * sin(rad));
}

/* Writes one line and returns from bddexportassvg_inner when the write
   fails, releasing the index that the function made. Used only there
   and undefined after it. */
#define sbddextended_writeLineOrReturn(str) \
    do { \
        if (!sbddextended_writeLine(str, fp)) { \
            sbddextended_printWriteError(); \
            if (is_made) { \
                delete index; \
            } \
            return; \
        } \
    } while (0)

/* Reads the j-th node of the level from the index. index->getNode
   would do, but it makes a DDNode<T>, whose construction default
   constructs a T for the node in the storage of the index; the
   exporter needs only the bddp, and must not leave that side effect
   (memory, and entries in getStorageRef) on an index the caller
   passed in. */
template<typename T>
sbddextended_INLINE_FUNC bddp ExportAsSvg_getBddp(DDIndex<T>* index,
                                                  int level, ullint j)
{
    return static_cast<bddp>(sbddextended_MyVector_get(
        &index->getRawPointer()->level_vec_arr[level],
        static_cast<llint>(j)));
}

template<typename T>
sbddextended_INLINE_FUNC
void bddexportassvg_inner(FILE* fp, bddp f,
                            DDIndex<T>* index, int is_zbdd
/*#ifdef __cplusplus */
                            , const WriteObject& sbddextended_writeLine
/*#endif */
                        )
{
    char ss[sbddextended_BUFSIZE];
    const int node_radius = 20;
    const int node_interval_x = 30;
    const int node_interval_y = 40;
    const int terminal_x = 30;
    const int terminal_y = 40;
    const int margin_x = 20;
    const int margin_y = 20;
    const int label_y = 7;
    const int arc_width = 3;
    /* A DD consisting only of a terminal has no internal nodes, so the
       root terminal is the only object to draw. Draw just that terminal
       so that the 0-terminal and 1-terminal DDs produce distinguishable
       images. For a DD with internal nodes, a terminal is drawn only
       when an arc reaches it (decided after the arcs are collected
       below); an unreachable terminal would be an isolated object in
       the figure. */
    bool draw_zero = (f == bddempty);
    bool draw_one = (f == bddsingle);

    /* bddnull is the value the import functions return on an error, not a */
    /* DD. Without this check it would be drawn as an empty index, that is, */
    /* as a picture of both terminals that looks like a valid result. */
    if (f == bddnull) {
        fprintf(stderr, "The SVG output cannot represent bddnull.\n");
        return;
    }

    bool is_made = false;
    if (index == NULL) {
        index = new DDIndex<T>(f);
        is_made = true;
    } else if (index->getRawPointer() == NULL) {
        /* an index built from bddnull or one that has been cleared */
        fprintf(stderr, "The given index does not hold a DD.\n");
        return;
    } else if (!bddNodeIndex_checkIndexOf(index->getRawPointer(), f,
                                            is_zbdd)) {
        /* checkIndexOf has reported the mismatch */
        return;
    }

    /* The write callback throws instead of returning false when the
       ostream has exceptions enabled, and the maps below can throw as
       well, so release the index this function made before letting the
       exception propagate. */
    try {

    std::map<bddp, std::pair<int, int> > pos_map;
    std::map<bddp, std::vector<ExportAsSvg_arcinfo> > dest_info;
    std::map<bddp, std::pair<int, int> > dest0_pos;
    std::map<bddp, std::pair<int, int> > dest1_pos;
    int y = margin_y + node_radius;
    ullint max_nodes = 0;
    for (int level = index->height(); level >= 1; --level) {
        if (max_nodes < index->size(level)) {
            max_nodes = index->size(level);
        }
    }
    if (max_nodes < 2) {
        max_nodes = 2;
    }
    /* The coordinates below are computed in int, so refuse a DD whose
       widest level does not fit in the drawing rather than produce
       negative widths and coordinates from the wrap-around. */
    const ullint max_nodes_limit = static_cast<ullint>(
        (INT_MAX - node_interval_x)
            / (2 * node_radius + 1 + node_interval_x));
    if (max_nodes > max_nodes_limit) {
        fprintf(stderr, "The DD is too wide to be drawn as an SVG "
                "(a level has more than %llu nodes).\n", max_nodes_limit);
        if (is_made) {
            delete index;
        }
        return;
    }
    const int node_x = static_cast<int>((2 * node_radius + 1)
        * max_nodes
        + node_interval_x * (max_nodes + 1));

    for (int level = index->height(); level >= 1; --level) {
        int num_nodes = static_cast<int>(index->size(level));
        int x = node_x / (num_nodes + 1) - (node_radius + node_interval_x - margin_x);
        for (ullint j = 0; j < static_cast<ullint>(num_nodes); ++j) {
            bddp g = ExportAsSvg_getBddp(index, level, j);
            bddp g0 = (is_zbdd ? bddgetchild0z(g) : bddgetchild0b(g));
            bddp g1 = (is_zbdd ? bddgetchild1z(g) : bddgetchild1b(g));
            pos_map[g] = std::make_pair(x, y);
            dest_info[g0].push_back(ExportAsSvg_arcinfo(g, 0, x, y));
            dest_info[g1].push_back(ExportAsSvg_arcinfo(g, 1, x, y));
            x += node_x / (num_nodes + 1);
        }
        y += 2 * node_radius + node_interval_y;
    }
    /* see the comment at the declarations of draw_zero and draw_one */
    if (dest_info.find(bddempty) != dest_info.end()) {
        draw_zero = true;
    }
    if (dest_info.find(bddsingle) != dest_info.end()) {
        draw_one = true;
    }
    y += terminal_y / 2 - node_radius;
    const int max_x = static_cast<int>(2 * node_radius * max_nodes
        + node_interval_x * (max_nodes - 1)
        + 2 * margin_x);
    const int max_y = y + terminal_y / 2 + margin_y;
    const int num_terms = (draw_zero ? 1 : 0) + (draw_one ? 1 : 0);
    int tx = node_x / (num_terms + 1) - (node_radius + node_interval_x - margin_x);
    if (draw_zero) {
        pos_map[bddempty] = std::make_pair(tx, y);
        tx += node_x / (num_terms + 1);
    }
    if (draw_one) {
        pos_map[bddsingle] = std::make_pair(tx, y);
    }

    std::map<bddp, std::vector<ExportAsSvg_arcinfo> >::iterator itor = dest_info.begin();

    while (itor != dest_info.end()) {
        bddp g = itor->first;
        std::vector<ExportAsSvg_arcinfo>& infovec = itor->second;
        std::sort(infovec.begin(), infovec.end(),
            ExportAsSvg_arcinfo_compare);
        double rad = 2.0 / 3.0 * M_PI;
        if (infovec.size() == 1) {
            rad = 1.0 / 2.0 * M_PI;
        }
        for (int i = 0; i < static_cast<int>(infovec.size()); ++i) {
            int posx = ExportAsSvg_getCirclePosX(pos_map[g].first, node_radius, rad);
            int posy = ExportAsSvg_getCirclePosY(pos_map[g].second, node_radius, rad);
            if (infovec[i].arc == 0) {
                dest0_pos[infovec[i].f] = std::make_pair(posx, posy);
            } else {
                assert(infovec[i].arc == 1);
                dest1_pos[infovec[i].f] = std::make_pair(posx, posy);
            }
            if (infovec.size() >= 2) {
                rad -= M_PI / 3.0 / static_cast<double>(infovec.size() - 1);
            }
        }
        ++itor;
    }

    /* draw svg */
    sbddextended_snprintf2(ss, sbddextended_BUFSIZE,
        "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        "width=\"%d\" height=\"%d\">", max_x, max_y);
    sbddextended_writeLineOrReturn(ss);

    sbddextended_writeLineOrReturn("<marker id=\"arrow\" viewBox=\"-10 -4 20 8\" "
        "markerWidth=\"10\" markerHeight=\"10\" orient=\"auto\">");
    sbddextended_writeLineOrReturn("    <polygon points=\"-10,-4 0,0 -10,4\" "
        "fill=\"#1b3966\" stroke=\"none\" />");
    sbddextended_writeLineOrReturn("</marker>");

    /* draw nodes */
    for (int level = index->height(); level >= 1; --level) {
        for (ullint j = 0; j < index->size(level); ++j) {
            bddp g = ExportAsSvg_getBddp(index, level, j);
            sbddextended_snprintf4(ss, sbddextended_BUFSIZE,
                "<circle cx=\"%d\" cy=\"%d\" r=\"%d\" fill=\"#deebf7\" "
                "stroke=\"#1b3966\" stroke-width=\"%d\" />",
                pos_map[g].first, pos_map[g].second, node_radius, arc_width);
            sbddextended_writeLineOrReturn(ss);
            sbddextended_snprintf3(ss, sbddextended_BUFSIZE,
                "<text x=\"%d\" y=\"%d\" text-anchor=\"middle\" "
                "font-size=\"24\">%u</text>", pos_map[g].first,
                pos_map[g].second + label_y,
                bddvaroflev(static_cast<bddvar>(level)));
            sbddextended_writeLineOrReturn(ss);
        }
    }

    /* draw arcs */
    for (int level = index->height(); level >= 1; --level) {
        for (ullint j = 0; j < index->size(level); ++j) {
            bddp g = ExportAsSvg_getBddp(index, level, j);
            int posx1 = ExportAsSvg_getCirclePosX(pos_map[g].first,
                node_radius, 4.0 / 3.0 * M_PI);
            int posy1 = ExportAsSvg_getCirclePosY(pos_map[g].second,
                node_radius, 4.0 / 3.0 * M_PI);
            int posx2 = dest0_pos[g].first;
            int posy2 = dest0_pos[g].second;
            sbddextended_snprintf5(ss, sbddextended_BUFSIZE,
                "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" "
                "stroke=\"#1b3966\" stroke-width=\"%d\" "
                "stroke-dasharray=\"10,5\" "
                "marker-end=\"url(#arrow)\" />",
                posx1, posy1, posx2, posy2, arc_width);
            sbddextended_writeLineOrReturn(ss);
            posx1 = ExportAsSvg_getCirclePosX(pos_map[g].first,
                node_radius, 5.0 / 3.0 * M_PI);
            posy1 = ExportAsSvg_getCirclePosY(pos_map[g].second,
                node_radius, 5.0 / 3.0 * M_PI);
            posx2 = dest1_pos[g].first;
            posy2 = dest1_pos[g].second;
            sbddextended_snprintf5(ss, sbddextended_BUFSIZE,
                "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" "
                "stroke=\"#1b3966\" stroke-width=\"%d\" "
                "marker-end=\"url(#arrow)\" />",
                posx1, posy1, posx2, posy2, arc_width);
            sbddextended_writeLineOrReturn(ss);
        }
    }

    /*draw terminals */
    if (draw_zero) {
        sbddextended_snprintf5(ss, sbddextended_BUFSIZE,
            "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" "
            "fill=\"#deebf7\" stroke=\"#1b3966\" stroke-width=\"%d\" />",
            pos_map[bddempty].first - terminal_x / 2,
            pos_map[bddempty].second - terminal_y / 2,
            terminal_x, terminal_y, arc_width);
        sbddextended_writeLineOrReturn(ss);
    }
    if (draw_one) {
        sbddextended_snprintf5(ss, sbddextended_BUFSIZE,
            "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" "
            "fill=\"#deebf7\" stroke=\"#1b3966\" stroke-width=\"%d\" />",
            pos_map[bddsingle].first - terminal_x / 2,
            pos_map[bddsingle].second - terminal_y / 2,
            terminal_x, terminal_y, arc_width);
        sbddextended_writeLineOrReturn(ss);
    }

    if (draw_zero) {
        sbddextended_snprintf2(ss, sbddextended_BUFSIZE,
            "<text x=\"%d\" y=\"%d\" text-anchor=\"middle\" "
            "font-size=\"24\">0</text>", pos_map[bddempty].first,
            pos_map[bddempty].second + label_y);
        sbddextended_writeLineOrReturn(ss);
    }

    if (draw_one) {
        sbddextended_snprintf2(ss, sbddextended_BUFSIZE,
            "<text x=\"%d\" y=\"%d\" text-anchor=\"middle\" "
            "font-size=\"24\">1</text>", pos_map[bddsingle].first,
            pos_map[bddsingle].second + label_y);
        sbddextended_writeLineOrReturn(ss);
    }

    sbddextended_writeLineOrReturn("</svg>");

    if (is_made) {
        delete index;
        index = NULL;
    }

    } catch (...) {
        if (is_made) {
            delete index;
        }
        throw;
    }
}

#undef sbddextended_writeLineOrReturn

template<typename T>
sbddextended_INLINE_FUNC
void exportBDDAsSvg(FILE* fp, const BDD& bdd,
                    std::map<std::string, std::string>* /*option*/,
                    DDIndex<T>* index)
{
    WriteObject wo(false, true, NULL);
    bddexportassvg_inner(fp, bdd.GetID(), index, 0, wo);
}

sbddextended_INLINE_FUNC
void exportBDDAsSvg(FILE* fp, const BDD& bdd,
                    std::map<std::string, std::string>* option = NULL)
{
    exportBDDAsSvg<int>(fp, bdd, option, NULL);
}

template<typename T>
sbddextended_INLINE_FUNC
void exportBDDAsSvg(std::ostream& ost, const BDD& bdd,
                    std::map<std::string, std::string>* /*option*/,
                    DDIndex<T>* index)
{
    WriteObject wo(true, true, &ost);
    bddexportassvg_inner(NULL, bdd.GetID(), index, 0, wo);
}

sbddextended_INLINE_FUNC
void exportBDDAsSvg(std::ostream& ost, const BDD& bdd,
                    std::map<std::string, std::string>* option = NULL)
{
    exportBDDAsSvg<int>(ost, bdd, option, NULL);
}

template<typename T>
sbddextended_INLINE_FUNC
void exportZBDDAsSvg(FILE* fp, const ZBDD& zbdd,
                        std::map<std::string, std::string>* /*option*/,
                        DDIndex<T>* index)
{
    WriteObject wo(false, true, NULL);
    bddexportassvg_inner(fp, zbdd.GetID(), index, 1, wo);
}

sbddextended_INLINE_FUNC
void exportZBDDAsSvg(FILE* fp, const ZBDD& zbdd,
                        std::map<std::string, std::string>* option = NULL)
{
    exportZBDDAsSvg<int>(fp, zbdd, option, NULL);
}

template<typename T>
sbddextended_INLINE_FUNC
void exportZBDDAsSvg(std::ostream& ost, const ZBDD& zbdd,
                        std::map<std::string, std::string>* /*option*/,
                        DDIndex<T>* index)
{
    WriteObject wo(true, true, &ost);
    bddexportassvg_inner(NULL, zbdd.GetID(), index, 1, wo);
}

sbddextended_INLINE_FUNC
void exportZBDDAsSvg(std::ostream& ost, const ZBDD& zbdd,
                        std::map<std::string, std::string>* option = NULL)
{
    exportZBDDAsSvg<int>(ost, zbdd, option, NULL);
}

#endif

/* The functions below are the old names of the functions that were renamed
   in the past. They used to be defined as object-like macros, but a macro
   ignores namespaces and rewrites every occurrence of the identifier,
   including the ones in the code of the user; defining them as inline
   functions instead avoids that. The change log of
   documents/reference.md lists the old names and the new ones. */

/* *************************** C version starts **************************** */

#ifdef __cplusplus
sbddextended_INLINE_FUNC
bddp bddconstructzbddfrombinary(FILE* fp, int root_level = -1)
#else
sbddextended_INLINE_FUNC
bddp bddconstructzbddfrombinary(FILE* fp, int root_level)
#endif
{
    return bddimportzbddasbinary(fp, root_level);
}

sbddextended_INLINE_FUNC
void bddwritezbddtobinary(FILE* fp, bddp f, int use_negative_arcs,
                          bddNodeIndex* node_index)
{
    bddexportzbddasbinary(fp, f, use_negative_arcs, node_index);
}

sbddextended_INLINE_FUNC
void bddwritebddforgraphillion(FILE* fp, bddp f, bddNodeIndex* node_index,
                               int root_level)
{
    bddexportbddasgraphillion(fp, f, node_index, root_level);
}

#ifdef __cplusplus
sbddextended_INLINE_FUNC
bddp bddconstructbddfromfileknuth(FILE* fp, int is_hex, int root_level = -1)
#else
sbddextended_INLINE_FUNC
bddp bddconstructbddfromfileknuth(FILE* fp, int is_hex, int root_level)
#endif
{
    return bddimportbddasknuth(fp, is_hex, root_level);
}

#ifdef __cplusplus
sbddextended_INLINE_FUNC
bddp bddconstructzbddfromfileknuth(FILE* fp, int is_hex, int root_level = -1)
#else
sbddextended_INLINE_FUNC
bddp bddconstructzbddfromfileknuth(FILE* fp, int is_hex, int root_level)
#endif
{
    return bddimportzbddasknuth(fp, is_hex, root_level);
}

sbddextended_INLINE_FUNC
void bddwritezbddtofileknuth(FILE* fp, bddp f, int is_hex,
                             bddNodeIndex* node_index)
{
    bddexportzbddasknuth(fp, f, is_hex, node_index);
}

sbddextended_INLINE_FUNC
void bddwritebddforgraphviz(FILE* fp, bddp f, bddNodeIndex* node_index)
{
    bddexportbddasgraphviz(fp, f, node_index);
}

/* ************************** C++ version starts *************************** */

#ifdef __cplusplus

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getAllSetsIncluding(const T& base_variables,
                         const std::vector<bddvar>& target_variables)
{
    return getPowerSetIncluding(base_variables, target_variables);
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getAllSetsIncluding(const T& base_variables,
                         const std::set<bddvar>& target_variables)
{
    return getPowerSetIncluding(base_variables, target_variables);
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getAllSetsIncluding(const T& base_variables, bddvar v)
{
    return getPowerSetIncluding(base_variables, v);
}

sbddextended_INLINE_FUNC
ZBDD getAllSetsIncluding(int n, const std::vector<bddvar>& target_variables)
{
    return getPowerSetIncluding(n, target_variables);
}

sbddextended_INLINE_FUNC
ZBDD getAllSetsIncluding(int n, const std::set<bddvar>& target_variables)
{
    return getPowerSetIncluding(n, target_variables);
}

sbddextended_INLINE_FUNC
ZBDD getAllSetsIncluding(int n, int v)
{
    return getPowerSetIncluding(n, v);
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getAllPowerSetsIncluding(const T& base_variables,
                              const std::vector<bddvar>& target_variables)
{
    return getPowerSetIncluding(base_variables, target_variables);
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getAllPowerSetsIncluding(const T& base_variables,
                              const std::set<bddvar>& target_variables)
{
    return getPowerSetIncluding(base_variables, target_variables);
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getAllPowerSetsIncluding(const T& base_variables, bddvar v)
{
    return getPowerSetIncluding(base_variables, v);
}

sbddextended_INLINE_FUNC
ZBDD getAllPowerSetsIncluding(int n,
                              const std::vector<bddvar>& target_variables)
{
    return getPowerSetIncluding(n, target_variables);
}

sbddextended_INLINE_FUNC
ZBDD getAllPowerSetsIncluding(int n, const std::set<bddvar>& target_variables)
{
    return getPowerSetIncluding(n, target_variables);
}

sbddextended_INLINE_FUNC
ZBDD getAllPowerSetsIncluding(int n, int v)
{
    return getPowerSetIncluding(n, v);
}

sbddextended_INLINE_FUNC
ZBDD getAllPowerSetsNotIncluding(int n,
                                 const std::vector<bddvar>& target_variables)
{
    return getPowerSetNotIncluding(n, target_variables);
}

sbddextended_INLINE_FUNC
ZBDD getAllPowerSetsNotIncluding(int n,
                                 const std::set<bddvar>& target_variables)
{
    return getPowerSetNotIncluding(n, target_variables);
}

sbddextended_INLINE_FUNC
ZBDD getAllPowerSetsNotIncluding(int n, int v)
{
    return getPowerSetNotIncluding(n, v);
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getAllSetsWithCard(const T& variables, int k)
{
    return getPowerSetWithCard(variables, k);
}

sbddextended_INLINE_FUNC
ZBDD getAllSetsWithCard(int n, int k)
{
    return getPowerSetWithCard(n, k);
}

template<typename T>
sbddextended_INLINE_FUNC
typename sbddh_EnableIfContainer<sbddh_IsContainer<T>::value, ZBDD>::type
getAllPowerSetsWithCard(const T& variables, int k)
{
    return getPowerSetWithCard(variables, k);
}

sbddextended_INLINE_FUNC
ZBDD getAllPowerSetsWithCard(int n, int k)
{
    return getPowerSetWithCard(n, k);
}

sbddextended_INLINE_FUNC
ZBDD constructZBDDFromBinary(FILE* fp, int root_level = -1)
{
    return importZBDDAsBinary(fp, root_level);
}

sbddextended_INLINE_FUNC
ZBDD constructZBDDFromBinary(std::istream& ist, int root_level = -1)
{
    return importZBDDAsBinary(ist, root_level);
}

sbddextended_INLINE_FUNC
ZBDD constructZBDDFromGraphillion(FILE* fp, int root_level = -1)
{
    return importZBDDAsGraphillion(fp, root_level);
}

sbddextended_INLINE_FUNC
ZBDD constructZBDDFromGraphillion(std::istream& ist, int root_level = -1)
{
    return importZBDDAsGraphillion(ist, root_level);
}

sbddextended_INLINE_FUNC
BDD constructBDDFromFileKnuth(FILE* fp, bool is_hex, int root_level = -1)
{
    return importBDDAsKnuth(fp, is_hex, root_level);
}

sbddextended_INLINE_FUNC
BDD constructBDDFromFileKnuth(std::istream& ist, bool is_hex,
                              int root_level = -1)
{
    return importBDDAsKnuth(ist, is_hex, root_level);
}

sbddextended_INLINE_FUNC
ZBDD constructZBDDFromFileKnuth(FILE* fp, bool is_hex, int root_level = -1)
{
    return importZBDDAsKnuth(fp, is_hex, root_level);
}

sbddextended_INLINE_FUNC
ZBDD constructZBDDFromFileKnuth(std::istream& ist, bool is_hex,
                                int root_level = -1)
{
    return importZBDDAsKnuth(ist, is_hex, root_level);
}

template<typename T>
sbddextended_INLINE_FUNC
void writeZBDDToBinary(FILE* fp, const ZBDD& zbdd, bool use_negative_arcs,
                       DDIndex<T>* node_index)
{
    exportZBDDAsBinary(fp, zbdd, use_negative_arcs, node_index);
}

sbddextended_INLINE_FUNC
void writeZBDDToBinary(FILE* fp, const ZBDD& zbdd,
                       bool use_negative_arcs = true)
{
    exportZBDDAsBinary(fp, zbdd, use_negative_arcs);
}

template<typename T>
sbddextended_INLINE_FUNC
void writeZBDDToBinary(std::ostream& ost, const ZBDD& zbdd,
                       bool use_negative_arcs, DDIndex<T>* node_index)
{
    exportZBDDAsBinary(ost, zbdd, use_negative_arcs, node_index);
}

sbddextended_INLINE_FUNC
void writeZBDDToBinary(std::ostream& ost, const ZBDD& zbdd,
                       bool use_negative_arcs = true)
{
    exportZBDDAsBinary(ost, zbdd, use_negative_arcs);
}

template<typename T>
sbddextended_INLINE_FUNC
void writeZBDDForGraphillion(FILE* fp, const ZBDD& zbdd, int root_level,
                             DDIndex<T>* node_index)
{
    exportZBDDAsGraphillion(fp, zbdd, root_level, node_index);
}

sbddextended_INLINE_FUNC
void writeZBDDForGraphillion(FILE* fp, const ZBDD& zbdd, int root_level = -1)
{
    exportZBDDAsGraphillion(fp, zbdd, root_level);
}

template<typename T>
sbddextended_INLINE_FUNC
void writeZBDDForGraphillion(std::ostream& ost, const ZBDD& zbdd,
                             int root_level, DDIndex<T>* node_index)
{
    exportZBDDAsGraphillion(ost, zbdd, root_level, node_index);
}

sbddextended_INLINE_FUNC
void writeZBDDForGraphillion(std::ostream& ost, const ZBDD& zbdd,
                             int root_level = -1)
{
    exportZBDDAsGraphillion(ost, zbdd, root_level);
}

template<typename T>
sbddextended_INLINE_FUNC
void writeZBDDToFileKnuth(FILE* fp, const ZBDD& zbdd, bool is_hex,
                          DDIndex<T>* node_index)
{
    exportZBDDAsKnuth(fp, zbdd, is_hex, node_index);
}

sbddextended_INLINE_FUNC
void writeZBDDToFileKnuth(FILE* fp, const ZBDD& zbdd, bool is_hex = false)
{
    exportZBDDAsKnuth(fp, zbdd, is_hex);
}

template<typename T>
sbddextended_INLINE_FUNC
void writeZBDDToFileKnuth(std::ostream& ost, const ZBDD& zbdd, bool is_hex,
                          DDIndex<T>* node_index)
{
    exportZBDDAsKnuth(ost, zbdd, is_hex, node_index);
}

sbddextended_INLINE_FUNC
void writeZBDDToFileKnuth(std::ostream& ost, const ZBDD& zbdd,
                          bool is_hex = false)
{
    exportZBDDAsKnuth(ost, zbdd, is_hex);
}

template<typename T>
sbddextended_INLINE_FUNC
void writeBDDForGraphviz(FILE* fp, const BDD& bdd,
                         std::map<std::string, std::string>* option,
                         DDIndex<T>* node_index)
{
    exportBDDAsGraphviz(fp, bdd, option, node_index);
}

sbddextended_INLINE_FUNC
void writeBDDForGraphviz(FILE* fp, const BDD& bdd,
                         std::map<std::string, std::string>* option = NULL)
{
    exportBDDAsGraphviz(fp, bdd, option);
}

template<typename T>
sbddextended_INLINE_FUNC
void writeBDDForGraphviz(std::ostream& ost, const BDD& bdd,
                         std::map<std::string, std::string>* option,
                         DDIndex<T>* node_index)
{
    exportBDDAsGraphviz(ost, bdd, option, node_index);
}

sbddextended_INLINE_FUNC
void writeBDDForGraphviz(std::ostream& ost, const BDD& bdd,
                         std::map<std::string, std::string>* option = NULL)
{
    exportBDDAsGraphviz(ost, bdd, option);
}

template<typename T>
sbddextended_INLINE_FUNC
void writeZBDDForGraphviz(FILE* fp, const ZBDD& zbdd,
                          std::map<std::string, std::string>* option,
                          DDIndex<T>* node_index)
{
    exportZBDDAsGraphviz(fp, zbdd, option, node_index);
}

sbddextended_INLINE_FUNC
void writeZBDDForGraphviz(FILE* fp, const ZBDD& zbdd,
                          std::map<std::string, std::string>* option = NULL)
{
    exportZBDDAsGraphviz(fp, zbdd, option);
}

template<typename T>
sbddextended_INLINE_FUNC
void writeZBDDForGraphviz(std::ostream& ost, const ZBDD& zbdd,
                          std::map<std::string, std::string>* option,
                          DDIndex<T>* node_index)
{
    exportZBDDAsGraphviz(ost, zbdd, option, node_index);
}

sbddextended_INLINE_FUNC
void writeZBDDForGraphviz(std::ostream& ost, const ZBDD& zbdd,
                          std::map<std::string, std::string>* option = NULL)
{
    exportZBDDAsGraphviz(ost, zbdd, option);
}

#endif /* __cplusplus */


#ifdef __cplusplus
} /* end of namespace sbddh */
#endif

#endif /* SBDD_HELPER_H */
