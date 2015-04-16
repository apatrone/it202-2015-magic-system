CC = gcc
CFLAGS = -Wall -Wextra -I. -o
LIB_THREAD = -DUSE_PTHREAD -lpthread

SRCDIR   = src
TSTDIR   = tst
BINDIR   = bin
TSTLIST  := $(wildcard $(TSTDIR)/*.c)
BINLIST  := $(wildcard $(BINDIR)/*)
SRCTHREAD = $(SRCDIR)/thread.c

ECHOGREEN = echo -ne "\e[32m"
ECHOWHITE = echo -ne "\e[0m"


binname = `basename $(tstfile) .c`

compilewithpthread = $(CC) $(CFLAGS) $(BINDIR)/$(binname) $(tstfile) $(LIB_THREAD)
compilewithourlib = $(CC) $(CFLAGS) $(BINDIR)/$(binname) $(tstfile) $(SRCTHREAD)

inforeachwithpthread = echo "Compiling $(binname)"; $(compilewithpthread);
inforeachwithourlib = echo "Compiling $(binname)"; $(compilewithourlib);

testbinary = $(BINDIR)/$(binfile)


with-ourlib:
	@mkdir -p bin
	@$(foreach tstfile,$(TSTLIST),$(inforeachwithourlib))
	@echo "Done !"

with-pthread:
	@mkdir -p bin
	@$(foreach tstfile,$(TSTLIST),$(inforeachwithpthread))
	@echo "Done !"

test:
	@$(ECHOGREEN)
	@echo "Cleaning ..."
	@$(ECHOWHITE)
	@make -s clean

	@$(ECHOGREEN)
	@echo -e "\nCompiling ..."
	@$(ECHOWHITE)
	@#make -s with-ourlib
	@make -s with-pthread

	@$(ECHOGREEN)
	@echo -e "\nTesting ..."
	@$(ECHOWHITE)
	@$(foreach binfile,$(BINLIST),$(testbinary)))




.PHONY: clean
clean:
	@rm -f $(BINDIR)/*
	@echo "Done !"

