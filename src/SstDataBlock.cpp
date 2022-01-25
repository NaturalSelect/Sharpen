#include <sharpen/SstDataBlock.hpp>

void sharpen::SstDataBlock::LoadFrom(const char *data, sharpen::Size size)
{
    if (size < 3)
    {
        throw std::invalid_argument("invalid buffer");
    }
    sharpen::Uint16 crc{0};
    std::memcpy(&crc, data, sizeof(crc));
    sharpen::Varuint64 groups{0};
    sharpen::Size offset{sizeof(crc)};
    //checksum
    if (crc != sharpen::Crc16(data + offset, size - offset))
    {
        throw sharpen::DataCorruptionException("data block corruption");
    }
    //number of groups
    groups.Set(data + offset, size - offset);
    offset += groups.ComputeSize();
    //load key value groups index
    sharpen::Size number{sharpen::IntCast<sharpen::Size>(groups.Get())};
    if (number * sizeof(sharpen::Uint64) + offset > size)
    {
        throw sharpen::DataCorruptionException("data block corruption");
    }
    std::vector<std::pair<sharpen::Size, sharpen::Size>> indexs;
    indexs.reserve(number);
    {
        sharpen::Uint64 off{0};
        for (sharpen::Size i = 1, count = number; i < count; ++i)
        {
            std::memcpy(&off, data + offset, sizeof(off));
            sharpen::Size begin{sharpen::IntCast<sharpen::Size>(off)};
            //check error
            if (begin > size)
            {
                throw sharpen::DataCorruptionException("data block corruption");
            }
            offset += sizeof(off);
            std::memcpy(&off, data + offset, sizeof(off));
            sharpen::Size end{off};
            //check error
            if (end > size || end < begin)
            {
                throw sharpen::DataCorruptionException("data block corruption");
            }
            indexs.emplace_back(begin, end - begin);
        }
        std::memcpy(&off, data + offset, sizeof(off));
        sharpen::Size begin{off};
        offset += sizeof(off);
        if (begin > size)
        {
            throw sharpen::DataCorruptionException("data block corruption");
        }
        indexs.emplace_back(begin, size - begin);
    }
    //load key value groups
    sharpen::SstKeyValueGroup group;
    this->groups_.clear();
    for (auto begin = indexs.begin(), end = indexs.end(); begin != end; ++begin)
    {
        try
        {
            group.LoadFrom(data + begin->first, begin->second);
            this->groups_.emplace_back(std::move(group));
        }
        catch (const std::exception &)
        {
            this->groups_.clear();
            throw;
        }
    }
}

void sharpen::SstDataBlock::LoadFrom(const sharpen::ByteBuffer &buf, sharpen::Size offset)
{
    assert(buf.GetSize() >= offset);
    this->LoadFrom(buf.Data() + offset, buf.GetSize() - offset);
}

sharpen::Size sharpen::SstDataBlock::ComputeSize() const noexcept
{
    sharpen::Size size{sizeof(sharpen::Uint16)};
    sharpen::Varuint64 builder{this->groups_.size()};
    size += builder.ComputeSize();
    size += builder.Get() * sizeof(sharpen::Uint64);
    for (auto begin = this->groups_.cbegin(), end = this->groups_.cend(); begin != end; ++begin)
    {
        size += begin->ComputeSize();
    }
    return size;
}

sharpen::Size sharpen::SstDataBlock::UnsafeStoreTo(char *data) const
{
    sharpen::Uint16 crc{0};
    sharpen::Varuint64 builder{this->groups_.size()};
    //store number
    sharpen::Size offset{builder.ComputeSize()};
    std::memcpy(data + sizeof(crc), builder.Data(), offset);
    offset += sizeof(crc);
    //store key value offsets and groups
    sharpen::Size last{offset + sizeof(sharpen::Uint64) * this->groups_.size()};
    for (sharpen::Size i = 0, count = this->groups_.size(); i != count; ++i)
    {
        sharpen::Size kvSize{this->groups_[i].UnsafeStoreTo(data + last)};
        sharpen::Uint64 off{last};
        std::memcpy(data + offset + i * sizeof(off), &off, sizeof(off));
        last += kvSize;
    }
    //store checksum
    crc = sharpen::Crc16(data + sizeof(crc), last - sizeof(crc));
    std::memcpy(data, &crc, sizeof(crc));
    return last;
}

