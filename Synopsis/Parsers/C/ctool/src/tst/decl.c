/*
int *( x() );
int (*y)();
void **z;
int (*a)[];
int *( b[] );
*/

int *x[]();
int (*x)[]();
int *(x[])();

char **m()[](int);
char ( *((*k)(char)) [])(int);
char ( *(*j(char))[]) (int);
int *(*(*(*x)(char))[10])();

int a[1][2][3];
