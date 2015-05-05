CC = gcc
CFLAGS = -Wall -Wextra -I. -g3
LIB_THREAD = -DUSE_PTHREAD -lpthread

SRCDIR   = src
TSTDIR   = tst
BASEBINDIR   = bin
PTHREADBINDIR = $(BASEBINDIR)/pthread
OURLIBBINDIR = $(BASEBINDIR)/ourlib
TSTLIST  := $(wildcard $(TSTDIR)/*.c)
SRCTHREAD = $(SRCDIR)/thread.c

ECHOGREEN = echo -ne "\e[32m"
ECHOWHITE = echo -ne "\e[0m"


binname = `basename $(tstfile) .c`

compilewithpthread = $(CC) $(CFLAGS) -o $(PTHREADBINDIR)/$(binname) $(tstfile) $(LIB_THREAD)
compilewithourlib = $(CC) $(CFLAGS) -o $(OURLIBBINDIR)/$(binname) $(tstfile) $(SRCTHREAD)

inforeachwithpthread = echo "Compiling $(binname)"; $(compilewithpthread);
inforeachwithourlib = echo "Compiling $(binname)"; $(compilewithourlib);


CURRENTOURLIBOUTPUT = /tmp/currentourliboutput.txt
CURRENTPTHREADOUTPUT = /tmp/currentpthreadoutput.txt

testbinary = $(BINDIR)/$(binfile)


all:
	@$(ECHOGREEN)
	@echo -e "Compiling with pthread lib ..."
	@$(ECHOWHITE)
	@make -s with-pthread
	@$(ECHOGREEN)
	@echo -e "\nCompiling with our lib ..."
	@$(ECHOWHITE)
	@make -s with-ourlib

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

	@make -s all

	@$(ECHOGREEN)
	@echo -e "\nTesting ..."
	@$(ECHOWHITE)
	#@$(foreach binfile,$(BINLIST),$(testbinary)))
	@$(OURLIBBINDIR)/01-main > $(CURRENTOURLIBOUTPUT)
	@$(PTHREADBINDIR)/01-main > $(CURRENTPTHREADOUTPUT)
	
	@diff -q $(CURRENTPTHREADOUTPUT) $(CURRENTOURLIBOUTPUT) 1>/dev/null
	#@[[ $? = "0" ]] && echo "same" || echo "different"

	#if [[ $? == "0" ]]
	#then
		#echo "The same"
	#else
		#echo "Not the same"
	#fi




.PHONY: clean
clean:
	@$(foreach tstfile,$(TSTLIST),rm -f {$(OURLIBBINDIR),$(PTHREADBINDIR)}/$(binname))
	@echo "Done !"

