################OPTION###################
CCOMPILE = gcc
CPPCOMPILE = $(CCACHE) g++
COMPILEOPTION = -fPIC -c -g

FRAMEWORKDIR = ../../Framework/
INCLUDEDIR = -I../ \
	-I$(FRAMEWORKDIR)/BaseCommon \
	-I$(FRAMEWORKDIR)/BaseServer \
	-I$(FRAMEWORKDIR)/BaseDB \
	-I../CoreData \

AR = ar
OUTPUT = libAvatar.a
AR_OPTION = rcs
LIBDIRS = 
OBJS = AvatarSvc.o

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
