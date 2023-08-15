#pragma once
#ifndef _SHARPEN_RAFTCONSENSUS_HPP
#define _SHARPEN_RAFTCONSENSUS_HPP

#include "Broadcaster.hpp"
#include "IConsensus.hpp"
#include "IMailReceiver.hpp"
#include "IQuorum.hpp"
#include "IRaftLogAccesser.hpp"
#include "IRaftMailBuilder.hpp"
#include "IRaftMailExtractor.hpp"
#include "IRaftSnapshotController.hpp"
#include "IStatusMap.hpp"
#include "IWorkerGroup.hpp"
#include "Noncopyable.hpp"
#include "RaftElectionRecord.hpp"
#include "RaftHeartbeatMailProvider.hpp"
#include "RaftLeaderCounter.hpp"
#include "RaftLeaderRecord.hpp"
#include "RaftLeaseStatus.hpp"
#include "RaftOption.hpp"
#include "RaftPrevoteRecord.hpp"
#include "RaftRole.hpp"
#include "RaftVoteRecord.hpp"
#include <initializer_list>
#include <map>
#include <queue>
#include <set>


namespace sharpen {
    class RaftConsensus
        : public sharpen::IConsensus
        , private sharpen::IMailReceiver
        , public sharpen::Noncopyable
        , public sharpen::Nonmovable {
    private:
        using Self = sharpen::RaftConsensus;

        // scheduler
        sharpen::IFiberScheduler *scheduler_;

        // the id of current actor
        sharpen::ActorId id_;
        // persistent status map
        std::unique_ptr<sharpen::IStatusMap> statusMap_;
        // storage logs
        std::unique_ptr<sharpen::ILogStorage> logs_;
        std::unique_ptr<sharpen::IRaftLogAccesser> logAccesser_;
        // snapshot provider
        std::unique_ptr<sharpen::IRaftSnapshotController> snapshotController_;
        // raft option
        sharpen::RaftOption option_;
        // cache
        std::atomic_uint64_t term_;
        sharpen::RaftVoteRecord vote_;
        std::atomic_uint64_t appliedIndex_;
        // role
        std::atomic<sharpen::RaftRole> role_;
        // election record
        sharpen::RaftElectionRecord electionRecord_;
        sharpen::RaftPrevoteRecord prevoteRecord_;
        sharpen::RaftLeaseStatus leaseStatus_;
        std::uint64_t prevLeaderCount_;
        std::shared_ptr<sharpen::RaftLeaderCounter> leaderCount_;

        // leader record
        // thread safty
        sharpen::RaftLeaderRecord leaderRecord_;

        // waiters
        sharpen::ConsensusResult lastResult_;
        std::atomic<sharpen::Future<sharpen::ConsensusResult> *> waiter_;
        std::atomic_uint64_t advancedCount_;
        std::atomic_uint64_t reachAdvancedCount_;

        // mail builder
        std::unique_ptr<sharpen::IRaftMailBuilder> mailBuilder_;
        // mail extractor
        std::unique_ptr<sharpen::IRaftMailExtractor> mailExtractor_;
        // quorums
        std::unique_ptr<sharpen::IQuorum> peers_;
        // quorum broadcasters
        std::unique_ptr<sharpen::Broadcaster> peersBroadcaster_;

        // quorum heartbeat provider
        std::unique_ptr<sharpen::RaftHeartbeatMailProvider> heartbeatProvider_;

        // learners
        std::set<sharpen::ActorId> learners_;
        // config change
        std::atomic_bool changeable_;
        sharpen::ConsensusPeersConfiguration peersConfig_;

        // workers
        std::unique_ptr<sharpen::IWorkerGroup> worker_;
        std::unique_ptr<sharpen::IWorkerGroup> logWorker_;

        sharpen::Optional<std::uint64_t> LoadUint64(sharpen::ByteSlice key);

        void SetUint64(sharpen::ByteSlice key, std::uint64_t value);

        void LoadTerm();

        void LoadAppliedIndex();

        void LoadCommitIndex();

        void LoadVoteFor();

        void LoadPeersConfig();

        void SetTerm(std::uint64_t term);

        sharpen::RaftVoteRecord GetVote() const noexcept;

        void SetVote(sharpen::RaftVoteRecord vote);

        std::uint64_t GetLastIndex() const;

        std::uint64_t GetPeerEpoch() const;

        sharpen::IRaftSnapshotProvider &GetSnapshotProvider() noexcept;

        const sharpen::IRaftSnapshotProvider &GetSnapshotProvider() const noexcept;

        sharpen::IRaftSnapshotInstaller &GetSnapshotInstaller() noexcept;

        const sharpen::IRaftSnapshotInstaller &GetSnapshotInstaller() const noexcept;

        sharpen::Optional<std::uint64_t> LookupTerm(std::uint64_t index) const;

        sharpen::Optional<std::uint64_t> LookupTermOfEntry(std::uint64_t index) const noexcept;

        bool CheckEntry(std::uint64_t index, std::uint64_t expectedTerm) const noexcept;

        void EnsureBroadcaster();

        void EnsureConfig() const;

        void EnsureHearbeatProvider();

        void OnStatusChanged(std::initializer_list<sharpen::ConsensusResultEnum> results);

        void RaiseElection();

        void RaisePrevote();

        void StepUp();

        void StepDown();

        void SetCommitIndex(std::uint64_t commitIndex) noexcept;

        // vote
        sharpen::Mail OnVoteRequest(const sharpen::RaftVoteForRequest &request);

        void OnVoteResponse(const sharpen::RaftVoteForResponse &response,
                            const sharpen::ActorId &actorId);

        // prevote
        sharpen::Mail OnPrevoteRequest(const sharpen::RaftPrevoteRequest &request);

        void OnPrevoteResponse(const sharpen::RaftPrevoteResponse &response,
                               const sharpen::ActorId &actorId);

        // heartbeat
        sharpen::Mail OnHeartbeatRequest(const sharpen::RaftHeartbeatRequest &request);

        void OnHeartbeatResponse(const sharpen::RaftHeartbeatResponse &response,
                                 const sharpen::ActorId &actorId);

        // snapshot
        sharpen::Mail OnSnapshotRequest(const sharpen::RaftSnapshotRequest &request);

        void OnSnapshotResponse(const sharpen::RaftSnapshotResponse &response,
                                const sharpen::ActorId &actorId);

        void NotifyWaiter(sharpen::Future<sharpen::ConsensusResult> *future) noexcept;

        virtual void NviWaitNextConsensus(
            sharpen::Future<sharpen::ConsensusResult> &future) override;

        virtual bool NviIsConsensusMail(const sharpen::Mail &mail) const noexcept override;

        virtual sharpen::Mail NviGenerateResponse(sharpen::Mail request) override;

        sharpen::Mail DoGenerateResponse(sharpen::Mail request);

        virtual void NviReceive(sharpen::Mail mail, const sharpen::ActorId &actorId) override;

        void DoReceive(sharpen::Mail mail, sharpen::ActorId actorId);

        virtual sharpen::ConsensusConfigResult NviConfiguratePeers(
            std::function<std::unique_ptr<sharpen::IQuorum>(sharpen::IQuorum *)> configurater)
            override;

        void DoSyncHeartbeatProvider();

        bool CheckInitPeers(const std::set<sharpen::ActorId> &peers) const noexcept;

        bool CheckChangePeers(const std::set<sharpen::ActorId> &oldPeers,
                              const std::set<sharpen::ActorId> &newPeers) const noexcept;

        sharpen::ConsensusConfigResult DoConfiguratePeers(
            std::function<std::unique_ptr<sharpen::IQuorum>(sharpen::IQuorum *)> configurater);

        void DoReleasePeers();

        sharpen::WriteLogsResult DoWrite(const sharpen::LogBatch *logs);

        virtual sharpen::WriteLogsResult NviWrite(const sharpen::LogBatch &logs) override;

        virtual void NviDropLogsUntil(std::uint64_t index) override;

        void DoAdvance();

        void DoStoreLastAppliedIndex(std::uint64_t index);

        void DoNotifyWaiterWhenClose() noexcept;

        virtual void NviStoreLastAppliedIndex(std::uint64_t index) override;

        virtual std::uint64_t NviGetLastAppliedIndex() const noexcept override;

        virtual sharpen::Optional<sharpen::ConsensusPeersConfiguration>
        NviGetPeersConfiguration() const override;

        sharpen::Optional<sharpen::ConsensusPeersConfiguration> DoGetPeersConfiguration() const;

        void DoStorePeersConfig(std::set<sharpen::ActorId> peers);

    public:
        constexpr static sharpen::ByteSlice voteKey{"vote", 4};

        constexpr static sharpen::ByteSlice termKey{"term", 4};

        constexpr static sharpen::ByteSlice lastAppliedKey{"lastAppiled", 11};

        constexpr static sharpen::ByteSlice peersConfigKey{"peersConfig", 11};

        RaftConsensus(const sharpen::ActorId &id,
                      std::unique_ptr<sharpen::IStatusMap> statusMap,
                      std::unique_ptr<sharpen::ILogStorage> logs,
                      std::unique_ptr<sharpen::IRaftLogAccesser> logAccesser,
                      std::unique_ptr<sharpen::IRaftSnapshotController> snapshotController,
                      const sharpen::RaftOption &option);

        RaftConsensus(const sharpen::ActorId &id,
                      std::unique_ptr<sharpen::IStatusMap> statusMap,
                      std::unique_ptr<sharpen::ILogStorage> logs,
                      std::unique_ptr<sharpen::IRaftLogAccesser> logAccesser,
                      std::unique_ptr<sharpen::IRaftSnapshotController> snapshotController,
                      std::shared_ptr<sharpen::RaftLeaderCounter> leaderCount,
                      const sharpen::RaftOption &option);

        RaftConsensus(const sharpen::ActorId &id,
                      std::unique_ptr<sharpen::IStatusMap> statusMap,
                      std::unique_ptr<sharpen::ILogStorage> logs,
                      std::unique_ptr<sharpen::IRaftLogAccesser> logAccesser,
                      std::unique_ptr<sharpen::IRaftSnapshotController> snapshotController,
                      std::shared_ptr<sharpen::RaftLeaderCounter> leaderCount,
                      const sharpen::RaftOption &option,
                      sharpen::IFiberScheduler &scheduler);

        virtual ~RaftConsensus() noexcept;

        inline const Self &Const() const noexcept {
            return *this;
        }

        void PrepareMailBuilder(std::unique_ptr<sharpen::IRaftMailBuilder> builder) noexcept;

        void PrepareMailExtractor(std::unique_ptr<sharpen::IRaftMailExtractor> extractor) noexcept;

        virtual void Advance() override;

        virtual bool Writable() const override;

        virtual bool Changable() const override;

        virtual const sharpen::ILogStorage &ImmutableLogs() const noexcept override;

        inline virtual sharpen::IMailReceiver &GetReceiver() noexcept override {
            return *this;
        }

        inline virtual const sharpen::IMailReceiver &GetReceiver() const noexcept override {
            return *this;
        }

        virtual sharpen::ConsensusWriter GetWriterId() const noexcept override;

        virtual std::uint64_t GetCommitIndex() const noexcept override;

        virtual void ReleasePeers() override;

        virtual std::uint64_t GetEpoch() const noexcept override;

        std::uint64_t GetTerm() const noexcept;
    };
}   // namespace sharpen

#endif