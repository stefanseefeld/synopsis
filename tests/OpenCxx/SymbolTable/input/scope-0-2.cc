int j = 24;
int main()
{
  int i = j, j;
  j = 42;
  if (42 == j && i == ::j)
  {
    return 0;
  }
  else 
  {
    return 1;
  }
}
