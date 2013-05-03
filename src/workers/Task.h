#pragma once
#include "workers/Platform.h"

#include <atomic>
#include <functional>
#include <future>

namespace async_cpp {
namespace workers {

/**
 * Interface for tasks which will be run by a worker. If a task fails to perform successfully, completion future will be marked as false.
 */
class WORKERS_API Task : public std::enable_shared_from_this<Task> {
public:
    Task();
    virtual ~Task();

    /**
     * Check if this task is complete, returning whether or not the task was completed.
     * @return True if task is complete
     */
    inline bool isComplete();

    /**
     * Determine whether this task was successful. Will block until ready. Use isComplete to check if this call will block
     * @return True if task completed successfully
     */
    inline bool wasSuccessful();

    /**
     * Perform the behavior of this task, invoking a function after the task complete promise is fulfilled.
     * @param afterCompleteFunction Function to invoke after task has fulfilled its promise
     */
    void perform(std::function<void(void)> afterCompleteFunction);

    /**
     * Mark this task as a failure by fulfilling its promise with false.
     */
    void failToPerform();

    /**
     * Reset this task, allowing it to be run on the manager again
     */
    void reset();

protected:
    virtual void performSpecific() = 0;

private:
    std::atomic<bool> mHasFulfilledPromise;
    std::future<bool> mTaskCompleteFuture;
    std::packaged_task<bool(bool, std::function<void(void)>)> mTask;
};

//inline implementations
//------------------------------------------------------------------------------
bool Task::isComplete()
{
#ifdef _MSC_VER //wait_for is broken in VC11 have to use MS specified _Is_ready
    return mTaskCompleteFuture._Is_ready();
#else
    return (std::future_status::ready == mTaskCompleteFuture.wait_for(std::chrono::milliseconds(0)));
#endif
}

//------------------------------------------------------------------------------
bool Task::wasSuccessful()
{
    return mTaskCompleteFuture.get();
}

}
}