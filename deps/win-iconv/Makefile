# Makefile for win-iconv
#
# Variables that can be overridden:
#
# CC, AR, RANLIB, DLLTOOL
# MKDIR_P, INSTALL, RM
# prefix, BINARY_PATH, INCLUDE_PATH, LIBRARY_PATH

CC ?= gcc
AR ?= ar
RANLIB ?= ranlib
DLLTOOL ?= dlltool

MKDIR_P = mkdir -p
INSTALL = install -c
RM = rm -f

# comma separated list (e.g. "iconv.dll,libiconv.dll")
DEFAULT_LIBICONV_DLL ?= \"\"

CFLAGS += -pedantic -Wall
CFLAGS += -DUSE_LIBICONV_DLL
CFLAGS += -DDEFAULT_LIBICONV_DLL=$(DEFAULT_LIBICONV_DLL)

prefix ?= /usr/local
BINARY_PATH = $(prefix)/bin
INCLUDE_PATH = $(prefix)/include
LIBRARY_PATH = $(prefix)/lib

all: iconv.dll libiconv.a win_iconv.exe

dist: test win_iconv.zip

iconv.dll: win_iconv.c
	$(CC) $(CFLAGS) -c win_iconv.c -DMAKE_DLL
	$(CC) -shared -o iconv.dll -Wl,-s -Wl,--out-implib=libiconv.dll.a -Wl,--export-all-symbols win_iconv.o $(SPECS_FLAGS)

libiconv.a: win_iconv.c
	$(CC) $(CFLAGS) -c win_iconv.c
	$(AR) rcs libiconv.a win_iconv.o
	$(RANLIB) libiconv.a

win_iconv.exe: win_iconv.c
	$(CC) $(CFLAGS) -s -o win_iconv.exe win_iconv.c -DMAKE_EXE

libmlang.a: mlang.def
	$(DLLTOOL) --kill-at --input-def mlang.def --output-lib libmlang.a

test:
	$(CC) $(CFLAGS) -s -o win_iconv_test.exe win_iconv_test.c
	./win_iconv_test.exe

win_iconv.zip: msvcrt msvcr70 msvcr71
	rm -rf win_iconv
	svn export . win_iconv
	cp msvcrt/iconv.dll msvcrt/win_iconv.exe win_iconv/
	mkdir win_iconv/msvcr70
	cp msvcr70/iconv.dll win_iconv/msvcr70/
	mkdir win_iconv/msvcr71
	cp msvcr71/iconv.dll win_iconv/msvcr71/
	zip -r win_iconv.zip win_iconv

msvcrt:
	svn export . msvcrt; \
	cd msvcrt; \
	$(MAKE);

msvcr70:
	svn export . msvcr70; \
	cd msvcr70; \
	gcc -dumpspecs | sed s/-lmsvcrt/-lmsvcr70/ > specs; \
	$(MAKE) "SPECS_FLAGS=-specs=$$PWD/specs";

msvcr71:
	svn export . msvcr71; \
	cd msvcr71; \
	gcc -dumpspecs | sed s/-lmsvcrt/-lmsvcr71/ > specs; \
	$(MAKE) "SPECS_FLAGS=-specs=$$PWD/specs";

install: iconv.dll libiconv.a win_iconv.exe
	-@$(MKDIR_P) '$(DESTDIR)$(BINARY_PATH)'
	-@$(MKDIR_P) '$(DESTDIR)$(INCLUDE_PATH)'
	-@$(MKDIR_P) '$(DESTDIR)$(LIBRARY_PATH)'
	-$(INSTALL) iconv.dll '$(DESTDIR)$(BINARY_PATH)'
	-$(INSTALL) win_iconv.exe '$(DESTDIR)$(BINARY_PATH)'
	-$(INSTALL) -m 0644 iconv.h '$(DESTDIR)$(INCLUDE_PATH)'
	-$(INSTALL) -m 0644 libiconv.dll.a '$(DESTDIR)$(LIBRARY_PATH)'
	-$(INSTALL) -m 0644 libiconv.a '$(DESTDIR)$(LIBRARY_PATH)'

uninstall:
	-$(RM) '$(DESTDIR)$(LIBRARY_PATH)'/libiconv.a
	-$(RM) '$(DESTDIR)$(LIBRARY_PATH)'/libiconv.dll.a
	-$(RM) '$(DESTDIR)$(INCLUDE_PATH)'/iconv.h
	-$(RM) '$(DESTDIR)$(BINARY_PATH)'/win_iconv.exe
	-$(RM) '$(DESTDIR)$(BINARY_PATH)'/iconv.dll

clean:
	rm -f win_iconv.exe
	rm -f win_iconv.o
	rm -f iconv.dll*
	rm -f libiconv.a
	rm -f libiconv.dll
	rm -f win_iconv_test.exe
	rm -f libmlang.a
	rm -rf win_iconv
	rm -rf win_iconv.zip
	rm -rf msvcrt
	rm -rf msvcr70
	rm -rf msvcr71

