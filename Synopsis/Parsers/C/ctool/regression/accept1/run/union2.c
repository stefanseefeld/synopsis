union st { int a ; } x ;

/****************************/
int f0(union st y) {

  union st z;
  
  z.a = x.a + y.a ;
  
  return z.a ;
}
/********** accepted ******************/
int f1(union st y) {

  union st ;
  
  union st *z ;
  
  return y.a ;
}
/****************************/
int f2(union st y) {

  union st ;
  
  union st2 { union st * c ; } z ;
  
  void g1 (union st2 *) ;
  int g2 (union st *, int) ;
  
  g1(&z) ;
    
  return g2(z.c, x.a + y.a) ;
}
/****************************/
int f3(union st y) {

  union st ;
  
  union st { int c ; } z ;
  
  z.c = x.a + y.a ;
  return z.c ;
}
/****************************/
int f4(union st y) {

  union st ;
  
  union st2 { union st * c ; } z ;
  
  union st { int b ; };
  
  void gg1 (union st2 *) ;
  int gg2 (union st *, int) ;
  
  gg1(&z) ;
  
  x.a = z.c->b ;
  
  return gg2(z.c, x.a + y.a) ;
}
/****************************/
