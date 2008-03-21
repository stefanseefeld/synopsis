#ifndef math_h_
#define math_h_

/* Support for various different standard error handling behaviors.  */
typedef enum
{
  IEEE = -1, /* According to IEEE 754/IEEE 854.  */
  SVID,	     /* According to System V, release 4.  */
  XOPEN,     /* Nowadays also Unix98.  */
  POSIX,
  ISOC       /* Actually this is ISO C99.  */
} LIB_VERSION_TYPE;

/* This variable can be changed at run-time to any of the values above to
   affect floating point error handling behavior (it may also be necessary
   to change the hardware FPU exception settings).  */
extern LIB_VERSION_TYPE LIB_VERSION;

struct exception
{
  int type;
  char *name;
  double arg1;
  double arg2;
  double retval;
};

extern int matherr (struct exception *exc);

/* @group Trigonometric functions { */

/* Arc cosine of X.  */
double acos(double x);
/* Arc sine of X.  */
double asin(double x);
/* Arc tangent of X.  */
double atan(double x);
/* Arc tangent of Y/X.  */
double atan2(double y, double x);

/* Cosine of X.  */
double cos(double x);
/* Sine of X.  */
double sin(double x);
/* Tangent of X.  */
double tan(double x);

/* } */
/* @group Hyperbolic functions {  */

/* Hyperbolic cosine of X.  */
double cosh(double x);
/* Hyperbolic sine of X.  */
double sinh(double x);
/* Hyperbolic tangent of X.  */
double tanh(double x);

/* Hyperbolic arc cosine of X.  */
double acosh(double x);
/* Hyperbolic arc sine of X.  */
double asinh(double x);
/* Hyperbolic arc tangent of X.  */
double atanh(double x);

/* } */
/* @group Exponential and logarithmic functions {  */

/* Exponential function of X.  */
double exp(double x);
/* Break VALUE into a normalized fraction and an integral power of 2.  */
double frexp(double x, int *e);

/* X times (two to the EXP power).  */
double ldexp(double x, int e);

/* Natural logarithm of X.  */
double log(double x);

/* Base-ten logarithm of X.  */
double log10(double x);

/* Break VALUE into integral and fractional parts.  */
double modf(double x, double *iptr);

/* A function missing in all standards: compute exponent to base ten.  */
double exp10(double x);
/* Another name occasionally used.  */
double pow10(double x);

/* } */
/* @group Power functions { */

/* Return X to the Y power.  */
double pow(double x, double y);

/* Return the square root of X.  */
double sqrt(double x);

/* Return `sqrt(X*X + Y*Y)'.  */
double hypot(double x, double y);

/* Return the cube root of X.  */
double cbrt(double x);


/* } */
/* @group Nearest integer, absolute value, and remainder functions { */

/* Smallest integral value not less than X.  */
double ceil(double x);

/* Absolute value of X.  */
double fabs(double x);

/* Largest integer not greater than X.  */
double floor(double x);

/* Floating-point modulo remainder of X/Y.  */
double fmod(double x, double y);

/* Return 0 if VALUE is finite or NaN, +1 if it
   is +Infinity, -1 if it is -Infinity.  */
int isinf(double x);

/* Return nonzero if VALUE is finite and not NaN.  */
int isfinite(double x);

/* Return nonzero if VALUE is not a number.  */
int isnan(double x);

/* } */

#endif
