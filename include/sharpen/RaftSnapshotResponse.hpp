#pragma once
#ifndef _SHARPEN_RAFTSNAPSHOTRESPONSE_HPP
#define _SHARPEN_RAFTSNAPSHOTRESPONSE_HPP

#include <cstddef>
#include <cstdint>
#include <utility>

#include "BinarySerializable.hpp"

namespace sharpen
{
    class RaftSnapshotResponse : public sharpen::BinarySerializable<sharpen::RaftSnapshotResponse>
    {
    private:
        using Self = sharpen::RaftSnapshotResponse;

        bool status_;
        std::uint64_t term_;

    public:
        RaftSnapshotResponse() noexcept;

        explicit RaftSnapshotResponse(bool status, std::uint64_t term) noexcept;

        RaftSnapshotResponse(const Self &other) noexcept = default;

        RaftSnapshotResponse(Self &&other) noexcept;

        Self &operator=(const Self &other) noexcept;

        Self &operator=(Self &&other) noexcept;

        ~RaftSnapshotResponse() noexcept = default;

        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline bool GetStatus() const noexcept
        {
            return this->status_;
        }

        inline void SetStatus(bool status) noexcept
        {
            this->status_ = status;
        }

        inline std::uint64_t GetTerm() const noexcept
        {
            return this->term_;
        }

        inline void SetTerm(std::uint64_t term) noexcept
        {
            this->term_ = term;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t UnsafeStoreTo(char *data) const noexcept;

        std::size_t LoadFrom(const char *data, std::size_t size);
    };
}   // namespace sharpen

#endif