//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_hh
#define _PTree_hh

#include <PTree/Node.hh>
#include <PTree/operations.hh>
#include <PTree/Atom.hh>
#include <PTree/List.hh>
#include <PTree/Atoms.hh>
#include <PTree/Lists.hh>
#include <PTree/generation.hh>

void MopErrorMessage(const char* method_name, const char* msg);
void MopErrorMessage2(const char* msg1, const char* msg2);
void MopWarningMessage(const char* method_name, const char* msg);
void MopWarningMessage2(const char* msg1, const char* msg2);
void MopMoreWarningMessage(const char* msg1, const char* msg2 = 0);

#endif
