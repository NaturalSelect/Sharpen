#pragma once
#ifndef _SHARPEN_CONSENSUSPEERSCONFIGURATION_HPP
#define _SHARPEN_CONSENSUSPEERSCONFIGURATION_HPP

#include "ActorId.hpp"
#include "BinarySerializable.hpp"
#include <set>

namespace sharpen {
    class ConsensusPeersConfiguration
        : public sharpen::BinarySerializable<sharpen::ConsensusPeersConfiguration> {
    private:
        using Self = sharpen::ConsensusPeersConfiguration;

        std::uint64_t epoch_;
        std::set<sharpen::ActorId> peers_;
        bool locked_;

    public:
        ConsensusPeersConfiguration() noexcept = default;

        ConsensusPeersConfiguration(const Self &other) = default;

        ConsensusPeersConfiguration(Self &&other) noexcept;

        inline Self &operator=(const Self &other) {
            if (this != std::addressof(other)) {
                Self tmp{other};
                std::swap(tmp, *this);
            }
            return *this;
        }

        Self &operator=(Self &&other) noexcept;

        ~ConsensusPeersConfiguration() noexcept = default;

        inline const Self &Const() const noexcept {
            return *this;
        }

        inline std::uint64_t GetEpoch() const noexcept {
            return this->epoch_;
        }

        inline void SetEpoch(std::uint64_t epoch) noexcept {
            this->epoch_ = epoch;
        }

        inline std::set<sharpen::ActorId> &Peers() noexcept {
            return this->peers_;
        }

        inline const std::set<sharpen::ActorId> &Peers() const noexcept {
            return this->peers_;
        }

        inline bool Locked() const noexcept {
            return this->locked_;
        }

        inline void Lock() noexcept {
            this->locked_ = true;
        }

        inline void Unlock() noexcept {
            this->locked_ = false;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t UnsafeStoreTo(char *data) const noexcept;

        std::size_t LoadFrom(const char *data, std::size_t size);
    };
}   // namespace sharpen

#endif