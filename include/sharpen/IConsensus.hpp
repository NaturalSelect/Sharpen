#pragma once
#ifndef _SHARPEN_ICONSENSUS_HPP
#define _SHARPEN_ICONSENSUS_HPP

#include "AwaitableFuture.hpp"
#include "ConsensusChangeResult.hpp"
#include "ConsensusPeersConfiguration.hpp"
#include "ConsensusResult.hpp"
#include "ConsensusWriter.hpp"
#include "ILogStorage.hpp"
#include "IMailReceiver.hpp"
#include "IQuorum.hpp"
#include "LogBatch.hpp"
#include "WriteLogsResult.hpp"
#include <memory>


namespace sharpen {
    class IConsensus {
    private:
        using Self = sharpen::IConsensus;

    protected:
        // returns current advanced count
        virtual void NviWaitNextConsensus(sharpen::Future<sharpen::ConsensusResult> &future) = 0;

        virtual bool NviIsConsensusMail(const sharpen::Mail &mail) const noexcept = 0;

        virtual sharpen::WriteLogsResult NviWrite(const sharpen::LogBatch &logs) = 0;

        virtual sharpen::Mail NviGenerateResponse(sharpen::Mail request) = 0;

        virtual void NviDropLogsUntil(std::uint64_t endIndex) = 0;

        virtual sharpen::ConsensusChangeResult NviConfiguratePeers(
            std::function<std::unique_ptr<sharpen::IQuorum>(sharpen::IQuorum *)> configurater) = 0;

        virtual void NviStoreLastAppliedIndex(std::uint64_t index) = 0;

        virtual std::uint64_t NviGetLastAppliedIndex() const noexcept = 0;

        virtual sharpen::Optional<sharpen::ConsensusPeersConfiguration>
        NviGetPeersConfiguration() const = 0;

        /*
            How to change peers:
                * Write a log to invoke prepare peers changes   (apply)
                * Write a log to invoke ConfigPeers             (apply)
        */
        virtual sharpen::ConsensusChangeResult NviPreparePeersChanges() = 0;

        /*
            If meet any errors during chaning:
                * Write a log to invoke abort peers changes     (apply)
        */
        virtual sharpen::ConsensusChangeResult NviAbortPeersChanges() = 0;
    public:
        IConsensus() noexcept = default;

        IConsensus(const Self &other) noexcept = default;

        IConsensus(Self &&other) noexcept = default;

        Self &operator=(const Self &other) noexcept = default;

        Self &operator=(Self &&other) noexcept = default;

        virtual ~IConsensus() noexcept = default;

        inline const Self &Const() const noexcept {
            return *this;
        }

        virtual void Advance() = 0;

        virtual bool Writable() const = 0;

        inline sharpen::WriteLogsResult Write(const sharpen::LogBatch &logs) {
            return this->NviWrite(logs);
        }

        virtual bool PeersChangeable() const = 0;

        inline void WaitNextConsensus(sharpen::Future<sharpen::ConsensusResult> &future) {
            this->NviWaitNextConsensus(future);
        }

        inline sharpen::ConsensusResult WaitNextConsensus() {
            sharpen::AwaitableFuture<sharpen::ConsensusResult> future;
            this->NviWaitNextConsensus(future);
            return future.Await();
        }

        virtual const sharpen::ILogStorage &ImmutableLogs() const noexcept = 0;

        inline bool IsConsensusMail(const sharpen::Mail &mail) const noexcept {
            if (mail.Empty()) {
                return false;
            }
            return this->NviIsConsensusMail(mail);
        }

        virtual sharpen::IMailReceiver &GetReceiver() noexcept = 0;

        virtual const sharpen::IMailReceiver &GetReceiver() const noexcept = 0;

        inline sharpen::Mail GenerateResponse(sharpen::Mail request) {
            if (!request.Empty() && this->IsConsensusMail(request)) {
                return this->NviGenerateResponse(std::move(request));
            }
            return sharpen::Mail{};
        }

        inline void DropLogsUntil(std::uint64_t endIndex) {
            this->NviDropLogsUntil(endIndex);
        }

        inline sharpen::ConsensusChangeResult ConfiguratePeers(
            std::function<std::unique_ptr<sharpen::IQuorum>(sharpen::IQuorum *)> configurater) {
            if (configurater) {
                return this->NviConfiguratePeers(std::move(configurater));
            }
            return sharpen::ConsensusChangeResult::Invalid;
        }

        template<typename _Fn,
                 typename... _Args,
                 typename _Check = sharpen::EnableIf<
                     sharpen::IsCompletedBindableReturned<std::unique_ptr<sharpen::IQuorum>,
                                                          _Fn,
                                                          sharpen::IQuorum *,
                                                          _Args...>::Value>>
        inline sharpen::ConsensusChangeResult ConfiguratePeers(_Fn &&fn, _Args &&...args) {
            std::function<std::unique_ptr<sharpen::IQuorum>(sharpen::IQuorum *)> config{std::bind(
                std::forward<_Fn>(fn), std::placeholders::_1, std::forward<_Args>(args)...)};
            return this->ConfiguratePeers(std::move(config));
        }

        virtual void ReleasePeers() = 0;

        virtual sharpen::ConsensusWriter GetWriterId() const noexcept = 0;

        virtual std::uint64_t GetEpoch() const noexcept = 0;

        virtual std::uint64_t GetCommitIndex() const noexcept = 0;

        inline void StoreLastAppliedIndex(std::uint64_t index) {
            this->NviStoreLastAppliedIndex(index);
        }

        inline std::uint64_t GetLastAppliedIndex() const noexcept {
            return this->NviGetLastAppliedIndex();
        }

        inline sharpen::Optional<sharpen::ConsensusPeersConfiguration> GetPeersConfiguration() const {
            return this->NviGetPeersConfiguration();
        }

        // should be invoked when apply log entry
        inline sharpen::ConsensusChangeResult PreparePeersChanges() {
            return this->NviPreparePeersChanges();
        }

        // should be invoked when apply log entry
        inline sharpen::ConsensusChangeResult AbortPeersChanges() {
            return this->NviAbortPeersChanges();
        }
    };
}   // namespace sharpen

#endif