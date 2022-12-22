io.nlm:		_G_config.h $(LIBIO_OBJECTS)
	-rm -rf io.nlm io.def
	echo "description \"libio\"" >> io.def
	echo "screenname \"NONE\"" >> io.def
	echo "version `echo $(VERSION) | sed 's|\.|,|g'`" >> io.def
	for obj in $(LIBIO_OBJECTS); do	\
		echo "input $${obj}";		\
	done >> io.def
	echo "output io.nlm" >> io.def
	$(NLMCONV) -l $(LD) -T io.def
	-rm -rf io.def

iostream.nlm:	_G_config.h $(LIBIOSTREAM_OBJECTS)
	-rm -rf iostream.nlm iostream.def
	echo "description \"libiostream\"" >> iostream.def
	echo "screenname \"NONE\"" >> iostream.def
	echo "version `echo $(VERSION) | sed 's|\.|,|g'`" >> iostream.def
	for obj in $(LIBIOSTREAM_OBJECTS); do	\
		echo "input $${obj}";		\
	done >> iostream.def
	echo "output iostream.nlm" >> iostream.def
	$(NLMCONV) -l $(LD) -T iostream.def
	-rm -rf iostream.def
