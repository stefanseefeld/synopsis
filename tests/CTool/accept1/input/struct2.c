struct st { int a ; } x ;

/****************************/
int f0(struct st y) {

  struct st z;
  
  z.a = x.a + y.a ;
  
  return z.a ;
}
/********** accepted ******************/
int f1(struct st y) {

  struct st ;
  
  struct st *z ;
  
  return y.a ;
}
/****************************/
int f2(struct st y) {

  struct st ;
  
  struct st2 { struct st * c ; } z ;
  
  void g1 (struct st2 *) ;
  int g2 (struct st *, int) ;
  
  g1(&z) ;
    
  return g2(z.c, x.a + y.a) ;
}
/****************************/
int f3(struct st y) {

  struct st ;
  
  struct st { int c ; } z ;
  
  z.c = x.a + y.a ;
  return z.c ;
}
/****************************/
int f4(struct st y) {

  struct st ;
  
  struct st2 { struct st * c ; } z ;
  
  struct st { int b ; };
  
  void gg1 (struct st2 *) ;
  int gg2 (struct st *, int) ;
  
  gg1(&z) ;
  
  x.a = z.c->b ;
  
  return gg2(z.c, x.a + y.a) ;
}
/****************************/
