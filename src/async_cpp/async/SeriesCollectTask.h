#pragma once
#include "async_cpp/async/Async.h"
#include "async_cpp/async/SeriesTerminalTask.h"

namespace async_cpp {
namespace async {

//------------------------------------------------------------------------------
template<class TDATA, class TRESULT>
class SeriesCollectTask : public ISeriesTask<TDATA> {
public:
    SeriesCollectTask(std::shared_ptr<tasks::IManager> mgr, 
        std::function<std::future<AsyncResult<TRESULT>>(AsyncResult<TDATA>&)> generateResult);
    virtual ~SeriesCollectTask();

    virtual void cancel();

    std::future<AsyncResult<TRESULT>> getFuture();
protected:
    virtual void performSpecific();

private:
    std::function<std::future<AsyncResult<TDATA>>(const AsyncResult<TDATA>&)> mGenerateResultFunc;
    std::shared_ptr<SeriesTerminalTask<TDATA>> mTerminalTask;
};

//inline implementations
//------------------------------------------------------------------------------
template<class TDATA, class TRESULT>
SeriesCollectTask<TDATA, TRESULT>::SeriesCollectTask(std::shared_ptr<tasks::IManager> mgr,
                                     std::function<std::future<AsyncResult<TRESULT>>(AsyncResult<TDATA>&)> generateResult)
                                     : ISeriesTask<TDATA>(mgr), 
                                     mGenerateResultFunc(generateResult),
                                     mTerminalTask(std::make_shared<SeriesTerminalTask<TRESULT>>(mgr))
{

}

//------------------------------------------------------------------------------
template<class TDATA, class TRESULT>
SeriesCollectTask<TDATA, TRESULT>::~SeriesCollectTask()
{

}

//------------------------------------------------------------------------------
template<class TDATA, class TRESULT>
void SeriesCollectTask<TDATA, TRESULT>::cancel()
{
    mTerminalTask->cancel();
}

//------------------------------------------------------------------------------
template<class TDATA, class TRESULT>
void SeriesCollectTask<TDATA, TRESULT>::performSpecific()
{
    if(!mIsCancelled)
    {
#ifdef _MSC_VER //wait_for is broken in VC11 have to use MS specific _Is_ready
        if(mForwardedFuture._Is_ready())
#else
        if(std::future_status::ready == mForwardedFuture.wait_for(std::chrono::milliseconds(0)))
#endif
        {
            mTerminalTask->forwardFuture(mGenerateResultFunc(mForwardedFuture.get()));
            mManager->run(mTerminalTask);
        }
        else
        {
            reset();
            mManager->run(shared_from_this());
        }
    }
}

//------------------------------------------------------------------------------
template<class TDATA, class TRESULT>
std::future<AsyncResult<TRESULT>> SeriesCollectTask<TDATA, TRESULT>::getFuture()
{
    return mTerminalTask->getFuture();
}

}
}