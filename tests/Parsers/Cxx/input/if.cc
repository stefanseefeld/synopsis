// If test
void func()
{
  int x = 0;
  if (x) cout << "Hi";           // test x = "`func()::x"
  if (x == 2) cout << "hi";      // test x = "`func()::x"
  if (!x) cout << "foo";         // test x = "`func()::x"
  if (x) cout << "one"; else cout << "two";
  if (x) {cout << "one"; cout << "two"; } else { cout<<"three"; }
}
