/*
  Copyright (C) 1997-2000 Shigeru Chiba, University of Tsukuba.

  Permission to use, copy, distribute and modify this software and   
  its documentation for any purpose is hereby granted without fee,        
  provided that the above copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in 
  supporting documentation.

  Shigeru Chiba makes no representations about the suitability of this 
  software for any purpose.  It is provided "as is" without express or
  implied warranty.
*/
/*
  Copyright (c) 1995, 1996 Xerox Corporation.
  All Rights Reserved.

  Use and copying of this software and preparation of derivative works
  based upon this software are permitted. Any copy of this software or
  of any derivative work must include the above copyright notice of
  Xerox Corporation, this paragraph and the one after it.  Any
  distribution of this software or derivative works must comply with all
  applicable United States export control laws.

  This software is made available AS IS, and XEROX CORPORATION DISCLAIMS
  ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE, AND NOTWITHSTANDING ANY OTHER PROVISION CONTAINED HEREIN, ANY
  LIABILITY FOR DAMAGES RESULTING FROM THE SOFTWARE OR ITS USE IS
  EXPRESSLY DISCLAIMED, WHETHER ARISING IN CONTRACT, TORT (INCLUDING
  NEGLIGENCE) OR STRICT LIABILITY, EVEN IF XEROX CORPORATION IS ADVISED
  OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#include "MetaClass.hh"
#include "PTree.hh"

#if USE_DLOADER
#include <iostream>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <cstdlib>	/* for exit() */

// in driver2.cc
extern void RunSoCompiler(const char* src_file);
extern void* LoadSoLib(char* file_name);
extern void* LookupSymbol(void* handle, char* symbol);

#if !USE_SO
extern void BaseClassUsed(char *, int);		// in driver2.cc
#endif /* USE_SO */

#endif /* USE_DLOADER */

extern bool verboseMode;	// defined in driver.cc
extern bool makeSharedLibrary;


// class QuoteClass
//
// Part of the implementation of QuoteClass is included here to let
// quote-class.cc linked to others if the compiler is produced by
// "g++ -o occ opencxx.a".

#include "QuoteClass.hh"

// The followings should be automatically generated but we write it
// down here for bootstrapping.  Also see Metaclass::IsBuiltinMetaclass()

static opcxx_ListOfMetaclass* QuoteClassCreator;
static opcxx_ListOfMetaclass* metaclassCreator;

static Class* CreateQuoteClass(PTree::Node *def, PTree::Node *marg)
{
    Class* metaobject = new QuoteClass;
    metaobject->InitializeInstance(def, marg);
    return metaobject;
}

opcxx_ListOfMetaclass* opcxx_init_QuoteClass()
{
    return new opcxx_ListOfMetaclass("QuoteClass", CreateQuoteClass,
				     QuoteClass::Initialize, 0);
}

static Class* CreateMetaclass(PTree::Node *def, PTree::Node *marg)
{
    Class* metaobject = new Metaclass;
    metaobject->InitializeInstance(def, marg);
    return metaobject;
}

opcxx_ListOfMetaclass* opcxx_init_Metaclass()
{
    return new opcxx_ListOfMetaclass("Metaclass", CreateMetaclass,
				     Metaclass::Initialize, 0);
}


bool QuoteClass::Initialize()
{
    return true;
}

const char* QuoteClass::MetaclassName()
{
    return "QuoteClass";
}


// class Metaclass

void Metaclass::do_init_static()
{
    QuoteClassCreator = new opcxx_ListOfMetaclass(
	    "QuoteClass", CreateQuoteClass, QuoteClass::Initialize, 0);
    metaclassCreator = new opcxx_ListOfMetaclass(
	    "Metaclass", CreateMetaclass, Metaclass::Initialize, 0);
}

Metaclass::Metaclass()
{
    new_function_name = 0;
    first_not_inlined_vf = -1;
}

bool Metaclass::Initialize()
{
    return true;
}

const char* Metaclass::MetaclassName()
{
    return "Metaclass";
}

