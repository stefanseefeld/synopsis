// vim: set ts=8 sts=2 sw=2 et:
// File: common.hh
// 
// Types common to many places

#ifndef H_SYNOPSIS_CPP_COMMON
#define H_SYNOPSIS_CPP_COMMON

#include <string>
#include <vector>

//. A scoped name, containing zero or more elements. This typedef makes it
//. easier to use scoped name types, and also makes it clearer than using the
//. raw vector in your code.
typedef std::vector<std::string> ScopedName;

//. Prototype for scoped name output operator (defined in builder.cc)
std::ostream& operator <<(std::ostream& out, const ScopedName& name);

//. Joins the elements of the scoped name using the separator string
std::string join(const ScopedName& strs, const std::string& sep = " ");

#endif
