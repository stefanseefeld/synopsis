// Test function templates

// Test template arg
template <class A>
int func1(A a) { return 0; };

// Test template return
template <class A>
A func2(int i) { return 0; };

// Test template arg and return
template <class A>
A func2(A a) { return 0; };

// Test template arg and return w/ different types
template <class A, class B>
B func2(A a) { return 0; };

// Test template arg and return w/ different types. Function declaration
template <class A, class B>
B func2(A a);
