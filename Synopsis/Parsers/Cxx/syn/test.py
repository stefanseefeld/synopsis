#!/usr/bin/python
# A test-suite for the C++ syntax highlighter

import os, types, sys

global gdb
gdb = None

gcc_include2 = "-I/usr/include/g++-3/ -I/usr/lib/gcc-lib/i386-linux/2.95.4/include "
gcc_include3 = "-I/usr/include/g++-v3/ -I/usr/include/g++-v3/i386-linux/ -I/usr/lib/gcc-lib/i386-linux/3.0.1/include "
gcc_include = gcc_include2
python_include = "-DPYTHON_INCLUDE='<python2.1/Python.h>' "


html_top = '<html><link rel="stylesheet" href="/home/chalky/src/Synopsis/demo/html.css"><body>'
class Test:
    flags = ""
    def run(self):
	"Return false if test failed"
	test_text = self.__class__.test
	test_base = self.__class__.__name__
	test_file = "/tmp/%s.cc"%test_base
	f = open(test_file, "w")
	f.write(test_text)
	f.close()
	return self.do_run(test_file, test_base)
    def do_run(self, test_file, test_base, flags=""):
	global gdb
	flags = flags + self.flags
	if gdb:
	    return not self.gdb_less(test_file, flags)
	else:
	    return not self.view_page(test_file, test_base, flags)
    def system(self, command):
	ret = os.system(command)
	return ret >> 8
    def gdb_less(self, test_file, flags = ""):
	return self.system("make debug && echo ./occ.gdb %s %s && ./occ.gdb %s %s 2>&1 | less"%(flags, test_file, flags, test_file))
    def view_page(self, file, base, flags=""):
	f = open("/tmp/%s.top"%base, "w")
	f.write(html_top)
	f.close()
	return self.system("make && "\
	    "echo \"Running Synopsis...\" && "\
	    "synopsis -v -Wc,parser=C++ -Wp,-t,-s,/tmp/%(base)s.links,-x,/tmp/%(base)s.xref %(flags)s -o /tmp/%(base)s.syn %(file)s && "\
	    "echo \"Running Linker...\" && "\
	    "./link-synopsis -i %(file)s -l /tmp/%(base)s.links -o /tmp/%(base)s.html.out && "\
	    "(echo \"Cleaning up...\") && "\
	    "(cat /tmp/%(base)s.top /tmp/%(base)s.html.out > /tmp/%(base)s.html) && "\
	    "web /tmp/%(base)s.html && "\
	    "echo rm /tmp/%(base)s.{links,syn,html.out,top}"%vars() )

class IfTest (Test):
    test = """
void func() {
    int x = 0;
    if (x) cout << "Hi";
    if (x == 2) cout << "hi";
    if (!x) cout << "foo";
    if (x) cout << "one"; else cout << "two";
    if (x) {cout << "one"; cout << "two"; } else { cout<<"three"; }
}
"""

class ForTest (Test):
    test = """
void func() {
    int x;
    for (int x = 3, y=10; x < y; x++) {
	cout << x;
    }
}
"""

class UsingTest (Test):
    test = """
namespace Foo {
    int x;
}
void func() {
    using namespace Foo;
    x;
}
void func2() {
    using namespace Foo = Bar;
    Bar::x;
}
void func3() {
    Foo::x;
}
void func4() {
    using Foo::x;
    x;
}
"""

class UsingTest2 (Test):
    test = """
// From C++WD'96 7.4.3.2 Example
namespace A {
    int i;
    namespace B {
	namespace C {
	    int i;
	}
	using namespace A::B::C;
	void f1() {
	    i = 5; // C::i hides A::i
	}
    }
    namespace D {
	using namespace B;
	using namespace C;
	void f2() {
	    i = 5; // ambiguous, B::C::i or A::i
	}
    }
    void f3() {
	i = 5; // uses A::i
    }
}
void f4() {
    i = 5; // ill-formed, neither i visible
}
"""

class UsingTest3 (Test):
    test = """
// From C++WD'96 7.4.3.3 Example 2
namespace A {
    int i;
}
namespace B {
    int i;
    int j;
    namespace C {
	namespace D {
	    using namespace A;
	    int j;
	    int k;
	    int a = i; // B::i hides A::i
	}
	using namespace D;
	int k = 89; // no problem yet
	int l = k; // ambiguous: C::k or D::k
	int m = i; // B::i hides A::i
	int n = j; // D::j hides B::j
    }
}
"""

