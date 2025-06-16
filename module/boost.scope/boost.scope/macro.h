#pragma once

#ifdef _MSC_VER
#  define BOOST_MSVC _MSC_VER
#endif

#if defined(BOOST_MSVC)
#  define BOOST_SCOPE_DETAIL_UNIQUE_VAR_TAG __COUNTER__
#else
#  define BOOST_SCOPE_DETAIL_UNIQUE_VAR_TAG __LINE__
#endif

#define BOOST_JOIN(X, Y) BOOST_DO_JOIN(X, Y)
#define BOOST_DO_JOIN(X, Y) BOOST_DO_JOIN2(X, Y)
#define BOOST_DO_JOIN2(X, Y) X##Y

#define BOOST_SCOPE_DEFER \
    boost::scope::defer_guard BOOST_JOIN(_boost_defer_guard_, BOOST_SCOPE_DETAIL_UNIQUE_VAR_TAG) =
