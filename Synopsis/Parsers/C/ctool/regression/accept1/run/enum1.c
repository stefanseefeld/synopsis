enum       {a0} ;
enum       {a1,} ;
enum enum1 {a11, a12} ;
enum enum1 ;
enum enum2 {a2=a0} ;
enum enum3 {a31=1, a32=a31+1, a33=a32<<1}; 
typedef enum enum4 {a4}  enum4, *p_enum4, t_enum4[4];
typedef enum       {a51} enum5, *p_enum5, t_enum5[4];
enum enum5 {a52};

enum enum6 ;
enum enum6 {a6};


/* Enumeration variable declaration */
enum enum1 v11;
extern const enum enum1 *p12;
enum enum2 v21=a2, v22;
enum enum3 *p31, v32=a0, *p33=&v32;

enum4 v4;
p_enum4 p4=&v4;
t_enum4 t4={a0, a2, a4};

static enum5 v5;
static p_enum5 p5=&v5;
static t_enum5 t5={a0, a2, a4, a51};


/* Enumeration type + variable declaration */
static enum enum7 {a7} v71;
static enum enum8 {a8} v81=a8, *p82=&v81;
