
#include <stddef.h>

int
main()
{
    wchar_t    b = L'H';
    wchar_t   *m = L"hello";

    char a = '\003';
    char c = '\201';

    m[0] = b;

    return 0;
}
