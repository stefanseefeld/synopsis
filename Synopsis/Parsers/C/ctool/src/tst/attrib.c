
int m    __attribute__ ( ( aligned (8) ) ) = 5;

typedef int myInt   __attribute__ ( ( __mode__ (SI) ) );

extern int *myLocation(void) __attribute__ ((__const__));
extern int *myFoo(int x) __attribute__ ((__noreturn__));

extern int
my_printf (void *my_obj, const char* my_fmt, ... )
        __attribute__ ((__format__ (printf, 2, 3)));

//me_printf (void *my_obj, const char* my_fmt, ... )
//        __attribute__ ((__format__ (printf, 2, 3)));

struct x
{
    int    a  __attribute__ ( ( aligned (16) ) );
    int    b  __attribute__ ( ( packed ) );
} y;
 
