//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_hh
#define _PTree_hh

#include <PTree/Node.hh>
#include <PTree/Atom.hh>
#include <PTree/List.hh>
#include <PTree/Atoms.hh>
#include <PTree/Lists.hh>

void MopErrorMessage(char* method_name, char* msg);
void MopErrorMessage2(char* msg1, char* msg2);
void MopWarningMessage(char* method_name, char* msg);
void MopWarningMessage2(char* msg1, char* msg2);
void MopMoreWarningMessage(char* msg1, char* msg2 = 0);

#endif
