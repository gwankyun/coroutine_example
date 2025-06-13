module;
#include <catch2/catch_test_macros.hpp>

export module catch2.modern;
export import catch2.base;
import std;

class FunctionTestInvokerAsMethod : public Catch::ITestInvoker
{
    std::function<void()> m_testCallable;

  public:
    FunctionTestInvokerAsMethod(std::function<void()> testCallable) noexcept : m_testCallable(testCallable)
    {
    }

    void invoke() const override
    {
        m_testCallable();
    }
};

namespace Catch
{
    Catch::Detail::unique_ptr<Catch::ITestInvoker> makeTestInvoker(std::function<void()> _fn)
    {
        using Catch::Detail::unique_ptr;
        using Invoker = FunctionTestInvokerAsMethod;
        unique_ptr<Invoker> functionInvoker(new Invoker(_fn));
        return functionInvoker;
    }
} // namespace Catch

export namespace Catch
{
    void test_case(
        std::string _name, std::string _tags, std::function<void()> _fn, std::source_location _location = Catch::current()
    )
    {
        CATCH_INTERNAL_START_WARNINGS_SUPPRESSION
        const Catch::AutoReg autoRegistrar(
            Catch::makeTestInvoker(_fn), Catch::lineInfo(_location), Catch::StringRef(), Catch::NameAndTags{_name, _tags}
        );
        CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION
    }

    void session(std::string _message, std::function<void()> _fn, std::source_location _location = Catch::current())
    {
        if (Catch::Section const& catch_internal_Section = Catch::Section(Catch::lineInfo(_location), _message))
        {
            _fn();
        }
    }
} // namespace Catch
