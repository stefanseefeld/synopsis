/* a value */
int value;
/* the String type */
typedef char *String;
/* a function */
void function(String);

struct Struct
{
  union Union
  {
    char four_bytes[4];
    double a_double;
  };
  int one;
  double two;
};

enum { ONE, TWO, THREE};
