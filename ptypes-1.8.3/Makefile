#
#
#  C++ Portable Types Library (PTypes)
#  Version 1.8.3  Released 25-Aug-2003
#
#  Copyright (c) 2001, 2002, 2003 Hovik Melikyan
#
#  http://www.melikyan.com/ptypes/
#  http://ptypes.sourceforge.net/
#
#
#
# Makefile for all Unix platforms, places the library in lib/,
# the shared library in so/. Also builds the test program 
# src/ptypes_test and the sample program bin/wshare.
#

UMAKEFILE=Makefile.`uname|sed 's|/|_|'`
TEMPDIRS=st mt

all:
	cd src ; mkdir -p $(TEMPDIRS) ; make -f $(UMAKEFILE) all
	cd wshare ; make -f $(UMAKEFILE) all

clean:
	cd src ; make -f $(UMAKEFILE) clean ; rm -rf $(TEMPDIRS)
	cd wshare ; make -f $(UMAKEFILE) clean

clean-src:
	cd src ; make -f $(UMAKEFILE) clean-src ; rm -rf $(TEMPDIRS)
	cd wshare ; make -f $(UMAKEFILE) clean-src

