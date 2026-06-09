module;

// Boost.Asio：異步 I/O 與定時器
#include <boost/asio.hpp>
// Boost.Fibers：用戶態協程（fiber）庫
#include <boost/fiber/all.hpp>
// 輪轉調度器：讓 fibers 按 FIFO 順序調度，而非默認 LIFO
#include <boost/fiber/algo/round_robin.hpp>
// RAII 資源清理工具
#include <boost/scope/defer.hpp>
// 日誌輸出
#include <spdlog/spdlog.h>

module test.fiber;

import std;

using namespace std::chrono_literals;
namespace asio = boost::asio;
using asio::steady_timer;
using boost::system::error_code;
namespace fibers = boost::fibers;

/**
 * @brief 將 Asio 異步定時器轉換爲同步阻塞等待
 *
 * 通過 std::promise/std::future 將 callback 風格的異步操作
 * 轉換爲同步接口：fiber 調用 future.get() 時掛起自己，
 * 定時器到期後回調設置 promise，fiber 被喚醒繼續執行。
 *
 * @param timer 已配置好超時時間的 steady_timer
 * @return error_code 空表示成功，非空表示錯誤或取消
 */
auto async_wait(steady_timer& timer) -> error_code
{
    std::promise<error_code> promise;
    auto future = promise.get_future();

    // 發起異步等待，回調中設置 promise 的值以喚醒等待的 fiber
    timer.async_wait(
        [promise = std::move(promise)](error_code e) mutable
        {
            promise.set_value(e);
        });

    // fiber 在此掛起，直到回調被調用
    return future.get();
}

/**
 * @brief 測試 Boost.Fiber + Boost.Asio 協同工作
 *
 * 創建 5 次 100ms 的定時器等待，result 最終應爲 "012223"
 * 展示如何用 fiber 以同步寫法實現異步等待。
 */
std::string test_fiber()
{
    // Asio 事件循環上下文
    asio::io_context context;

    // 工作守護：防止 io_context 在沒有 pending 任務時提前退出
    auto work_guard = asio::make_work_guard(context);

    // 在獨立線程中運行事件循環，不阻塞當前線程的 fiber 調度
    std::thread io_thread([&]{ context.run(); });

    // 設置當前線程的 fiber 調度器爲輪轉調度（round-robin）
    // 默認是 LIFO（後進先出），round_robin 更適合需要公平調度的場景
    fibers::use_scheduling_algorithm<fibers::algo::round_robin>();

    // 結果字符串，初始爲 "0"
    std::string result = "0";
    auto time = 100ms;

    // 創建一個 fiber（用戶態協程）執行串行等待邏輯
    fibers::fiber fiber([&]
    {
        {
            // 第 1 次等待：100ms
            steady_timer accept(context, time);
            auto e = async_wait(accept);
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }
            result += "1";
        }

        // 第 2~4 次等待：循環 3 次，每次 100ms
        for (int i = 0; i < 3; ++i)
        {
            steady_timer read(context, time);
            auto e = async_wait(read);
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }
            result += "2";
        }

        // 第 5 次等待：100ms
        {
            steady_timer write(context, time);
            auto e = async_wait(write);
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }
            result += "3";
        }
    });

    // 阻塞當前線程等待 fiber 執行完畢
    fiber.join();

    // 停止事件循環（此時所有工作已完成）
    context.stop();

    // 等待 io 線程退出
    io_thread.join();

    return result;
}
