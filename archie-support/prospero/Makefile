#
# Copyright (c) 1991-1994 by the University of Southern California
#
# For copying and distribution information, please see the file
# <usc-license.h>.
#
#
# This is the top level Makefile for the Prospero Directory Service.

# Include configuration options.
SOURCEBASE = .
include Makefile.config

# Programs that live in subdirectories, and have makefiles of their own.
# The ordering is important here; libraries are built before the parts of 
# Prospero that have to be linked with them are.

# put OPTIONAL_LIB_SUBDIRS at the end here, because lib/psrv/wais_gw contains
# executables that are needed by WAIS.
LIB_SUBDIRS = \
	lib/pfs \
	lib/ardp \
	lib/psrv \
	lib/filters \
        $(OPTIONAL_LIB_SUBDIRS)

BIN_SUBDIRS =  \
        $(OPTIONAL_BIN_SUBDIRS) \
	server \
        user

SUBDIRS = \
        $(LIB_SUBDIRS) \
	$(BIN_SUBDIRS)

all: all_subdirs

install: installdirs installfiles

installdirs:
	-mkdir $(DIR755) $(DIR1777)
	-chown $(OWNER)  $(DIR755) $(DIR1777)
	-chgrp $(GROUP)  $(DIR755) $(DIR1777)
	-chmod 755 $(DIR755)
	-chmod 1777 $(DIR1777)

# alternative version of above if you have a good version of (INSTALL) available:
#-for i in $(DIR755) ; \
#       do $(INSTALL) -d -o $(OWNER) -g $(GROUP) -m 755 $$i ;\
#	done
#-for i in $(DIR1777) ; \
#       do $(INSTALL) -d -o $(OWNER) -g $(GROUP) -m 1777 $$i ;\
#       done

installfiles:
	-for i in $(BINSUBDIRS); do \
	    (cd $$i; \
	    $(MAKE) $(PASTHRU) install); \
	    done


# Dependencies