void Metaclass::TranslateClass(Environment* env)
{
    PTree::Node *name = Name();

    if(!IsBuiltinMetaclass(name)){
	CheckObsoleteness();
	InsertInitialize();
#if defined(_MSC_VER) || defined(PARSE_MSVC)
	AddClassSpecifier(PTree::make("__declspec(dllexport)"));
#endif
	AppendMember(PTree::make("public: char* MetaclassName() {\n"
				 "    return \"%p\"; }\n",
				 Name()));

	PTree::Node *tmpname = PTree::gen_sym();
	PTree::Node *tmpname2 = PTree::gen_sym();
	PTree::Node *finalizer = GetFinalizer();
	AppendAfterToplevel(env, PTree::make(
		"static Class* %p(PTree::Node *def, PTree::Node *marg){\n"
		"    Class* c = new %p;\n"
		"    c->InitializeInstance(def, marg);\n"
		"    return c; }\n"
		"static opcxx_ListOfMetaclass %p(\"%p\", %p,\n"
		"    %p::Initialize, %p);\n",
		tmpname, name, tmpname2, name, tmpname, name, finalizer));

	if(makeSharedLibrary){
	    ProduceInitFile(name);
	    first_not_inlined_vf = FindFirstNotInlinedVirtualFunction();
	    new_function_name = tmpname;
	    if(first_not_inlined_vf < 0)
		AppendHousekeepingCode(env, Name(), tmpname, finalizer);
	}
    }

    Class::TranslateClass(env);
}

PTree::Node *Metaclass::GetFinalizer()
{
    Member m;
    if(LookupMember("FinalizeClass", m) && m.Supplier() == this){
        if(!m.IsStatic())
	    ErrorMessage("FinalizeClass() must be static in ", Name(),
			 Definition());

	return PTree::make("%p::FinalizeClass", Name());
    }
    else
      return PTree::make("0");
}

void Metaclass::CheckObsoleteness()
{
    Member m;

    if(LookupMember("Finalize", m) && m.Supplier() == this)
	WarningMessage("Finalize() is obsolete.  Use FinalizeInstance() in ",
		       Name(),
		       Definition());
}

void Metaclass::ProduceInitFile(PTree::Node *class_name)
{
#if USE_DLOADER
#if USE_SO
  const char* fname = PTree::make("%p-init.cc", class_name)->ToString();
    if(verboseMode)
	std::cerr << "Produce " << fname << " ..\n";

    std::ofstream src_file(fname);
    if(!src_file){
        perror(fname);
	exit(1);
    }

    src_file << "extern void LoadMetaclass(char*);\n";
    src_file << "char* opcxx_Init_" << class_name << "(){\n";

    PTree::Node *base_name;
    for(int i = 0; (base_name = NthBaseClassName(i)) != 0; ++i)
	if(*base_name != "Class")
	    src_file << "  LoadMetaclass(\"" << base_name << "\");\n";

    src_file <<	"    return 0;\n}\n";

    src_file.close();

    RunSoCompiler(fname);
#else
    // Push base class names forward to RunCompiler
    PTree::Node *base_name;
    for (int i = 0; (base_name = NthBaseClassName(i)) != 0; ++i)
	if (*base_name != "Class" && *base_name != "TemplateClass")
	    BaseClassUsed(base_name->position(), base_name->length());
#endif /* USE_SO */
#endif /* USE_DLOADER */
}

bool Metaclass::IsBuiltinMetaclass(PTree::Node *name)
{
  return  (*name == "Class" || *name == "Metaclass" ||
	   *name == "TemplateClass" ||
	   *name == "QuoteClass");
}

void Metaclass::InsertInitialize()
{
    Member m;
    if(!LookupMember("Initialize", m) || m.Supplier() != this){
#if !defined(_MSC_VER) || (_MSC_VER >= 1100)
      AppendMember(PTree::make(
		"public: static bool Initialize() { return 1; }\n"));
#else
      AppendMember(PTree::make(
		"public: static int Initialize() { return 1; }\n"));
#endif
    }
    else if(!m.IsStatic())
	ErrorMessage("Initialize() must be static in ", Name(),
		     Definition());
}

int Metaclass::FindFirstNotInlinedVirtualFunction()
{
    Member m;
    for(int i = 0; NthMember(i, m); ++i)
	if(m.IsFunction() && m.IsVirtual() && !m.IsInline()
	   && m.Supplier() == this)
	    return i;

    WarningMessage("a metaclass should include at least one"
		   " not-inlined virtual function: ", Name(), Name());
    return -1;
}

