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

#if defined(IRIX_CC)
// for open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fstream.h>
#include "types.h"

// g++ recognizes a .ii file as preprocessed C++ source code.
// CC recognizes a .i file as preprocessed C++ source code.

#if defined(IRIX_CC)
#define OUTPUT_EXT	".i"
#else
#define OUTPUT_EXT	".ii"
#endif

#define CPP_EXT		".occ"
#define SLIB_EXT	".so"
#define OBJ_EXT		".o"

extern "C" {
#if !defined(IRIX_CC) && !defined(__GLIBC__) && !defined(__STRICT_ANSI__)
    int execvp(...);
#endif
    int wait(int*);
}

#if defined(IRIX_CC)
const char* compilerName = "CC";
#else
const char* compilerName = "g++";
#endif
const char* linkerName = "ld";
const char* opencxxErrorMessage = " Error(s).  OpenC++ stops.\n";

// defined in driver.cc

extern bool showProgram;
extern bool doCompile;
extern bool makeExecutable;
extern bool doPreprocess;
extern bool doTranslate;
extern bool verboseMode;
extern bool regularCpp;
extern bool makeSharedLibrary;
extern char* sharedLibraryName;
extern bool preprocessTwice;

extern const char* cppArgv[];
extern const char* ccArgv[];

extern void ParseCmdOptions(int from, int argc, char** argv, char*& source);
extern void AddCppOption(const char* arg);
extern void AddCcOption(const char* arg);
extern void CloseCcOptions();
extern void ShowCommandLine(const char* cmd, const char** args);

bool ParseTargetSpecificOptions(char* arg, char*& source_file);
void RunLinker();
char* RunPreprocessor(const char* src);
char* OpenCxxOutputFileName(const char* src);
void RunCompiler(const char* src, const char* occsrc);
void RunSoCompiler(const char* src_file);
void* LoadSoLib(char* file_name);
void* LookupSymbol(void* handle, char* symbol);

#if !SHARED_OPTION
static void RunSoLinker(const char* org_src, char* target);
#endif
static char* MakeTempFilename(const char* src, const char* suffix);


bool ParseTargetSpecificOptions(char*, char*&)
{
    return false;
}

void RunLinker()
{
    if(!doCompile || !makeExecutable){
	cerr << "OpenC++: no source file.\n";
	return;
    }

    const char* linker = compilerName;
    char* slib = nil;
    if(makeSharedLibrary){
#if SHARED_OPTION
#if defined(IRIX_CC)
	AddCcOption("-n32");
#else
	AddCcOption("-fPIC");
#endif
	AddCcOption("-shared");
#else /* SHARED_OPTION */
	AddCcOption("-Bshareable");
	linker = linkerName;
#endif
	if(sharedLibraryName != nil && *sharedLibraryName != '\0'){
	    slib = MakeTempFilename(sharedLibraryName, SLIB_EXT);
	    AddCcOption("-o");
	    AddCcOption(slib);
	}
    }

    ccArgv[0] = linker;
    CloseCcOptions();

    if(verboseMode){
	cerr << "[Link... ";
	ShowCommandLine(linker, ccArgv);
	cerr << "]\n";
    }

    if(fork() == 0){
	execvp(linker, (char**)ccArgv);
	perror("cannot invoke a compiler");
    }
    else{
	int status;

	wait(&status);
	if(status != 0)
	    exit(1);
    }

    delete [] slib;
}

char* RunPreprocessor(const char* src)
{
    char* dest = MakeTempFilename(src, CPP_EXT);
    if(!regularCpp)
	AddCppOption("-D__opencxx");

    AddCppOption("-E");
#if defined(IRIX_CC)
    AddCppOption("-n32");
#else
    AddCppOption("-o");
    AddCppOption(dest);
    AddCppOption("-x");
    AddCppOption("c++");
#endif
    AddCppOption(src);
    AddCppOption((char*)0);

    if(verboseMode){
	cerr << "[Preprocess... ";
	ShowCommandLine(compilerName, cppArgv);
#if defined(IRIX_CC)
	cerr << " > " << dest;
#endif
	cerr << "]\n";
    }

    if(fork() == 0){
#if defined(IRIX_CC)
	int fd = open(dest, O_WRONLY | O_CREAT, 0666);
	if (fd < 0) {
	    perror(dest);
	    exit(1);
	}
	dup2(fd, 1);
#endif
	execvp(compilerName, (char**)cppArgv);
	perror("cannot invoke a compiler");
    }
    else{
	int status;

	wait(&status);
	if(status != 0)
	    exit(1);
    }

    return dest;
}

char* OpenCxxOutputFileName(const char* src)
{
    return MakeTempFilename(src, OUTPUT_EXT);
}

