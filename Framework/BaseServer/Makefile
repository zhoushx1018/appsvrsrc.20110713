################OPTION###################
CCOMPILE = gcc
CPPCOMPILE = $(CCACHE) g++
COMPILEOPTION = -fPIC -c -g
INCLUDEDIR = -I../BaseCommon
AR = ar
OUTPUT = libBaseServer.a
AR_OPTION = rcs
LIBDIRS = 
OBJS = BaseArchive.o  Packet.o Serializer.o Session.o DateTime.o GWProxy.o \
	SSClient.o LuaState.o Timer.o NewTimer.o TimerManager.o

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
