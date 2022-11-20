#include <sharpen/IRemoteActorProposer.hpp>

void sharpen::IRemoteActorProposer::DoPropose(sharpen::Future<bool> *future,const sharpen::Mail *mail) noexcept
{
    (void)mail;
    try
    {
        if(!this->isOpened_)
        {
            this->actor_->Open();
            this->isOpened_ = true;
        }
        sharpen::Mail response{this->actor_->Post(*mail)};
        bool result{this->OnResponse(response)};
        future->Complete(result);
    }
    catch(const std::exception &)
    {
        future->Fail(std::current_exception());
    }
}

void sharpen::IRemoteActorProposer::ProposeAsync(sharpen::Future<bool> &future,const sharpen::Mail &mail)
{
    this->worker_->Submit(&Self::DoPropose,this,&future,&mail);
}

sharpen::IRemoteActorProposer::IRemoteActorProposer(std::unique_ptr<sharpen::WorkerGroup> singleWorker,sharpen::IRemoteActor &actor) noexcept
    :actor_(&actor)
    ,worker_(std::move(singleWorker))
    ,isOpened_(false)
{}

void sharpen::IRemoteActorProposer::Cancel() noexcept
{
    if(this->isOpened_)
    {
        this->actor_->Close();
        this->isOpened_ = false;
    }
}