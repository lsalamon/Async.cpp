#include "async_cpp/async/AsyncResult.h"
#include "async_cpp/async/Parallel.h"

#include "async_cpp/tasks/AsioManager.h"

#include <chrono>

#pragma warning(disable:4251)
#include <gtest/gtest.h>

using namespace async_cpp;
using namespace async_cpp::async;

TEST(PARALLEL_TEST, BASIC)
{
    auto manager = std::make_shared<tasks::AsioManager>(5);
    std::atomic<int> runCount(1);
    std::vector<int> taskRunOrder(6, 0);

    Parallel<bool>::operation_t opsArray[] = {
        [&taskRunOrder, &runCount](Parallel<bool>::callback_t cb)->void { 
            taskRunOrder[0] = runCount.fetch_add(1); 
            cb(AsyncResult());
        }, 
        [&taskRunOrder, &runCount](Parallel<bool>::callback_t cb)->void { 
            taskRunOrder[1] = runCount.fetch_add(1); 
            cb(AsyncResult());
        },
        [&taskRunOrder, &runCount](Parallel<bool>::callback_t cb)->void { 
            taskRunOrder[2] = runCount.fetch_add(1); 
            cb(AsyncResult());
        },
        [&taskRunOrder, &runCount](Parallel<bool>::callback_t cb)->void { 
            taskRunOrder[3] = runCount.fetch_add(1); 
            cb(AsyncResult());
        },
        [&taskRunOrder, &runCount](Parallel<bool>::callback_t cb)->void { 
            taskRunOrder[4] = runCount.fetch_add(1); 
            cb(AsyncResult());
        }
    };

    AsyncResult result;
    ASSERT_NO_THROW(result = Parallel<bool>(manager, opsArray, 5).then(
        [&taskRunOrder, &runCount](std::exception_ptr ex, std::vector<bool>&&)->void {
            if(ex)
            {
                std::rethrow_exception(ex);
            }
            taskRunOrder[5] = runCount;
        } ) );

    EXPECT_NO_THROW(result.check());

    EXPECT_LE(5, runCount);

    manager->shutdown();
}

TEST(PARALLEL_TEST, INTERRUPT)
{
    auto manager(std::make_shared<tasks::AsioManager>(5));
    std::atomic<int> runCount(1);
    std::vector<int> taskRunOrder(6, 0);

    Parallel<bool>::operation_t opsArray[] = {
        [&taskRunOrder, &runCount](Parallel<bool>::callback_t cb)->void { 
            taskRunOrder[0] = runCount.fetch_add(1); 
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            cb(AsyncResult());
        }, 
        [&taskRunOrder, &runCount](Parallel<bool>::callback_t cb)->void { 
            taskRunOrder[1] = runCount.fetch_add(1); 
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            cb(AsyncResult());
        },
        [&taskRunOrder, &runCount](Parallel<bool>::callback_t cb)->void { 
            taskRunOrder[2] = runCount.fetch_add(1); 
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            cb(AsyncResult());
        },
        [&taskRunOrder, &runCount](Parallel<bool>::callback_t cb)->void { 
            taskRunOrder[3] = runCount.fetch_add(1); 
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            cb(AsyncResult());
        },
        [&taskRunOrder, &runCount](Parallel<bool>::callback_t cb)->void { 
            taskRunOrder[4] = runCount.fetch_add(1); 
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            cb(AsyncResult());
        }
    };

    AsyncResult result;
    EXPECT_NO_THROW(result = Parallel<bool>(manager, opsArray, 5).then(
        [&taskRunOrder, &runCount](std::exception_ptr ex, std::vector<bool>&&)->void{
            if(ex) std::rethrow_exception(ex);
            taskRunOrder[5] = runCount;
        } ) );

    manager->shutdown();
    EXPECT_THROW(result.check(), std::runtime_error);
}

TEST(PARALLEL_TEST, TIMING)
{
    typedef std::chrono::high_resolution_clock::time_point data_t;
    auto manager(std::make_shared<tasks::AsioManager>(5));

    auto func = [](Parallel<data_t>::callback_t cb)->void { 
        cb(std::chrono::high_resolution_clock::now());
    };

    auto ops = std::vector<Parallel<data_t>::operation_t>(5, func);

    Parallel<data_t> parallel(manager, ops);
    AsyncResult result;
    auto start = std::chrono::high_resolution_clock::now();
    auto maxDur = std::chrono::high_resolution_clock::duration::min();
    result = parallel.then([&start, &maxDur](std::exception_ptr ex, std::vector<data_t>&& results)->void {
        if(ex)
        {
            std::rethrow_exception(ex);
        }
        for(auto& taskFinish : results)
        {
            maxDur = std::max(maxDur, taskFinish - start);
        }
    } );
    ASSERT_NO_THROW(result.check());
    auto totalDur = std::chrono::high_resolution_clock::now() - start;

    EXPECT_GE(totalDur, maxDur);

    manager->shutdown();
}