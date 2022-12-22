# Elf with shared libm, so we can link it into the shared libstdc++.

LIBS    = $(SHLIB)
DEPLIBS = ../libstdc++.so
SHDEPS  = -lm
