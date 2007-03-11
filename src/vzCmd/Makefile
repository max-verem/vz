# This is makefile is used under Linux operation system
# for builing shared library

LONG_VERSION_SVN=$(shell svnversion -nc . | sed -e s/^[^:]*:// | sed -e s/M//)
LONG_VERSION=1.1.$(LONG_VERSION_SVN)
SHORT_VERSION=1
THIS_LIB=vz_cmd

ifeq ($(INSTALL_PATH),)
INSTALL_PATH=/usr/local
endif
INSTALL_LIB=$(INSTALL_PATH)/lib
INSTALL_INC=$(INSTALL_PATH)/include

all:	lib$(THIS_LIB).so
OBJS=vz_cmd_serial.o vz_cmd_shell.o vz_cmd.o 

libvz_cmd.so: $(OBJS)
	$(CC) -g -shared -Wl,-soname,lib$(THIS_LIB).so.$(SHORT_VERSION) -o lib$(THIS_LIB).so.$(LONG_VERSION) $(OBJS) $(DEBUG)
	ln -sf lib$(THIS_LIB).so.$(LONG_VERSION) lib$(THIS_LIB).so.$(SHORT_VERSION)
	ln -sf lib$(THIS_LIB).so.$(SHORT_VERSION) lib$(THIS_LIB).so
    
vz_cmd_serial.o: vz_cmd.h vz_cmd_serial.c vz_cmd-private.h
	$(CC) -fPIC -Wall -g -c vz_cmd_serial.c $(DEBUG)

vz_cmd_shell.o: vz_cmd.h vz_cmd_shell.c vz_cmd-private.h
	$(CC) -fPIC -Wall -g -c vz_cmd_shell.c $(DEBUG)

vz_cmd.o: vz_cmd.h vz_cmd.c vz_cmd-private.h
	$(CC) -fPIC -Wall -g -c vz_cmd.c $(DEBUG)

clean:
	@echo $(LONG_VERSION_SVN)
	rm -f lib$(THIS_LIB).* $(OBJS)

install:
	mkdir -p $(INSTALL_LIB)
	install -c -m 0755 lib$(THIS_LIB).so.$(LONG_VERSION) $(INSTALL_LIB)
	ln -sf lib$(THIS_LIB).so.$(LONG_VERSION) $(INSTALL_LIB)/lib$(THIS_LIB).so.$(SHORT_VERSION)
	ln -sf lib$(THIS_LIB).so.$(SHORT_VERSION) $(INSTALL_LIB)/lib$(THIS_LIB).so
	mkdir -p $(INSTALL_INC)
	install -c -m 0644 vz_cmd.h $(INSTALL_INC)
