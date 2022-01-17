#include <sharpen/SstKeyValuePair.hpp>

sharpen::SstKeyValuePair::SstKeyValuePair()
    :sharedSize_(0)
    ,uniquedSize_(0)
    ,key_()
    ,value_()
{}

sharpen::SstKeyValuePair::SstKeyValuePair(sharpen::Uint64 sharedSize,sharpen::Uint64 uniquedSize,sharpen::ByteBuffer key,sharpen::ByteBuffer value)
    :sharedSize_(sharedSize)
    ,uniquedSize_(uniquedSize)
    ,key_(std::move(key))
    ,value_(std::move(value))
{}

sharpen::Size sharpen::SstKeyValuePair::LoadFrom(const char *data,sharpen::Size size)
{
    if(size < 3)
    {
        throw std::invalid_argument("invalid buffer");
    }
    sharpen::Varuint64 builder{data,size};
    sharpen::Size offset{builder.ComputeSize()};
    this->sharedSize_ = builder.Get();
    builder.Set(data + offset,size - offset);
    this->uniquedSize_ = builder.Get();
    offset += builder.ComputeSize();
    sharpen::Size keySize{sharpen::IntCast<sharpen::Size>(this->uniquedSize_ + this->sharedSize_)};
    this->key_.ExtendTo(keySize);
    if(this->uniquedSize_)
    {
        std::memcpy(this->key_.Data() + this->sharedSize_,data + offset,this->uniquedSize_);
        offset = sharpen::IntCast<sharpen::Size>(this->uniquedSize_ + offset);
    }
    builder.Set(data + offset,size - offset);
    this->value_.ExtendTo(sharpen::IntCast<sharpen::Size>(builder.Get()));
    offset += builder.ComputeSize();
    if (builder.Get())
    {
        std::memcpy(this->value_.Data(),data + offset,builder.Get());
        offset = sharpen::IntCast<sharpen::Size>(builder.Get() + offset);
    }
    return offset;
}

sharpen::Size sharpen::SstKeyValuePair::LoadFrom(const sharpen::ByteBuffer &buf,sharpen::Size offset)
{
    return this->LoadFrom(buf.Data() + offset,buf.GetSize() - offset);
}

sharpen::Size sharpen::SstKeyValuePair::ComputeSize() const noexcept
{
    sharpen::Varuint64 builder{this->sharedSize_};
    sharpen::Size size{builder.ComputeSize()};
    builder.Set(this->uniquedSize_);
    size += builder.ComputeSize();
    assert(this->uniquedSize_ + this->sharedSize_ == this->key_.GetSize());
    size = sharpen::IntCast<sharpen::Size>(this->uniquedSize_ + size);
    builder.Set(this->value_.GetSize());
    size += builder.ComputeSize();
    size += this->value_.GetSize();
    return size;
}

sharpen::Size sharpen::SstKeyValuePair::InternalStoreTo(char *data) const
{
    sharpen::Varuint64 builder{this->sharedSize_};
    sharpen::Size offset{builder.ComputeSize()};
    std::memcpy(data,builder.Data(),offset);
    builder.Set(this->uniquedSize_);
    sharpen::Size size{builder.ComputeSize()};
    std::memcpy(data + offset,builder.Data(),size);
    offset += size;
    assert(this->uniquedSize_ + this->sharedSize_ == this->key_.GetSize());
    if(this->uniquedSize_)
    {
        std::memcpy(data + offset,this->key_.Data(),sharpen::IntCast<sharpen::Size>(this->uniquedSize_));
    }
    offset = sharpen::IntCast<sharpen::Size>(this->uniquedSize_ + offset);
    builder.Set(this->value_.GetSize());
    size = builder.ComputeSize();
    std::memcpy(data + offset,builder.Data(),size);
    offset += size;
    if (!this->value_.Empty())
    {
        std::memcpy(data + offset,this->value_.Data(),this->value_.GetSize());   
    }
    offset += this->value_.GetSize();
    return offset;
}

sharpen::Size sharpen::SstKeyValuePair::StoreTo(char *data,sharpen::Size size) const
{
    sharpen::Size needSize{this->ComputeSize()};
    if(needSize > size)
    {
        throw std::invalid_argument("buffer too small");
    }
    return this->InternalStoreTo(data);
}

sharpen::Size sharpen::SstKeyValuePair::StoreTo(sharpen::ByteBuffer &buf,sharpen::Size offset) const
{
    sharpen::Size needSize{this->ComputeSize()};
    sharpen::Size size{buf.GetSize() - offset};
    if(needSize > size)
    {
        buf.Extend(needSize - size);
    } 
    return this->InternalStoreTo(buf.Data());
}