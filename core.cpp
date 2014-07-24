// -*-mode:c++; coding:utf-8-*-
#include <core.hpp>

#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <new>
#include <exception>

#include <boost/context/all.hpp>
namespace ctx = boost::context;

namespace coroutine
{
    
    enum status_t
    {
        S_COMPLETE = 0,
        S_SUSPEND = 1,
        S_RUNNING = 2
    };

    struct coroutine
    {
        int status;
        routine_t f;
        intptr_t data;
        void *context;
        void *caller;
        bool has_std_exception;
        bool has_unknown_exception;
        bool unwinded;
        bool need_unwind;
        bool force_unwind;

        coroutine() :
            status(S_COMPLETE), f(NULL), data(0),
            context(NULL), caller(NULL),
            has_std_exception(false),
            has_unknown_exception(false),
            unwinded(false),
            need_unwind(true),
            force_unwind(false)
            {}
    };

    struct forced_unwind {};
    struct exception_unknown {};
    struct exception_std : public std::exception
    {
        virtual ~exception_std() throw() {}
        const char* what() const throw() {
            return "catch std::exception from coroutine";
        }
    };

    static
    void routine_starter(intptr_t data)
    {
        coroutine_t co = reinterpret_cast<coroutine_t>(data);
        try
        {
            co->data = co->f(co->data);
        }
        catch (const forced_unwind &)
        {
            co->unwinded = true;
        }
        catch (const std::exception &)
        {
            co->has_std_exception = true;
        }
        catch (...)
        {
            co->has_unknown_exception = true;
        }
        co->status = S_COMPLETE;
        ctx::jump_fcontext((ctx::fcontext_t*)co->context,
                           (ctx::fcontext_t*)co->caller,
                           co->data);
    }
        
    coroutine_t create(routine_t f, int stack)
    {
        void *p = std::malloc(stack + sizeof(coroutine));
        char *top = (char *)p + stack;
        // alloc coroutine at top of stack and the stack is growing
        // downward.
        coroutine_t co = new(top) coroutine;
        co->status = S_SUSPEND;
        co->f = f;
        co->context = ctx::make_fcontext(top, stack,
                                         routine_starter);
        return co;
    }

    void destroy(coroutine_t c)
    {
        if(! is_complete(c) && c->need_unwind)
        {
            // unwind the stack
            c->force_unwind = true;
            resume(c);
        }
        // adjust pointer to head of memory
        ctx::fcontext_t *ctx = (ctx::fcontext_t*)c->context;
        void *p = (char*)c - ctx->fc_stack.size;
        c->~coroutine();
        std::free(p);
    }

    intptr_t resume(coroutine_t c, intptr_t data)
    {
        assert(c->status == S_SUSPEND);
        c->status = S_RUNNING;
        c->data = data;
        ctx::fcontext_t caller;
        c->caller = &caller;
        ctx::jump_fcontext((ctx::fcontext_t*)c->caller,
                           (ctx::fcontext_t*)c->context,
                           reinterpret_cast<intptr_t>(c));

        if(c->has_std_exception) throw exception_std();
        if(c->has_unknown_exception) throw exception_unknown();
            
        return c->data;
    }

    intptr_t yield(coroutine_t c, intptr_t data)
    {
        assert(c->status == S_RUNNING);
        c->status = S_SUSPEND;
        c->data = data;
        ctx::jump_fcontext((ctx::fcontext_t*)c->context,
                           (ctx::fcontext_t*)c->caller,
                           reinterpret_cast<intptr_t>(c));
        if(c->force_unwind)
        {
            throw forced_unwind();
        }
        return c->data;
    }

    bool is_complete(coroutine_t c)
    {
        return c->status == S_COMPLETE;
    }

}

