

int static short unsigned i;

unsigned int    *a;
unsigned int    **b;

int blk[5];

int c, *d, e;

int ****f;
int ****g[5];

int alpha(char a,char b);

int *h(char);
int (*hp)(char);

int (*pj)(char);
int (*pj)(char a,int b,float *c);

int *(*(*(*x)(char))[10])();

int o=5;
int p=5, q=7, r=9;
int s[5] = { 1, 2, 3 };

int t[3][3] = { { 'X', 'O', '_' },
                { 'O', 'X', '_' },
                { 'O', '_', 'X' } };

enum foo1 { a1, b1, c1 };
enum foo2 { a2=2, b2, c2 };
enum foo3 { a3=2, b3=1, c3=3 };

struct foo4 { int a;  float b; };
union foo5 { int a;  float b; };

struct foo6 { int a, b;  float c; };
union foo7 { int a, b;  float c; };
struct foo8 { int a:5;  float c; };
struct foo9 { unsigned int a:5, b:7;  float c; };

struct foo6 b6[3];
union foo7 b7[3];
enum foo4 b4[3];

typedef int foo0;

typedef struct fooz {
    int a;
    int b:3;
} fooz, *pFooz;

typedef int    (*PFI)(int    val);

