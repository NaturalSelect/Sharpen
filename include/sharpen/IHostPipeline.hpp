#pragma once
#ifndef _SHARPEN_IHOSTPIPELINE_HPP
#define _SHARPEN_IHOSTPIPELINE_HPP

#include <memory>
#include <cassert>
#include <new>
#include <type_traits>

#include "INetStreamChannel.hpp"
#include "IHostPipelineStep.hpp"

namespace sharpen
{
    class IHostPipeline
    {
    private:
        using Self = sharpen::IHostPipeline;
    protected:

        virtual void NviConsume(sharpen::NetStreamChannelPtr channel) noexcept = 0;

        virtual void NviRegisterStep(std::unique_ptr<sharpen::IHostPipelineStep> step) = 0;
    public:
    
        IHostPipeline() noexcept = default;
    
        IHostPipeline(const Self &other) noexcept = default;
    
        IHostPipeline(Self &&other) noexcept = default;
    
        Self &operator=(const Self &other) noexcept = default;
    
        Self &operator=(Self &&other) noexcept = default;
    
        virtual ~IHostPipeline() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        virtual bool Active() const noexcept = 0;

        virtual void Stop() noexcept = 0;

        inline void Consume(sharpen::NetStreamChannelPtr channel)
        {
            if(channel && this->Active())
            {
                this->NviConsume(std::move(channel));
            }
        }

        inline Self &RegisterStep(std::unique_ptr<sharpen::IHostPipelineStep> step)
        {
            assert(step != nullptr);
            this->NviRegisterStep(std::move(step));
            return *this;
        }

        template<typename _Impl,typename ..._Args,typename _Check = decltype(std::declval<sharpen::IHostPipelineStep*&>() = std::declval<_Impl*>(),_Impl{std::declval<_Args>()...})>
        inline Self &RegisterStep(_Args &&...args)
        {
            std::unique_ptr<sharpen::IHostPipelineStep> step{nullptr};
            step.reset(new (std::nothrow) _Impl{std::forward<_Args>(args)...});
            if(!step)
            {
                throw std::bad_alloc{};
            }
            return this->RegisterStep(std::move(step));
        }
    };
}

#endif