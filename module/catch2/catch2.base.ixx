module;
#include <catch2/catch_test_macros.hpp>

export module catch2.base;
export import catch2.session;
import std;

namespace detail
{
    auto lineInfo(std::source_location _location)
    {
        return ::Catch::SourceLineInfo(_location.file_name(), static_cast<std::size_t>(_location.line()));
    }
 
    constexpr std::source_location current(std::source_location _location = std::source_location::current())
    {
        return _location;
    }
} // namespace detail

using detail::lineInfo;

void test(
    bool _expression, std::string _message, Catch::StringRef _macroName,
    Catch::ResultDisposition::Flags _resultDisposition, const std::source_location _location = detail::current()
)
{
    do
    {
        Catch::AssertionHandler catchAssertionHandler(
            _macroName, lineInfo(_location), Catch::StringRef(_message), _resultDisposition
        );
        try
        {
            catchAssertionHandler.handleExpr(Catch::Decomposer() <= _expression);
        }
        catch (...)
        {
            (catchAssertionHandler).handleUnexpectedInflightException();
        }
        catchAssertionHandler.complete();
    } while ((void)0, (false) && static_cast<const bool&>(!!(_expression)));
}

export namespace Catch
{
    using detail::current;
    using detail::lineInfo;

    void require(bool _expression, std::string _message = "", const std::source_location _location = current())
    {
        test(_expression, _message, "REQUIRE"_catch_sr, Catch::ResultDisposition::Normal, _location);
    }

    void check(bool _expression, std::string _message = "", const std::source_location _location = current())
    {
        test(_expression, _message, "CHECK"_catch_sr, Catch::ResultDisposition::ContinueOnFailure, _location);
    }
} // namespace Catch
