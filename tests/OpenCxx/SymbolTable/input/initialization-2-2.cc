inline double fd() { return 1.0; }
extern double d1;
double d2 = d1;   // unspecified:
                  // may be statically initialized to 0.0 or
                  // dynamically initialized to 1.0
double d1 = fd(); // may be initialized statically to 1.0

