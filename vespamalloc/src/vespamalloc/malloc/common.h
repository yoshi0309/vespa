// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <new>
#include <atomic>
#include <vespamalloc/util/osmem.h>

extern "C" void MallocRecurseOnSuspend(bool recurse) __attribute__ ((noinline));

namespace vespamalloc {

#define VESPA_DLL_EXPORT __attribute__ ((visibility("default")))

#define NELEMS(a) sizeof(a)/sizeof(a[0])

#define NUM_SIZE_CLASSES 32   // Max 64G

#define NUM_THREADS 16384
#define UNUSED(a)
#ifdef ENABLE_DEBUG
#define DEBUG(a) a
#else
#define DEBUG(a)
#endif

#ifndef PARANOID_LEVEL
#define PARANOID_LEVEL 0
#endif

#if (PARANOID_LEVEL >= 0)
#define PARANOID_CHECK0(a) a
#else
#define PARANOID_CHECK0(a)
#endif

#if (PARANOID_LEVEL >= 1)
#define PARANOID_CHECK1(a) a
#else
#define PARANOID_CHECK1(a)
#endif

#if (PARANOID_LEVEL >= 2)
#define PARANOID_CHECK2(a) a
#else
#define PARANOID_CHECK2(a)
#endif

#if (PARANOID_LEVEL >= 3)
#define PARANOID_CHECK3(a) a
#else
#define PARANOID_CHECK3(a)
#endif

typedef MmapMemory OSMemory;

typedef int SizeClassT;

   
inline int msbIdx(uint64_t v) {
    return v ? 63 - __builtin_clzl(v) : 0;
}    

template <size_t MinClassSizeC>
class CommonT
{
public:
    enum {MinClassSize = MinClassSizeC};
    static inline SizeClassT sizeClass(size_t sz) {
        SizeClassT tmp(msbIdx(sz - 1) - (MinClassSizeC - 1));
        return (sz <= (1 << MinClassSizeC )) ? 0 : tmp;
    }
    static inline size_t classSize(SizeClassT sc) { return (size_t(1) << (sc + MinClassSizeC)); }
};

inline void crash() { *((unsigned *) NULL) = 0; }

template <typename T>
inline void swap(T & a, T & b)      { T tmp(a); a = b; b = tmp; }

class Mutex
{
public:
    Mutex() : _mutex(), _use(false) { }
    ~Mutex()           { quit(); }
    void lock();
    void unlock();
    static void addThread()      { _threadCount.fetch_add(1); }
    static void subThread()      { _threadCount.fetch_sub(1); }
    static void stopRecursion()  { _stopRecursion = true; }
    static void allowRecursion() { _stopRecursion = false; }
    void init();
    void quit();
private:
    static std::atomic<uint32_t> _threadCount;
    static bool     _stopRecursion;
    Mutex(const Mutex & org);
    Mutex & operator = (const Mutex & org);
    pthread_mutex_t  _mutex;
    bool             _use;
};

class Guard
{
public:
    Guard(Mutex & m);
    ~Guard()                      { _mutex->unlock(); }
private:
    Mutex * _mutex;
};

class IAllocator
{
public:
    virtual ~IAllocator() {}
    virtual bool initThisThread() = 0;
    virtual bool quitThisThread() = 0;
    virtual void enableThreadSupport() = 0;
    virtual void setReturnAddressStop(const void * returnAddressStop) = 0;
    virtual size_t getMaxNumThreads() const = 0;
};

void info();

}

