# This is makefile is used under Linux operation system
# for builing shared library

LONG_VERSION_SVN=$(shell svnversion -nc . | sed -e s/^[^:]*:// | sed -e s/M//)
LONG_VERSION=1.1.$(LONG_VERSION_SVN)
SHORT_VERSION=1
VZ_CMD=vz_cmd
VZ_CMD_SEND=vz_cmd_send
DEBUG=-O0 -ggdb

ifeq ($(INSTALL_PATH),)
INSTALL_PATH=/usr/local/vz
endif
INSTALL_BIN=$(INSTALL_PATH)/bin
INSTALL_LIB=$(INSTALL_PATH)/lib
INSTALL_INC=$(INSTALL_PATH)/include

ifneq ($(PHP),)
PHP_INCLUDE_DIR=$(shell php-config --include-dir)
PHP_CFLAGS=-I$(PHP_INCLUDE_DIR)/Zend -I$(PHP_INCLUDE_DIR)/main -I$(PHP_INCLUDE_DIR)/TSRM -I$(PHP_INCLUDE_DIR)
PHP_MODULE=vz_php.so
endif

TARGETS=libvz_cmd.so libvz_cmd_send.so vz_cmd_sender $(PHP_MODULE)
all: $(TARGETS)
OBJS_VZ_CMD=vz_cmd_serial.o vz_cmd_shell.o vz_cmd.o
OBJS_VZ_CMD_SEND=vz_cmd_send.o
OBJS_VZ_CMD_SENDER=vz_cmd_send.o vz_cmd_sender.o vz_cmd_serial.o vz_cmd_shell.o vz_cmd.o
OBJS_VZ_PHP=vz_cmd_send.o vz_php.o vz_cmd_serial.o vz_cmd_shell.o vz_cmd.o

vz_cmd_sender: $(OBJS_VZ_CMD_SENDER)
	$(CC) -o vz_cmd_sender $(OBJS_VZ_CMD_SENDER) $(DEBUG)

libvz_cmd.so: $(OBJS_VZ_CMD)
	$(CC) -g -shared -Wl,-soname,lib$(VZ_CMD).so.$(SHORT_VERSION) -o lib$(VZ_CMD).so.$(LONG_VERSION) $(OBJS_VZ_CMD) $(DEBUG)
	ln -sf lib$(VZ_CMD).so.$(LONG_VERSION) lib$(VZ_CMD).so.$(SHORT_VERSION)
	ln -sf lib$(VZ_CMD).so.$(SHORT_VERSION) lib$(VZ_CMD).so

libvz_cmd_send.so: $(OBJS_VZ_CMD_SEND)
	$(CC) -g -shared -Wl,-soname,lib$(VZ_CMD_SEND).so.$(SHORT_VERSION) -o lib$(VZ_CMD_SEND).so.$(LONG_VERSION) $(OBJS_VZ_CMD_SEND) $(DEBUG) -l$(VZ_CMD) -L.
	ln -sf lib$(VZ_CMD_SEND).so.$(LONG_VERSION) lib$(VZ_CMD_SEND).so.$(SHORT_VERSION)
	ln -sf lib$(VZ_CMD_SEND).so.$(SHORT_VERSION) lib$(VZ_CMD_SEND).so

vz_php.so: $(OBJS_VZ_PHP)
	$(CC) -shared -rdynamic -Wall -O3 -o vz_php.so $(OBJS_VZ_PHP)


vz_cmd_serial.o: vz_cmd.h vz_cmd_serial.c vz_cmd-private.h
	$(CC) -fPIC -Wall -g -c vz_cmd_serial.c $(DEBUG)

vz_cmd_shell.o: vz_cmd.h vz_cmd_shell.c vz_cmd-private.h
	$(CC) -fPIC -Wall -g -c vz_cmd_shell.c $(DEBUG)

vz_cmd.o: vz_cmd.h vz_cmd.c vz_cmd-private.h
	$(CC) -fPIC -Wall -g -c vz_cmd.c $(DEBUG)

vz_cmd_send.o: vz_cmd_send.h vz_cmd_send.c vz_cmd.h
	$(CC) -fPIC -Wall -g -c vz_cmd_send.c $(DEBUG)

vz_cmd_sender.o: vz_cmd_send.h vz_cmd_sender.c
	$(CC) -fPIC -Wall -g -c vz_cmd_sender.c $(DEBUG) -DSVNVERSION='"$(LONG_VERSION_SVN)"'

vz_php.o: vz_php.c vz_cmd_send.h vz_cmd.h
	$(CC) -fPIC -Wall -g -c vz_php.c $(DEBUG) $(PHP_CFLAGS) -DSVNVERSION='"$(LONG_VERSION_SVN)"'

clean:
	@echo $(LONG_VERSION_SVN)
	rm -f lib$(VZ_CMD).* lib$(VZ_CMD_SEND).* *.o $(TARGETS)

install:
	mkdir -p $(INSTALL_LIB)
	install -c -m 0755 lib$(VZ_CMD).so.$(LONG_VERSION) $(INSTALL_LIB)
	install -c -m 0755 lib$(VZ_CMD_SEND).so.$(LONG_VERSION) $(INSTALL_LIB)
	ln -sf lib$(VZ_CMD).so.$(LONG_VERSION) $(INSTALL_LIB)/lib$(VZ_CMD).so.$(SHORT_VERSION)
	ln -sf lib$(VZ_CMD).so.$(SHORT_VERSION) $(INSTALL_LIB)/lib$(VZ_CMD).so
	ln -sf lib$(VZ_CMD_SEND).so.$(LONG_VERSION) $(INSTALL_LIB)/lib$(VZ_CMD_SEND).so.$(SHORT_VERSION)
	ln -sf lib$(VZ_CMD_SEND).so.$(SHORT_VERSION) $(INSTALL_LIB)/lib$(VZ_CMD_SEND).so
	mkdir -p $(INSTALL_INC)
	install -c -m 0644 vz_cmd.h $(INSTALL_INC)
	install -c -m 0644 vz_cmd_send.h $(INSTALL_INC)
	mkdir -p $(INSTALL_BIN)
	install -c -m 0755 vz_cmd_sender $(INSTALL_BIN)
