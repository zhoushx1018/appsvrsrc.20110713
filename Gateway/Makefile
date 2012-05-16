

CPPCOMPILE = $(CCACHE) g++
INSTALL_DIR=~/run/gameServer
LOG_DIR = $(INSTALL_DIR)/log

INCLUDEDIR = -I../Framework/BaseCommon
LIBDIR = -L../Framework/BaseCommon 
APPENDLIB = -lpthread -lrt -lBaseCommon
DEP_LIB = 

CFLAGS = -Wall -g -pipe -D_NEW_LIC -D_GNU_SOURCE -fPIC -D_REENTRANT $(SRCDEBUG)

LDFLAGS =  $(CFLAGS) $(INCLUDEDIR) $(LIBDIR) $(APPENDLIB)

OO = ConnectSession.o AppGwSvc.o System.o Option.o PkgBuff.o PkgHead.o

BIN = gateway

all: $(OO) $(BIN)
.cpp.o:
	$(CPPCOMPILE)  $(CFLAGS) -c  $(INCLUDEDIR) $<

%:%.cpp $(OO) $(DEP_LIB)
	$(CPPCOMPILE) $(CFLAGS) -o $@ $< $(OO) $(LDFLAGS)

clean:
	rm -f *.o
	rm -f $(BIN)
	
installbin:
	make
	cp $(BIN) $(INSTALL_DIR)

install:
	make
	mkdir -p $(LOG_DIR)
	cp $(BIN) $(INSTALL_DIR) 
	cp ./gateway.ini  $(INSTALL_DIR)
	