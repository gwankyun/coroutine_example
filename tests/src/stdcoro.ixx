export module stdcoro;

import std;

export namespace coro
{
    /// 类型擦除的协程句柄，只能 resume/destroy/done，不能访问 promise
    using handle = std::coroutine_handle<>;

    /// fire-and-forget 協程返回對象，僅作為啟動協程的「令牌」，調用者啓動後即失去控制權
    /*
    struct detached_task
    {
        struct promise_type
        {
            detached_task get_return_object()
            {
                return {}; // 不持有 coroutine_handle，調用者無法控制協程
            }
            std::suspend_never initial_suspend()
            {
                return {}; // 協程立即執行，不需外部 .resume() 啟動
            }
            std::suspend_never final_suspend() noexcept
            {
                return {}; // 協程結束自動銷燬幀，無泄漏但也無法事後訪問
            }
            void unhandled_exception()
            {
                std::terminate(); // 異常直接崩潰，無恢復機制
            }
            void return_void() {}
        };
    };
    */

    /// eager 協程返回對象：協程立即執行，調用者可 detach 放棄所有權或持有至析構
    ///
    /// 生命周期規則：
    ///   - 默認 owned：析構時 destroy() 銷燬幀，協程被強行終止
    ///   - detach() 後：協程結束時 final_suspend 不掛起，幀自動銷燬（fire-and-forget）
    ///
    /// 異常處理：
    ///   - 默認 unhandled_exception() → std::terminate()
    ///   - set_on_error() 可註冊自定義異常回調，避免 terminate
    struct detached_task
    {
        struct promise_type
        {
            detached_task get_return_object()
            {
                return detached_task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            /// suspend_never：協程創建後立即執行，不需外部 resume 啟動
            std::suspend_never initial_suspend()
            {
                return {};
            }
            /// 動態掛起：detached 為 true 時不掛起（幀自動銷燬），false 時掛起（等 destroy）
            auto final_suspend() noexcept
            {
                struct final_awaiter
                {
                    bool detached;
                    /// 返回 true → 不掛起，協程幀自動銷燬（detached 模式）
                    /// 返回 false → 掛起，等待外部 destroy()（owned 模式）
                    bool await_ready() noexcept
                    {
                        return detached;
                    }
                    void await_suspend(handle) noexcept {}
                    void await_resume() noexcept {}
                };
                return final_awaiter{detached};
            }
            /// 異常處理：有 on_error 回調則調用，否則 terminate
            void unhandled_exception()
            {
                if (on_error)
                    on_error(std::current_exception());
                else
                    std::terminate();
            }
            void return_void() {}

            bool detached = false;                            ///< 是否已 detach，控制 final_suspend 行為
            std::function<void(std::exception_ptr)> on_error; ///< 自定義異常回調
        };

        explicit detached_task(handle _h) : h(_h) {}
        detached_task() = default;
        detached_task(detached_task&& o) noexcept : h(std::exchange(o.h, nullptr)) {}
        detached_task(const detached_task&) = delete;
        detached_task& operator=(const detached_task&) = delete;
        /// 析構時若仍持有 handle，destroy() 銷燬協程幀（強行終止未完成的協程）
        ~detached_task()
        {
            if (h)
                h.destroy();
        }

        /// 查詢協程是否已執行到終點（co_return 後）
        bool done() const
        {
            return h.done();
        }

        /// 放棄所有權：協程結束時自清理，調用者不再負責銷燬幀
        /// 調用後 h 置空，析構不會 destroy()
        void detach()
        {
            if (h)
            {
                auto& p = get_promise();
                p.detached = true;
                h = nullptr;
            }
        }

        /// 註冊異常回調，替代默認的 std::terminate()
        void set_on_error(std::function<void(std::exception_ptr)> fn)
        {
            if (h)
            {
                auto& p = get_promise();
                p.on_error = std::move(fn);
            }
        }

      private:
        handle h{}; ///< 協程句柄，空表示已 detach 或默認構造

        /// 通過 from_address 還原為 coroutine_handle<promise_type> 以訪問 promise
        promise_type& get_promise()
        {
            return std::coroutine_handle<promise_type>::from_address(h.address()).promise();
        }
    };

