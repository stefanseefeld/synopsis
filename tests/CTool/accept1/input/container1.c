enum e2;

struct s
{ int  s1 ;
  char s2, s3 ;
  
  double ;
  
  struct  
  { int     ss1 ;
    enum e2 ss2, ss3 ;
  } ;
  
  union 
  { int       su1 ;
    struct s *su2, *su3 ;
    ;
  } s4, s5 ;
  
} v;

enum    {v_0 = sizeof (struct s) } ;
enum    {v_11, 
         v_12 = sizeof (union {int su1 ; struct s *su2, *su3 ; } ) } ;
enum e2 {v2} ;
enum e3 {v31, v32} ;


