#
#       !!!! Do NOT edit this makefile with an editor which replace tabs by spaces !!!!   
#
##############################################################################################
#
# On command line:
#
# make all = Create project
#
# make clean = Clean project files.
#
# To rebuild project do "make clean" and "make all".
#
# Included originally in the yagarto projects. Original Author : Michael Fischer
# Modified to suit our purposes by Hussam Al-Hertani
# Use at your own risk!!!!!
##############################################################################################
# Start of default section
#
INSTALL=install

ifdef PROJECT_VERSION
override PROJECT_VERSION := -DPROJECT_VERSION=$(PROJECT_VERSION)
endif


LIB_FILES = unipiid unipihostname uhelper sw_485 bootcount
SBIN_FILES = os-configurator switchboot
PY_FILES = os-configurator.py

INCDIRS += ../unipi-hardware-id/include

INCDIR  = $(patsubst %,-I%, $(INCDIRS))
#CPFLAGS = -Wall -Wstrict-prototypes

all: $(LIB_FILES) $(PY_FILES)

%.o: %.c
	$(CC) -c $(CPFLAGS) -I . $(INCDIR) $(PROJECT_VERSION) $< -o $@

sw_485: sw_485.o
	$(CC) $< -o $@

unipiid:  unipiid.o unipiutil.o
	$(CC) unipiid.o unipiutil.o -o $@
	@chmod +x $@

uhelper:  uhelper.o
	$(CC) $< -o $@

unipihostname: unipiid
	@ln -s $< $@

bootcount:  bootcount.o
	$(CC) $< -o $@
	@chmod +x $@

install:
	$(INSTALL) -D $(LIB_FILES) $(PY_FILES) -t $(DESTDIR)/usr/lib/unipi
	$(INSTALL) -D $(SBIN_FILES) -t $(DESTDIR)/usr/sbin
	@mkdir -p $(DESTDIR)/etc/unipi

clean:
	-rm -f *.o
	-rm -f bootcount unipihostname uhelper unipiid sw_485

