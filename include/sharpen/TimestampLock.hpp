#pragma once
#ifndef _SHARPEN_TIMETICK_HPP
#define _SHARPEN_TIMETICK_HPP

#include <unordered_map>
#include <mutex>

#include "SpinLock.hpp"
#include "TypeDef.hpp"

namespace sharpen
{
    template<typename _Key>
    class TimestampLock:public sharpen::Noncopyable
    {
    private:
        using Self = sharpen::TimestampLock;
        using Map = std::unordered_map<_Key,sharpen::Uint64>;

        sharpen::SpinLock lock_;
        Map map_;
    public:
    
        TimestampLock() = default;
    
        TimestampLock(Self &&other) noexcept
            :lock_()
            ,map_(std::move(other.map_))
        {}
    
        Self &operator=(Self &&other) noexcept
        {
            if(this != std::addressof(other))
            {
                this->map_ = std::move(other.map_);
            }
            return *this;
        }
    
        ~TimestampLock() noexcept = default;

        sharpen::Uint64 GetTimestamp(const _Key &key)
        {
            sharpen::Uint64 time{0};
            {
                std::unique_lock<sharpen::SpinLock> lock{this->lock_};
                auto pair = this->map_.emplace(key,time);
                if(!pair.second)
                {
                    time = ++(pair.first->first);
                }
            }
            return time;
        }

        bool Valid(const _Key &key,sharpen::Uint64 time)
        {
            std::unique_lock<sharpen::SpinLock> lock{this->lock_};
            return this->map_[key] == time;
        }
    };
}

#endif