class UsingTest4 (Test):
    test = """
// From C++WD'96 7.4.3.6 Example
namespace D {
    int d1;
    void f(char);
}
using namespace D;
int d1; // ok, no conflict with D::d1
namespace E {
    int e;
    void f(int);
}
namespace D { // namespace extension
    int d2;
    using namespace E;
    void f(int);
}
void f()
{
    d1++;    // error: ambiguous: ::d1 or D::d1
    ::d1++;  // ok
    D::d1++; // ok
    d2++;    // ok D::d2
    e++;     // ok E::e
    f(1);    // error: ambiguous: D::f(int) or E::f(int)
    f('a');  // ok: D::f(char)
}
"""

class CastTest (Test):
    test = """
typedef int Foo;
void func() {
    (Foo)1;
    (Foo*)1;
    (const Foo&)1;
}
"""

class TryTest (Test):
    test = """
void func() {
    try {
	cout << bar;
    }
    catch (string foo) {
	cout << "Error: " << foo << endl;
    }
    catch (...) {
	cout << "Catchall";
    }
}
"""

class MacroTest (Test):
    test = """
int x;
#define LONGER 12345678
#define SHORTER 1234
#define LINKINSIDE x
#define ARGS(a, b, c) x
int A = LONGER, A2 = x;
int B = SHORTER, B2 = x;
int C = LINKINSIDE, C2 = x;
int D = ARGS(1, 2, 3), D2 = x;
"""

class FuncTest (Test):
    test = """
void func(char);
void func(int);
void func(double);
void func(const char*);
void test() {
    func('c');
    func(123);
    func(1.2);
    func("s");
}"""

class OperTest (Test):
    test = """
struct A {};
struct B {};
A operator +(const B&, const B&);
int operator +(const A&, const A&);
void func(A);
void func(B);
void func(int);
void main() {
    B x, y;
    func( (x + y) + (x + y) ); // should call func(int)
}
"""

class KoenigTest (Test):
    test = """
namespace NS {
    struct A {};
    int operator +(A, A);
};
void func(int);
void func(NS::A) {
    NS::A x, y;
    func(x + y); // should call func(int)
}
"""

class TemplateTest (Test):
    test = """
struct Object {
    float f;
    double func();
    Object();
    Object(const Object&);
    Object& operator = (const Object&);
};
    
template <typename T>
class list {
    T* m_array;
    int m_size;
public:
    list();
    list(T*);

    T& operator [] (int index) { return m_array[index]; }
    int size() { return m_size; }

    void replace(int index, const T& with) { m_array[index] = with; }
};

void main() {
    list<Object> a_list;
    a_list.replace(1, Object());
    Object b(a_list[1]);
    a_list[2].func();
    b = a_list[3];
}
"""

class StdTest (Test):
    flags = gcc_include + python_include + "-Wp,-f "
    test = """
#include <vector>
namespace Foo {
    void func(std::vector<int> array);
}
"""

class StaticCastTest (Test):
    test = """
typedef unsigned int size_type;
int really_big_number = static_cast<size_type>(-1);
"""

class Link (Test):
    def run(self):
	return self.do_run("link.cc", "link", gcc_include + python_include)

class Builder (Test):
    def run(self):
	return self.do_run("builder.cc", "builder", gcc_include + python_include + "-I../")

class Synopsis (Test):
    def run(self):
	return self.do_run("synopsis.cc", "synopsis", gcc_include + python_include)

class AST (Test):
    def run(self):
	return self.do_run("ast.hh", "ast", gcc_include + python_include)

if __name__ == "__main__":
    tests = {}
    for key in globals().keys():
	obj = globals()[key]
	if type(obj) is types.ClassType and Test in obj.__bases__:
	    tests[key] = obj
    test = None
    for arg in sys.argv[1:]:
	if arg[0] == '-':
	    if arg[1] == 'g':
		global gdb
		gdb = 1
	    else:
		print "Unknown argument:",arg
	else:
	    test = arg
    if not tests.has_key(test): test = None
    if test is None:
	print "Choose a test from:", tests.keys()
	sys.exit(1)
    test = tests[test]
    if not test().run(): print "failed"

