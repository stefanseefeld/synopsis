#define CAT(a, b) CAT_D(a, b)
#define CAT_D(a, b) a ## b

#define AB(x, y) CAT(x, y)

// There should be a variable XY here
int
CAT(A, B)(X, Y)
;
