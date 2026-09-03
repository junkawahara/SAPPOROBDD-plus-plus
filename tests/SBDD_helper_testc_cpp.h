/*
One header library for SAPPOROBDD C/C++ version test code

Copyright (c) 2017 -- 2023 Jun Kawahara

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> /* va_list in test_snprintf_corner_cases */
#include <math.h>   /* HUGE_VAL in test_snprintf */
#include <limits.h> /* INT_MAX in test_getsingleandpowerset */

/* for the process ID used by the temporary file names */
#ifdef _WIN32
#include <process.h>
#define sbddh_test_getpid _getpid
#else
#include <unistd.h>
#define sbddh_test_getpid getpid
#endif

#include "bddc.h"

/* The only change made to the upstream test sources: this repository
   bundles the distributed single header in sbdd_helper/, and neither the
   sbdd_helper devel/ sources nor its SBDDH_TEST_RELEASE_HEADER switch
   exist here. */
#include "../sbdd_helper/SBDD_helper.h"

#ifdef __cplusplus
using namespace sbddh;
#endif

#define test(b) testfunc((llint)(b), __FILE__, __LINE__)
#define test_eq(v1, v2) testfunc_eq((llint)(v1), (llint)(v2), __FILE__, __LINE__)

/* The C and the C++ test drivers write the temporary files into the
   current directory, so the names must contain the process ID; otherwise
   running testc and testcpp at the same time makes one of them remove or
   overwrite the file of the other. initialize_filenames fills them in. */
char g_filename1[64];
char g_filename2[64];
char g_filename3[64];

void initialize_filenames(void)
{
    int pid = (int)sbddh_test_getpid();
    sprintf(g_filename1, "SBDD_helper_test_tempdata1_%d.txt", pid);
    sprintf(g_filename2, "SBDD_helper_test_tempdata2_%d.txt", pid);
    sprintf(g_filename3, "SBDD_helper_test_tempdata3_%d.txt", pid);
}

/* fopen that exits with a diagnostic instead of returning NULL when
   the file cannot be opened, so that a test never passes NULL to the
   I/O functions silently */
FILE* test_fopen(const char* filename, const char* mode)
{
    FILE* fp = fopen(filename, mode);
    if (fp == NULL) {
        fprintf(stderr, "file %s cannot be opened\n", filename);
        exit(1);
    }
    return fp;
}

void testfunc(llint b, const char* filename, int error_line)
{
    if (b == 0) {
        fprintf(stderr, "not expected value at %s line %d\n", filename, error_line);
        exit(1);
    }
}

void testfunc_eq(llint v1, llint v2, const char* filename, int error_line)
{
    if (v1 != v2) {
        fprintf(stderr, "%lld != %lld at %s line %d\n", v1, v2, filename, error_line);
        exit(1);
    }
}

