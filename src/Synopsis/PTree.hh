//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_PTree_hh_
#define Synopsis_PTree_hh_

#include <Synopsis/PTree/Encoding.hh>
#include <Synopsis/PTree/Node.hh>
#include <Synopsis/PTree/operations.hh>
#include <Synopsis/PTree/Atoms.hh>
#include <Synopsis/PTree/Lists.hh>
#include <Synopsis/PTree/TypeVisitor.hh>
#include <Synopsis/PTree/generation.hh>

namespace Synopsis
{
void MopErrorMessage(const char* method_name, const char* msg);
void MopErrorMessage2(const char* msg1, const char* msg2);
void MopWarningMessage(const char* method_name, const char* msg);
void MopWarningMessage2(const char* msg1, const char* msg2);
void MopMoreWarningMessage(const char* msg1, const char* msg2 = 0);
}

#endif