sharpen::Size sharpen::SstDataBlock::StoreTo(char *data, sharpen::Size size) const
{
    sharpen::Size needSize{this->ComputeSize()};
    if (size < needSize)
    {
        throw std::invalid_argument("buffer too small");
    }
    return this->UnsafeStoreTo(data);
}

sharpen::Size sharpen::SstDataBlock::StoreTo(sharpen::ByteBuffer &buf, sharpen::Size offset) const
{
    assert(buf.GetSize() >= offset);
    sharpen::Size needSize{this->ComputeSize()};
    sharpen::Size size{buf.GetSize() - offset};
    if (needSize > size)
    {
        buf.Extend(needSize - size);
    }
    return this->UnsafeStoreTo(buf.Data());
}

bool sharpen::SstDataBlock::Less(const sharpen::SstKeyValueGroup &group, const sharpen::ByteBuffer &key) noexcept
{
    assert(!group.Empty());
    return group.Last().GetKey() < key;
}

sharpen::SstDataBlock::Iterator sharpen::SstDataBlock::FindGroup(const sharpen::ByteBuffer &key)
{
    using FnPtr = bool (*)(const sharpen::SstKeyValueGroup &, const sharpen::ByteBuffer &);
    return std::lower_bound(this->groups_.begin(), this->groups_.end(), key, static_cast<FnPtr>(&Self::Less));
}

sharpen::SstDataBlock::ConstIterator sharpen::SstDataBlock::FindGroup(const sharpen::ByteBuffer &key) const
{
    using FnPtr = bool (*)(const sharpen::SstKeyValueGroup &, const sharpen::ByteBuffer &);
    return std::lower_bound(this->groups_.begin(), this->groups_.end(), key, static_cast<FnPtr>(&Self::Less));
}

sharpen::SstDataBlock::Iterator sharpen::SstDataBlock::FindInsertGroup(const sharpen::ByteBuffer &key)
{
    auto ite = this->FindGroup(key);
    if (ite == this->End())
    {
        if (this->Empty())
        {
            return ite;
        }
        return sharpen::IteratorBackward(ite, 1);
    }
    return ite;
}

sharpen::SstDataBlock::ConstIterator sharpen::SstDataBlock::FindInsertGroup(const sharpen::ByteBuffer &key) const
{
    auto ite = this->FindGroup(key);
    if (ite == this->End())
    {
        if (this->Empty())
        {
            return ite;
        }
        return sharpen::IteratorBackward(ite, 1);
    }
    return ite;
}

bool sharpen::SstDataBlock::Exist(const sharpen::ByteBuffer &key) const
{
    auto ite = this->FindGroup(key);
    return ite != this->End() && ite->Exist(key);
}

