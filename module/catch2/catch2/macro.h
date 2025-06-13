#pragma once

#ifndef CATCH_STRING
#  define CATCH_STRING(_str) #_str
#else
#  error "CATCH_STRING has defined."
#endif // !CATCH_STRING

#ifndef CATCH_EXPRESSION
#  define CATCH_EXPRESSION(_expression) _expression, CATCH_STRING(_expression), Catch::current()
#else
#  error "CATCH_EXPRESSION has defined."
#endif // !CATCH_EXPRESSION

#ifndef CATCH_ON
#  define CATCH_ON(_expression) (CATCH_EXPRESSION(_expression))
#endif // !CATCH_ON
