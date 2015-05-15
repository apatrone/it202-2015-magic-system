CC = gcc
CFLAGS = -Wall -Wextra -I. -g3 -lrt
VALGRINDFLAGS = --db-attach=yes -v --leak-check=yes --track-origins=yes --show-reachable=yes
LIB_THREAD = -DUSE_PTHREAD -lpthread

SRCDIR   = src
TSTDIR   = tst
BASEBINDIR   = bin
PTHREADBINDIR = $(BASEBINDIR)/pthread
OURLIBBINDIR = $(BASEBINDIR)/ourlib
BINWITHARGS = "01-main" "02-switch" "11-join" "12-join-main" "21-create-many 5" "22-create-many-recursive 5" "23-create-many-once 5" "31-switch-many 5 23" "32-switch-many-joini 5 23" "51-fibonacci 4"

TSTLIST  := $(wildcard $(TSTDIR)/*.c)
SRCTHREAD = $(SRCDIR)/thread.c


ECHOGREEN = echo -ne "\e[32m"
ECHOBLUE = echo -ne "\e[34m"
ECHOWHITE = echo -ne "\e[0m"


binname = `basename $(tstfile) .c`
ourbin = $(OURLIBBINDIR)/$(binname)

compilewithpthread = $(CC) $(CFLAGS) -o $(PTHREADBINDIR)/$(binname) $(tstfile) $(LIB_THREAD)
compilewithourlib = $(CC) $(CFLAGS) -o $(OURLIBBINDIR)/$(binname) $(tstfile) $(SRCTHREAD)

inforeachwithpthread = echo "Compiling $(binname)"; $(compilewithpthread);
inforeachwithourlib = echo "Compiling $(binname)"; $(compilewithourlib);

shortbinarytest = $(ECHOBLUE); echo "testing $(binname)"; $(ECHOWHITE); valgrind $(OURLIBBINDIR)/$(binname) $(args) 2>&1 >/dev/null | grep "ERROR SUMMARY" | cut -c25-1000 ;

fullbinarytest = valgrind $(VALGRINDFLAGS) $(OURLIBBINDIR)/






with-ourlib:
	@$(ECHOGREEN)
	@echo -e "\nCompiling with our lib ..."
	@$(ECHOWHITE)
	@mkdir -p bin/ourlib
	@$(foreach tstfile,$(TSTLIST),$(inforeachwithourlib))
	@echo "Done !"


all:
	@make -s with-pthread
	@make -s with-ourlib


with-pthread:
	@$(ECHOGREEN)
	@echo -e "Compiling with pthread lib ..."
	@$(ECHOWHITE)
	@mkdir -p bin/pthread
	@$(foreach tstfile,$(TSTLIST),$(inforeachwithpthread))
	@echo "Done !"


test-all:
	@$(ECHOGREEN)
	@echo -e "\nTesting ..."
	@$(ECHOWHITE)

	@$(eval binname := "01-main")
	@$(eval args := "")
	@$(shortbinarytest)

	@$(eval binname := "02-switch")
	@$(eval args := "")
	@$(shortbinarytest)

	@$(eval binname := "11-join")
	@$(eval args := "")
	@$(shortbinarytest)

	@$(eval binname := "12-join-main")
	@$(eval args := "")
	@$(shortbinarytest)

	@$(eval binname := "21-create-many")
	@$(eval args := "5")
	@$(shortbinarytest)

	@$(eval binname := "22-create-many-recursive")
	@$(eval args := "5")
	@$(shortbinarytest)

	@$(eval binname := "23-create-many-once")
	@$(eval args := "5")
	@$(shortbinarytest)

	@$(eval binname := "31-switch-many")
	@$(eval args := "5 23")
	@$(shortbinarytest)

	@$(eval binname := "32-switch-many-join")
	@$(eval args := "5 23")
	@$(shortbinarytest)

	@$(eval binname := "51-fibonacci")
	@$(eval args := "4")
	@$(shortbinarytest)
	@echo "Done !"


test:
	@$(fullbinarytest)$(bin) $(arg)


.PHONY: clean
clean:
	@$(ECHOGREEN)
	@echo "Cleaning ..."
	@$(ECHOWHITE)
	@$(foreach tstfile,$(TSTLIST),rm -f {$(OURLIBBINDIR),$(PTHREADBINDIR)}/$(binname))
	@echo "Done !"


.PHONY: help
help:
	@$(ECHOBLUE)
	@echo help
	@$(ECHOWHITE)
	@echo -e "\tPrint help (this message) and exit"

	@$(ECHOBLUE)
	@echo all
	@$(ECHOWHITE)
	@echo -e "\tCompile all the test files with our lib and the pthread lib"

	@$(ECHOBLUE)
	@echo with-ourlib
	@$(ECHOWHITE)
	@echo -e "\tCompile all the test files with our lib"

	@$(ECHOBLUE)
	@echo with-pthread
	@$(ECHOWHITE)
	@echo -e "\tCompile all the test files with the pthread lib"

	@$(ECHOBLUE)
	@echo clean
	@$(ECHOWHITE)
	@echo -e "\tRemove generated binaries"

	@$(ECHOBLUE)
	@echo test-all
	@$(ECHOWHITE)
	@echo -e "\tQuick test every binary compiled with our library"

	@$(ECHOBLUE)
	@echo "test bin=\$$bin arg=\$$arg"
	@$(ECHOWHITE)
	@echo -e "\tPerform a deep test on the given binary with the given argument (do not write 'arg=' if the binary do not need an argument)"
	@echo -e "\t Example: 'make test bin=21-create-many arg=5'"
