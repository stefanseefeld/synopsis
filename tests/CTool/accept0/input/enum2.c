
#include <stdio.h>

enum a { false, true, maybe };

enum b {
    high = true,
    low  = false + 1
};

int main()
{
    printf("high is: %d\n", (int) high);
    printf("low is: %d\n", (int) low);
    return 0;
}
