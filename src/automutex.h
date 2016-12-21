#pragma once

#include <pthread.h>
#include <assert.h>

// класс-оболочка, создающий и удаляющий рекурсивный мютекс (Unix)
class CAutoMutex
{
    pthread_mutex_t m_mutex;

    CAutoMutex(const CAutoMutex&);
    CAutoMutex& operator=(const CAutoMutex&);

public:
    CAutoMutex() {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    ~CAutoMutex() {
        pthread_mutex_destroy(&m_mutex);
    }
    pthread_mutex_t& get() {
        return m_mutex;
    }
};

// класс-оболочка, занимающий и освобождающий мютекс
class CMutexLock
{
    pthread_mutex_t m_mutex;

    // запрещаем копирование
    CMutexLock(const CMutexLock&);
    CMutexLock& operator=(const CMutexLock&);
public:
    // занимаем мютекс при конструировании объекта
    CMutexLock(pthread_mutex_t mutex): m_mutex(mutex) {
        const int res = pthread_mutex_lock(&mutex);
        assert(res == 0);
    }
    // освобождаем мютекс при удалении объекта
    ~CMutexLock() {
        const int res = pthread_mutex_unlock(&m_mutex);
        //assert(res == 0);
    }
};


// макрос, занимающий мютекс до конца области действия
#define SCOPE_LOCK_MUTEX(hMutex) CMutexLock _tmp_mtx_capt(hMutex);