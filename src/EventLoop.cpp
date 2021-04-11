#include <sharpen/EventLoop.hpp>

#include <cassert>
#include <mutex>

thread_local sharpen::EventLoop *sharpen::EventLoop::localLoop_(nullptr);

sharpen::EventLoop::EventLoop(SelectorPtr selector)
    :selector_(selector)
    ,tasks_(std::make_shared<TaskVector>())
    ,exectingTask_(false)
    ,lock_(std::make_shared<sharpen::SpinLock>())
    ,running_(false)
{
    assert(selector != nullptr);
}

sharpen::EventLoop::EventLoop(SelectorPtr selector,TaskVectorPtr tasks,LockPtr lock)
    :selector_(selector)
    ,tasks_(tasks)
    ,exectingTask_(false)
    ,lock_(lock)
    ,running_(false)
{
    assert(selector != nullptr);
}

sharpen::EventLoop::~EventLoop() noexcept
{
    this->Stop();
}

void sharpen::EventLoop::Bind(WeakChannelPtr channel)
{
    this->selector_->Resister(channel);
}

void sharpen::EventLoop::QueueInLoop(Task task)
{
    if (this->GetLocalLoop() == this)
    {
        try
        {
            task();
        }
        catch(const std::exception& ignore)
        {
            assert(ignore.what() == nullptr);
            (void)ignore;
        }
        return;
    }
    bool execting(true);
    {
        std::unique_lock<Lock> lock(*this->lock_);
        this->tasks_->push_back(std::move(task));
        std::swap(execting,this->exectingTask_);
    }
    if (!execting)
    {
        this->selector_->Notify();
    }
}

void sharpen::EventLoop::ExecuteTask()
{
    TaskVector tasks;
    {
        std::unique_lock<Lock> lock(*this->lock_);
        this->exectingTask_ = false;
        if (this->tasks_->empty())
        {
            return;
        }
        std::swap(*this->tasks_,tasks);
    }
    for (auto begin = tasks.begin(),end = tasks.end();begin != end;++begin)
    {
        try
        {
            (*begin)();
        }
        catch(const std::exception& ignore)
        {
            assert(ignore.what() == nullptr);
            (void)ignore;
        }
    }
}

void sharpen::EventLoop::Run()
{
    if (sharpen::EventLoop::IsInLoop())
    {
        throw std::logic_error("now is in event loop");
    }
    sharpen::EventLoop::localLoop_ = this;
    EventVector events;
    this->running_ = true;
    while (this->running_)
    {
        this->selector_->Select(events);
        for (auto begin = events.begin(),end = events.end();begin != end;++begin)
        {
            bool validate = (*begin)->ValidateChannel();
            if (validate)
            {
                (*begin)->GetChannel()->OnEvent(*begin);
            }
        }
        events.clear();
        this->ExecuteTask();
    }
    sharpen::EventLoop::localLoop_ = nullptr;
}

void sharpen::EventLoop::Stop() noexcept
{
    if (this->running_)
    {
        this->running_ = false;
        this->selector_->Notify();
    }
}

sharpen::EventLoop *sharpen::EventLoop::GetLocalLoop() noexcept
{
    return sharpen::EventLoop::localLoop_;
}

bool sharpen::EventLoop::IsInLoop() noexcept
{
    return sharpen::EventLoop::GetLocalLoop() != nullptr;
}