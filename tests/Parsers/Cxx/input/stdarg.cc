#include <cstdarg>

void function(char *s, ...)
{
  va_list args;
  va_start(args, s);
  for(;;)
    char *p = va_arg(args, char *);
  va_end(args);
}
