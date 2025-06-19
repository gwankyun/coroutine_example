module;
#include <boost/intrusive/list.hpp>

export module boost.intrusive;
export import boost.intrusive.detail;
export import boost.intrusive.link_mode;

export namespace boost::intrusive
{
    using boost::intrusive::list_base_hook;
    using boost::intrusive::list;
}
