// Instantiation file for the -*- C++ -*- string classes.
// Copyright (C) 1994 Free Software Foundation

// This file is part of the GNU ANSI C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with GNU CC; see the file COPYING.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

// As a special exception, if you link this library with files
// compiled with a GNU compiler to produce an executable, this does not cause
// the resulting executable to be covered by the GNU General Public License.
// This exception does not however invalidate any other reasons why
// the executable file might be covered by the GNU General Public License.

// Written by Jason Merrill based upon the specification by Takanori Adachi
// in ANSI X3J16/94-0013R2.

#ifdef __GNUG__
#ifdef TRAITS
#ifdef C
#pragma implementation "straits"
#endif
#endif
#endif

#if 0
#define _G_NO_EXTERN_TEMPLATES
#endif
#include <bastring.ccI>

#ifdef C
typedef char c;
#endif
#ifdef W
typedef wchar_t c;
#endif

#ifdef TRAITS
template class string_char_traits <c>;
#endif

#define STRING basic_string <c, string_char_traits <c> >
typedef class STRING s;
#define BSREP __bsrep <c, string_char_traits <c> >
typedef class BSREP r;

#ifdef REP
template class BSREP;
r s::nilRep = { 0, 0, 1 };
#ifdef _G_ALLOC_CONTROL
bool (*r::excess_slop) (size_t, size_t) = r::default_excess;
size_t (*r::frob_size) (size_t) = r::default_frob;
#endif
#endif

#ifdef MAIN
template class STRING;
#endif

#ifdef ADDSS
template s operator+ (const s&, const s&);
#endif
#ifdef ADDPS
template s operator+ (const c*, const s&);
#endif
#ifdef ADDCS
template s operator+ (c, const s&);
#endif
#ifdef ADDSP
template s operator+ (const s&, const c*);
#endif
#ifdef ADDSC
template s operator+ (const s&, c);
#endif
#ifdef EQSS
template bool operator== (const s&, const s&);
#endif
#ifdef EQPS
template bool operator== (const c*, const s&);
#endif
#ifdef EQCS
template bool operator== (c, const s&);
#endif
#ifdef EQSP
template bool operator== (const s&, const c*);
#endif
#ifdef EQSC
template bool operator== (const s&, c);
#endif
#ifdef NESS
template bool operator!= (const s&, const s&);
#endif
#ifdef NEPS
template bool operator!= (const c*, const s&);
#endif
#ifdef NECS
template bool operator!= (c, const s&);
#endif
#ifdef NESP
template bool operator!= (const s&, const c*);
#endif
#ifdef NESC
template bool operator!= (const s&, c);
#endif
#ifdef EXTRACT
template istream& operator>> (istream&, s&);
#endif // EXTRACT
#ifdef INSERT
template ostream& operator<< (ostream&, s);
#endif // INSERT
