// Methods for type_info for the -*- C++ -*- Run Time Type Identification.
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

// Written by Kung Hsu based upon the specification in the 20 September 1994
// C++ working paper, ANSI document X3J16/94-0158.

#include <stdlib.h>
#include <typeinfo.h>

// in case we manage to call these somehow.
type_info::type_info(const type_info&) { abort();}
type_info& type_info::operator=(const type_info&) { abort();}

// Offset functions for the class type.

// 0 is returned if the cast is invalid, otherwise the converted
// object pointer that points to the sub-object that is matched is
// returned.

void* __class_type_info::__rtti_match(const type_info& desired, int is_public,
				      void *objptr) const
{
  if (__rtti_compare(desired) == 0)
    return objptr;

  void *match_found = 0;
  for (int i = 0; i < n_bases; i++) {
    if (is_public && access_list[i] != _RTTI_ACCESS_PUBLIC)
      continue;
    void *p = objptr + offset_list[i];
    if (is_virtual_list[i])
      p = *(void **)p;

    if ((p=base_list[i]->__rtti_match(desired, is_public, p))
	!= 0)
      if (match_found == 0)
	match_found = p;
      else if (match_found != p) {
	// base found at two different pointers,
	// conversion is not unique
	return 0;
      }
  }

  return match_found;
}
