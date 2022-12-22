// Instantiation file for the -*- C++ -*- complex number classes.
// Copyright (C) 1994 Free Software Foundation

#ifdef F
typedef float f;
#endif
#ifdef D
typedef double f;
#endif
#ifdef LD
typedef long double f;
#endif

#if defined (MAIN) && defined (__GNUG__)
#ifdef D
#pragma implementation "dcomplex"
#endif
#ifdef LD
#pragma implementation "ldcomplex"
#endif
#endif

#if 0
#define _G_NO_EXTERN_TEMPLATES
#endif
#include <complext.ccI>

typedef __complex<f> c;

#ifdef MAIN
template class __complex<f>;

#ifdef F
template f imag (c);
template f real (c);
template c _float_complex (const __complex<double>&);
template c _float_complex (const __complex<long double>&);
#endif
#endif

#ifdef ADDCC
template c operator+ (c, c);
#endif
#ifdef ADDCF
template c operator+ (c, f);
#endif
#ifdef ADDFC
template c operator+ (f, c);
#endif
#ifdef SUBCC
template c operator- (c, c);
#endif
#ifdef SUBCF
template c operator- (c, f);
#endif
#ifdef SUBFC
template c operator- (f, c);
#endif
#ifdef MULCC
template c operator* (c, c);
#endif
#ifdef MULCF
template c operator* (c, f);
#endif
#ifdef MULFC
template c operator* (f, c);
#endif
#ifdef DIVCC
template c operator/ (c, c);
#endif
#ifdef DIVCF
template c operator/ (c, f);
#endif
#ifdef DIVFC
template c operator/ (f, c);
#endif
#ifdef PLUS
template c operator+ (c);
#endif
#ifdef MINUS
template c operator- (c);
#endif
#ifdef EQCC
template bool operator== (c, c);
#endif
#ifdef EQCF
template bool operator== (c, f);
#endif
#ifdef EQFC
template bool operator== (f, c);
#endif
#ifdef NECC
template bool operator!= (c, c);
#endif
#ifdef NECF
template bool operator!= (c, f);
#endif
#ifdef NEFC
template bool operator!= (f, c);
#endif
#ifdef ABS
template f abs (c);
#endif
#ifdef ARG
template f arg (c);
#endif
#ifdef POLAR
template c polar (f, f);
#endif
#ifdef CONJ
template c conj (c);
#endif
#ifdef NORM
template f norm (c);
#endif
#ifdef COS
template c cos (c);
#endif
#ifdef COSH
template c cosh (c);
#endif
#ifdef EXP
template c exp (c);
#endif
#ifdef LOG
template c log (c);
#endif
#ifdef POWCC
template c pow (c, c);
#endif
#ifdef POWCF
template c pow (c, f);
#endif
#ifdef POWCI
template c pow (c, int);
#endif
#ifdef POWFC
template c pow (f, c);
#endif
#ifdef SIN
template c sin (c);
#endif
#ifdef SINH
template c sinh (c);
#endif
#ifdef SQRT
template c sqrt (c);
#endif
#ifdef EXTRACT
template istream& operator>> (istream&, __complex<f>&);
#endif
#ifdef INSERT
template ostream& operator<< (ostream&, __complex<f>);
#endif
