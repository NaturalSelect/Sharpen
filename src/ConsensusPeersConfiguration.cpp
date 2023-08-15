#include <sharpen/ConsensusPeersConfiguration.hpp>

#include <sharpen/Varint.hpp>

sharpen::ConsensusPeersConfiguration::ConsensusPeersConfiguration(Self &&other) noexcept
    : epoch_(other.epoch_)
    , peers_(std::move(other.peers_))
    , locked_(other.locked_) {
    other.epoch_ = 0;
    other.locked_ = false;
}

sharpen::ConsensusPeersConfiguration &sharpen::ConsensusPeersConfiguration::operator=(
    Self &&other) noexcept {
    if (this != std::addressof(other)) {
        this->epoch_ = other.epoch_;
        this->peers_ = std::move(other.peers_);
        this->locked_ = other.locked_;
        other.epoch_ = 0;
        other.locked_ = false;
    }
    return *this;
}

std::size_t sharpen::ConsensusPeersConfiguration::ComputeSize() const noexcept {
    sharpen::Varuint64 builder{this->epoch_};
    std::size_t size{0};
    size += sharpen::BinarySerializator::ComputeSize(builder);
    size += sharpen::BinarySerializator::ComputeSize(this->peers_);
    size += sharpen::BinarySerializator::ComputeSize(sizeof(std::uint8_t));
    return size;
}

std::size_t sharpen::ConsensusPeersConfiguration::LoadFrom(const char *data, std::size_t size) {
    if (size < 3) {
        throw sharpen::CorruptedDataError{"corrupted consensus peers"};
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += sharpen::BinarySerializator::LoadFrom(builder,data,size);
    this->epoch_ = builder.Get();
    if (size < 2 + offset) {
        throw sharpen::CorruptedDataError{"corrupted consensus peers"};
    }
    std::set<sharpen::ActorId> peer;
    offset += sharpen::BinarySerializator::LoadFrom(peer, data + offset, size - offset);
    this->peers_ = std::move(peer);
    if (size < 1 + offset) {
        throw sharpen::CorruptedDataError{"corrupted consensus peers"};
    }
    std::uint8_t locked;
    offset += sharpen::BinarySerializator::LoadFrom(locked,data + offset,size - offset);
    this->locked_ = locked;
    return offset;
}

std::size_t sharpen::ConsensusPeersConfiguration::UnsafeStoreTo(char *data) const noexcept {
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->epoch_};
    offset += sharpen::BinarySerializator::UnsafeStoreTo(builder, data + offset);
    offset += sharpen::BinarySerializator::UnsafeStoreTo(this->peers_, data + offset);
    std::uint8_t locked{this->locked_};
    offset += sharpen::BinarySerializator::UnsafeStoreTo(locked,data + offset);
    return offset;
}