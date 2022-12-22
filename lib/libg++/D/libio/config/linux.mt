# Since the Linux C library has libio, we have to be very careful.

# We have _G_config.h in /usr/include.
_G_CONFIG_H=

# We have those in libc.a.
IO_OBJECTS=
STDIO_WRAP_OBJECTS=
OSPRIM_OBJECTS=

# We have the rest in /usr/include.
USER_INCLUDES=PlotFile.h SFile.h builtinbuf.h editbuf.h fstream.h \
	indstream.h iomanip.h iostream.h istream.h ostream.h \
	parsestream.h pfstream.h procbuf.h stdiostream.h stream.h \
	streambuf.h strfile.h strstream.h
