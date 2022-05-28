////////////////////////////////////////////////////////////////////////
// Coroutine support types
////////////////////////////////////////////////////////////////////////
#pragma once

#include <cassert>
#include <coroutine>

// TODO: Tasks can be converted into awaitables, and register their continuation into the job system
// The job system, on the other hand, can be explicitly manipulated via a Job/Dependency API, and is agnostic to Tasks and coroutines.

namespace core
{
    class Promise;

    class Task
    {
    public:
        Task(Promise* p)
            : m_promise(p)
        {}

    private:
        Promise* m_promise{};
    };

    class Promise
    {
    public:
        auto initial_suspend() { return std::suspend_never(); }
        auto final_suspend() { return std::suspend_always(); }
        auto return_void() {
            assert(false && "Not implemented");
        }
        auto get_return_object()
        {
            return Task(this);
        };
        
        void unhandled_exception()
        {
            assert(false); // Exceptions not supported
        }
    };

} // namespace core

template<>
struct std::coroutine_traits<core::Task>
{
    using promise_type = core::Promise;
};