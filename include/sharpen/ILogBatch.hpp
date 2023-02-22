#pragma once
#ifndef _SHARPEN_ILOGBATCH_HPP
#define _SHARPEN_ILOGBATCH_HPP

#include <memory>

#include "ByteBuffer.hpp"
#include "IConsensusChanges.hpp"

namespace sharpen
{
    class ILogBatch
    {
    private:
        using Self = sharpen::ILogBatch;
    protected:
    public:
    
        ILogBatch() noexcept = default;
    
        ILogBatch(const Self &other) noexcept = default;
    
        ILogBatch(Self &&other) noexcept = default;
    
        Self &operator=(const Self &other) noexcept = default;
    
        Self &operator=(Self &&other) noexcept = default;
    
        virtual ~ILogBatch() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        virtual void Append(sharpen::ByteBuffer log) = 0;

        virtual std::unique_ptr<sharpen::IConsensusChanges> CreateChanges() = 0;

        virtual std::size_t GetSize() const noexcept = 0;

        virtual sharpen::ByteBuffer &Get(std::size_t index) = 0;

        virtual const sharpen::ByteBuffer &Get(std::size_t index) const = 0;

        virtual void Reverse(std::size_t size);
    };   
}

#endif