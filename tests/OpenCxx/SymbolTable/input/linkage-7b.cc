namespace X 
{
  void p()
  {
    q();                 // error: q not yet declared
    extern void q();     // q is a member of namespace X
  }
  void middle()
  {
    // q();              // error: q not yet declared
  }
  void q() { /* ... */ } // definition of X::q
}

void q() { /* ... */ }   // some other, unrelated q
