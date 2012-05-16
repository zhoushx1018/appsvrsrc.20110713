################OPTION###################

CC = $(CCACHE) g++
INSTALL_DIR=~/run/gameServer
LOG_DIR = $(INSTALL_DIR)/log
MAP_DIR = $(INSTALL_DIR)/map

CFLAGS = -Wall -g -pipe -D_NEW_LIC -D_GNU_SOURCE -fPIC -D_REENTRANT $(SRCDEBUG)

LIB = -lpthread -lrt

LDFLAGS =  $(CFLAGS) $(INCLUDE) $(LIBDIR) $(LIB)

OO = PK_PE_base.o PK_thread.o log.o common.o net.o ini_file.o PK_main_thread.o

BIN = pkServer

all: $(OO) $(BIN)
.cpp.o:
	$(CC)  $(CFLAGS) -c  -I./ $(INCLUDE) $<

%:%.cpp $(OO) $(DEP_LIB)
	$(CC) $(CFLAGS) -o $@ $< $(OO) $(LDFLAGS)

clean:
	rm -f *.o
	rm -f $(BIN)
	
installbin:
	make
	cp $(BIN) $(INSTALL_DIR)

install:
	make
	mkdir -p $(LOG_DIR)
	mkdir -p $(MAP_DIR)	
	cp $(BIN) $(INSTALL_DIR) 
	cp ./pkServer.ini  $(INSTALL_DIR)
	#cp ./map/* $(INSTALL_DIR)/map/

