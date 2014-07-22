// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_CORE_HPP_
#define _COROUTINE_CORE_HPP_

#include <stdint.h>

namespace coroutine
{
    struct coroutine;

    typedef intptr_t (*routine_t)(intptr_t);
    typedef coroutine * coroutine_t;

    enum status_t
    {
        S_DEAD = 0,
        S_SUSPEND = 1,
        S_RUNNING = 2
    };

    coroutine_t create(routine_t f, int stack=64*1024);
    void destroy(coroutine_t c);

    intptr_t resume(coroutine_t c, intptr_t data=0);
    intptr_t yield(coroutine_t c, intptr_t data=0);
        
}

#endif  // _COROUTINE_CORE_HPP_
