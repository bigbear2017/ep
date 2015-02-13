OUTPUT=$(patsubst %.cpp,%, $(wildcard *.cpp))

WORKROOT = ../../..

ULIB=$(WORKROOT)/lib2

PUBLIC=$(WORKROOT)/public

HARDWARE_PLATFORM := $(shell uname -m)
ifeq ($(HARDWARE_PLATFORM),x86_64)	
	ULIB = $(WORKROOT)/lib2-64
endif


ULIBPATH=$(ULIB)/ullib
EPDPOOL=$(PUBLIC)/ependingpool
NSHEAD=$(PUBLIC)/nshead

GCC = g++

CPPFLAGS = -g -finline-functions -Wall -Winline -pipe $(INCLUDES)

INCLUDES = -I$(NSHEAD)/ -I$(ULIBPATH)/include -I$(EPDPOOL)/

LDFLAGS = -L$(NSHEAD) -lnshead -L$(EPDPOOL) -lependingpool -L$(ULIBPATH)/lib -lullib -lm -lcrypto -lpthread

all	: $(OUTPUT) 
	if [ ! -d output ]; then mkdir output; fi
	cp $(OUTPUT) output
	rm -f *.o

$(OUTPUT): %: %.o
	$(GCC) -o $@ $< $(INCLUDES) $(LDFLAGS)  
%.o	: %.cpp
	$(GCC) $(CPPFLAGS) -c $< -o $@ $(INCLUDES) 

clean:
	rm -f *.o $(OUTPUT)
	rm -rf output
