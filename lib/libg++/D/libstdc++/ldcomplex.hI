// The -*- C++ -*- long_double_complex class.
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

// Written by Jason Merrill based upon the specification in the 27 May 1994
// C++ working paper, ANSI document X3J16/94-0098.

#ifdef __GNUG__
#pragma interface "ldcomplex"
#endif

class __complex<long double>
{
public:
  __complex (long double r = 0, long double i = 0): re (r), im (i) { }
  __complex (const __complex<float>& r): re (real (r)), im (imag (r)) { }
  __complex (const __complex<double>& r): re (real (r)), im (imag (r)) { }

  __complex& operator+= (__complex);
  __complex& operator-= (__complex);
  __complex& operator*= (__complex);
  __complex& operator/= (__complex);
private:
  double re, im;

  friend long double real (__complex x) { return x.re; }
  friend long double imag (__complex x) { return x.im; }
};

inline __complex<double>
_double_complex(const __complex<long double>& r)
{
  return __complex<double> (real (r), imag (r));
}