int is_expected_str(FILE* fp, const char* str)
{
    long v;
    int b, c;
    size_t len;
    char* buf;

    len = strlen(str);

    v = ftell(fp);
    if (v < 0) {
        fprintf(stderr, "ftell failed\n");
        exit(1);
    }
    if (v >= 1000000) {
        fprintf(stderr, "Too much temporary file size\n");
        exit(1);
    }
    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    /* The content is read before the length is compared, so that a
       failure shows what was written whether the length or the content
       differs. */
    buf = (char*)malloc((size_t)v + 1); /* +1 for '\0' */
    if (buf == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    buf[0] = '\0'; /* the loop below does not run for an empty file */
    c = 0;
    while (c < v && fgets(buf + c, (int)v + 1 - c, fp) != NULL) {
        c += (int)strlen(buf + c);
    }
    /* An empty file matches an empty expected string; only a non-empty
       file that could not be read at all is an error. */
    if (v > 0 && c == 0) {
        fprintf(stderr, "fgets failed!\n");
        exit(1);
    }
    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    b = (v == (long)len && strncmp(buf, str, (size_t)v) == 0 ? 1 : 0);
    if (!b) {
        fprintf(stderr, "unexpected str (length %ld, expected %ld): %s\n",
                v, (long)len, buf);
    }
    free(buf);
    return b;
}


/* fp の内容のうち、substr を含む行の数を返す。
   呼び出しの前後で、ファイル位置は先頭に移動する */
int count_lines_containing(FILE* fp, const char* substr)
{
    char buf[1024];
    int count = 0;

    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    while (fgets(buf, (int)sizeof(buf), fp) != NULL) {
        if (strstr(buf, substr) != NULL) {
            ++count;
        }
    }
    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    return count;
}

/* 2つのファイルの内容が一致するかどうかを返す */
int is_same_file(const char* filename1, const char* filename2)
{
    FILE* fp1;
    FILE* fp2;
    int c1, c2, b;

    fp1 = test_fopen(filename1, "rb");
    fp2 = test_fopen(filename2, "rb");
    b = 1;
    do {
        c1 = fgetc(fp1);
        c2 = fgetc(fp2);
        if (c1 != c2) {
            b = 0;
            break;
        }
    } while (c1 != EOF);
    fclose(fp1);
    fclose(fp2);
    return b;
}


void initialize(void)
{
    initialize_filenames();

    bddinit(1000ll, 10000000ll);

    bddnewvarn(100);
}

bddp make_test_zbdd(void)
{
    int i;
    bddp f;
    bddp g[7];

    g[0] = bddchange(bddsingle, 1);
    g[1] = bddchange(g[0], 2);

    g[2] = bddchange(bddsingle, 1);
    g[3] = bddchange(g[2], 3);

    g[4] = bddchange(bddsingle, 2);
    g[5] = bddchange(g[4], 3);

    g[6] = bddunion(g[1], g[3]);
    f = bddunion(g[5], g[6]);

    for (i = 0; i < 7; ++i) {
        bddfree(g[i]);
    }
    return f;
}

void test_MyVector(void)
{
    int N = 1024 * 1024 + 1024 + 1;
    int i;
    sbddextended_MyVector v0;
    sbddextended_MyVector v;
    sbddextended_MyVector v1;

    sbddextended_MyVector_initialize(&v0);
    test_eq(v0.count, 0);
    sbddextended_MyVector_deinitialize(&v0);

    sbddextended_MyVector_initialize(&v);
    sbddextended_MyVector_initialize(&v1);

    for (i = 0; i < N; ++i) {
        sbddextended_MyVector_add(&v, (llint)i * 2);
    }
    test_eq(v.count, N);

    for (i = 0; i < N; ++i) {
        test_eq(sbddextended_MyVector_get(&v, (llint)i), (llint)i * 2);
    }

    for (i = 0; i < N; i += 2) {
        sbddextended_MyVector_set(&v, (llint)i, (llint)i * 3);
    }

    for (i = 0; i < N; ++i) {
        if (i % 2 == 0) {
            test_eq(sbddextended_MyVector_get(&v, (llint)i), (llint)i * 3);
        } else {
            test_eq(sbddextended_MyVector_get(&v, (llint)i), (llint)i * 2);
        }
    }

    sbddextended_MyVector_copy(&v1, &v);

    test_eq(v1.count, v.count);

    for (i = 0; i < N; ++i) {
        if (i % 2 == 0) {
            test_eq(sbddextended_MyVector_get(&v1, (llint)i), (llint)i * 3);
        } else {
            test_eq(sbddextended_MyVector_get(&v1, (llint)i), (llint)i * 2);
        }
    }
    sbddextended_MyVector_pop_back(&v1);
    sbddextended_MyVector_pop_back(&v1);
    sbddextended_MyVector_pop_back(&v1);
    test_eq(v.count, v1.count + 3);

    sbddextended_MyVector_deinitialize(&v1);
    sbddextended_MyVector_deinitialize(&v);

    /* state after deinitialization */
    test_eq(v.count, 0);
    test_eq(v1.count, 0);
#ifdef __cplusplus
    test(v.vec == NULL);
#else
    test(v.buf == NULL);
    test_eq(v.capacity, 0);
#endif

    /* deinitializing twice is safe */
    sbddextended_MyVector_deinitialize(&v);
    test_eq(v.count, 0);

    /* a deinitialized vector can be reused after re-initialization */
    sbddextended_MyVector_initialize(&v);
    sbddextended_MyVector_initialize(&v1);
    for (i = 0; i < 5; ++i) {
        sbddextended_MyVector_add(&v, (llint)i + 10);
    }

    /* self-copy preserves the elements */
    sbddextended_MyVector_copy(&v, &v);
    test_eq(v.count, 5);
    for (i = 0; i < 5; ++i) {
        test_eq(sbddextended_MyVector_get(&v, (llint)i), (llint)i + 10);
    }

    /* copy of a vector that has become small after many pop_backs */
    for (i = 0; i < N; ++i) {
        sbddextended_MyVector_add(&v1, (llint)i * 5);
    }
    while (v1.count > 3) {
        sbddextended_MyVector_pop_back(&v1);
    }
    sbddextended_MyVector_copy(&v, &v1);
    test_eq(v.count, 3);
    for (i = 0; i < 3; ++i) {
        test_eq(sbddextended_MyVector_get(&v, (llint)i), (llint)i * 5);
    }
#ifndef __cplusplus
    /* the copy allocates memory for the logical size, not for the */
    /* reserved capacity of the source */
    test_eq(v.capacity, sbddextended_MyVector_INITIAL_BUFSIZE);
#endif

    sbddextended_MyVector_deinitialize(&v);
    sbddextended_MyVector_deinitialize(&v1);
}

#ifndef __cplusplus

/* Returns the height of the subtree whose root is "node" if it is a valid
   AVL tree whose keys are sorted, and -1 otherwise. */
llint test_MyDict_checkTree(const sbddextended_MyDictNode* node)
{
    llint hl;
    llint hr;

    if (node == NULL) {
        return 0;
    }
    hl = test_MyDict_checkTree(node->left);
    hr = test_MyDict_checkTree(node->right);
    if (hl < 0 || hr < 0) {
        return -1;
    }
    if (node->left != NULL && !(node->left->key < node->key)) {
        return -1;
    }
    if (node->right != NULL && !(node->key < node->right->key)) {
        return -1;
    }
    if (hl - hr > 1 || hr - hl > 1) {
        return -1;
    }
    if ((llint)node->height != (hl > hr ? hl : hr) + 1) {
        return -1;
    }
    return (llint)node->height;
}

/* Tests that the tree is a valid AVL tree of height at most
   2 * log2(count + 1). */
void test_MyDict_testBalanced(const sbddextended_MyDict* d)
{
    llint height;
    int log_count;

    height = test_MyDict_checkTree(d->root);
    test(height > 0);
    log_count = 0;
    while (((llint)1 << log_count) < (llint)d->count + 1) {
        ++log_count;
    }
    test(height <= 2 * log_count);
}

#endif

void test_MyDict(void)
{
    int N = 1024 + 1;
    llint i;
    llint value = 0;
    sbddextended_MyDict d0;
    sbddextended_MyDict d;
    sbddextended_MyDict d1;

    sbddextended_MyDict_initialize(&d0);
    test_eq(d0.count, 0);
    sbddextended_MyDict_deinitialize(&d0);

    sbddextended_MyDict_initialize(&d);
    sbddextended_MyDict_initialize(&d1);

    for (i = 0; i < N; ++i) {
        sbddextended_MyDict_add(&d, i * 2, N - i * 2);
        sbddextended_MyDict_add(&d, 5 * N - i * 2, 3 * N + i);
    }
    test_eq(d.count, N * 2);

#ifndef __cplusplus
    /* The keys are inserted in the increasing and in the decreasing
       order, which would build a tree of height N if the tree were not
       rebalanced. */
    test_MyDict_testBalanced(&d);
#endif

    for (i = 0; i < N; ++i) {
        test_eq(sbddextended_MyDict_find(&d, i * 2, &value), 1);
        test_eq(value, N - i * 2);
        test_eq(sbddextended_MyDict_find(&d, 5 * N - i * 2, &value), 1);
        test_eq(value, 3 * N + i);
    }

    for (i = 0; i < N; ++i) {
        test_eq(sbddextended_MyDict_find(&d, i * 2 - 1, &value), 0);
    }

    for (i = 0; i < N; ++i) {
        sbddextended_MyDict_add(&d, i * 2, 16 * N + i);
    }
    test_eq(d.count, N * 2);

    for (i = 0; i < N; ++i) {
        test_eq(sbddextended_MyDict_find(&d, i * 2, &value), 1);
        test_eq(value, 16 * N + i);
    }

    sbddextended_MyDict_copy(&d1, &d);

    test_eq(d1.count, d.count);

#ifndef __cplusplus
    test_MyDict_testBalanced(&d1);
#endif

    for (i = 0; i < N; ++i) {
        test_eq(sbddextended_MyDict_find(&d1, i * 2, &value), 1);
        test_eq(value, 16 * N + i);
    }

    /* self-copy preserves the entries */
    sbddextended_MyDict_copy(&d, &d);
    test_eq(d.count, N * 2);
    for (i = 0; i < N; ++i) {
        test_eq(sbddextended_MyDict_find(&d, i * 2, &value), 1);
        test_eq(value, 16 * N + i);
    }

    sbddextended_MyDict_deinitialize(&d1);
    sbddextended_MyDict_deinitialize(&d);
}

#ifdef __cplusplus

#include <new>

/* Replace the global allocation functions so that the tests can inject */
/* an allocation failure. While g_test_alloc_fail_countdown is negative */
/* (the initial state), allocations behave as usual. When a test sets   */
/* it to n >= 0, the (n+1)-th allocation from that point throws         */
/* std::bad_alloc and resets the countdown to the initial state.        */
llint g_test_alloc_fail_countdown = -1;

void* operator new(std::size_t size)
#if __cplusplus < 201103L
    throw(std::bad_alloc)
#endif
{
    void* p;

    if (g_test_alloc_fail_countdown >= 0) {
        if (g_test_alloc_fail_countdown == 0) {
            g_test_alloc_fail_countdown = -1;
            throw std::bad_alloc();
        }
        --g_test_alloc_fail_countdown;
    }
    p = malloc(size > 0 ? size : 1);
    if (p == NULL) {
        throw std::bad_alloc();
    }
    return p;
}

/* The replaced operator new above allocates with malloc, so the free   */
/* below is correct, but GCC does not know that and warns that a        */
/* pointer from operator new is passed to free.                         */
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 11
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif
void operator delete(void* p)
#if __cplusplus < 201103L
    throw()
#else
    noexcept
#endif
{
    free(p);
}

#if __cplusplus >= 201402L
void operator delete(void* p, std::size_t) noexcept
{
    free(p);
}
#endif
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 11
#pragma GCC diagnostic pop
#endif

/* Tests that the C++ version of MyDict stays consistent               */
/* (count == dict->size() and the content is intact) after a std::map  */
/* node allocation throws std::bad_alloc during an addition or a copy. */
void test_MyDict_allocFailure(void)
{
    llint i;
    llint value = 0;
    int nfailures;
    int succeeded;
    sbddextended_MyDict d;
    sbddextended_MyDict d1;

    sbddextended_MyDict_initialize(&d);
    sbddextended_MyDict_add(&d, 1, 10);
    sbddextended_MyDict_add(&d, 2, 20);
    sbddextended_MyDict_add(&d, 3, 30);

    /* addition: fail the (i+1)-th allocation for i = 0, 1, ... until */
    /* the addition succeeds. Each failed addition must leave the     */
    /* dict unchanged.                                                */
    succeeded = 0;
    nfailures = 0;
    for (i = 0; i < 100 && !succeeded; ++i) {
        g_test_alloc_fail_countdown = i;
        try {
            sbddextended_MyDict_add(&d, 4, 40);
            succeeded = 1;
        } catch (const std::bad_alloc&) {
            ++nfailures;
        }
        g_test_alloc_fail_countdown = -1;
        if (!succeeded) {
            test_eq(d.count, 3);
            test_eq(d.dict->size(), 3);
            test_eq(sbddextended_MyDict_find(&d, 4, &value), 0);
        }
    }
    test(succeeded);
    /* inserting the new key allocates at least one map node */
    test(nfailures >= 1);
    test_eq(d.count, 4);
    test_eq(d.dict->size(), 4);
    for (i = 1; i <= 4; ++i) {
        test_eq(sbddextended_MyDict_find(&d, i, &value), 1);
        test_eq(value, i * 10);
    }

    /* overwriting an existing key must not change count */
    sbddextended_MyDict_add(&d, 4, 44);
    test_eq(d.count, 4);
    test_eq(d.dict->size(), 4);
    test_eq(sbddextended_MyDict_find(&d, 4, &value), 1);
    test_eq(value, 44);
    sbddextended_MyDict_add(&d, 4, 40); /* restore for the checks below */

    /* copy: fail the (i+1)-th allocation for i = 0, 1, ... until the */
    /* copy succeeds. Each failed copy must leave dest unchanged.     */
    sbddextended_MyDict_initialize(&d1);
    sbddextended_MyDict_add(&d1, 100, 1000);

    succeeded = 0;
    nfailures = 0;
    for (i = 0; i < 100 && !succeeded; ++i) {
        g_test_alloc_fail_countdown = i;
        try {
            sbddextended_MyDict_copy(&d1, &d);
            succeeded = 1;
        } catch (const std::bad_alloc&) {
            ++nfailures;
        }
        g_test_alloc_fail_countdown = -1;
        if (!succeeded) {
            test_eq(d1.count, 1);
            test_eq(d1.dict->size(), 1);
            test_eq(sbddextended_MyDict_find(&d1, 100, &value), 1);
            test_eq(value, 1000);
            test_eq(sbddextended_MyDict_find(&d1, 1, &value), 0);
        }
    }
    test(succeeded);
    /* copying the four entries allocates at least four map nodes, so */
    /* some injections must have failed in the middle of the copy     */
    test(nfailures >= 2);
    test_eq(d1.count, 4);
    test_eq(d1.dict->size(), 4);
    for (i = 1; i <= 4; ++i) {
        test_eq(sbddextended_MyDict_find(&d1, i, &value), 1);
        test_eq(value, i * 10);
    }
    test_eq(sbddextended_MyDict_find(&d1, 100, &value), 0);

    sbddextended_MyDict_deinitialize(&d1);
    sbddextended_MyDict_deinitialize(&d);
}

#endif /* __cplusplus */

void test_MySet(void)
{
    int N = 1024 + 1;
    llint i;
    sbddextended_MySet s0;
    sbddextended_MySet s;
    sbddextended_MySet s1;

    sbddextended_MySet_initialize(&s0);
    test_eq(sbddextended_MySet_count(&s0), 0);
    sbddextended_MySet_deinitialize(&s0);

    sbddextended_MySet_initialize(&s);
    sbddextended_MySet_initialize(&s1);

    for (i = 0; i < N; ++i) {
        sbddextended_MySet_add(&s, i * 2);
        sbddextended_MySet_add(&s, 5 * N - i * 2);
    }
    test_eq(sbddextended_MySet_count(&s), N * 2);

    for (i = 0; i < N; ++i) {
        test_eq(sbddextended_MySet_exists(&s, i * 2), 1);
        test_eq(sbddextended_MySet_exists(&s, 5 * N - i * 2), 1);
    }

    for (i = 0; i < N; ++i) {
        test_eq(sbddextended_MySet_exists(&s, i * 2 - 1), 0);
    }

    for (i = 0; i < N; ++i) {
        sbddextended_MySet_add(&s, i * 2);
    }
    test_eq(sbddextended_MySet_count(&s), N * 2);

    for (i = 0; i < N; ++i) {
        test_eq(sbddextended_MySet_exists(&s, i * 2), 1);
    }

    sbddextended_MySet_copy(&s1, &s);

    test_eq(sbddextended_MySet_count(&s1),
        sbddextended_MySet_count(&s));

    for (i = 0; i < N; ++i) {
        test_eq(sbddextended_MySet_exists(&s1, i * 2), 1);
    }

    sbddextended_MySet_deinitialize(&s1);
    sbddextended_MySet_deinitialize(&s);

    /* state after deinitialization */
#ifdef __cplusplus
    test(s.se == NULL);
    test(s1.se == NULL);
#else
    test(s.dict.root == NULL);
    test_eq(s.dict.count, 0);
#endif

    /* deinitializing twice is safe */
    sbddextended_MySet_deinitialize(&s);

    /* a deinitialized set can be reused after re-initialization */
    sbddextended_MySet_initialize(&s);
    sbddextended_MySet_initialize(&s1);
    for (i = 0; i < 5; ++i) {
        sbddextended_MySet_add(&s, i + 10);
    }

    /* self-copy preserves the elements */
    sbddextended_MySet_copy(&s, &s);
    test_eq(sbddextended_MySet_count(&s), 5);
    for (i = 0; i < 5; ++i) {
        test_eq(sbddextended_MySet_exists(&s, i + 10), 1);
    }

    /* copy and deinitialization of a set that is large enough to make
       the tree deeper than in the tests above */
    for (i = 0; i < (1 << 17); ++i) {
        sbddextended_MySet_add(&s1, i * 3);
    }
    sbddextended_MySet_copy(&s, &s1);
    test_eq(sbddextended_MySet_count(&s), (1 << 17));
    for (i = 0; i < (1 << 17); i += 1000) {
        test_eq(sbddextended_MySet_exists(&s, i * 3), 1);
        test_eq(sbddextended_MySet_exists(&s, i * 3 + 1), 0);
    }

    sbddextended_MySet_deinitialize(&s);
    sbddextended_MySet_deinitialize(&s1);
}

#ifdef __cplusplus

/* Tests that the C++ version of MySet keeps the destination unchanged  */
/* when a std::set node allocation throws std::bad_alloc during a copy  */
/* (the same strong guarantee as MyDict_copy and MyVector_copy).        */
void test_MySet_allocFailure(void)
{
    llint i;
    int nfailures;
    int succeeded;
    sbddextended_MySet s;
    sbddextended_MySet s1;

    sbddextended_MySet_initialize(&s);
    for (i = 0; i < 10; ++i) {
        sbddextended_MySet_add(&s, i);
    }

    sbddextended_MySet_initialize(&s1);
    sbddextended_MySet_add(&s1, 100);
    sbddextended_MySet_add(&s1, 200);

    /* copy: fail the (i+1)-th allocation for i = 0, 1, ... until the */
    /* copy succeeds. Each failed copy must leave dest unchanged.     */
    succeeded = 0;
    nfailures = 0;
    for (i = 0; i < 100 && !succeeded; ++i) {
        g_test_alloc_fail_countdown = i;
        try {
            sbddextended_MySet_copy(&s1, &s);
            succeeded = 1;
        } catch (const std::bad_alloc&) {
            ++nfailures;
        }
        g_test_alloc_fail_countdown = -1;
        if (!succeeded) {
            test_eq(sbddextended_MySet_count(&s1), 2);
            test_eq(sbddextended_MySet_exists(&s1, 100), 1);
            test_eq(sbddextended_MySet_exists(&s1, 200), 1);
            test_eq(sbddextended_MySet_exists(&s1, 0), 0);
        }
    }
    test(succeeded);
    /* copying the ten elements allocates at least ten set nodes, so */
    /* some injections must have failed in the middle of the copy    */
    test(nfailures >= 2);
    test_eq(sbddextended_MySet_count(&s1), 10);
    for (i = 0; i < 10; ++i) {
        test_eq(sbddextended_MySet_exists(&s1, i), 1);
    }
    test_eq(sbddextended_MySet_exists(&s1, 100), 0);
    test_eq(sbddextended_MySet_exists(&s1, 200), 0);

    sbddextended_MySet_deinitialize(&s1);
    sbddextended_MySet_deinitialize(&s);
}

#endif /* __cplusplus */

/* make zbdd representing {{2}} from file */
bddp construct_singleton(void)
{
    bddp f;
    FILE* fp;

    fp = test_fopen(g_filename1, "w");
    fputs("_i 2\n_o 1\n_n 1\n0 2 F T\n0\n", fp);
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    bddimportz(fp, &f, 1);
    fclose(fp);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    test_eq(bddcard(f), 1);
    test_eq(bddsize(f), 1);
    test_eq(bddgetvar(f), 2);
    test_eq(bddgetlev(f), 2);
    test(bddgetchild0z(f) == bddempty);
    test(bddgetchild1z(f) == bddsingle);
    return f;
}

void test_bddfunctions(void)
{
    bddp f, s2, g1, g2, g3, g4;
    bddp fs[10];
    bddp bps[10];
    bddvar vs2[2];
    int i;

    f = make_test_zbdd();

    test(bddisconstant(bddempty));
    test(bddisconstant(bddsingle));
    test(!bddisconstant(f));

    /* bddisnegative and bddtakenot will be tested at */
    /* test_getsingleandpowerset */

    test(bddis64bitversion());

    test(bddisterminal(bddempty));
    test(bddisterminal(bddsingle));
    test(!bddisterminal(f));

    test(!bddisemptymember(bddempty));
    test(bddisemptymember(bddsingle));
    test(!bddisemptymember(f));
    test(bddisemptymember(bddunion(f, bddsingle)));
    /* bddnull is an error sentinel, not a family containing the empty set */
    test(!bddisemptymember(bddnull));
    test(!bddismemberz(bddnull, NULL, 0));

    test_eq(bddgetvar(bddempty), 0);
    test_eq(bddgetvar(bddsingle), 0);
    test_eq(bddgetvar(f), 3);

    test_eq(bddgetlev(bddempty), 0);
    test_eq(bddgetlev(bddsingle), 0);
    test_eq(bddgetlev(f), 3);

    test(bddgetchild1z(bddgetchild0z(f))
            == bddgetchild0z(bddgetchild1z(f)));

    test(bddgetchild1g(bddgetchild0g(f, 1, 0), 1, 0)
            == bddgetchild0g(bddgetchild1g(f, 1, 0), 1, 0));

    test(bddgetchildg(bddgetchildg(f, 0, 1, 0), 1, 1, 0)
            == bddgetchildg(bddgetchildg(f, 1, 1, 0), 0, 1, 0));

    g1 = bddmakenodez(1, bddempty, bddsingle);
    g2 = bddmakenodez(2, bddempty, g1);
    g3 = bddmakenodez(2, g1, bddsingle);
    g4 = bddmakenodez(3, g2, g3);
    test(g4 == f);

    test(bddgetterminal(0, 0) == bddfalse);
    test(bddgetterminal(0, 1) == bddempty);
    test(bddgetterminal(1, 0) == bddtrue);
    test(bddgetterminal(1, 1) == bddsingle);

    s2 = construct_singleton();
    test(s2 == bddgetsingleton(2));
    test(bddrshift(s2, 1) == bddgetsingleton(1));

    for (i = 1; i <= 50; ++i) {
        test(bddlshift(s2, (bddvar)i) == bddgetsingleton((bddvar)(2 + i)));
    }

    /* test bddprimenot */
    f = bddat1(bddxor(bddprime((bddvar)1), bddprime((bddvar)2)),
               (bddvar)2); /* compute (x_1 xor x_2)|_{x_2 = 0}, i.e., bar(x_1) */
    test(f == bddprimenot((bddvar)1));

    for (i = 2; i <= 50; ++i) {
        f = bddat1(bddxor(bddprime((bddvar)i), bddprime((bddvar)1)),
                   (bddvar)1); /* compute (x_1 xor x_i)|_{x_i = 0}, i.e., bar(x_i) */
        test(f == bddprimenot((bddvar)i));
    }

    fs[8] = bddmakenodez(1, bddempty, bddsingle);
    fs[9] = bddmakenodez(1, bddsingle, bddsingle);
    fs[5] = bddmakenodez(2, fs[8], fs[9]);
    fs[6] = bddmakenodez(2, fs[9], bddsingle);
    fs[7] = bddmakenodez(2, bddsingle, fs[9]);
    fs[3] = bddmakenodez(2, bddempty, bddsingle);
    fs[4] = bddmakenodez(2, fs[9], fs[8]);
    fs[1] = bddmakenodez(3, fs[5], fs[6]);
    fs[2] = bddmakenodez(3, fs[6], fs[7]);

    bps[0] = fs[1];
    test_eq(bddcountnodes(NULL, 0, 0), 0);
    test_eq(bddcountnodes(bps, 1, 0), 5);
    bps[1] = bddnull;
    test_eq(bddcountnodes(bps, 2, 0), 0);
    bps[1] = fs[2];
    test_eq(bddcountnodes(bps, 2, 0), 7);
    bps[2] = fs[3];
    bps[3] = fs[4];
    test_eq(bddcountnodes(bps, 4, 0), 9);
    bps[4] = fs[8];
    test_eq(bddcountnodes(bps, 5, 0), 9);
    bps[2] = bddnull;
    test_eq(bddcountnodes(bps, 5, 0), 0);

    /* raw mode (is_raw = 1) must count a physical node once even when
       it is reached through both a positive and a negative reference,
       so the counts below must match bddsize */
    vs2[0] = 1;
    vs2[1] = 2;
    bps[0] = bddgetpowerset(vs2, 2);
    test_eq(bddcountnodes(bps, 1, 0), 2);
    test_eq(bddcountnodes(bps, 1, 1), (llint)bddsize(bps[0]));
    bddfree(bps[0]);
    g1 = bddxor(bddprime(1), bddprime(2));
    bps[0] = g1;
    test_eq(bddcountnodes(bps, 1, 0), 3);
    test_eq(bddcountnodes(bps, 1, 1), (llint)bddsize(g1));
    bddfree(g1);

    /* a BDD together with bddnull must return 0 (in both orders), not
       be reported as a BDD/ZBDD mixture */
    g1 = bddprime(1);
    bps[0] = g1;
    bps[1] = bddnull;
    test_eq(bddcountnodes(bps, 2, 0), 0);
    bps[0] = bddnull;
    bps[1] = g1;
    test_eq(bddcountnodes(bps, 2, 0), 0);
    bddfree(g1);
}

void test_getsingleandpowerset(void)
{
    int N = 35;
    int i;
    bddp f, f0, f1;
    bddvar* vararr;
    bddvar badarr[2];
    FILE* fp;

    vararr = (bddvar*)malloc((size_t)(N + 3) * sizeof(bddvar));
    if (vararr == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    for (i = 0; i < N; ++i) {
        vararr[i] = 2 * (bddvar)i + 1;
    }
    /* add duplicated values in purpose */
    vararr[N] = (bddvar)(2 * (135 % N) + 1);
    vararr[N + 1] = (bddvar)(2 * (223 % N) + 1);
    vararr[N + 2] = (bddvar)(2 * (157 % N) + 1);

    /* test getsingleset */
    f = bddgetsingleset(vararr, N + 3);

    test_eq(bddsize(f), N);
    test_eq(bddcard(f), 1);

    for (i = N - 1; i >= 0; --i) {
        test_eq(bddgetvar(f), 2 * i + 1);
        test_eq(bddgetlev(f), 2 * i + 1);
        test(bddgetchild0z(f) == bddempty);
        f = bddgetchild1z(f);
    }
    test(f == bddsingle);

    /* test getsinglesetv */

    f = bddgetsinglesetv(5, 2, 3, 5, 7, 11);

    test_eq(bddsize(f), 5);
    test_eq(bddcard(f), 1);

    fp = test_fopen(g_filename1, "w+");

    /* an empty file must match an empty expected string */
    test(is_expected_str(fp, ""));
    fprintf(stderr, "(the following \"unexpected str\" message is expected)\n");
    test(!is_expected_str(fp, "x"));
    fprintf(stderr, "(end of the expected message)\n");

    bddprintzbddelements(fp, f, "\n", " ");

    test(is_expected_str(fp, "11 7 5 3 2"));
    fclose(fp);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    test(bddgetsinglesetv(0) == bddsingle);

    /* duplicated arguments are regarded as one element, as in
       bddgetsingleset */
    test(bddgetsinglesetv(4, 3, 2, 3, 2) == bddgetsinglesetv(2, 2, 3));

    fprintf(stderr, "(the following \"out of range\" messages are expected)\n");
    badarr[0] = 1;
    badarr[1] = bddvarused() + 1;
    test(bddgetsingleset(badarr, 2) == bddnull);
    badarr[1] = 0;
    test(bddgetsingleset(badarr, 2) == bddnull);
    test(bddgetsinglesetv(2, 2, bddvarused() + 1) == bddnull);
    fprintf(stderr, "(end of the expected messages)\n");
    fprintf(stderr, "(the following \"negative\" messages are expected)\n");
    test(bddgetsingleset(badarr, -1) == bddnull);
    test(bddgetsinglesetv(-1) == bddnull);
    fprintf(stderr, "(end of the expected messages)\n");

    /* bddnull contains nothing (and no message is printed) */
    badarr[0] = 1;
    badarr[1] = 2;
    test(!bddismemberz(bddnull, badarr, 2));
    test(!bddismemberz(bddnull, badarr, 0));

    /* test getpowerset */
    /* the power set of the empty set is {{}}, i.e., bddsingle */
    test(bddgetpowerset(NULL, 0) == bddsingle);

    f = bddgetpowerset(vararr, N);

    test_eq(bddsize(f), N);
    /* bddcard saturates at bddnull (2^39 - 1 in the 64-bit build), so
       2^N must stay below it for the comparison below to be exact */
    test(N <= 38);
    test_eq(bddcard(f), (1llu << N)); /* 2^N */

    /* test also bddisnegative and bddtakenot */
    while (f != bddempty) {
        f0 = bddgetchild0zraw(f);
        f1 = bddgetchild1zraw(f);
        test(bddtakenot(f0) == f1);
        test(bddtakenot(f1) == f0);
        test(bddaddnot(f0) == f1);
        test(bddaddnot(f1) == f1);
        test(bdderasenot(f1) == f0);
        test(bdderasenot(f0) == f0);
        test(!bddisnegative(f0));
        test(bddisnegative(f1));
        f = f0;
    }

    /* test getpowersetn */
    {
        bddvar vs[3];
        bddp g;
        vs[0] = 1, vs[1] = 2, vs[2] = 3;
        g = bddgetpowerset(vs, 3);
        f = bddgetpowersetn(3);
        test(f == g);
        bddfree(f);
        bddfree(g);
        /* an invalid n must be rejected without ending the process */
        printf("(the following \"bddgetpowersetn\" messages are expected)\n");
        test(bddgetpowersetn(-1) == bddnull);
        test(bddgetpowersetn(INT_MAX) == bddnull);
        printf("(end of the expected messages)\n");
    }

    free(vararr);
}

void test_xrand(void)
{
    ullint state;
    ullint v1, v2;

    /* 0 is a fixed point of the raw XOR shift, so the helper must */
    /* escape it instead of returning 0 forever. */
    state = 0;
    v1 = sbddextended_getXRand(&state);
    test(v1 != 0);
    test(state != 0);
    v2 = sbddextended_getXRand(&state);
    test(v2 != 0);
    test(v2 != v1);

    state = 1;
    v1 = sbddextended_getXRand(&state);
    test(v1 != 0);
    test(state != 0);
}

void test_ismemberz(void)
{
    bddp f;
    bddvar vararr[3];

    f = make_test_zbdd();

    vararr[0] = 1, vararr[1] = 2, vararr[2] = 3;
    test(!bddismemberz(f, vararr, 3));

    vararr[0] = 1, vararr[1] = 2;
    test(bddismemberz(f, vararr, 2));

    vararr[0] = 1, vararr[1] = 3;
    test(bddismemberz(f, vararr, 2));

    vararr[0] = 2, vararr[1] = 3;
    test(bddismemberz(f, vararr, 2));

    vararr[0] = 1;
    test(!bddismemberz(f, vararr, 1));

    vararr[0] = 2;
    test(!bddismemberz(f, vararr, 1));

    vararr[0] = 3;
    test(!bddismemberz(f, vararr, 1));

    test(!bddismemberz(f, vararr, 0));

    /* duplicated variables are regarded as one element */
    vararr[0] = 1, vararr[1] = 2, vararr[2] = 2;
    test(bddismemberz(f, vararr, 3));

    vararr[0] = 2, vararr[1] = 2, vararr[2] = 1;
    test(bddismemberz(f, vararr, 3));

    vararr[0] = 1, vararr[1] = 1;
    test(!bddismemberz(f, vararr, 2));

    vararr[0] = 1, vararr[1] = 2, vararr[2] = 3;
    f = bddgetpowerset(vararr, 3);

    test(bddismemberz(f, vararr, 3));
    test(bddismemberz(f, vararr, 2));
    test(bddismemberz(f, vararr, 1));
    test(bddismemberz(f, vararr, 0));
}

void ullint_to_vararr(ullint v, bddvar* vararr, int* num)
{
    int count = 1;
    *num = 0;
    while (v > 0) {
        if ((v & 1llu) != 0) {
            vararr[*num] = (bddvar)count;
            ++(*num);
        }
        v >>= 1;
        ++count;
    }
}

void test_at_random(void)
{
    const size_t w = 30; /* number of variables */
    const size_t N = 1000; /* number of cardinality of the constructed ZDD */
    int i, j, num, found;
    ullint w_pow, c;
    ullint* ar;
    size_t sp = 0;
    bddp f, g, h;
    bddvar* vararr;
    FILE* fp;
    bddNodeIndex* node_index;

    w_pow = (1llu << w);

    ar = (ullint*)malloc(N * sizeof(ullint));
    if (ar == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    vararr = (bddvar*)malloc(w * sizeof(bddvar));
    if (vararr == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    /* make array whose elements are distinct */
    while (sp < N) {
        /* 2^32 is a multiple of w_pow (2^30), so a shift of 32
           would make the first draw contribute nothing to the
           remainder; 15 is the smallest number of bits that
           rand() gives on any implementation. */
        c = (((ullint)rand() << 15) | ((ullint)rand())) % w_pow;
        if (c == 0) {
            continue;
        }
        for (i = 0; i < (int)sp; ++i) {
            if (ar[i] == c) {
                break;
            }
        }
        if (i < (int)sp) {
            continue;
        }
        ar[sp] = c;
        ++sp;
    }

    f = bddempty;

    for (i = 0; i < (int)N; ++i) {
        ullint_to_vararr(ar[i], vararr, &num);
        g = bddgetsingleset(vararr, num);
        h = bddunion(f, g);
        bddfree(f);
        bddfree(g);
        f = h;
    }
    test_eq(bddcard(f), N);

    for (i = 0; i < (int)N; ++i) {
        ullint_to_vararr(ar[i], vararr, &num);
        test(bddismemberz(f, vararr, num));
    }

    for (i = 0; i < 2 * (int)N; ++i) {
        c = (ullint)rand() % w_pow;
        found = 0;
        for (j = 0; j < (int)N; ++j) {
            if (ar[j] == c) {
                found = 1;
                break;
            }
        }
        ullint_to_vararr(c, vararr, &num);
        test_eq(bddismemberz(f, vararr, num), found);
    }
    test(!bddismemberz(f, NULL, 0));

    g = bddunion(f, bddsingle);
    bddfree(f);
    f = g;
    test(bddismemberz(f, NULL, 0));

    fp = test_fopen(g_filename1, "w+");
    bddexportzbddasknuth(fp, f, 0, NULL);

    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    g = bddimportzbddasknuth(fp, 0, -1);

    test(f == g);

    fclose(fp);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    fp = test_fopen(g_filename1, "wb+");
    bddexportzbddasbinary(fp, f, 1, NULL);

    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    g = bddimportzbddasbinary(fp, -1);

    test(f == g);

    fclose(fp);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    node_index = bddNodeIndex_makeIndexZ(g);
    test_eq(bddNodeIndex_count(node_index), bddcard(g));
    bddNodeIndex_destruct(node_index);
    free(node_index);

    node_index = bddNodeIndex_makeRawIndexZ(g);
    test_eq(bddNodeIndex_count(node_index), bddcard(g));
    test_eq(bddNodeIndex_size(node_index), bddsize(g));
    bddNodeIndex_destruct(node_index);
    free(node_index);

    free(vararr);
    free(ar);
}

void test_io(void)
{
    bddp f, g, h;
    FILE* fp1;
    FILE* fp2;
    bddvar vararr[3];
    const char* var_name_map[] = {"dummy", "e", "d", "c", "b", "a"};

    /* open as binary because treating '\n' as a normal charactor */
    fp1 = test_fopen(g_filename1, "wb+");
    f = make_test_zbdd();
    bddprintzbddelements(fp1, f, "$", " ");

    test(is_expected_str(fp1, "3 2$3 1$2 1"));

    bddprintzbddelementswithmap(fp1, f, "$", " ", var_name_map);

    test(is_expected_str(fp1, "c d$c e$d e"));

    bddexportzbddasknuth(fp1, f, 0, NULL);
    test(is_expected_str(fp1, "#1\n2:3,4\n#2\n3:0,5\n4:5,1\n#3\n5:0,1\n"));

    fp2 = test_fopen(g_filename2, "w+");
    fputs("#1\n2:3,4\n#2\n3:0,5\n4:5,1\n#3\n5:0,1\n", fp2);
    fclose(fp2);

    fp2 = test_fopen(g_filename2, "r");

    g = bddimportzbddasknuth(fp2, 0, -1);

    fclose(fp2);
    if (remove(g_filename2) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    test(f == g);

    vararr[0] = 2, vararr[1] = 3, vararr[2] = 5;
    h = bddgetpowerset(vararr, 3);

    bddprintzbddelements(fp1, h, "!", " ");

    test(is_expected_str(fp1, "!5 3 2!5 3!5 2!5!3 2!3!2"));

    bddprintzbddelementswithmap(fp1, h, "\n", " ", var_name_map);

    test(is_expected_str(fp1, "\na c d\na c\na d\na\nc d\nc\nd"));

    fclose(fp1);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    fp1 = test_fopen(g_filename3, "w+");
    fputs("1 2\n1 3\n2 3\n", fp1);

    if (fseek(fp1, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    f = make_test_zbdd();
    g = bddconstructzbddfromelements(fp1);
    fclose(fp1);
    if (remove(g_filename3) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
    test(f == g);

    fp1 = test_fopen(g_filename3, "w+");
    fputs("\n1 2\n1 3\n2 3\n", fp1);

    if (fseek(fp1, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    f = make_test_zbdd();
    f = bddunion(f, bddsingle);
    g = bddconstructzbddfromelements(fp1);
    fclose(fp1);
    if (remove(g_filename3) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
    test(f == g);
}

/* graphviz 形式で出力された DD の構造を検査する。
   size は（否定枝表現を用いない）ノードの個数、height は根のレベル、
   t0_count は 0-終端が出力されるか（0-枝が 0-終端に到達しない DD、
   例えばべき集合の ZBDD では出力されない） */
void test_graphviz_content(FILE* fp, int size, int height, int t0_count)
{
    /* 1つのノードにつき、ノードの定義行と 0-枝、1-枝の行が出力される */
    test_eq(count_lines_containing(fp, "shape = circle"), size);
    test_eq(count_lines_containing(fp, ", style = dotted];"), size);
    test_eq(count_lines_containing(fp, ", penwidth = 2.5];"), size);
    /* 各レベルと終端のための rank 行 */
    test_eq(count_lines_containing(fp, "{rank = same;"), height + 1);
    /* レベルを縦に並べるための不可視のノードと枝 */
    test_eq(count_lines_containing(fp, "style = invis"), height + 2);
    /* 到達される終端だけが出力される */
    test_eq(count_lines_containing(fp, "t0 [label = \"0\""), t0_count);
    test_eq(count_lines_containing(fp, "t1 [label = \"1\""), 1);
    test_eq(count_lines_containing(fp, "digraph {"), 1);
}

/* f を graphviz 形式で filename に書き出す */
void export_graphviz_to_file(const char* filename, bddp f, int is_zbdd,
                                bddNodeIndex* node_index)
{
    FILE* fp;

    fp = test_fopen(filename, "wb");
    if (is_zbdd != 0) {
        bddexportzbddasgraphviz(fp, f, node_index);
    } else {
        bddexportbddasgraphviz(fp, f, node_index);
    }
    fclose(fp);
}

void test_graphviz_dd(bddp f, int is_zbdd, int t0_count)
{
    FILE* fp;
    bddNodeIndex* node_index;
    int size, height;

    if (is_zbdd != 0) {
        node_index = bddNodeIndex_makeIndexZWithoutCount(f);
    } else {
        node_index = bddNodeIndex_makeIndexBWithoutCount(f);
    }
    size = (int)bddNodeIndex_size(node_index);
    height = (int)bddgetlev(f);

    export_graphviz_to_file(g_filename1, f, is_zbdd, NULL);

    fp = test_fopen(g_filename1, "rb");
    test_graphviz_content(fp, size, height, t0_count);
    fclose(fp);

    /* あらかじめ構築したインデックスを渡しても同じ内容が出力される */
    export_graphviz_to_file(g_filename2, f, is_zbdd, node_index);
    test(is_same_file(g_filename1, g_filename2));

    bddNodeIndex_destruct(node_index);
    free(node_index);

    if (remove(g_filename1) != 0 || remove(g_filename2) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* 終端のみからなる DD では、終端の定義だけが出力される */
void test_graphviz_terminal(bddp f, int t0_count, int t1_count)
{
    FILE* fp;

    fp = test_fopen(g_filename1, "wb+");
    bddexportzbddasgraphviz(fp, f, NULL);
    test_eq(count_lines_containing(fp, "digraph {"), 1);
    test_eq(count_lines_containing(fp, "t0 [label = \"0\""), t0_count);
    test_eq(count_lines_containing(fp, "t1 [label = \"1\""), t1_count);
    test_eq(count_lines_containing(fp, "shape = circle"), 0);
    fclose(fp);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* 0-終端に到達する枝が 1 本もない ZBDD（{{},{1},{2}}）では、
   t0 のボックスが孤立ノードとして描かれないことを確認する */
void test_graphviz_unreachable_zero(void)
{
    FILE* fp;
    bddp s1, s2, u, f;

    s1 = bddgetsingleton(1); /* {{1}} */
    s2 = bddgetsingleton(2); /* {{2}} */
    u = bddunion(s1, s2);
    f = bddunion(u, bddsingle); /* {{},{1},{2}} */
    bddfree(s1);
    bddfree(s2);
    bddfree(u);

    fp = test_fopen(g_filename1, "wb+");
    bddexportzbddasgraphviz(fp, f, NULL);
    test_eq(count_lines_containing(fp, "digraph {"), 1);
    test_eq(count_lines_containing(fp, "t0 [label = \"0\""), 0);
    test_eq(count_lines_containing(fp, "t1 [label = \"1\""), 1);
    test_eq(count_lines_containing(fp, "shape = circle"), 2);
    test_eq(count_lines_containing(fp, "-> t0"), 0);
    fclose(fp);

    bddfree(f);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* raw モードで構築したインデックスを渡すと、エラーメッセージを表示して
   何も出力しないことを確認する */
void test_graphviz_raw_index(void)
{
    FILE* fp;
    bddp f;
    bddNodeIndex* node_index;

    fprintf(stderr, "(the following \"raw mode\" message is expected)\n");

    f = make_test_zbdd();
    node_index = bddNodeIndex_makeRawIndexZ(f);

    fp = test_fopen(g_filename1, "wb+");
    bddexportzbddasgraphviz(fp, f, node_index);
    test_eq(count_lines_containing(fp, "digraph {"), 0);
    test_eq(count_lines_containing(fp, "shape = circle"), 0);
    fclose(fp);

    bddNodeIndex_destruct(node_index);
    free(node_index);
    bddfree(f);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    fprintf(stderr, "(end of the expected message)\n");
}

void test_graphviz(void)
{
    bddp f, g;
    bddvar vararr[3];

    test_graphviz_terminal(bddempty, 1, 0);
    test_graphviz_terminal(bddsingle, 0, 1);
    test_graphviz_unreachable_zero();
    test_graphviz_raw_index();

    f = make_test_zbdd();
    test_graphviz_dd(f, 1, 1);
    bddfree(f);

    /* べき集合の ZBDD は 0-終端に到達する枝を持たないため、
       t0 は出力されない */
    vararr[0] = 2, vararr[1] = 3, vararr[2] = 5;
    f = bddgetpowerset(vararr, 3);
    test_graphviz_dd(f, 1, 0);
    bddfree(f);

    f = bddgetsingleton(1);
    test_graphviz_dd(f, 1, 1);
    bddfree(f);

    /* BDD の場合（非定数の BDD は必ず両方の終端に到達する） */
    f = bddand(bddprime(1), bddprime(2));
    g = bddor(f, bddprime(3));
    test_graphviz_dd(f, 0, 1);
    test_graphviz_dd(g, 0, 1);
    bddfree(f);
    bddfree(g);
}

/* content を elements 形式として読み込んだ結果が expected と一致することを
   確認する */
void test_elementsformat_content(const char* content, bddp expected)
{
    bddp f;
    FILE* fp;

    fp = test_fopen(g_filename3, "wb+");
    fputs(content, fp);

    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    f = bddconstructzbddfromelements(fp);
    fclose(fp);

    if (remove(g_filename3) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
    test(f == expected);
    bddfree(f);
}

/* f を elements 形式で出力してから読み込むと、元の ZBDD に戻ることを
   確認する */
void test_elementsformat_roundtrip(bddp f)
{
    bddp g;
    FILE* fp;

    fp = test_fopen(g_filename3, "wb+");
    bddprintzbddelements(fp, f, "\n", " ");

    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    g = bddconstructzbddfromelements(fp);
    fclose(fp);

    if (remove(g_filename3) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
    test(f == g);
    bddfree(g);
}

void test_elementsformat(void)
{
    bddp f;
    bddvar vararr[3];

    /* 1文字目が T の場合は空集合のみからなる族、
       B, E, F の場合は空の族を表す */
    test_elementsformat_content("T\n", bddsingle);
    test_elementsformat_content("B\n", bddempty);
    test_elementsformat_content("E\n", bddempty);
    test_elementsformat_content("F\n", bddempty);
    /* the exporter writes "N" for bddnull; the round trip must work
       without a format error */
    test_elementsformat_content("N\n", bddnull);

    /* 空行は空集合を表す */
    test_elementsformat_content("\n", bddsingle);

    f = make_test_zbdd(); /* {{1, 2}, {1, 3}, {2, 3}} */
    test_elementsformat_content("1 2\n1 3\n2 3\n", f);
    /* 集合や変数の順序は任意でよく、区切りは空白文字の並びでよい */
    test_elementsformat_content("2 1\n3 1\n  3   2  \n", f);
    test_elementsformat_content("1\t2\n1\t3\n2\t3\n", f);
    /* 最終行に改行がなくてもよい */
    test_elementsformat_content("1 2\n1 3\n2 3", f);
    /* 同じ集合が複数回現れてもよい */
    test_elementsformat_content("1 2\n2 3\n1 3\n1 2\n", f);

    test_elementsformat_roundtrip(f);
    bddfree(f);

    test_elementsformat_roundtrip(bddempty);
    test_elementsformat_roundtrip(bddsingle);
    test_elementsformat_roundtrip(bddnull);

    vararr[0] = 2, vararr[1] = 3, vararr[2] = 5;
    f = bddgetpowerset(vararr, 3);
    test_elementsformat_roundtrip(f);
    bddfree(f);
}

void test_index(void)
{
    int i, count;
    bddp f;
    bddNodeIndex* node_index;
    bddNodeIterator* itor;
    bddvar vararr[40];
    ullint level_sizes[4];

    f = make_test_zbdd();
    node_index = bddNodeIndex_makeIndexZ(f);
    test_eq(bddNodeIndex_count(node_index), 3);
    test_eq(bddNodeIndex_size(node_index), 4);

    /* sizeEachLevel overwrites the whole output including arr[0] */
    test_eq(node_index->height, 3);
    for (i = 0; i <= 3; ++i) {
        level_sizes[i] = 0xdeadbeefull; /* sentinel */
    }
    bddNodeIndex_sizeEachLevel(node_index, level_sizes);
    test_eq(level_sizes[0], 0);
    for (i = 1; i <= 3; ++i) {
        test_eq(level_sizes[i], bddNodeIndex_sizeAtLevel(node_index, i));
    }

    itor = bddNodeIterator_make(node_index);
    count = 0;
    while (bddNodeIterator_hasNext(itor)) {
        bddNodeIterator_next(itor);
        ++count;
    }
    test_eq(count, 4);
    bddNodeIterator_destruct(itor);
    bddNodeIndex_destruct(node_index);
    free(node_index);

    for (i = 0; i < 40; ++i) {
        vararr[i] = (bddvar)i + 1;
    }
    f = bddgetpowerset(vararr, 40);
    node_index = bddNodeIndex_makeIndexZ(f);
    test_eq(bddNodeIndex_count(node_index), 1ll << 40);
    test_eq(bddNodeIndex_size(node_index), 40);
    bddNodeIndex_destruct(node_index);
    free(node_index);

    f = make_test_zbdd();
    node_index = bddNodeIndex_makeRawIndexZ(f);
    test_eq(bddNodeIndex_count(node_index), 3);
    test_eq(bddNodeIndex_size(node_index), 4);
    bddNodeIndex_destruct(node_index);
    free(node_index);

    f = bddgetpowerset(vararr, 40);
    node_index = bddNodeIndex_makeRawIndexZ(f);
    test_eq(bddNodeIndex_count(node_index), 1ll << 40);
    test_eq(bddNodeIndex_size(node_index), bddsize(f));
    bddNodeIndex_destruct(node_index);
    free(node_index);

    /* for a terminal, sizeEachLevel writes the height-0 result (arr[0]) */
    node_index = bddNodeIndex_makeIndexZ(bddsingle);
    test_eq(node_index->height, 0);
    level_sizes[0] = 0xdeadbeefull; /* sentinel */
    bddNodeIndex_sizeEachLevel(node_index, level_sizes);
    test_eq(level_sizes[0], 0);
    bddNodeIndex_destruct(node_index);
    free(node_index);

    /* the kind check accepts a terminal for either kind because a
       terminal is shared between the BDD and ZBDD representations
       (the auto-detecting make functions record it as a ZBDD) */
    node_index = bddNodeIndex_makeIndex(bddtrue);
    test_eq(bddNodeIndex_checkIndexOf(node_index, bddtrue, 0), 1);
    test_eq(bddNodeIndex_checkIndexOf(node_index, bddtrue, 1), 1);
    bddNodeIndex_destruct(node_index);
    free(node_index);

    /* the index owns a reference to f, so it stays usable after the
       caller releases f and the garbage collection runs */
    f = make_test_zbdd();
    node_index = bddNodeIndex_makeIndexZ(f);
    bddfree(f);
    bddgc();
    test_eq(bddNodeIndex_count(node_index), 3);
    test_eq(bddNodeIndex_size(node_index), 4);
    bddNodeIndex_destruct(node_index);
    free(node_index);
}

void test_index_copy(void)
{
    int i;
    size_t j;
    llint value;
    bddp f, node;
    bddNodeIndex* node_index;
    bddNodeIndex* copied;
    ullint count, size;
    ullint size_at_level[8];

    f = make_test_zbdd();
    node_index = bddNodeIndex_makeIndexZ(f);
    count = bddNodeIndex_count(node_index);
    size = bddNodeIndex_size(node_index);
    test(node_index->height < 8);
    for (i = 1; i <= node_index->height; ++i) {
        size_at_level[i] = bddNodeIndex_sizeAtLevel(node_index, i);
    }

    copied = (bddNodeIndex*)malloc(sizeof(bddNodeIndex));
    if (copied == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    bddNodeIndex_copy(copied, node_index);

    /* destruct the source so that the copy must be independent of it */
    bddNodeIndex_destruct(node_index);
    free(node_index);

    test_eq(bddNodeIndex_count(copied), count);
    test_eq(bddNodeIndex_size(copied), size);
    test_eq(copied->is_zbdd, 1);
    test_eq(copied->is_raw, 0);
    for (i = 1; i <= copied->height; ++i) {
        test_eq(bddNodeIndex_sizeAtLevel(copied, i), size_at_level[i]);
        /* every node in level_vec_arr must be registered in node_dict_arr */
        /* with its position in the level as the value */
        for (j = 0; j < copied->level_vec_arr[i].count; ++j) {
            node = (bddp)sbddextended_MyVector_get(&copied->level_vec_arr[i],
                                                    (llint)j);
            test_eq(sbddextended_MyDict_find(&copied->node_dict_arr[i],
                                                (llint)node, &value), 1);
            test_eq(value, (llint)j);
        }
    }
    bddNodeIndex_destruct(copied);
    free(copied);

    /* the case that f is a terminal, where all the arrays are NULL */
    node_index = bddNodeIndex_makeIndexZ(bddsingle);
    copied = (bddNodeIndex*)malloc(sizeof(bddNodeIndex));
    if (copied == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    bddNodeIndex_copy(copied, node_index);
    bddNodeIndex_destruct(node_index);
    free(node_index);

    test_eq(bddNodeIndex_count(copied), 1);
    test_eq(bddNodeIndex_size(copied), 0);
    test_eq(copied->is_zbdd, 1);
    test(copied->node_dict_arr == NULL);
    test(copied->level_vec_arr == NULL);
    test(copied->offset_arr == NULL);
    test(copied->count_arr == NULL);
    bddNodeIndex_destruct(copied);
    free(copied);
}

void test_elementIterator(void)
{
    bddp f;
    bddvar* arr;
    bddElementIterator* itor;
    bddp g;

    f = make_test_zbdd();
    /* f is expected to be {{3, 2}, {3, 1}, {2, 1}} */
    /* bddElementIterator_getValue needs bddgetlev(f) + 1 elements: the
       variables of the element and the terminator. */
    arr = (bddvar*)malloc((size_t)(bddgetlev(f) + 1) * sizeof(bddvar));
    if (arr == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    itor = bddElementIterator_make(f);
    test(bddElementIterator_hasNext(itor) != 0);
    bddElementIterator_next(itor, arr);
    test_eq(arr[0], 3);
    test_eq(arr[1], 2);
    test_eq(arr[2], (bddvar)-1);

    test(bddElementIterator_hasNext(itor) != 0);
    bddElementIterator_next(itor, arr);
    test_eq(arr[0], 3);
    test_eq(arr[1], 1);
    test_eq(arr[2], (bddvar)-1);

    test(bddElementIterator_hasNext(itor) != 0);
    bddElementIterator_next(itor, arr);
    test_eq(arr[0], 2);
    test_eq(arr[1], 1);
    test_eq(arr[2], (bddvar)-1);

    test(bddElementIterator_hasNext(itor) == 0);

    bddElementIterator_destruct(itor);

    g = bddunion(f, bddsingle);
    /* g is expected to be {{}, {3, 2}, {3, 1}, {2, 1}} */

    itor = bddElementIterator_make(g);
    test(bddElementIterator_hasNext(itor) != 0);
    bddElementIterator_next(itor, arr);
    test_eq(arr[0], 3);
    test_eq(arr[1], 2);
    test_eq(arr[2], (bddvar)-1);

    test(bddElementIterator_hasNext(itor) != 0);
    bddElementIterator_next(itor, arr);
    test(bddElementIterator_hasNext(itor) != 0);
    bddElementIterator_next(itor, arr);
    test(bddElementIterator_hasNext(itor) != 0);
    bddElementIterator_next(itor, arr);
    test_eq(arr[0], (bddvar)-1);
    test(bddElementIterator_hasNext(itor) == 0);

    /* calling next after the end is a complete no-op: arr must not */
    /* be written either */
    arr[0] = 12345;
    bddElementIterator_next(itor, arr);
    test(bddElementIterator_hasNext(itor) == 0);
    test_eq(arr[0], 12345);

    bddElementIterator_destruct(itor);

    /* destructing NULL is safe (like free) */
    bddElementIterator_destruct(NULL);

    /* bddempty test */
    itor = bddElementIterator_make(bddempty);
    test(bddElementIterator_hasNext(itor) == 0);
    bddElementIterator_destruct(itor);

    /* bddsingle test */
    itor = bddElementIterator_make(bddsingle);
    test(bddElementIterator_hasNext(itor) != 0);
    bddElementIterator_next(itor, arr);
    test_eq(arr[0], (bddvar)-1);
    test(bddElementIterator_hasNext(itor) == 0);
    bddElementIterator_destruct(itor);

    /* a BDD that is not a terminal must be rejected */
    fprintf(stderr, "(the following \"only ZDD\" message is expected)\n");
    g = bddand(bddprime(1), bddprime(2));
    itor = bddElementIterator_make(g);
    test(bddElementIterator_hasNext(itor) == 0);
    bddElementIterator_destruct(itor);
    bddfree(g);
    fprintf(stderr, "(end of the expected message)\n");

    free(arr);
}

void test_bddbinaryformat_f(bddp f)
{
    bddp g;
    FILE* fp;

    fp = test_fopen(g_filename1, "wb+");
    bddexportzbddasbinary(fp, f, 1, NULL);

    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    g = bddimportzbddasbinary(fp, -1);

    fclose(fp);
    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    test(f == g);
}

/* buf の先頭 len バイトだけからなるファイルを読み込ませ、
   bddnull が返ることを確認する */
void test_bddbinaryformat_truncated_len(const unsigned char* buf, long len)
{
    bddp g;
    FILE* fp;

    fp = test_fopen(g_filename1, "wb");
    if (len > 0) {
        if (fwrite(buf, (size_t)1, (size_t)len, fp) != (size_t)len) {
            fprintf(stderr, "fwrite failed\n");
            exit(1);
        }
    }
    fclose(fp);

    fp = test_fopen(g_filename1, "rb");
    g = bddimportzbddasbinary(fp, -1);
    fclose(fp);

    test(g == bddnull);
}

/* 切り詰められたバイナリを読み込んでも、未初期化の値を使わずに
   bddnull を返すことを確認する */
void test_bddbinaryformat_truncated(void)
{
    bddp f;
    FILE* fp;
    long file_size, len;
    unsigned char* buf;

    f = make_test_zbdd();

    fp = test_fopen(g_filename1, "wb+");
    bddexportzbddasbinary(fp, f, 1, NULL);
    file_size = ftell(fp);
    test(file_size > 0);

    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    buf = (unsigned char*)malloc((size_t)file_size);
    if (buf == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    if (fread(buf, (size_t)1, (size_t)file_size, fp) != (size_t)file_size) {
        fprintf(stderr, "fread failed\n");
        exit(1);
    }
    fclose(fp);

    fprintf(stderr, "(the following \"Unexpected end\" messages are expected)\n");

    /* ヘッダ部・レベルごとのノード数・根 ID・ノード列の各所で切り詰める */
    for (len = 0; len < file_size; len = len * 2 + 1) {
        test_bddbinaryformat_truncated_len(buf, len);
    }
    test_bddbinaryformat_truncated_len(buf, file_size / 4);
    test_bddbinaryformat_truncated_len(buf, file_size / 2);
    test_bddbinaryformat_truncated_len(buf, file_size * 3 / 4);
    test_bddbinaryformat_truncated_len(buf, file_size - 1);

    fprintf(stderr, "(end of the expected messages)\n");

    free(buf);
    bddfree(f);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* buf の offset バイト目の uint64 を value に書き換えたファイルを
   読み込ませ、bddnull が返ることを確認する */
void test_bddbinaryformat_corrupted_at(const unsigned char* buf, long file_size,
                                       long offset, ullint value)
{
    bddp g;
    FILE* fp;
    unsigned char* buf2;

    buf2 = (unsigned char*)malloc((size_t)file_size);
    if (buf2 == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    memcpy(buf2, buf, (size_t)file_size);
    /* バイナリ形式はリトルエンディアン固定なので、ホストのバイト順に
       依存する memcpy ではなく明示的な変換で書き込む */
    sbddextended_uint64ToBytes(value, buf2 + offset);

    fp = test_fopen(g_filename1, "wb");
    if (fwrite(buf2, (size_t)1, (size_t)file_size, fp) != (size_t)file_size) {
        fprintf(stderr, "fwrite failed\n");
        exit(1);
    }
    fclose(fp);
    free(buf2);

    fp = test_fopen(g_filename1, "rb");
    g = bddimportzbddasbinary(fp, -1);
    fclose(fp);

    test(g == bddnull);
}

/* 範囲外のノード ID を含むバイナリを読み込んでも、未初期化の bddp を
   使ったり領域外を読んだりせずに bddnull を返すことを確認する */
void test_bddbinaryformat_corrupted(void)
{
    bddp f;
    FILE* fp;
    long file_size, header_size, root_id_offset, node_offset;
    ullint max_level;
    unsigned char* buf;

    /* ヘッダのサイズ（'BDD' + version + type + number_of_arcs
       + number_of_terminals + number_of_bits_for_level
       + number_of_bits_for_id + use_negative_arcs + max_level
       + number_of_roots + reserved）*/
    header_size = 3 + 1 + 1 + 2 + 4 + 1 + 1 + 1 + 8 + 8 + 64;

    f = make_test_zbdd();

    fp = test_fopen(g_filename1, "wb+");
    bddexportzbddasbinary(fp, f, 1, NULL);
    file_size = ftell(fp);
    test(file_size > header_size);

    if (fseek(fp, 0l, SEEK_SET) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    buf = (unsigned char*)malloc((size_t)file_size);
    if (buf == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    if (fread(buf, (size_t)1, (size_t)file_size, fp) != (size_t)file_size) {
        fprintf(stderr, "fread failed\n");
        exit(1);
    }
    fclose(fp);

    /* max_level はヘッダの 14 バイト目から 8 バイト（リトルエンディアン） */
    max_level = sbddextended_bytesToUint64(buf + 14);
    root_id_offset = header_size + (long)max_level * 8;
    node_offset = root_id_offset + 8;
    /* 1ノードは 0-child と 1-child の ID（各 8 バイト）からなる */
    test_eq((file_size - node_offset) % 16, 0);
    test(file_size > node_offset);

    fprintf(stderr, "(the following \"out of range\" / \"Cannot allocate\" "
                    "messages are expected)\n");

    /* レベル 1 のノード数が確保不可能なほど大きい（2^56 個）。
       64 ビット環境ではノード用バッファの malloc が失敗し、32 ビット
       環境では総ノード数の上限チェックに掛かる。どちらの場合も
       exit(1) でプロセスが終了せず bddnull が返ることを確認する */
    test_bddbinaryformat_corrupted_at(buf, file_size, header_size,
                                      0x0100000000000000ull);
    /* 根の ID が範囲外 */
    test_bddbinaryformat_corrupted_at(buf, file_size, root_id_offset,
                                      0xfffffffffffffffeull);
    /* 先頭ノードの 0-child / 1-child が範囲外 */
    test_bddbinaryformat_corrupted_at(buf, file_size, node_offset,
                                      0xfffffffffffffffeull);
    test_bddbinaryformat_corrupted_at(buf, file_size, node_offset + 8,
                                      0xfffffffffffffffeull);
    /* 先頭ノードの 0-child / 1-child がまだ読んでいないノード（自分自身）を指す。
       否定枝を使うので ID は 2 倍されている */
    test_bddbinaryformat_corrupted_at(buf, file_size, node_offset, 2ull * 2ull);
    test_bddbinaryformat_corrupted_at(buf, file_size, node_offset + 8, 2ull * 2ull);

    /* 壊れたバイナリを bddvarused() より大きい root_level で読み込ませ
       ても、構造検査を終える前には変数が追加されないため、グローバル
       な変数表が変化しないことを確認する（直前の呼び出しが g_filename1
       に残した、1-child が同一レベルの先行ノードを指すバイナリを使う） */
    {
        bddvar used_before = bddvarused();
        bddp g;
        FILE* fp2 = test_fopen(g_filename1, "rb");
        g = bddimportzbddasbinary(fp2, (int)used_before + 5);
        fclose(fp2);
        test(g == bddnull);
        test_eq(bddvarused(), used_before);
    }

    fprintf(stderr, "(end of the expected messages)\n");

    free(buf);
    bddfree(f);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

void test_bddbinaryformat(void)
{
    bddp f;

    test_bddbinaryformat_f(bddempty);
    test_bddbinaryformat_f(bddsingle);

    f = bddgetsingleton(1);
    test_bddbinaryformat_f(f);
    bddfree(f);

    f = make_test_zbdd();
    test_bddbinaryformat_f(f);
    bddfree(f);

    test_bddbinaryformat_truncated();
    test_bddbinaryformat_corrupted();
}

/* 空のファイルを読み込んでも、未初期化の buf を参照せずに
   bddnull を返すことを確認する */
void test_graphillionformat_empty(void)
{
    bddp f;
    FILE* fp;

    fp = test_fopen(g_filename1, "w");
    fclose(fp);

    fprintf(stderr, "(the following \"Unexpected end\" messages are expected)\n");

    fp = test_fopen(g_filename1, "r");
    f = bddimportbddasgraphillion(fp, -1);
    fclose(fp);
    test(f == bddnull);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasgraphillion(fp, -1);
    fclose(fp);
    test(f == bddnull);

    fprintf(stderr, "(end of the expected messages)\n");

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* ファイル中の最大レベルより大きい root_level を指定しても、必要な数の
   変数が作られ、指定した根のレベルで復元されることを確認する */
void test_graphillionformat_root_level(void)
{
    const int root_level = 120;
    bddp f, g;
    FILE* fp;

    /* ノードは 1 つだけ（graphillion のレベル 1 = 根のレベル）。
       ファイル中の最大レベルは 1 で、root_level より小さい */
    fp = test_fopen(g_filename1, "w");
    fprintf(fp, "0 1 B T\n.\n");
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasgraphillion(fp, root_level);
    fclose(fp);

    test(f != bddnull);
    test((int)bddvarused() >= root_level);
    g = bddgetsingleton(bddvaroflev((bddvar)root_level));
    test(f == g);
    bddfree(f);
    bddfree(g);

    fp = test_fopen(g_filename1, "r");
    f = bddimportbddasgraphillion(fp, root_level);
    fclose(fp);

    test(f != bddnull);
    g = bddprime(bddvaroflev((bddvar)root_level));
    test(f == g);
    bddfree(f);
    bddfree(g);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* exporter が DD の高さより小さい正の root_level を拒否し、
   不正なレベル（0 以下）を含むデータを出力しないことを確認する */
void test_graphillionformat_export_root_level(void)
{
    bddp f, g;
    FILE* fp;
    long file_size;

    while (bddvarused() < 3) {
        bddnewvar();
    }
    /* 根のレベルが 3 の ZBDD（高さ 3） */
    f = bddgetsingleton(bddvaroflev(3));

    fprintf(stderr, "(the following \"root_level\" messages are expected)\n");

    /* root_level (1) < 高さ (3) はエラーになり、何も出力されない */
    fp = test_fopen(g_filename1, "w");
    bddexportzbddasgraphillion(fp, f, NULL, 1);
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    file_size = ftell(fp);
    fclose(fp);
    test(file_size == 0);

    fprintf(stderr, "(end of the expected messages)\n");

    /* root_level == 高さは受理され、往復で同じ ZBDD に戻る */
    fp = test_fopen(g_filename1, "w");
    bddexportzbddasgraphillion(fp, f, NULL, 3);
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    /* graphillion 形式のレベルは根からの相対値なので、
       もとのレベルに戻すには import 側にも root_level を渡す */
    g = bddimportzbddasgraphillion(fp, 3);
    fclose(fp);
    test(g == f);
    bddfree(g);
    bddfree(f);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* content の内容のファイルを graphillion 形式として読み込ませ、
   bddnull が返ることを確認する */
void test_graphillionformat_corrupted_content(const char* content)
{
    bddp f;
    FILE* fp;

    fp = test_fopen(g_filename1, "w");
    fputs(content, fp);
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasgraphillion(fp, -1);
    fclose(fp);
    test(f == bddnull);

    fp = test_fopen(g_filename1, "r");
    f = bddimportbddasgraphillion(fp, -1);
    fclose(fp);
    test(f == bddnull);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* 未登録の子ノード ID を含むファイルを読み込んでも、未初期化値を bddp として
   使わずに bddnull を返すことを確認する */
void test_graphillionformat_corrupted(void)
{
    fprintf(stderr, "(the following \"is not found\" messages are expected)\n");

    /* 0-child (9) が定義されていない */
    test_graphillionformat_corrupted_content("0 1 9 T\n.\n");
    /* 1-child (9) が定義されていない */
    test_graphillionformat_corrupted_content("0 1 B 9\n.\n");
    /* 子ノード (1) が自分より後に定義されている（前方参照） */
    test_graphillionformat_corrupted_content("0 1 1 T\n1 2 B T\n.\n");

    /* 数値トークンの後にゴミが続く */
    test_graphillionformat_corrupted_content("0junk 1 B T\n.\n");
    test_graphillionformat_corrupted_content("0 1junk B T\n.\n");
    /* 終端トークンの後にゴミが続く */
    test_graphillionformat_corrupted_content("0 1 Bjunk T\n.\n");
    test_graphillionformat_corrupted_content("0 1 B Tjunk\n.\n");
    /* ID + 2 が llint でオーバーフローする（LLONG_MAX） */
    test_graphillionformat_corrupted_content(
        "9223372036854775807 1 B T\n.\n");
    /* ID が llint の範囲を超える */
    test_graphillionformat_corrupted_content(
        "99999999999999999999 1 B T\n.\n");
    /* ID が負 */
    test_graphillionformat_corrupted_content("-1 1 B T\n.\n");
    /* レベルが 0 */
    test_graphillionformat_corrupted_content("0 0 B T\n.\n");
    /* 余分な 5 番目のトークン */
    test_graphillionformat_corrupted_content("0 1 B T x\n.\n");

    /* 0-child (1) が同じレベル (2) のノード（graphillion 形式のレベルは
       根からの相対値なので、子のレベルは親より真に大きくなければ
       ならない） */
    test_graphillionformat_corrupted_content("1 2 B T\n2 2 1 T\n.\n");
    /* 1-child (1) が同じレベルのノード */
    test_graphillionformat_corrupted_content("1 2 B T\n2 2 B 1\n.\n");
    /* 0-child (1) のレベル (1) が親のレベル (2) より小さい */
    test_graphillionformat_corrupted_content("1 1 B T\n2 2 1 T\n.\n");

    fprintf(stderr, "(end of the expected messages)\n");
}

/* content の内容のファイルを Knuth 形式として読み込ませ、
   bddnull が返ることを確認する */
void test_knuthformat_empty_content(const char* content)
{
    bddp f;
    FILE* fp;

    fp = test_fopen(g_filename1, "w");
    fputs(content, fp);
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    f = bddimportbddasknuth(fp, 0, -1);
    fclose(fp);
    test(f == bddnull);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasknuth(fp, 0, -1);
    fclose(fp);
    test(f == bddnull);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* ノードが 1 つも含まれないファイルを読み込んでも、bddnode_buf の
   領域外を読まずに bddnull を返すことを確認する */
void test_knuthformat_empty(void)
{
    fprintf(stderr, "(the following \"Unexpected end\" messages are expected)\n");

    /* 空のファイル */
    test_knuthformat_empty_content("");
    /* レベルのヘッダ行のみ */
    test_knuthformat_empty_content("#1\n");
    /* ノード行のないヘッダ行の並び */
    test_knuthformat_empty_content("#1\n#2\n");

    fprintf(stderr, "(end of the expected messages)\n");
}

/* content の内容のファイルを Knuth 形式として読み込ませ、
   bddnull が返ることを確認する */
void test_knuthformat_corrupted_content(const char* content, int is_hex)
{
    bddp f;
    FILE* fp;

    fp = test_fopen(g_filename1, "w");
    fputs(content, fp);
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    f = bddimportbddasknuth(fp, is_hex, -1);
    fclose(fp);
    test(f == bddnull);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasknuth(fp, is_hex, -1);
    fclose(fp);
    test(f == bddnull);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* 範囲外の子ノード ID や、より低いレベルにない子ノード ID を含む
   ファイルを読み込んでも、bddnode_buf の領域外を読んだり誤った DD を
   返したりせずに bddnull を返すことを確認する */
void test_knuthformat_corrupted(void)
{
    fprintf(stderr, "(the following \"lower level\" messages are expected)\n");

    /* 0-child (9) がノード数を超えている */
    test_knuthformat_corrupted_content("#1\n2:9,1\n", 0);
    /* 1-child (9) がノード数を超えている */
    test_knuthformat_corrupted_content("#1\n2:0,9\n", 0);
    /* 0-child (2) が自分自身 */
    test_knuthformat_corrupted_content("#1\n2:2,1\n", 0);
    /* 0-child (2) が自分 (3) より小さい（まだ構築されていないノード） */
    test_knuthformat_corrupted_content("#1\n2:0,1\n#2\n3:2,0\n", 0);
    /* ノード 3 の構築後にノード 2 でエラーになる（構築済みノードの解放） */
    test_knuthformat_corrupted_content("#1\n2:9,1\n#2\n3:0,1\n", 0);
    /* 16 進表記で、符号付きに変換すると負になる 0-child */
    test_knuthformat_corrupted_content("#1\n2:ffffffffffffffff,1\n", 1);
    /* 0-child (3) が同じレベルのノード */
    test_knuthformat_corrupted_content("#1\n2:3,1\n3:0,1\n", 0);
    /* 1-child (3) が同じレベルのノード */
    test_knuthformat_corrupted_content("#1\n2:0,3\n3:0,1\n", 0);
    /* 0-child (4) が同じレベル (2) のノード */
    test_knuthformat_corrupted_content("#1\n2:0,3\n#2\n3:4,1\n4:0,1\n", 0);

    fprintf(stderr, "(end of the expected messages)\n");
}

/* レベルのヘッダ行が #1, #2, ... の順になっていないファイルを読み込むと、
   NDEBUG の有無によらず bddnull が返ることを確認する */
void test_knuthformat_wrong_level(void)
{
    fprintf(stderr, "(the following \"Format error\" messages are expected)\n");

    /* 最初のヘッダ行が #1 でない */
    test_knuthformat_corrupted_content("#2\n2:0,1\n", 0);
    /* 2 番目のヘッダ行が #2 でない（レベルの飛ばし） */
    test_knuthformat_corrupted_content("#1\n2:0,1\n#3\n3:2,0\n", 0);
    /* ヘッダ行のレベルが逆順 */
    test_knuthformat_corrupted_content("#1\n2:0,1\n#1\n3:2,0\n", 0);

    fprintf(stderr, "(end of the expected messages)\n");
}

/* レベルヘッダ行の数が bddvarmax を超えるファイルは、全体を読み
   終える前に bddnull で拒否されることを確認する */
void test_knuthformat_too_many_levels(void)
{
    bddp f;
    FILE* fp;
    int i;

    fprintf(stderr, "(the following \"number of levels\" message is "
            "expected)\n");

    fp = test_fopen(g_filename1, "w");
    /* #1 から #(bddvarmax + 1) までのヘッダ行。高さが bddvarmax を
       超えた時点で拒否されなければならない */
    for (i = 1; i <= (int)bddvarmax + 1; ++i) {
        fprintf(fp, "#%d\n", i);
    }
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasknuth(fp, 0, -1);
    fclose(fp);
    test(f == bddnull);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    fprintf(stderr, "(end of the expected message)\n");
}

/* 最初のレベルヘッダ行より前に置かれた行が読み飛ばされずに、
   bddnull が返ることを確認する */
void test_knuthformat_leading_garbage(void)
{
    fprintf(stderr, "(the following \"Format error\" messages are expected)\n");

    /* 先頭のごみ行 */
    test_knuthformat_corrupted_content("garbage\n#1\n2:0,1\n", 0);
    /* ヘッダ行より前のノード行 */
    test_knuthformat_corrupted_content("2:0,1\n#1\n2:0,1\n", 0);
    /* ごみ行の後の端子 */
    test_knuthformat_corrupted_content("garbage\n0\n", 0);
    /* 端子行の後にごみが続く */
    test_knuthformat_corrupted_content("0garbage\n", 0);
    /* 空行 */
    test_knuthformat_corrupted_content("\n#1\n2:0,1\n", 0);

    fprintf(stderr, "(end of the expected messages)\n");
}

/* content の内容のファイルを要素形式として読み込ませ、
   bddnull が返ることを確認する */
void test_elementsformat_out_of_range_content(const char* content)
{
    bddp f;
    FILE* fp;

    fp = test_fopen(g_filename1, "w");
    fputs(content, fp);
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    f = bddconstructzbddfromelements(fp);
    fclose(fp);
    test(f == bddnull);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* 使用中の変数の個数を超える変数番号を含むファイルを読み込んでも、
   異常終了せずに bddnull を返すことを確認する
   （変数番号 999999 は bddvarmax を超えるので、常に範囲外である） */
void test_elementsformat_out_of_range(void)
{
    fprintf(stderr, "(the following \"out of range\" messages are expected)\n");

    /* 変数番号 0 は存在しない */
    test_elementsformat_out_of_range_content("0\n");
    /* 変数番号が使用中の変数の個数を超えている */
    test_elementsformat_out_of_range_content("999999\n");
    /* 2 行目でエラーになる（構築済みの ZBDD の解放） */
    test_elementsformat_out_of_range_content("1 2\n1 999999\n");
    /* 最終行に改行がない場合 */
    test_elementsformat_out_of_range_content("1 2\n1 999999");
    /* int に収まらない変数番号（桁あふれ） */
    test_elementsformat_out_of_range_content("99999999999999999999\n");
    test_elementsformat_out_of_range_content("1 2\n1 99999999999999999999\n");

    fprintf(stderr, "(end of the expected messages)\n");
}

/* content の内容のファイルを要素形式として読み込ませ、
   expected が返ることを確認する */
void test_elementsformat_terminal_content(const char* content, bddp expected)
{
    bddp f;
    FILE* fp;

    fp = test_fopen(g_filename1, "w");
    fputs(content, fp);
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    f = bddconstructzbddfromelements(fp);
    fclose(fp);
    test(f == expected);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* 端子マーカー（T/B/E/F）は、前後の空白の有無によらず入力全体を
   表すときだけ受理され、後ろにごみが続く場合や 2 行目以降に現れる
   場合は bddnull が返ることを確認する */
void test_elementsformat_terminal(void)
{
    /* マーカーのみ */
    test_elementsformat_terminal_content("T", bddtrue);
    test_elementsformat_terminal_content("B", bddfalse);
    test_elementsformat_terminal_content("E", bddfalse);
    test_elementsformat_terminal_content("F", bddfalse);
    /* 前後の空白や末尾の改行は許容される */
    test_elementsformat_terminal_content(" T", bddtrue);
    test_elementsformat_terminal_content("T \n", bddtrue);
    test_elementsformat_terminal_content("  E\n", bddfalse);

    /* 空の入力（0 バイト、空白のみ、空行 1 つ）は改行のない 1 つの空行と
       同じく、空集合のみからなる集合族 {{}} である（空の族ではない）。
       区切り文字指定版のエクスポータが {{}} を空の出力として書き出すのと
       対応している。 */
    test_elementsformat_terminal_content("", bddsingle);
    test_elementsformat_terminal_content("   ", bddsingle);
    test_elementsformat_terminal_content("\n", bddsingle);
    test_elementsformat_terminal_content(" \t\n", bddsingle);

    fprintf(stderr, "(the following \"not allowed\" messages are expected)\n");

    /* マーカーの直後にごみが続く */
    test_elementsformat_terminal_content("Tgarbage 1 2 3", bddnull);
    /* マーカーの後に別のトークンが続く */
    test_elementsformat_terminal_content("T 1", bddnull);
    test_elementsformat_terminal_content("B\n1 2\n", bddnull);
    /* 空行（空集合を表す）の後のマーカーは先頭ではないので拒否される */
    test_elementsformat_terminal_content("\nT", bddnull);

    fprintf(stderr, "(end of the expected messages)\n");
}

/* The delimiters and the variable names of the element printer are
   user-supplied strings without a length limit, so they must be
   written directly and never through the sbddextended_BUFSIZE buffer,
   which the unbounded sprintf fallback of C++98 once turned into a
   stack overflow and a bounded formatter would still truncate. The strings below are longer than the buffer, so
   both a truncation and an overflow fail this test (the latter as a
   crash or under AddressSanitizer). */
void test_elementsformat_long_strings(void)
{
    const size_t long_len = (size_t)sbddextended_BUFSIZE * 2 + 3;
    char* long_delim;
    char* long_name;
    char* expected;
    const char* var_name_map[6];
    bddp f;
    FILE* fp;
    size_t i;

    long_delim = (char*)malloc(long_len + 1);
    long_name = (char*)malloc(long_len + 1);
    /* "3 2" + delim + "3 1" + delim + "2 1" is the longest output */
    expected = (char*)malloc(3 * long_len + 32);
    if (long_delim == NULL || long_name == NULL || expected == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    for (i = 0; i < long_len; ++i) {
        long_delim[i] = (char)('a' + (char)(i % 26));
        long_name[i] = (char)('A' + (char)(i % 26));
    }
    long_delim[long_len] = '\0';
    long_name[long_len] = '\0';

    fp = test_fopen(g_filename1, "wb+");
    f = make_test_zbdd(); /* {{3, 2}, {3, 1}, {2, 1}} */

    bddprintzbddelements(fp, f, long_delim, " ");
    strcpy(expected, "3 2");
    strcat(expected, long_delim);
    strcat(expected, "3 1");
    strcat(expected, long_delim);
    strcat(expected, "2 1");
    test(is_expected_str(fp, expected));

    /* the variable 1 gets a name longer than the buffer */
    var_name_map[0] = "dummy";
    var_name_map[1] = long_name;
    var_name_map[2] = "d";
    var_name_map[3] = "c";
    var_name_map[4] = "b";
    var_name_map[5] = "a";
    bddprintzbddelementswithmap(fp, f, "$", " ", var_name_map);
    strcpy(expected, "c d$c ");
    strcat(expected, long_name);
    strcat(expected, "$d ");
    strcat(expected, long_name);
    test(is_expected_str(fp, expected));

#ifdef __cplusplus
    /* the C++ overloads that take std::string reach the same printer */
    {
        std::ostringstream oss;
        /* ZBDD_ID takes over the reference, so give it its own copy */
        printZBDDElements(oss, ZBDD_ID(bddcopy(f)), std::string(long_delim),
                            " ");
        strcpy(expected, "3 2");
        strcat(expected, long_delim);
        strcat(expected, "3 1");
        strcat(expected, long_delim);
        strcat(expected, "2 1");
        test(oss.str() == expected);
    }
#endif

    fclose(fp);
    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
    bddfree(f);
    free(expected);
    free(long_name);
    free(long_delim);
}

/* 最終行に改行のないファイルを、行長超過と誤判定せずに読み込めることを
   確認する（C 版の readLine） */
void test_readline_no_newline_at_end(void)
{
    bddp f, g;
    FILE* fp;

    /* Knuth 形式。レベル 1 のノードが 1 つだけで、最終行に改行がない */
    fp = test_fopen(g_filename1, "w");
    fputs("#1\n2:0,1", fp);
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasknuth(fp, 0, -1);
    fclose(fp);
    g = bddgetsingleton(bddvaroflev(1));
    test(f == g);
    bddfree(f);
    bddfree(g);

    fp = test_fopen(g_filename1, "r");
    f = bddimportbddasknuth(fp, 0, -1);
    fclose(fp);
    g = bddprime(bddvaroflev(1));
    test(f == g);
    bddfree(f);
    bddfree(g);

    /* graphillion 形式。ノードが 1 つだけで、最終行に改行がない */
    fp = test_fopen(g_filename1, "w");
    fputs("0 1 B T", fp);
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasgraphillion(fp, -1);
    fclose(fp);
    g = bddgetsingleton(bddvaroflev(1));
    test(f == g);
    bddfree(f);
    bddfree(g);

    fp = test_fopen(g_filename1, "r");
    f = bddimportbddasgraphillion(fp, -1);
    fclose(fp);
    g = bddprime(bddvaroflev(1));
    test(f == g);
    bddfree(f);
    bddfree(g);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* 長すぎる行や行中の NUL 文字を含む入力は、他のフォーマットエラーと
   同様に、プロセスを終了せずに bddnull を返すことを確認する */
void test_readline_errors(void)
{
    bddp f;
    FILE* fp;
    int i;
    char* longline;

    fprintf(stderr, "(the following \"line\" messages are expected)\n");

    /* 1024 文字以上の行 */
    longline = (char*)malloc((size_t)2048);
    if (longline == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    for (i = 0; i < 2000; ++i) {
        longline[i] = '1';
    }
    longline[2000] = '\n';
    longline[2001] = '\0';

    fp = test_fopen(g_filename1, "w");
    fputs(longline, fp);
    fclose(fp);
    free(longline);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasgraphillion(fp, -1);
    fclose(fp);
    test(f == bddnull);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasknuth(fp, 0, -1);
    fclose(fp);
    test(f == bddnull);

    /* 行中の NUL 文字 */
    fp = test_fopen(g_filename1, "wb");
    if (fwrite("0 1 B\0T\n.\n", (size_t)1, (size_t)10, fp) != (size_t)10) {
        fprintf(stderr, "fwrite failed\n");
        exit(1);
    }
    fclose(fp);

    fp = test_fopen(g_filename1, "r");
    f = bddimportzbddasgraphillion(fp, -1);
    fclose(fp);
    test(f == bddnull);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    fprintf(stderr, "(end of the expected messages)\n");
}

/* g_filename1 のファイルが空であることを確認して削除する */
void test_export_bddnull_check_empty(void)
{
    FILE* fp;
    long file_size;

    fp = test_fopen(g_filename1, "r");
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "fseek failed\n");
        exit(1);
    }
    file_size = ftell(fp);
    fclose(fp);
    test(file_size == 0);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* export 関数に bddnull を渡すと、正当な結果に見える出力を書かずに
   エラーを報告することを確認する（binary・Knuth・graphillion・Graphviz
   を検査する。SVG は C++ 専用なので testcpp 側で検査する） */
void test_export_bddnull(void)
{
    FILE* fp;

    fprintf(stderr, "(the following \"cannot represent bddnull\" "
            "messages are expected)\n");

    fp = test_fopen(g_filename1, "wb");
    bddexportzbddasbinary(fp, bddnull, 1, NULL);
    fclose(fp);
    test_export_bddnull_check_empty();

    fp = test_fopen(g_filename1, "w");
    bddexportzbddasknuth(fp, bddnull, 0, NULL);
    fclose(fp);
    test_export_bddnull_check_empty();

    fp = test_fopen(g_filename1, "w");
    bddexportzbddasgraphillion(fp, bddnull, NULL, -1);
    fclose(fp);
    test_export_bddnull_check_empty();

    fp = test_fopen(g_filename1, "w");
    bddexportbddasgraphviz(fp, bddnull, NULL);
    fclose(fp);
    test_export_bddnull_check_empty();

    fprintf(stderr, "(end of the expected messages)\n");

    /* 多値終端もバイナリ形式では表現できない */
    fprintf(stderr, "(the following \"multi-valued terminal\" "
            "message is expected)\n");
    fp = test_fopen(g_filename1, "wb");
    bddexportzbddasbinary(fp, bddconst(2), 0, NULL);
    fclose(fp);
    test_export_bddnull_check_empty();
    fprintf(stderr, "(end of the expected message)\n");
}

/* 旧関数名（マクロではなくインライン関数として提供している別名）が、
   新しい名前と同じ結果を返すことを確認する。 */
void test_compatibility(void)
{
    bddp f;
    bddp b;
    bddp p1;
    bddp p2;
    bddp g;
    bddp h;
    FILE* fp;
    bddNodeIndex* zindex;
    bddNodeIndex* bindex;

    f = make_test_zbdd();
    p1 = bddprime(1);
    p2 = bddprime(2);
    b = bddand(p1, p2);
    bddfree(p1);
    bddfree(p2);
    zindex = bddNodeIndex_makeIndexZWithoutCount(f);
    bindex = bddNodeIndex_makeIndexBWithoutCount(b);

    /* binary 形式 */
    fp = test_fopen(g_filename1, "wb");
    bddwritezbddtobinary(fp, f, 1, NULL);
    fclose(fp);
    fp = test_fopen(g_filename2, "wb");
    bddexportzbddasbinary(fp, f, 1, NULL);
    fclose(fp);
    test(is_same_file(g_filename1, g_filename2));

    fp = test_fopen(g_filename1, "rb");
    g = bddconstructzbddfrombinary(fp, -1);
    fclose(fp);
    fp = test_fopen(g_filename1, "rb");
    h = bddimportzbddasbinary(fp, -1);
    fclose(fp);
    test(g == h);
    bddfree(g);
    bddfree(h);

    /* graphillion 形式 */
    fp = test_fopen(g_filename1, "w");
    bddwritebddforgraphillion(fp, f, zindex, -1);
    fclose(fp);
    fp = test_fopen(g_filename2, "w");
    bddexportbddasgraphillion(fp, f, zindex, -1);
    fclose(fp);
    test(is_same_file(g_filename1, g_filename2));

    /* Knuth 形式 */
    fp = test_fopen(g_filename1, "w");
    bddwritezbddtofileknuth(fp, f, 0, NULL);
    fclose(fp);
    fp = test_fopen(g_filename2, "w");
    bddexportzbddasknuth(fp, f, 0, NULL);
    fclose(fp);
    test(is_same_file(g_filename1, g_filename2));

    fp = test_fopen(g_filename1, "r");
    g = bddconstructzbddfromfileknuth(fp, 0, -1);
    fclose(fp);
    fp = test_fopen(g_filename1, "r");
    h = bddimportzbddasknuth(fp, 0, -1);
    fclose(fp);
    test(g == h);
    bddfree(g);
    bddfree(h);

    fp = test_fopen(g_filename1, "w");
    bddexportbddasknuth(fp, b, 0, NULL);
    fclose(fp);
    fp = test_fopen(g_filename1, "r");
    g = bddconstructbddfromfileknuth(fp, 0, -1);
    fclose(fp);
    fp = test_fopen(g_filename1, "r");
    h = bddimportbddasknuth(fp, 0, -1);
    fclose(fp);
    test(g == h);
    bddfree(g);
    bddfree(h);

    /* graphviz 形式 */
    fp = test_fopen(g_filename1, "w");
    bddwritebddforgraphviz(fp, b, bindex);
    fclose(fp);
    fp = test_fopen(g_filename2, "w");
    bddexportbddasgraphviz(fp, b, bindex);
    fclose(fp);
    test(is_same_file(g_filename1, g_filename2));

    bddNodeIndex_destruct(bindex);
    free(bindex);
    bddNodeIndex_destruct(zindex);
    free(zindex);
    bddfree(b);

    if (remove(g_filename1) != 0 || remove(g_filename2) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }
}

/* The helper below reaches sbddextended_vsnprintf without the format
   attribute of sbddextended_snprintf, so that the corner cases (a
   conversion of C99, an unsupported conversion, a null string) can be
   tested without -Wformat warnings in the C++98 mode. */
int test_snprintf_call(char* str, size_t size, const char* format, ...)
{
    int v;
    va_list args;

    va_start(args, format);
    v = sbddextended_vsnprintf(str, size, format, args);
    va_end(args);
    return v;
}

void test_snprintf(void)
{
    char buf[64];
    char small[8];
    /* volatile hides the null from the compiler, which would otherwise
       warn about the deliberate null %s argument below */
    const char* volatile nullstr = NULL;
    void* addr = (void*)0x1f4;
    int n;

    /* the varargs wrapper, literal characters and %% */
    n = sbddextended_snprintf(buf, sizeof(buf), "hello %d%%", 42);
    test_eq(n, 9);
    test(strcmp(buf, "hello 42%") == 0);
    n = sbddextended_snprintf(buf, sizeof(buf), "%s=%d, %s=%d",
                                "a", 1, "bc", -23);
    test_eq(n, 11);
    test(strcmp(buf, "a=1, bc=-23") == 0);

    /* signed integers, flags, width and precision */
    test_eq(test_snprintf_call(buf, sizeof(buf), "%d", -12345), 6);
    test(strcmp(buf, "-12345") == 0);
    test_snprintf_call(buf, sizeof(buf), "%7d", 123);
    test(strcmp(buf, "    123") == 0);
    test_snprintf_call(buf, sizeof(buf), "%-7d", 123);
    test(strcmp(buf, "123    ") == 0);
    test_snprintf_call(buf, sizeof(buf), "%07d", -123);
    test(strcmp(buf, "-000123") == 0);
    test_snprintf_call(buf, sizeof(buf), "%+d % d %+d", 5, 6, -7);
    test(strcmp(buf, "+5  6 -7") == 0);
    test_snprintf_call(buf, sizeof(buf), "%8.5d", 42);
    test(strcmp(buf, "   00042") == 0);
    test_snprintf_call(buf, sizeof(buf), "%08.5d", 42);
    test(strcmp(buf, "   00042") == 0); /* '0' yields to a precision */
    test_eq(test_snprintf_call(buf, sizeof(buf), "%.0d", 0), 0);
    test(strcmp(buf, "") == 0);
    test_snprintf_call(buf, sizeof(buf), "%*d", 5, 42);
    test(strcmp(buf, "   42") == 0);
    test_snprintf_call(buf, sizeof(buf), "%*d", -5, 42);
    test(strcmp(buf, "42   ") == 0);
    test_snprintf_call(buf, sizeof(buf), "%.*d", 4, 42);
    test(strcmp(buf, "0042") == 0);

    /* unsigned integers and the alternative forms */
    test_snprintf_call(buf, sizeof(buf), "%u", 4294967295u);
    test(strcmp(buf, "4294967295") == 0);
    test_snprintf_call(buf, sizeof(buf), "%x %X %#x %#X %#x",
                        255, 255, 255, 255, 0);
    test(strcmp(buf, "ff FF 0xff 0XFF 0") == 0);
    test_snprintf_call(buf, sizeof(buf), "%o %#o %#o", 8, 8, 0);
    test(strcmp(buf, "10 010 0") == 0);

    /* the length modifiers */
    test_snprintf_call(buf, sizeof(buf), "%lld",
                        -9223372036854775807LL - 1);
    test(strcmp(buf, "-9223372036854775808") == 0);
    test_snprintf_call(buf, sizeof(buf), "%llu",
                        18446744073709551615ull);
    test(strcmp(buf, "18446744073709551615") == 0);
    test_snprintf_call(buf, sizeof(buf), "%llx",
                        81985529216486895ull);
    test(strcmp(buf, "123456789abcdef") == 0);
    test_snprintf_call(buf, sizeof(buf), "%ld %lu", (long)-12,
                        (unsigned long)34);
    test(strcmp(buf, "-12 34") == 0);
    test_snprintf_call(buf, sizeof(buf), "%hd %hu", 70000, 70000);
    test(strcmp(buf, "4464 4464") == 0);
    test_snprintf_call(buf, sizeof(buf), "%zu", (size_t)123);
    test(strcmp(buf, "123") == 0);

    /* characters and strings */
    test_snprintf_call(buf, sizeof(buf), "[%c][%3c][%-3c]",
                        'A', 'B', 'C');
    test(strcmp(buf, "[A][  B][C  ]") == 0);
    test_snprintf_call(buf, sizeof(buf), "[%5s][%-5s][%.2s][%5.2s]",
                        "abc", "abc", "abcdef", "abcdef");
    test(strcmp(buf, "[  abc][abc  ][ab][   ab]") == 0);
    test_snprintf_call(buf, sizeof(buf), "%s", nullstr);
    test(strcmp(buf, "(null)") == 0);

    /* a pointer */
    test_snprintf_call(buf, sizeof(buf), "%p", addr);
    test(strcmp(buf, "0x1f4") == 0);

    /* floating points */
    test_snprintf_call(buf, sizeof(buf), "%f", 1.5);
    test(strcmp(buf, "1.500000") == 0);
    test_snprintf_call(buf, sizeof(buf), "%.2f %10.2f %-10.2f|",
                        3.14159, 3.14159, 3.14159);
    test(strcmp(buf, "3.14       3.14 3.14      |") == 0);
    test_snprintf_call(buf, sizeof(buf), "%010.2f", -3.5);
    test(strcmp(buf, "-000003.50") == 0);
    test_snprintf_call(buf, sizeof(buf), "%.2e %.2E %g %g",
                        12345.678, 12345.678, 0.0001, 1e10);
    test(strcmp(buf, "1.23e+04 1.23E+04 0.0001 1e+10") == 0);
    test_snprintf_call(buf, sizeof(buf), "%.1F", 2.5);
    test(strcmp(buf, "2.5") == 0);
    test_snprintf_call(buf, sizeof(buf), "%f %06f %F",
                        HUGE_VAL, -HUGE_VAL, HUGE_VAL);
    test(strcmp(buf, "inf   -inf INF") == 0);
    /* %E makes sprintf produce the uppercase INF, which must be
       recognized as non-numeric and padded with spaces, not zeros */
    test_snprintf_call(buf, sizeof(buf), "%08.2E", -HUGE_VAL);
    test(strcmp(buf, "    -INF") == 0);

    /* truncation, the sizes 0 and 1, and the return value */
    test_eq(test_snprintf_call(small, sizeof(small), "%d %d",
                                12345, 6789), 10);
    test(strcmp(small, "12345 6") == 0);
    strcpy(buf, "xyz");
    test_eq(test_snprintf_call(buf, 0, "%d", 12345), 5);
    test(strcmp(buf, "xyz") == 0); /* nothing is written when size is 0 */
    test_eq(test_snprintf_call(NULL, 0, "%d", 12345), 5);
    test_eq(test_snprintf_call(buf, 1, "%d", 12345), 5);
    test(strcmp(buf, "") == 0);

    /* an unsupported conversion is printed verbatim */
    test_eq(test_snprintf_call(buf, sizeof(buf), "%q %-3q"), 7);
    test(strcmp(buf, "%q %-3q") == 0);
    test_eq(test_snprintf_call(buf, sizeof(buf), "abc%"), 4);
    test(strcmp(buf, "abc%") == 0);

    /* the arguments of the unsupported conversions a and A (a double
       or a long double) are consumed so that the following conversions
       still read their own arguments */
    test_snprintf_call(buf, sizeof(buf), "%a %d", 1.5, 42);
    test(strcmp(buf, "%a 42") == 0);
    test_snprintf_call(buf, sizeof(buf), "%LA %d", (long double)1.5, 43);
    test(strcmp(buf, "%LA 43") == 0);

    /* for n, lc, ls and the j and t length modifiers, the exact type
       of the argument is not known, so the rest of the format is
       printed verbatim and no further argument is read (reading the
       argument with a guessed type would be undefined behavior) */
    test_eq(test_snprintf_call(buf, sizeof(buf), "%n %d", &n, 44), 5);
    test(strcmp(buf, "%n %d") == 0);
    test_snprintf_call(buf, sizeof(buf), "%lc %d", (int)'w', 45);
    test(strcmp(buf, "%lc %d") == 0);
    test_snprintf_call(buf, sizeof(buf), "x%jdy %d", 46ll, 47);
    test(strcmp(buf, "x%jdy %d") == 0);
    test_snprintf_call(buf, sizeof(buf), "%td %d", (char*)buf - (char*)buf, 48);
    test(strcmp(buf, "%td %d") == 0);

    /* %zd reads the signed type of the width of size_t */
    {
        /* ptrdiff_t has the width of size_t on the supported platforms */
        ptrdiff_t pd = -5;
        test_snprintf_call(buf, sizeof(buf), "%zd", pd);
        test(strcmp(buf, "-5") == 0);
    }

    /* a floating precision above the supported maximum (128) is used
       as 128, in both the output and the returned length (a documented
       deviation from snprintf) */
    test_eq(test_snprintf_call(buf, sizeof(buf), "%.200f", 1.0), 130);
    test(strncmp(buf, "1.000", 5) == 0);

    /* a huge width is counted without being stored; this must return
       immediately instead of looping once per padding character */
    test_eq(test_snprintf_call(NULL, 0, "%1000000000d", 5), 1000000000);
    test_eq(test_snprintf_call(small, sizeof(small), "%-1000000000d", 5),
            1000000000);
    test(strcmp(small, "5      ") == 0);
}

/* ZBDD（Z 型）用の子取得 API を検査する。bddgetchildz と
   bddgetchildzraw は他のテストからは呼ばれない */
void test_zbdd_child_functions(void)
{
    bddp f, f0, f1, nf, pos, neg;

    /* {{1,2},{1,3},{2,3}}。根は変数 3 のノードで、空集合を含まない
       ので否定枝は付いていない */
    f = make_test_zbdd();
    test(!bddisnegative(f));

    f0 = bddgetchild0z(f);
    f1 = bddgetchild1z(f);
    test(bddgetchildz(f, 0) == f0);
    test(bddgetchildz(f, 1) == f1);
    test(bddgetchild0(f) == f0);
    test(bddgetchild1(f) == f1);
    test(bddgetchild(f, 0) == f0);
    test(bddgetchild(f, 1) == f1);

    /* raw 版: 親に否定枝が付いているとき、0-枝の子はそれを受け継ぎ、
       1-枝の子は受け継がない */
    nf = bddtakenot(f);
    pos = f;
    neg = nf;
    test(bddgetchild0zraw(pos) == bddgetchild0z(pos));
    test(bddgetchild1zraw(pos) == bddgetchild1z(pos));
    test(bddgetchild0zraw(neg) == bddtakenot(bddgetchild0z(neg)));
    test(bddgetchild1zraw(neg) == bddgetchild1z(neg));
    test(bddgetchildzraw(neg, 0) == bddgetchild0zraw(neg));
    test(bddgetchildzraw(neg, 1) == bddgetchild1zraw(neg));
    test(bddgetchildraw(neg, 0) == bddgetchild0zraw(neg));
    test(bddgetchildraw(neg, 1) == bddgetchild1zraw(neg));

    bddfree(f);
}

/* BDD（B 型）用の C API（子取得・bddmakenodeb・バイナリ入出力）を
   検査する */
void test_bdd_b_functions(void)
{
    bddp p1, p2, f, nf, pos, neg, g;
    FILE* fp;

    p1 = bddprime(1);
    p2 = bddprime(2);
    f = bddand(p1, p2);

    /* 非 raw の子取得: 根は変数 2 のノード */
    test_eq(bddgetvar(f), 2);
    test(bddgetchild1b(f) == p1);
    test(bddgetchild0b(f) == bddfalse);
    test(bddgetchildb(f, 1) == p1);
    test(bddgetchildb(f, 0) == bddfalse);
    test(bddgetchild1(f) == p1);
    test(bddgetchild0(f) == bddfalse);
    test(bddgetchild(f, 1) == p1);
    test(bddgetchild(f, 0) == bddfalse);

    /* raw 版: 親に否定枝が付いているときは子にも否定が付く */
    nf = bddtakenot(f);
    test(bddisnegative(f) != bddisnegative(nf));
    pos = bddisnegative(f) ? nf : f;
    neg = bddisnegative(f) ? f : nf;
    test(bddgetchild0braw(pos) == bddgetchild0b(pos));
    test(bddgetchild1braw(pos) == bddgetchild1b(pos));
    test(bddgetchild0braw(neg) == bddtakenot(bddgetchild0b(neg)));
    test(bddgetchild1braw(neg) == bddtakenot(bddgetchild1b(neg)));
    test(bddgetchildbraw(neg, 0) == bddgetchild0braw(neg));
    test(bddgetchildbraw(neg, 1) == bddgetchild1braw(neg));
    test(bddgetchildraw(pos, 0) == bddgetchild0braw(pos));
    test(bddgetchildraw(pos, 1) == bddgetchild1braw(pos));

    /* bddmakenodeb: 子から元のノードを再構成できる */
    g = bddmakenodeb((bddvar)2, bddfalse, p1);
    test(g == f);
    bddfree(g);

    /* BDD 版バイナリ入出力の往復（否定枝あり・なし） */
    fp = test_fopen(g_filename1, "wb+");
    bddexportbddasbinary(fp, f, 1, NULL);
    rewind(fp);
    g = bddimportbddasbinary(fp, -1);
    fclose(fp);
    test(g == f);
    bddfree(g);

    fp = test_fopen(g_filename1, "wb+");
    bddexportbddasbinary(fp, f, 0, NULL);
    rewind(fp);
    g = bddimportbddasbinary(fp, -1);
    fclose(fp);
    test(g == f);
    bddfree(g);

    if (remove(g_filename1) != 0) {
        fprintf(stderr, "remove failed\n");
        exit(1);
    }

    bddfree(f);
    bddfree(p1);
    bddfree(p2);
}

/* bddnewvarrev で変数番号とレベルが一致しない構成を作り、var と lev を
   取り違えていないことを検査する。グローバルな変数順を変更するため、
   start_test からは呼ばず、各ドライバの main で他のすべてのテストの
   後に呼び出すこと。 */
void test_newvarrev(void)
{
    bddvar used, v1;
    bddvar vararr[2];
    bddp f, g, s;

    used = bddvarused();
    bddnewvarrev(1);
    v1 = bddvarused();
    test_eq(v1, used + 1);
    /* 新しい変数はレベル 1 に挿入され、既存の変数のレベルは 1 つ上がる */
    test_eq(bddlevofvar(v1), 1);
    test_eq(bddvaroflev(1), v1);
    test_eq(bddlevofvar(1), 2);
    test_eq(bddvaroflev(2), 1);
    test_eq(bddlevofvar(used), used + 1);

    /* シングルトンの変数とレベル */
    s = bddgetsingleton(v1);
    test_eq(bddgetvar(s), v1);
    test_eq(bddgetlev(s), 1);
    bddfree(s);

    /* 変数 1（レベル 2）と v1（レベル 1）のべき集合での基本操作 */
    vararr[0] = 1;
    vararr[1] = v1;
    f = bddgetpowerset(vararr, 2);
    test_eq(bddcard(f), 4);
    test(bddismemberz(f, vararr, 2));
    /* 根はレベルの高い方の変数 1 のノード */
    test_eq(bddgetvar(f), 1);
    test_eq(bddgetlev(f), 2);
    g = bddgetchild1z(f);
    test_eq(bddgetvar(g), v1);
    test(bddgetchild0z(f) == g);
    bddfree(f);
}

/* bddgetsingleset は与えられた順序によらず変数をレベルの昇順で加える。
   根に近い変数から並んだ（bddprintzbddelements が書き出す順序の）
   要素を並んだままの順序で bddchange に渡すと、要素数の 2 乗の時間と
   要素数に比例した深さの再帰が必要になり、SAPPOROBDD の再帰上限
   （8192）を超えるとプロセスが終了する。上限を超える個数の変数を
   追加するので、他のすべてのテストの後に呼び出すこと。 */
#define TEST_MANYVARS_N 9000
void test_singleset_manyvars(void)
{
    int i, n;
    bddvar* vararr;
    bddp f, g;

    /* C++ 版のドライバは既に多数の変数を作っている */
    if (bddvarused() < TEST_MANYVARS_N) {
        bddnewvarn(TEST_MANYVARS_N - bddvarused());
    }
    n = (int)bddvarused();
    test(n >= TEST_MANYVARS_N);

    vararr = (bddvar*)malloc((size_t)n * sizeof(bddvar));
    if (vararr == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    /* レベルの降順（根に近い方が先） */
    for (i = 0; i < n; ++i) {
        vararr[i] = bddvaroflev((bddvar)(n - i));
    }
    f = bddgetsingleset(vararr, n);
    test(f != bddnull);
    /* bddcard や bddsize は DD の高さの深さまで再帰するので、この高さの
       DD には使えない。根から 1-枝を反復でたどって検査する。 */
    g = f;
    for (i = 0; i < n; ++i) {
        test_eq(bddgetvar(g), vararr[i]);
        test(bddgetchild0z(g) == bddempty);
        g = bddgetchild1z(g);
    }
    test(g == bddsingle);
    test(bddismemberz(f, vararr, n));

    /* 昇順でも同じ ZBDD になる */
    for (i = 0; i < n; ++i) {
        vararr[i] = bddvaroflev((bddvar)(i + 1));
    }
    g = bddgetsingleset(vararr, n);
    test(g == f);
    bddfree(g);

    bddfree(f);
    free(vararr);
}

void start_test(void)
{
    srand(1);

    initialize();

    test_snprintf();
    test_MyVector();
    test_MyDict();
#ifdef __cplusplus
    test_MyDict_allocFailure();
#endif
    test_MySet();
#ifdef __cplusplus
    test_MySet_allocFailure();
#endif
    test_bddfunctions();
    test_bdd_b_functions();
    test_zbdd_child_functions();
    test_getsingleandpowerset();
    test_ismemberz();
    test_xrand();
    test_io();
    test_graphviz();
    test_elementsformat();
    test_at_random();
    test_index();
    test_index_copy();
    test_elementIterator();
    test_bddbinaryformat();
    test_graphillionformat_empty();
    test_graphillionformat_root_level();
    test_graphillionformat_export_root_level();
    test_graphillionformat_corrupted();
    test_knuthformat_empty();
    test_knuthformat_corrupted();
    test_knuthformat_wrong_level();
    test_knuthformat_too_many_levels();
    test_knuthformat_leading_garbage();
    test_elementsformat_out_of_range();
    test_elementsformat_terminal();
    test_elementsformat_long_strings();
    test_readline_no_newline_at_end();
    test_readline_errors();
    test_export_bddnull();
    test_compatibility();
}
