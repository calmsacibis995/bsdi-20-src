/* 
Copyright (C) 1994 Free Software Foundation

This file is part of the GNU IO Library.  This library is free
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

#include "libioP.h"

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

int
snprintf
#ifdef __STDC__
  (char *string, _IO_size_t maxlen, const char *format, ...)
#else
(string, maxlen, format, va_alist)
     char *string;
     _IO_size_t maxlen;
     char *format;
     va_dcl
#endif
{
  int ret;
  va_list args;
  _IO_va_start(args, format);
  ret = vsnprintf(string, maxlen, format, args);
  va_end(args);
  return ret;
}
