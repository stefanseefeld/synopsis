
struct foo {
    int    c,d,e;

    union myunion {
        int a;
        double d;
    } rook;

} m;


int main()
{
  int r;

  m.rook.a = 5;

  return 0;
}


