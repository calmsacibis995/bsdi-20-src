/* 
Copyright (C) 1993 Free Software Foundation

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

char*
_IO_fgets(buf, n, fp)
     char* buf;
     int n;
     _IO_FILE* fp;
{
  _IO_size_t count;
  CHECK_FILE(fp, NULL);
  if (n <= 0)
    return NULL;
  count = _IO_getline(fp, buf, n - 1, '\n', 1);
  if (count == 0 || (fp->_IO_file_flags & _IO_ERR_SEEN))
    return NULL;
  buf[count] = 0;
  return buf;
}
