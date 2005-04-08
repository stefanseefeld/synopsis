//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "Kit.hh"

using namespace Synopsis;
using namespace TypeAnalysis;

Kit::Kit()
{
}

Type const *Kit::builtin(std::string const &name)
{
}

Type const *Kit::enum_(std::string const &name)
{
}

Type const *Kit::class_(std::string const &name)
{
}

Type const *Kit::union_(std::string const &name)
{
}

Type const *Kit::pointer(Type const *type)
{
}

Type const *Kit::reference(Type const *type)
{
}

Type const *Kit::array(Type const *type)
{
}

Type const *Kit::pointer_to_member(Type const *container, Type const *member)
{
}
