int IDENT=0, STRING=1, RNUM=2;

int hello;

enum hiya {
    FALSE = 0,
    TRUE = 1,
    MAYBE,
    DONT_KNOW
};

typedef    int INT;

struct x
{
    int a;
    int b;
} m;

void foo( int bar, int baz )
{
    goto foo2;
    return bar;

  foo2:

    return baz;
}

int main( int argc, char** argv )
{
    int   done=MAYBE;

    restart:

        return(0);
}

/*  ###############################################################  */
