
class string 
{
public:
  string() {}
  string(const string &) {}
  string & operator=(const string &) 
  { 
    return *this; 
  }
};
struct C 
{
  string s;
  C(): s() { }
  C(const C& x): s(x.s) { }
  C& operator=(const C& x) { s = x.s; return *this; }
  ~C() { }
};
