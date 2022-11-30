#include <sharpen/TcpPoster.hpp>

#include <new>
#include <cassert>

#include <sharpen/SystemError.hpp>

void sharpen::TcpPoster::DoClose() noexcept
{
    sharpen::NetStreamChannelPtr channel{nullptr};
    {
        assert(this->lock_);
        std::unique_lock<sharpen::SpinLock> lock{*this->lock_};
        std::swap(channel,this->channel_);
    }
    if(channel)
    {
        channel->Close();
    }
}

void sharpen::TcpPoster::DoOpen()
{
    sharpen::NetStreamChannelPtr channel{nullptr};
    {
        assert(this->lock_);
        std::unique_lock<sharpen::SpinLock> lock{*this->lock_};
        if(!this->channel_)
        {
            assert(this->factory_);
            this->channel_ = this->factory_->Produce();
        }
        channel = this->channel_;
    }
    assert(channel);
    try
    {
        channel->ConnectAsync(*this->remoteEndpoint_);
    }
    catch(const std::system_error &error)
    {
        sharpen::ErrorCode errorCode{static_cast<sharpen::ErrorCode>(error.code().value())};
        switch (errorCode)
        {
        case sharpen::ErrorIsConnected:
            return;
        case sharpen::ErrorCancel:
            throw sharpen::RemotePosterOpenError{"poster is closed by operator"};
            break;
        case sharpen::ErrorConnectRefused:
            throw sharpen::RemotePosterOpenError{"fail to open poster"};
            break;
        }
        throw;
    }
    catch(const std::exception&)
    {
        throw;
    }
}

sharpen::Mail sharpen::TcpPoster::DoPost(const sharpen::Mail &mail)
{
    sharpen::NetStreamChannelPtr channel{nullptr};
    {
        assert(this->lock_);
        std::unique_lock<sharpen::SpinLock> lock{*this->lock_};
        channel = this->channel_;
    }
    if(!channel)
    {
        //already closed
        throw sharpen::RemotePosterClosedError{"poster already closed"};
    }
    //post mail
    if(!mail.Header().Empty())
    {
        channel->WriteAsync(mail.Header());
    }
    if(!mail.Content().Empty())
    {
        channel->WriteAsync(mail.Content());
    }
    //receive mail
    sharpen::ByteBuffer buffer{4096};
    sharpen::Mail response;
    while (!this->parser_->Completed())
    {
        std::size_t sz{0};
        try
        {
            sz = channel->ReadAsync(buffer);
        }
        catch(const std::system_error &error)
        {
            if(error.code().value() != sharpen::ErrorCancel)
            {
                throw;
            }
            throw sharpen::RemotePosterClosedError{"poster is closed by operator"};
        }
        catch(const std::exception&)
        {
            throw;   
        }
        if(!sz)
        {
            throw sharpen::RemotePosterClosedError{"poster already closed"};
        }
        sharpen::ByteSlice slice{buffer.Data(),sz};
        this->parser_->Parse(slice);
    }
    response = this->parser_->PopCompletedMail();
    return response;
}

std::uint64_t sharpen::TcpPoster::DoGetId() const noexcept
{
    return this->remoteEndpoint_->GetHashCode64();
}

sharpen::TcpPoster::TcpPoster(std::unique_ptr<sharpen::IEndPoint> endpoint,std::unique_ptr<sharpen::IMailParser> parser,sharpen::INetSteamFactory *factory)
    :lock_(nullptr)
    ,channel_(nullptr)
    ,remoteEndpoint_(std::move(endpoint))
    ,parser_(std::move(parser))
    ,factory_(factory)
{
    assert(this->remoteEndpoint_);
    assert(this->parser_);
    assert(this->factory_);
    this->lock_.reset(new (std::nothrow) sharpen::SpinLock{});
    if(!this->lock_)
    {
        throw std::bad_alloc{};
    }
}

sharpen::TcpPoster::TcpPoster(Self &&other) noexcept
    :lock_(std::move(other.lock_))
    ,channel_(std::move(other.channel_))
    ,remoteEndpoint_(std::move(other.remoteEndpoint_))
    ,parser_(std::move(other.parser_))
    ,factory_(other.factory_)
{
    other.factory_ = nullptr;
}

sharpen::TcpPoster &sharpen::TcpPoster::operator=(Self &&other) noexcept
{
    if(this != std::addressof(other))
    {
        this->lock_ = std::move(other.lock_);
        this->channel_ = std::move(other.channel_);
        this->remoteEndpoint_ = std::move(other.remoteEndpoint_);
        this->parser_ = std::move(other.parser_);
        this->factory_ = other.factory_;
        other.factory_ = nullptr;
    }
    return *this;
}