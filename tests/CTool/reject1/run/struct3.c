struct st { int a ; } x ;

/********** accepted ******************/
int f2(struct st y) {

  struct st ;
  
  struct st2 { struct st * c ; } z ;
  
  void g1 (struct st2 *) ;
  int g2 (struct st *, int) ;
  
  g1(&z) ;
    
  return g2(z.c, x.a + y.a) ;
}
/*********** accepted *****************/
int f3(struct st y) {

  struct st ;
  
  struct st { int c ; } z ;
  
  z.c = x.a + y.a ;
  return z.c ;
}
/*********** accepted *****************/
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
/********** rejected ******************/
int f5(struct st y) {

  struct st ;
  
  struct st2 { struct st * c ; } z ;
  
  void ggg1 (struct st2 *) ;
  int ggg2 (struct st *, int) ;
  
  ggg1(&z) ;
  
  x.a = z.c->a ; /* error: improper member use: a */
    
  return ggg2(z.c, x.a + y.a) ;
}
/***********          *****************/
