################OPTION###################
CCOMPILE = gcc
CPPCOMPILE = $(CCACHE) g++
COMPILEOPTION = -fPIC -c -g
INCLUDEDIR = 
AR = ar
OUTPUT = libBaseCommon.a
AR_OPTION = rcs
LIBDIRS = 
OBJS = Log.o

SHAREDLIB = 
APPENDLIB = 
$(OUTPUT): $(OBJS) 
	$(AR) $(AR_OPTION) $(OUTPUT) $(LIBDIRS) $(OBJS) 

clean: 
	rm -f $(OBJS)
	rm -f $(OUTPUT)

all: clean $(OUTPUT)
.cpp.o:
	$(CPPCOMPILE) -c -o $*.o $(COMPILEOPTION) $(INCLUDEDIR)  $*.cpp $(SRCDEBUG)
