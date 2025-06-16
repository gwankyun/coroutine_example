module;
#include <boost/scope/defer.hpp>
#include <boost/scope/scope_exit.hpp>

export module boost.scope;

export namespace boost::scope
{
    using boost::scope::defer_guard;
    using boost::scope::make_scope_exit;
}
