// Test function pointers as template arguments

template <class Foo>
struct function { };

// Test template class with function argument
// (not really a template function)
template <class A1, class A2>
struct function<void (A1,A2)> { };

// Test return type fptr
template <class R>
struct function<R (void)> { };

// Test return type fptr
template <class R>
struct function<R (int, int)> { };
