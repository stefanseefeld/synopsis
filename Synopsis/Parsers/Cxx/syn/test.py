#!/usr/bin/python
# A test-suite for the C++ syntax highlighter

import os, types, sys

gdb = None

html_top = '<html><link rel="stylesheet" href="/home/chalky/src/Synopsis/demo/html.css"><body>'
class Test:
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
	if gdb:
	    return not self.gdb_less(test_file, flags)
	else:
	    return not self.view_page(test_file, test_base, flags)
    def system(self, command):
	ret = os.system(command)
	return ret >> 8;
    def gdb_less(self, test_file, flags = ""):
	return self.system("make debug && ./occ.gdb %s %s | less"%(flags, test_file));
    def view_page(self, file, base, flags=""):
	f = open("/tmp/%s.top"%base, "w")
	f.write(html_top)
	f.close()
	return self.system("make && "\
	    "echo \"Running Synopsis...\" && "\
	    "synopsis -p C++ -Wp,-s,/tmp/%(base)s.links %(flags)s -o /tmp/%(base)s.syn %(file)s && "\
	    "echo \"Running Linker...\" && "\
	    "./link-synopsis -i %(file)s -l /tmp/%(base)s.links -o /tmp/%(base)s.html.out && "\
	    "(echo \"Cleaning up...\") && "\
	    "(cat /tmp/%(base)s.top /tmp/%(base)s.html.out > /tmp/%(base)s.html) && "\
	    "web /tmp/%(base)s.html && "\
	    "rm /tmp/%(base)s.{links,syn,html.out,top}"%vars() )

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
void func(float);
void func(char*);
void test() {
    func('c');
    func(123);
    func(1.2);
    func("s");
}"""

class Link (Test):
    def run(self):
	return self.do_run("link.cc", "link", "-Wp,-DPYTHON_MAJOR=1,-DPYTHON_MINOR=5")

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

