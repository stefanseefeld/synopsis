/////////////////////////////////////////////////////////////
// Code segment must be in a scope to test.                //
/////////////////////////////////////////////////////////////

int main() 
{
  int x = 999;
  {
    int x = x;
    if (x == 999) return 1;
  }
  return 0;
}