    /// lazy 協程返回對象：協程創建後掛起在起點，調用者通過 resume() 驅動執行
    ///
    /// 生命周期規則：
    ///   - initial_suspend 返回 suspend_always：協程不自動執行，需外部 resume()
    ///   - final_suspend 返回 suspend_always：協程結束後幀保留，析構時 destroy()
    ///   - 調用者必須保證 task 存活到協程 done()，否則析構強行銷燬未完成的幀
    ///
    /// 異常處理：
    ///   - unhandled_exception() 捕獲異常存入 ex
    ///   - resume() 後檢查 ex，有異常則 rethrow 給調用者
    struct task
    {
        struct promise_type
        {
            std::exception_ptr ex; ///< 暫存協程內拋出的異常
            task get_return_object()
            {
                return task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            /// suspend_always：協程創建後停在起點，等待 resume() 啟動
            std::suspend_always initial_suspend()
            {
                return {};
            }
            /// suspend_always：協程 co_return 後掛起，幀保留，等析構 destroy()
            /// 保證調用者可在 done() 後調 get_result() 取結果
            std::suspend_always final_suspend() noexcept
            {
                return {};
            }
            /// 捕獲異常存入 ex，不 terminate，由 resume() 時 rethrow
            void unhandled_exception()
            {
                ex = std::current_exception();
            }
            void return_void() {}
        };
        task() = default;
        explicit task(handle _h) : h(_h) {}
        task(task&& o) noexcept : h(std::exchange(o.h, nullptr)) {}
        task(const task&) = delete;
        task& operator=(const task&) = delete;
        /// 析構時若仍持有 handle，destroy() 銷燬協程幀
        ~task()
        {
            if (h)
                h.destroy();
        }
        /// 恢復協程執行，從 co_await 掛起點繼續
        /// 恢復後檢查 promise.ex，有異常則 rethrow 給調用者
        void resume()
        {
            h.resume();
            auto& p = get_promise();
            if (p.ex)
                std::rethrow_exception(p.ex);
        }
        /// 查詢協程是否已執行到終點（co_return 後）
        bool done() const
        {
            return h.done();
        }

      private:
        handle h{}; ///< 協程句柄

        /// 通過 from_address 還原為 coroutine_handle<promise_type> 以訪問 promise
        promise_type& get_promise()
        {
            return std::coroutine_handle<promise_type>::from_address(h.address()).promise();
        }
    };

    /// 橋接協程與異步回調的核心機制：
    /// co_await awaiter{fn} → 協程掛起 → fn(調用者handle) 執行 →
    /// fn 內發起異步操作並註冊回調 → 回調完成時 .resume() → 協程從 co_await 後恢復
    struct awaiter
    {
        using Fn = std::function<void(handle)>;
        explicit awaiter(Fn _f) : f(_f) {}
        /// 始終返回 false，強制掛起協程
        bool await_ready()
        {
            return false;
        }
        /// 掛起時調用 f(_h)，_h 為當前協程的 handle，由回調負責在異步完成時 resume
        void await_suspend(handle _h)
        {
            f(_h);
        }
        /// 無返回值，co_await 表達式為 void
        void await_resume() {}

      private:
        Fn f; ///< 業務回調，接收協程 handle
    };

    /// @brief 掛起當前協程，將協程 handle 傳出到 _coroutine 引用，並執行業務回調
    /// @param _coroutine 輸出參數，掛起時被賦值為當前協程的 handle
    /// @param _f 業務回調，在裏面發起異步操作，完成時調 _coroutine.resume() 恢復
    /// @return 可供 co_await 的 awaiter
    awaiter suspend_async(handle _coroutine, std::function<void()> _f)
    {
        auto fn = [&, _f](handle _coro) {
            _coroutine = _coro; // 把協程句柄傳出去
            _f();               // 業務回調
        };
        return awaiter{fn};
    }

    /// @brief 掛起當前協程，將協程 handle 傳給業務回調
    /// @param _f 業務回調，接收 handle，在裏面發起異步操作，完成時調 handle.resume() 恢復
    /// @return 可供 co_await 的 awaiter
    awaiter suspend_async(std::function<void(handle)> _f)
    {
        auto fn = [&, _f](handle _coro) {
            _f(_coro); // 業務回調
        };
        return awaiter{fn};
    }
} // namespace coro
