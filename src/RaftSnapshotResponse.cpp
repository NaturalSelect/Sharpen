#include <sharpen/RaftSnapshotResponse.hpp>

#include <sharpen/ConsensusWriter.hpp>
#include <sharpen/Varint.hpp>

sharpen::RaftSnapshotResponse::RaftSnapshotResponse() noexcept
    : status_(false)
    , term_(sharpen::ConsensusWriter::noneEpoch)
    , peersEpoch_(0)
    , leaseRound_(0) {
}

sharpen::RaftSnapshotResponse::RaftSnapshotResponse(bool status, std::uint64_t term) noexcept
    : status_(status)
    , term_(term)
    , peersEpoch_(0)
    , leaseRound_(0) {
}

sharpen::RaftSnapshotResponse::RaftSnapshotResponse(Self &&other) noexcept
    : status_(other.status_)
    , term_(other.term_)
    , peersEpoch_(other.peersEpoch_)
    , leaseRound_(other.leaseRound_) {
    other.status_ = false;
    other.term_ = sharpen::ConsensusWriter::noneEpoch;
    other.peersEpoch_ = 0;
    other.leaseRound_ = 0;
}

sharpen::RaftSnapshotResponse &sharpen::RaftSnapshotResponse::operator=(
    const Self &other) noexcept {
    if (this != std::addressof(other)) {
        this->status_ = other.status_;
        this->term_ = other.term_;
        this->peersEpoch_ = other.peersEpoch_;
        this->leaseRound_ = other.leaseRound_;
    }
    return *this;
}

sharpen::RaftSnapshotResponse &sharpen::RaftSnapshotResponse::operator=(Self &&other) noexcept {
    if (this != std::addressof(other)) {
        this->status_ = other.status_;
        this->term_ = other.term_;
        this->peersEpoch_ = other.peersEpoch_;
        this->leaseRound_ = other.leaseRound_;
        other.status_ = false;
        other.term_ = sharpen::ConsensusWriter::noneEpoch;
        other.peersEpoch_ = 0;
        other.leaseRound_ = 0;
    }
    return *this;
}

std::size_t sharpen::RaftSnapshotResponse::ComputeSize() const noexcept {
    std::size_t size{sizeof(std::uint8_t)};
    sharpen::Varuint64 builder{this->term_};
    size += sharpen::BinarySerializator::ComputeSize(builder);
    builder.Set(this->peersEpoch_);
    size += sharpen::BinarySerializator::ComputeSize(builder);
    builder.Set(this->leaseRound_);
    size += sharpen::BinarySerializator::ComputeSize(builder);
    return size;
}

std::size_t sharpen::RaftSnapshotResponse::LoadFrom(const char *data, std::size_t size) {
    if (size < 4) {
        throw sharpen::CorruptedDataError{"corrupted raft snapshot response"};
    }
    std::size_t offset{0};
    std::uint8_t status{0};
    std::memcpy(&status, data, sizeof(status));
    this->status_ = status;
    offset += sizeof(status);
    sharpen::Varuint64 builder{0};
    offset += sharpen::BinarySerializator::LoadFrom(builder,data + offset,size - offset);
    this->term_ = builder.Get();
    if (size < 2 + offset) {
        throw sharpen::CorruptedDataError{"corrupted raft snapshot response"};
    }
    offset += sharpen::BinarySerializator::LoadFrom(builder,data + offset,size - offset);
    this->peersEpoch_ = builder.Get();
    if (size < 1 + offset) {
        throw sharpen::CorruptedDataError{"corrupted raft snapshot response"};
    }
    offset += sharpen::BinarySerializator::LoadFrom(builder,data + offset,size - offset);
    this->leaseRound_ = builder.Get();
    return offset;
}

std::size_t sharpen::RaftSnapshotResponse::UnsafeStoreTo(char *data) const noexcept {
    std::size_t offset{0};
    std::uint8_t status{this->status_};
    std::memcpy(data, &status, sizeof(status));
    offset += sizeof(status);
    sharpen::Varuint64 builder{this->term_};
    offset += sharpen::BinarySerializator::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->peersEpoch_);
    offset += sharpen::BinarySerializator::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->leaseRound_);
    offset += sharpen::BinarySerializator::UnsafeStoreTo(builder,data + offset);
    return offset;
}