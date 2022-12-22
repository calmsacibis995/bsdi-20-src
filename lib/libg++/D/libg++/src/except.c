/*  This is part of GNU C++ Library.
Copyright (C) 1994 Free Software Foundation

This file is part of the GNU C++ Library.  This library is free
software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option)
any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

As a special exception, if you link this library with files
compiled with a GNU compiler to produce an executable, this does not cause
the resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why
the executable file might be covered by the GNU General Public License. */

/* terminate(), unexpected(), set_terminate(), set_unexpected() as
   well as the default terminate func and default unexpected func */

#if 0
extern int printf();
#endif

typedef void (*vfp)();

void
__default_terminate()
{
  abort();
}

void
__default_unexpected()
{
  __default_terminate();
}

static vfp __terminate_func = __default_terminate;
static vfp __unexpected_func = __default_unexpected;

vfp
set_terminate(func)
vfp func;
{
  vfp old = __terminate_func;

  __terminate_func = func;
  return old;
}

vfp
set_unexpected(func)
vfp func;
{
  vfp old = __unexpected_func;

  __unexpected_func = func;
  return old;
}

void
terminate()
{
  __terminate_func();
}

void
unexpected()
{
  __unexpected_func();
}
