#include <sharpen/RaftSnapshotMetadata.hpp>

sharpen::RaftSnapshotMetadata::RaftSnapshotMetadata() noexcept
    : lastIndex_(0)
    , lastTerm_(0)
    , peers_() {
}

sharpen::RaftSnapshotMetadata::RaftSnapshotMetadata(const Self &other) noexcept
    : lastIndex_(other.lastIndex_)
    , lastTerm_(other.lastTerm_)
    , peers_(other.peers_) {
}

sharpen::RaftSnapshotMetadata::RaftSnapshotMetadata(Self &&other) noexcept
    : lastIndex_(other.lastIndex_)
    , lastTerm_(other.lastTerm_)
    , peers_(std::move(other.peers_)) {
    other.lastIndex_ = 0;
    other.lastTerm_ = 0;
}

sharpen::RaftSnapshotMetadata &sharpen::RaftSnapshotMetadata::operator=(
    const Self &other) noexcept {
    if (this != std::addressof(other)) {
        this->lastIndex_ = other.lastIndex_;
        this->lastTerm_ = other.lastTerm_;
        this->peers_ = std::move(other.peers_);
    }
    return *this;
}

sharpen::RaftSnapshotMetadata &sharpen::RaftSnapshotMetadata::operator=(Self &&other) noexcept {
    if (this != std::addressof(other)) {
        this->lastIndex_ = other.lastIndex_;
        this->lastTerm_ = other.lastTerm_;
        other.lastIndex_ = 0;
        other.lastTerm_ = 0;
    }
    return *this;
}

void sharpen::RaftSnapshotMetadata::SetLastIndex(std::uint64_t index) noexcept {
    this->lastIndex_ = index;
    if (!this->lastIndex_) {
        this->lastTerm_ = 0;
    }
}

std::size_t sharpen::RaftSnapshotMetadata::ComputeSize() const noexcept {
    sharpen::Varuint64 builder{this->lastIndex_};
    std::size_t size{builder.ComputeSize()};
    builder.Set(this->lastTerm_);
    size += builder.ComputeSize();
    size += sharpen::BinarySerializator::ComputeSize(this->peers_);
    return size;
}

std::size_t sharpen::RaftSnapshotMetadata::LoadFrom(const char *data, std::size_t size) {
    std::size_t offset{0};
    if (size < 3) {
        throw sharpen::CorruptedDataError{"corrupted raft snapshot metadata"};
    }
    sharpen::Varuint64 builder{0};
    offset += builder.LoadFrom(data, size);
    std::uint64_t lastIndex{builder.Get()};
    if (size < 2 + offset) {
        throw sharpen::CorruptedDataError{"corrupted raft snapshot metadata"};
    }
    offset += builder.LoadFrom(data + offset, size - offset);
    std::uint64_t lastTerm{builder.Get()};
    sharpen::ConsensusPeersConfiguration peers;
    if (size < 1 + offset) {
        throw sharpen::CorruptedDataError{"corrupted raft snapshot metadata"};
    }
    offset += sharpen::BinarySerializator::LoadFrom(peers,data + offset,size - offset);
    this->lastIndex_ = lastIndex;
    this->lastTerm_ = lastTerm;
    this->peers_ = std::move(peers);
    return offset;
}

std::size_t sharpen::RaftSnapshotMetadata::UnsafeStoreTo(char *data) const noexcept {
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->lastIndex_};
    offset += builder.UnsafeStoreTo(data);
    builder.Set(this->lastTerm_);
    offset += builder.UnsafeStoreTo(data + offset);
    offset += sharpen::BinarySerializator::UnsafeStoreTo(this->peers_,data + offset);
    return offset;
}