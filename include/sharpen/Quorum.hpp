#pragma once
#ifndef _SHARPEN_QUORUM_HPP
#define _SHARPEN_QUORUM_HPP

#include <cassert>
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

#include "TypeDef.hpp"
#include "Future.hpp"
#include "IteratorOps.hpp"
#include "QuorumConcepts.hpp"

namespace sharpen
{
    class Quorum
    {
    private:
        using Self = sharpen::Quorum;

        struct Waiter
        {
            std::atomic_size_t finishCounter_;
            std::atomic_size_t successCounter_;
            std::atomic_size_t errorCounter_;
            sharpen::Future<bool>* continuation_;
            sharpen::Future<void>* finish_;
            sharpen::Size majority_;
            std::vector<sharpen::Future<bool>> futures_;
        };

        static void CompletedCallback(sharpen::Future<bool> &future,std::shared_ptr<Waiter> waiterPtr)
        {
            bool success = !future.IsError() && future.Get();
            sharpen::Size finishCounter = waiterPtr->finishCounter_.fetch_sub(1) - 1;
            if (success)
            {
                sharpen::Size successCounter = waiterPtr->successCounter_.fetch_add(1) + 1;
                if(successCounter == waiterPtr->majority_)
                {
                    sharpen::Future<bool> *continuation{nullptr};
                    std::swap(continuation,waiterPtr->continuation_);
                    if(continuation)
                    {
                        continuation->Complete(true);
                    }
                }
            }
            else
            {
                sharpen::Size errorCount = waiterPtr->errorCounter_.fetch_add(1);
                if(errorCount == waiterPtr->majority_)
                {
                    sharpen::Future<bool> *continuation{nullptr};
                    std::swap(continuation,waiterPtr->continuation_);
                    if(continuation)
                    {
                        continuation->Complete(false);
                    }
                }
            }
            if(finishCounter == 0)
            {
                waiterPtr->finish_->Complete();
            }
        }

    public:
        template<typename _Iterator,typename _Proposal,typename _Check = sharpen::EnableIf<sharpen::IsQuorumProposerIterator<_Iterator,_Proposal>::Value>>
        static void ProposeAsync(_Iterator begin,_Iterator end,_Proposal &&proposal,sharpen::Future<bool> &continuation,sharpen::Future<void> &finish)
        {
            using FnPtr = void(*)(sharpen::Future<bool>&,std::shared_ptr<Waiter>);
            //init waiter
            std::shared_ptr<Waiter> waiterPtr = std::make_shared<Waiter>();
            sharpen::Size size = sharpen::GetRangeSize(begin,end);
            waiterPtr->finish_ = &finish;
            waiterPtr->continuation_ = &continuation;
            waiterPtr->finishCounter_.store(size);
            waiterPtr->successCounter_.store(0);
            waiterPtr->majority_ = (size + 1)/2;
            waiterPtr->futures_.resize(size);
            waiterPtr->errorCounter_.store(0);
            sharpen::Size index{0};
            //launch operations
            while (begin != end)
            {
                waiterPtr->futures_[index].SetCallback(std::bind(static_cast<FnPtr>(&Self::CompletedCallback),std::placeholders::_1,waiterPtr));
                begin->ProposeAsync(proposal,waiterPtr->futures_[index]);
                ++index;
                ++begin;
            }
        }
    };
}

#endif