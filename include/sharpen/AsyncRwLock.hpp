#pragma once
#ifndef _SHARPEN_ASYNCREADWRITELOCK_HPP
#define _SHARPEN_ASYNCREADWRITELOCK_HPP

#include <vector>
#include <mutex>

#include "AwaitableFuture.hpp"

namespace sharpen
{
    enum class RwLockState
    {
        Free,
        SharedReading,
        UniquedWriting
    };

    class AsyncRwLock:public sharpen::Noncopyable,public sharpen::Nonmovable
    {
    private:
        using Self = sharpen::AsyncRwLock;
        using MyFuture = sharpen::AwaitableFuture<sharpen::RwLockState>;
        using MyFuturePtr = MyFuture*;
        using Waiters = std::vector<MyFuturePtr>;

        sharpen::RwLockState state_;
        Waiters readWaiters_;
        Waiters writeWaiters_;
        sharpen::SpinLock lock_;
        std::uint32_t readers_;

        void WriteUnlock() noexcept;

        void ReadUnlock() noexcept;

        friend class std::unique_lock<Self>;

        //basic lockable requirement
        //never use me
        //you should use LockWrite or LockRead
        inline void lock()
        {
            this->LockWrite();
        }
    public:
        AsyncRwLock();

        //return prev status
        sharpen::RwLockState LockRead();

        bool TryLockRead();

        bool TryLockRead(sharpen::RwLockState &prevStatus);

        //return prev status
        sharpen::RwLockState LockWrite();

        bool TryLockWrite();

        bool TryLockWrite(sharpen::RwLockState &prevStatus);

        //upgrade to write lock
        sharpen::RwLockState UpgradeFromRead();

        //downgrade to read lock
        sharpen::RwLockState DowngradeFromWrite();

        void Unlock() noexcept;

        inline void unlock() noexcept
        {
            this->Unlock();
        }

        ~AsyncRwLock() noexcept = default;
    };
    
}

#endif