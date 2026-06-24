#pragma once

#ifndef DECLSPEC
#  ifdef _MSC_VER // __declspec是MSVC關鍵字
#    define DECLSPEC(x) __declspec(x)
#  else
#    define DECLSPEC(x)
#  endif
#endif

#ifndef SHARED_EXPORT // 動態庫需要定義
#  define SHARED_API DECLSPEC(dllimport)
#else
#  define SHARED_API DECLSPEC(dllexport)
#endif
