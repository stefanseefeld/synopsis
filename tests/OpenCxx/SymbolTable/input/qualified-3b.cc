class X {};
class C 
{
  class X {};
  static const int number = 50;
  static X arr[number];
};
X C::arr[number];   // ill-formed
                    // equivalent to: ::X C::arr[C::number];
