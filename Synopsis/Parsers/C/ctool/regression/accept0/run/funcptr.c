
 /* PFI - Pointer to function returning int */
typedef int    (*PFI)(int val);

int myK(int in)
{
  return in;
}

int main()
{
    int    x,y;
    PFI    k;

    k = myK;

    x = (k)(2);
    y = (myK)(2);

    return x + y;
}
