// According to spec, the ## operator must produce a valid PP token.
// Unfortunately, no compilers known to man enforce this rule except UCPP, so
// this test is to make sure it is 'fixed' to ignore the error.
#define CAT(a,b) a ## b

class Foo {};
void CAT(operator,+) (const Foo&, const Foo&) {}
void operator CAT(+,=) (const Foo&, const Foo&) {}