/*
   To create a shared library foo.so from foo.cc,

   SunOS, Solaris, Linux (v2.0, gcc 2.7.2.2):
		g++ -fPIC -shared -o foo.so foo.cc

   Irix with naitive CC:
		CC -shared -n32 -o foo.so foo.cc

   FreeBSD:	g++ -fPIC -c foo.cc
		ld -Bshareable -o foo.so foo.o

*/
void RunCompiler(const char* org_src, const char* occ_src)
{
    char* slib = nil;
    if(makeSharedLibrary){
	const char* name = org_src;
	if(sharedLibraryName != nil && *sharedLibraryName != '\0')
	    name = sharedLibraryName;

	slib = MakeTempFilename(name, SLIB_EXT);
#if SHARED_OPTION
#if defined(IRIX_CC)
	AddCcOption("-n32");
#else
	AddCcOption("-fPIC");
#endif
	AddCcOption("-shared");
	if(makeExecutable){
	    AddCcOption("-o");
	    AddCcOption(slib);
	}
	else
	    AddCcOption("-c");
#else /* SHARED_OPTION */
	AddCcOption("-fPIC");
	AddCcOption("-c");
#endif
    }
    else
	if(!makeExecutable)
	    AddCcOption("-c");

#if !defined(IRIX_CC)
    if(preprocessTwice){
	AddCcOption("-x");
	AddCcOption("c++");
    }
#endif

    AddCcOption(occ_src);
    CloseCcOptions();

    if(verboseMode){
	cerr << "[Compile... ";
	ShowCommandLine(compilerName, ccArgv);
	cerr << "]\n";
    }

    if(fork() == 0){
	execvp(compilerName, (char**)ccArgv);
	perror("cannot invoke a compiler");
    }
    else{
	int status;

	wait(&status);
	if(status != 0)
	    exit(1);
    }

#if !SHARED_OPTION
    if(makeSharedLibrary && makeExecutable)
	RunSoLinker(org_src, slib);
#endif

    delete [] slib;
}

void RunSoCompiler(const char* src_file)
{
    const char* cc_argv[8];
    int i = 0;

    char* slib = MakeTempFilename(src_file, SLIB_EXT);
    cc_argv[i++] = compilerName;
#if SHARED_OPTION
#if defined(IRIX_CC)
    cc_argv[i++] = "-n32";
#else
    cc_argv[i++] = "-fPIC";
#endif
    cc_argv[i++] = "-shared";
    cc_argv[i++] = "-o";
    cc_argv[i++] = slib;
#else
    cc_argv[i++] = "-fPIC";
    cc_argv[i++] = "-c";
#endif
    cc_argv[i++] = src_file;
    cc_argv[i++] = (char*)0;

    if(verboseMode){
	cerr << "[Compile... ";
	ShowCommandLine(compilerName, cc_argv);
	cerr << "]\n";
    }

    if(fork() == 0){
	execvp(compilerName, (char**)cc_argv);
	perror("cannot invoke a compiler");
    }
    else{
	int status;

	wait(&status);
	if(status != 0)
	    exit(1);
    }

#if !SHARED_OPTION
    RunSoLinker(src_file, slib);
#endif

    delete [] slib;
}

void* LoadSoLib(char* file_name)
{
    void* handle = nil;
#if USE_DLOADER
    handle = dlopen(file_name, RTLD_GLOBAL | RTLD_LAZY);
    handle = dlopen(file_name, RTLD_GLOBAL | RTLD_LAZY);
    if(handle == NULL){
 	cerr << "dlopen(" << file_name << ") failed: " << dlerror() << '\n';
 	exit(1);
    }
#endif /* USE_DLOADER */

    return handle;
}

void* LookupSymbol(void* handle, char* symbol)
{
    void* func = nil;
#if USE_DLOADER
    func = dlsym(handle, symbol);
    if(func == NULL){
 	cerr << "dlsym() failed (non metaclass?): " << dlerror() << '\n';
 	exit(1);
    }
#endif
    return func;
}

// RunSoLinker() is used only if SHARED_OPTION is false (FreeBSD).
#if !SHARED_OPTION

static void RunSoLinker(const char* org_src, char* target)
{
    const char* ld_argv[6];
    ld_argv[0] = linkerName;
    ld_argv[1] = "-Bshareable";
    ld_argv[2] = "-o";
    ld_argv[3] = target;
    ld_argv[4] = MakeTempFilename(org_src, OBJ_EXT);
    ld_argv[5] = (char*)0;

    if(verboseMode){
	cerr << "[Link... ";
	ShowCommandLine(linkerName, ld_argv);
	cerr << "]\n";
    }

    if(fork() == 0){
	execvp(linkerName, (char**)ld_argv);
	perror("cannot invoke a linker");
    }
    else{
	int status;

	wait(&status);
	if(status != 0)
	    exit(1);
    }

    unlink(ld_argv[4]);
    delete [] ld_argv[4];
}
#endif /* SHARED_OPTION */

/*
   For example, if src is "../foo.cc", MakeTempFilename() makes
   "foo.<suffix>".
*/
static char* MakeTempFilename(const char* src, const char* suffix)
{
    const char* start;
    const char* end;

    start = strrchr(src, '/');
    if(start == nil)
	start = src;
    else
	++start;

    end = strrchr(start, '.');
    if(end == nil)
	end = src + strlen(src);

    char* result = new char[end - start + strlen(suffix) + 1];
    strncpy(result, start, end - start);
    result[end - start] = '\0';
    strcat(result, suffix);
    return result;
}