void sharpen::SstDataBlock::Put(sharpen::ByteBuffer key, sharpen::ByteBuffer value)
{
    auto ite = this->FindInsertGroup(key);
    if (ite != this->groups_.end())
    {
        //check and insert key
        if (ite->CheckKey(key))
        {
            if (ite->GetSize() >= maxKeyPerGroups_)
            {
                //div this group to 2 groups
                sharpen::SstKeyValueGroup group;
                auto begin = sharpen::IteratorForward(ite->Begin(), maxKeyPerGroups_);
                auto end = ite->End();
                group.Reserve(sharpen::GetRangeSize(begin, end) + 1);
                this->groups_.reserve(this->groups_.size() + 1);
                for (auto i = begin; i != end; ++i)
                {
                    sharpen::ByteBuffer v{std::move(i->Value())};
                    sharpen::ByteBuffer k{std::move(*i).MoveKey()};
                    group.Put(std::move(k),std::move(v));
                }
                bool succ = group.TryPut(std::move(key), std::move(value));
                assert(succ);
                static_cast<void>(succ);
                ite = sharpen::IteratorForward(ite, 1);
                if (ite == this->groups_.end())
                {
                    this->groups_.emplace_back(std::move(group));
                    return;
                }
                this->groups_.emplace(ite, std::move(group));
            }
            else
            {
                bool succ = ite->TryPut(std::move(key), std::move(value));
                assert(succ);
                static_cast<void>(succ);
            }
            return;
        }
        //need to create a new group
        sharpen::SstKeyValueGroup group;
        group.Put(std::move(key), std::move(value));
        ite = sharpen::IteratorForward(ite, 1);
        if (ite == this->End())
        {
            this->groups_.emplace_back(std::move(group));
        }
        else
        {
            this->groups_.emplace(ite, std::move(group));
        }
        return;
    }
    sharpen::SstKeyValueGroup group;
    group.Put(std::move(key), std::move(value));
    this->groups_.emplace_back(std::move(group));
}

void sharpen::SstDataBlock::Delete(const sharpen::ByteBuffer &key)
{
    auto ite = this->FindGroup(key);
    if (ite != this->End())
    {
        ite->Delete(key);
        if (ite->Empty())
        {
            this->groups_.erase(ite);
        }
    }
}

sharpen::Size sharpen::SstDataBlock::ComputeKeyCount() const noexcept
{
    sharpen::Size size{0};
    for (auto begin = this->Begin(), end = this->End(); begin != end; ++begin)
    {
        size += begin->GetSize();
    }
    return size;
}

sharpen::ByteBuffer &sharpen::SstDataBlock::Get(const sharpen::ByteBuffer &key)
{
    auto ite = this->FindGroup(key);
    if (ite == this->groups_.end())
    {
        throw std::out_of_range("key doesn't exists");
    }
    return ite->GetValue(key);
}

const sharpen::ByteBuffer &sharpen::SstDataBlock::Get(const sharpen::ByteBuffer &key) const
{
    auto ite = this->FindGroup(key);
    if (ite == this->groups_.end())
    {
        throw std::out_of_range("key doesn't exists");
    }
    return ite->GetValue(key);
}

void sharpen::SstDataBlock::Combine(sharpen::SstDataBlock block, bool reserveCurrent)
{
    if (reserveCurrent)
    {
        for (auto begin = this->Begin(), end = this->End(); begin != end; ++begin)
        {
            for (auto keyBegin = begin->Begin(), keyEnd = begin->End(); keyBegin < keyEnd; ++keyBegin)
            {
                sharpen::ByteBuffer value{std::move(keyBegin->Value())};
                sharpen::ByteBuffer key{std::move(*keyBegin).MoveKey()};
                block.Put(std::move(key), std::move(value));
            }
        }
    }
    else
    {
        block.Combine(*this,true);
    }
    *this = std::move(block);
}


sharpen::SstDataBlock sharpen::SstDataBlock::Split()
{
    sharpen::SstDataBlock block;
    sharpen::Size keyCount{this->ComputeKeyCount() / 2};
    sharpen::Size movedKey{0};
    auto begin = this->ReverseBegin();
    auto end = this->ReverseEnd();
    for (; movedKey < keyCount && begin != end; ++begin)
    {
        block.groups_.emplace_back(std::move(*begin));
        movedKey += block.groups_.back().GetSize();
    }
    sharpen::Size moved{sharpen::GetRangeSize(this->ReverseBegin(), begin)};
    this->groups_.erase(sharpen::IteratorBackward(this->End(), moved), this->End());
    return block;
}

bool sharpen::SstDataBlock::Atomic() const noexcept
{
    sharpen::Size size{this->ComputeKeyCount()/2};
    for (auto begin = this->Begin(), end = this->End(); begin != end; ++begin)
    {
        if (begin->GetSize() > size)
        {
            return true;
        }
    }
    return false;
}