// Test.cc
// Test previous comments (begin with < and are mapped to previous declaration
// if -Wl,-p,prev used.)
//
// synopsis -p C++ -Wp,-t -Wl,-p,ssd,-p,prev -f ASCII test.hh

//. A test class
class Class {
    //. An enum
    enum Enum {
	//. Comment before ALPHA
	ALPHA = 1, //.< PrevComment after ALPHA
	BETA = 2
	//.< PrevComment after BETA
    };
    //.< PrevComment after the enum but before the end of the class
};
