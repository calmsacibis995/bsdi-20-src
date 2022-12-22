# HPUX uses the .sl suffix for shared libraries.

SHLIB   = libstdc++.sl
LIBS    = $(SHLIB)
DEPLIBS = ../libstdc++.sl
SHFLAGS = $(LIBCXXFLAGS)
