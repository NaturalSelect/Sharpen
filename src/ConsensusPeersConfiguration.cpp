#include <sharpen/ConsensusPeersConfiguration.hpp>

#include <sharpen/Varint.hpp>

sharpen::ConsensusPeersConfiguration::ConsensusPeersConfiguration(Self &&other) noexcept
    : epoch_(other.epoch_)
    , peers_(std::move(other.peers_)) {
    other.epoch_ = 0;
}

sharpen::ConsensusPeersConfiguration &sharpen::ConsensusPeersConfiguration::operator=(
    Self &&other) noexcept {
    if (this != std::addressof(other)) {
        this->epoch_ = other.epoch_;
        this->peers_ = std::move(other.peers_);
        other.epoch_ = 0;
    }
    return *this;
}

std::size_t sharpen::ConsensusPeersConfiguration::ComputeSize() const noexcept {
    sharpen::Varuint64 builder{this->epoch_};
    std::size_t size{sharpen::BinarySerializator::ComputeSize(builder)};
    size += sharpen::BinarySerializator::ComputeSize(this->peers_);
    return size;
}

std::size_t sharpen::ConsensusPeersConfiguration::LoadFrom(const char *data, std::size_t size) {
    if (size == 0) {
        throw sharpen::CorruptedDataError{"corrupted consensus peers"};
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += sharpen::BinarySerializator::LoadFrom(builder, data, size);
    if (size <= offset) {
        throw sharpen::CorruptedDataError{"corrupted consensus peers"};
    }
    std::set<sharpen::ActorId> peer;
    offset += sharpen::BinarySerializator::LoadFrom(peer, data + offset, size - offset);
    return offset;
}

std::size_t sharpen::ConsensusPeersConfiguration::UnsafeStoreTo(char *data) const noexcept {
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->epoch_};
    offset += sharpen::BinarySerializator::UnsafeStoreTo(builder, data);
    offset += sharpen::BinarySerializator::UnsafeStoreTo(this->peers_, data + offset);
    return offset;
}