void Metaclass::TranslateMemberFunction(Environment* env, Member& m)
{
    if(m.Nth() != first_not_inlined_vf)
	return;

    if(m.IsInline()){
	ErrorMessage("This member function should not be inlined: ",
		     m.Name(), m.ArgumentList());
	return;
    }

    AppendHousekeepingCode(env, Name(), new_function_name,
			   GetFinalizer());
}

void Metaclass::AppendHousekeepingCode(Environment* env, PTree::Node *class_name,
				       PTree::Node *creator_name,
				       PTree::Node *finalizer)
{
#if !defined(_MSC_VER)
  AppendAfterToplevel(env, PTree::make(
		"opcxx_ListOfMetaclass* opcxx_init_%p(){\n"
		"    return new opcxx_ListOfMetaclass(\"%p\", %p,\n"
		"                   %p::Initialize, %p); }\n",
		class_name, class_name, creator_name, class_name,
		finalizer));
#endif
}

void LoadMetaclass(const char* metaclass_name)
{
#if USE_DLOADER
    if(metaclass_name != 0 && *metaclass_name != '\0')
	if(!opcxx_ListOfMetaclass::AlreadyRecorded(metaclass_name))
	    Metaclass::Load(metaclass_name, strlen(metaclass_name));
#endif
}

void Metaclass::Load(PTree::Node *metaclass_name)
{
#if USE_DLOADER
    if(opcxx_ListOfMetaclass::AlreadyRecorded(metaclass_name))
	return;

    Load(metaclass_name->position(), metaclass_name->length());
#endif
}

void Metaclass::Load(const char* metaclass_name, int len)
{
#if USE_DLOADER
#if USE_SO
#if DLSYM_NEED_UNDERSCORE
    static char header[] = "_opcxx_Init_";
#else
    static char header[] = "opcxx_Init_";
#endif
#if defined(IRIX_CC)
    static char tail[] = "__Gv";
#else
    static char tail[] = "__Fv";
#endif

    char* func_name = new char[len + sizeof(header) + sizeof(tail)];
    strcpy(func_name, header);
    memmove(func_name + sizeof(header) - 1, metaclass_name, len);
    strcpy(func_name + sizeof(header) + len - 1, tail);

    // here, func_name is "_opcxx_Init_<metaclass>__Fv" or ".._Gv".

    char* file_name = new char[len + 9];
    memmove(file_name, metaclass_name, len);

    strcpy(file_name + len, "-init.so");
    void* handle_init = LoadSoLib(file_name);	// load <metaclass>-init.so

    // call opcxx_Init_<metaclass>() in <metaclass>-init.so

    void (*loader)();
    loader = (void (*)())LookupSymbol(handle_init, func_name);
    (*loader)();

    strcpy(file_name + len, ".so");
    void* handle = LoadSoLib(file_name);	// load <metaclass>.so

    if(verboseMode)
	std::cerr << "Initialize.. ";

    // call opcxx_init_<metaclass>() in <metaclass>.so

    func_name[sizeof(header) - 6] = 'i';
    void (*func)();
    func = (void (*)())LookupSymbol(handle, func_name);
    (*func)();

    delete [] file_name;
    delete [] func_name;

    if(verboseMode)
	std::cerr << "Done.\n";

#else /* USE_SO */

    // Win32 uses auto-initialization of DLL's static variables
    static char ext[] = ".dll";
    char* file_name = new char[len + sizeof(ext)];
    memmove(file_name, metaclass_name, len);
    strcpy(file_name + len, ext);
    void* handle = LoadSoLib(file_name);	// load <metaclass>.dll

    delete [] file_name;

    if(verboseMode)
	std::cerr << "Done.\n";
#endif /* USE_SO */
#endif /* USE_DLOADER */
}

void* Metaclass::LoadSoLib(const char* file_name)
{
    void* handle = 0;
#if USE_DLOADER
    if(verboseMode)
	std::cerr << "Load " << file_name << ".. ";

    handle = ::LoadSoLib(file_name);
#endif /* USE_DLOADER */

    return handle;
}

void* Metaclass::LookupSymbol(void* handle, const char* symbol)
{
    void* func = 0;
#if USE_DLOADER
    func = ::LookupSymbol(handle, symbol);
#endif
    return func;
}
