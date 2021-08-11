#pragma once
#ifndef _SHARPEN_ASYNCOPS_HPP

#include <type_traits>
#include <thread>

#include "AwaitableFuture.hpp"
#include "AsyncHelper.hpp"
#include "ITimer.hpp"

namespace sharpen
{
    template<typename _Fn,typename ..._Args>
    inline void Launch(_Fn &&fn,_Args &&...args)
    {
        sharpen::EventEngine &engine = sharpen::EventEngine::GetEngine();
        engine.Launch(std::forward<_Fn>(fn),std::forward<_Args>(args)...);
    }

    template<typename _Fn,typename ..._Args,typename _Result = decltype(std::declval<_Fn>()(std::declval<_Args>()...))>
    inline sharpen::AwaitableFuturePtr<_Result> Async(_Fn &&fn,_Args &&...args)
    {
        auto future = sharpen::MakeAwaitableFuture<_Result>();
        std::function<typename _Result()> func = std::bind(std::forward<_Fn>(fn),std::forward<_Args>(args)...);
        sharpen::Launch([func,future]() mutable
        {
            sharpen::AsyncHelper<std::function<_Result()>,_Result>::RunAndSetFuture(func,*future);
        });
        return future;
    }

    template<typename _Rep,typename _Period>
    inline void Delay(const std::chrono::duration<_Rep,_Period> &time)
    {
        sharpen::TimerPtr timer = sharpen::MakeTimer(sharpen::EventEngine::GetEngine());
        timer->Await(time);
    }
}

#endif
