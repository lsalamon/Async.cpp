#pragma once
#include "async_cpp/async/Async.h"
#include "async_cpp/async/AsyncResult.h"
#include "async_cpp/async/detail/IParallelTask.h"

#include "async_cpp/tasks/IManager.h"

#include <functional>
#include <vector>

namespace async_cpp {
namespace async {
namespace detail {

/**
 * Task which collects a set of futures from parallel tasks.
 */
template<class TDATA>
class ParallelTerminalTask : public IParallelTask {
public:
    /**
     * Create an asynchronous task that does not take in information and returns an AsyncResult via a packaged_task.
     * @param generateResult packaged_task that will produce the AsyncResult
     */
    ParallelTerminalTask(std::weak_ptr<tasks::IManager> mgr);
    ParallelTerminalTask(ParallelTerminalTask&& other);
    virtual ~ParallelTerminalTask();

    std::future<AsyncResult<TDATA>> getFuture();

    void forwardResult(std::future<AsyncResult<TDATA>>&& futureResult);
protected:
    virtual void performSpecific() final;
    virtual void notifyFailureToPerform() final;

private:
    std::packaged_task<AsyncResult<TDATA>(AsyncResult<TDATA>&&)> mFutureTask;
    std::future<AsyncResult<TDATA>> mGeneratedFuture;
};

//inline implementations
//------------------------------------------------------------------------------
template<class TDATA>
ParallelTerminalTask<TDATA>::ParallelTerminalTask(std::weak_ptr<tasks::IManager> mgr) : IParallelTask(mgr)
{
    mFutureTask = std::packaged_task<AsyncResult<TDATA>(AsyncResult<TDATA>&&)>(
        [](AsyncResult<TDATA>&& result) -> AsyncResult<TDATA> 
        {
            return std::move(result);
        }
    );
}

//------------------------------------------------------------------------------
template<class TDATA>
ParallelTerminalTask<TDATA>::ParallelTerminalTask(ParallelTerminalTask&& other) 
    : IParallelTask(other.mManager),
    mFutureTask(std::move(other.mFutureTask)),
    mGeneratedFuture(std::move(other.mGeneratedFuture))
{
    
}

//------------------------------------------------------------------------------
template<class TDATA>
ParallelTerminalTask<TDATA>::~ParallelTerminalTask()
{

}

//------------------------------------------------------------------------------
template<class TDATA>
void ParallelTerminalTask<TDATA>::performSpecific()
{
#ifdef _MSC_VER //wait_for is broken in VC11 have to use MS specific _Is_ready
    if(mGeneratedFuture._Is_ready())
#else
    if(std::future_status::ready == mGeneratedFuture.wait_for(std::chrono::milliseconds(0)))
#endif
    {
        mFutureTask(std::move(mGeneratedFuture.get()));
    }
    else
    {
        if(auto mgr = mManager.lock())
        {
            mgr->run(std::make_shared<ParallelTerminalTask<TDATA>>(std::move(*this)));
        }
        else
        {
            notifyFailureToPerform();
        }
    }
}

//------------------------------------------------------------------------------
template<class TDATA>
void ParallelTerminalTask<TDATA>::notifyFailureToPerform()
{
    mFutureTask(std::move(AsyncResult<TDATA>("ParallelTerminalTask: Failed to perform")));
}

//------------------------------------------------------------------------------
template<class TDATA>
std::future<AsyncResult<TDATA>> ParallelTerminalTask<TDATA>::getFuture()
{
    return mFutureTask.get_future();
}

//------------------------------------------------------------------------------
template<class TDATA>
void ParallelTerminalTask<TDATA>::forwardResult(std::future<AsyncResult<TDATA>>&& futureResult)
{
    mGeneratedFuture = std::move(futureResult);
}

}
}
}