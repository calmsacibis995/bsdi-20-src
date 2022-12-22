SHLIB      = libg++.so.2.6
BUILD_LIBS = $(SHLIB)
LIBS       = -L./$(TOLIBGXX) -lg++ -lcurses -ltermcap -Wl,-assert,nodefinitions
SHFLAGS    = $(LIBCFLAGS)
