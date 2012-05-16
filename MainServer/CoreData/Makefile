################OPTION###################
CCOMPILE = gcc
CPPCOMPILE = $(CCACHE) g++
COMPILEOPTION = -fPIC -c -g
FRAMEWORKDIR = ../../Framework/
INCLUDEDIR = -I../ \
	-I$(FRAMEWORKDIR)/BaseCommon \
	-I$(FRAMEWORKDIR)/BaseServer \
	-I$(FRAMEWORKDIR)/BaseDB

AR = ar
OUTPUT = libCoreData.a
AR_OPTION = rcs
LIBDIRS = 
OBJS = Creature.o Role.o Npc.o Monster.o \
	CoreData.o Map.o Team.o

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
