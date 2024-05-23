#include "../include/on_exit.h"

namespace common
{
    OnExit::OnExit(Fn _fn) : fn(_fn)
    {
    }

    OnExit::~OnExit()
    {
        fn();
    }
} // namespace common
