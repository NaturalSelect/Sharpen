#pragma once
#ifndef _SHARPEN_ASYNCOPS_HPP

#include <type_traits>
#include <thread>
#include <cassert>

#include "AwaitableFuture.hpp"
#include "AsyncHelper.hpp"
#include "TimerPool.hpp"
#include "IteratorOps.hpp"
#include "TypeTraits.hpp"

namespace sharpen
{
    template<typename _Fn,typename ..._Args,typename _Check = sharpen::EnableIf<sharpen::IsCompletedBindableReturned<void,_Fn,_Args...>::Value>>
    inline void Launch(_Fn &&fn,_Args &&...args)
    {
        sharpen::EventEngine &engine = sharpen::EventEngine::GetEngine();
        engine.Launch(std::forward<_Fn>(fn),std::forward<_Args>(args)...);
    }

    template<typename _Fn,typename ..._Args,typename _Check = sharpen::EnableIf<sharpen::IsCompletedBindableReturned<void,_Fn,_Args...>::Value>>
    inline void LaunchSpecial(std::size_t stackSize,_Fn &&fn,_Args &&...args)
    {
        sharpen::EventEngine &engine = sharpen::EventEngine::GetEngine();
        engine.LaunchSpecial(stackSize,std::forward<_Fn>(fn),std::forward<_Args>(args)...);
    }

    template<typename _Fn,typename ..._Args,typename _Result = decltype(std::declval<_Fn>()(std::declval<_Args>()...))>
    inline sharpen::AwaitableFuturePtr<_Result> Async(_Fn &&fn,_Args &&...args)
    {
        auto future = sharpen::MakeAwaitableFuture<_Result>();
        std::function<_Result()> func = std::bind(std::forward<_Fn>(fn),std::forward<_Args>(args)...);
        sharpen::Launch([func,future]() mutable
        {
            sharpen::AsyncHelper<std::function<_Result()>,_Result>::RunAndSetFuture(func,*future);
        });
        return future;
    }

    template<typename _Fn,typename ..._Args,typename _Result = decltype(std::declval<_Fn>()(std::declval<_Args>()...))>
    inline sharpen::AwaitableFuturePtr<_Result> AsyncSpecial(std::size_t stackSize,_Fn &&fn,_Args &&...args)
    {
        auto future = sharpen::MakeAwaitableFuture<_Result>();
        std::function<_Result()> func = std::bind(std::forward<_Fn>(fn),std::forward<_Args>(args)...);
        sharpen::LaunchSpecial(stackSize,[func,future]() mutable
        {
            sharpen::AsyncHelper<std::function<_Result()>,_Result>::RunAndSetFuture(func,*future);
        });
        return future;
    }

    struct DelayHelper
    {
    private:
        using Maker = sharpen::TimerPtr(*)(sharpen::EventEngine*);

        static std::unique_ptr<sharpen::TimerPool> delayTimerPool_;
        static std::once_flag flag_;

        static void InitTimerPool(sharpen::EventEngine *engine,Maker maker);
    public:

        static sharpen::TimerPool &GetTimerPool(sharpen::EventEngine &engine,Maker maker);
    };

    inline sharpen::TimerPool &GetDelayTimerPool()
    {
        return sharpen::DelayHelper::GetTimerPool(sharpen::EventEngine::GetEngine(),nullptr);   
    }

    template<typename _Rep,typename _Period>
    inline void Delay(const std::chrono::duration<_Rep,_Period> &time)
    {
        sharpen::TimerPool &pool{sharpen::GetDelayTimerPool()};
        sharpen::TimerPtr timer{pool.GetTimer()};
        timer->Await(time);
        pool.PutTimer(std::move(timer));
    }

    void ParallelFor(std::size_t begin,std::size_t end,std::size_t grainsSize,std::function<void(std::size_t)> fn);

    inline void ParallelFor(std::size_t begin,std::size_t end,std::function<void(std::size_t)> fn)
    {
        sharpen::ParallelFor(begin,end,1000,std::move(fn));
    }

    template<typename _Iterator,typename _HasForward = decltype(sharpen::IteratorForward(std::declval<_Iterator>(),std::declval<std::size_t>()))>
    void ParallelForeach(_Iterator begin,_Iterator end,std::size_t grainsSize,std::function<void(_Iterator)> fn)
    {
        if (begin == end)
        {
            return;
        }
        std::size_t parallelNumber{sharpen::EventEngine::GetEngine().LoopCount()};
        std::size_t count{sharpen::GetRangeSize(begin,end)};
        std::size_t max{count};
        std::atomic_size_t ite{0};
        std::atomic_size_t comp{parallelNumber};
        max /= parallelNumber;
        if (max > grainsSize)
        {
            max = grainsSize;
        }
        sharpen::AwaitableFuture<void> future;
        for (size_t i = 0; i < parallelNumber; i++)
        {
            sharpen::Launch([parallelNumber,&ite,&end,count,max,&begin,&fn,&future,&comp]()
            {
                while (true)
                {
                    std::size_t iterator = ite.load();
                    std::size_t size{0};
                    //get size
                    do
                    {
                        assert(count >= iterator);
                        size = count - iterator;
                        if(size > max)
                        {
                            size = max;
                        }
                    } while (!ite.compare_exchange_weak(iterator,iterator + size));
                    auto i = sharpen::IteratorForward(begin,iterator);
                    auto bound = sharpen::IteratorForward(i,size);
                    if (i == bound)
                    {
                        std::size_t copy = comp.fetch_sub(1);
                        copy -= 1;
                        if (copy == 0)
                        {
                            future.Complete();
                        }
                        return;
                    }
                    while (i != bound)
                    {
                        assert(fn);
                        fn(i);
                        ++i;
                    }
                }
            });
        }
        future.Await();
    }

    template<typename _Iterator>
    auto inline ParallelForeach(_Iterator begin,_Iterator end,std::function<void(_Iterator)> fn) -> decltype(sharpen::ParallelForeach(begin,end,1000,fn))
    {
        return sharpen::ParallelForeach(std::move(begin),std::move(end),1000,std::move(fn));
    }
}

#endif
