# Elf without shared libm -- we pass -z nodefs to the linker when using
# the shared libstdc++ so that it won't complain about missing math functions.

LIBS    = $(SHLIB)
DEPLIBS = ../libstdc++.so
LDLIBS  = -L.. -lstdc++ -Wl,-z,nodefs
MLDLIBS = -L.. -lstdc++ -lm
