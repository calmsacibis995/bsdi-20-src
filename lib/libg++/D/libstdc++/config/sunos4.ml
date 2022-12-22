# SunOS doesn't provide a shared libm -- we pass -assert nodefinitions to the
# linker when using the shared libstdc++ so it won't complain about missing
# math functions.  SunOS also requires a version number in shared library
# filenames.

SHLIB   = libstdc++.so.2.6
LIBS    = $(SHLIB)
DEPLIBS = ../libstdc++.so.2.6
LDLIBS	= -L.. -lstdc++ -Wl,-assert,nodefinitions
MLDLIBS = -L.. -lstdc++ -lm
SHFLAGS	= $(LIBCFLAGS)
