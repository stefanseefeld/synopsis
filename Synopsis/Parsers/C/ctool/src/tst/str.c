
struct foo
{
    int  a, b, c:2;
    double x[25];
    union {
        int a;
        float b;
    } uFoo;
    int    q[0];
} fooIt;

int main()
{
    fooIt.b = 2;

    return 0;
}
