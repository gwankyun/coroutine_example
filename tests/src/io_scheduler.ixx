module;

#include <boost/asio.hpp>

export module io_scheduler;

import std;

export {
    /// 協程容器類型別名，按 ID 索引協程對象。
    /// 每個協程以 unique_ptr 存儲，協程完成時 operator bool() 返回 false。
    template <typename Coro>
    using coro_map = std::unordered_map<int, std::unique_ptr<Coro>>;

    /// 事件驅動調度器：循環運行 io_context，檢查就緒隊列並喚醒對應協程。
    ///
    /// 協程通過回調將自身 ID 推入 awake_cont 請求喚醒，
    /// 調度器從 awake_cont 取出 ID，查找並 resume 對應協程，
    /// 直到所有協程完成（coro_cont 爲空）。
    ///
    /// @param io_ctx     io_context 實例，驅動異步事件
    /// @param coro_cont  協程容器，存儲 ID → unique_ptr<Coro> 映射
    /// @param awake_cont 就緒隊列，存儲等待喚醒的協程 ID
    /// @param resume     喚醒函數，接收 unique_ptr<Coro>&，
    ///                   調用者需根據協程類型提供正確的 resume 操作
    template <typename Coro, typename ResumeFn>
    void run_scheduler(boost::asio::io_context& io_ctx, coro_map<Coro>& coro_cont, std::queue<int>& awake_cont,
                       ResumeFn resume)
    {
        while (!coro_cont.empty())
        {
            // 阻塞等待異步事件完成，回調會將就緒協程 ID 推入 awake_cont
            io_ctx.run();

            if (!awake_cont.empty())
            {
                auto id = awake_cont.front();
                awake_cont.pop();

                auto iter = coro_cont.find(id);
                if (iter != coro_cont.end())
                {
                    // 協程已完成（operator bool() 返回 false），移除並跳過
                    if (!*iter->second)
                    {
                        coro_cont.erase(id);
                        continue;
                    }
                    // 喚醒協程，讓協程從暫停點繼續執行
                    resume(*(iter->second));
                }
            }
        }
    }
}
