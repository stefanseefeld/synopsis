
int main()
{
    int    a = 5;
    char   b = 'm';

    switch (a)
    {
        case 1 ... 10:
        case 14:
            a++;
            break;
    }

    switch (b)
    {
        case 'A'...'Z':
            b += ('a' - 'A');
            break;

        case 'a'...'z':
            --b;
            break;
    }

    return 0;
}
