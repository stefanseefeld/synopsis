struct st { int a ; } x ;

/********** accepted ******************/
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
/********** rejected ******************/
int f2(struct st y) {

  struct st ;
  
  struct st z ; /* error: incomplete struct/union/enum st: z */
  
  return y.a ;
}
/**********          ******************/
