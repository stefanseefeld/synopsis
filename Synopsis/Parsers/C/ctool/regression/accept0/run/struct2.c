
struct x {
    int a;
};

struct x *y;

struct y {
    struct x *y;
};

struct z {
    struct x a, b, c;
};

