
#CCOMPILE = gcc
CPPCOMPILE = $(CCACHE) g++
INSTALL_DIR=~/run/gameServer
LOG_DIR = $(INSTALL_DIR)/log
COMPILEOPTION = -c -g $(SRCDEBUG)
INCLUDEDIR = -I../Framework/BaseCommon/ \
	-I../Framework/BaseServer/ \
	-I../Framework/BaseDB/ \
	-I./Account \
	-I./Avatar \
	-I./Task \
	-I./Friend \
	-I./Trade \
	-I./Mail \
	-I./Pet \
	-I./Npc \
	-I./Bag \
	-I./Chat \
	-I./StoreBag \
	-I./Shop \
	-I./Team \
	-I./PK \
	-I./Login \
	-I./Map \
	-I./RoleInfo \
	-I./OffLineUpdate \
	-I./Guide \
	-I./Delegate \
	-I./CoreData
	
	
DEP_LIB = ../Framework/BaseCommon/libBaseCommon.a  \
	../Framework/BaseServer/libBaseServer.a \
	../Framework/BaseDB/libBaseDB.a \
	./Account/libAccount.a \
	./Avatar/libAvatar.a \
	./Task/libTask.a \
	./Pet/libPet.a \
	./Npc/libNpc.a \
	./Bag/libBag.a \
	./Chat/libChat.a \
	./StoreBag/libStoreBag.a \
	./Shop/libShop.a \
	./Team/libTeam.a \
	./PK/libPK.a \
	./Login/libLogin.a \
	./Map/libMap.a \
	./RoleInfo/libRoleInfo.a \
	./OffLineUpdate/libOffLineUpdate.a \
	./Trade/libTrade.a \
	./Guide/libGuide.a \
	./Delegate/libDelegate.a \
	./CoreData/libCoreData.a 
	
	
BIN = mainServer
APPENDLIB = -lpthread \
	-lBaseCommon \
	-lAccount \
	-lAvatar \
	-lTask \
	-lFriend \
	-lTrade \
	-lMail \
	-lOffLineUpdate \
	-lGuide \
	-lDelegate \
	-lNpc \
	-lBag \
	-lChat \
	-lStoreBag \
	-lShop \
	-lTeam \
	-lPet \
	-lPK \
	-lLogin \
	-lMap \
	-lRoleInfo \
	-lCoreData \
	-lBaseServer \
	-lBaseDB \
	-lmysqlclient_r \
	-llua \
	-ldl \
	-ljson
	
APPENDOBJS = 

LINK = $(CCACHE) g++
LINKOPTION = -g -o $(BIN)
LIBDIRS = -L../Framework/BaseCommon/ \
	-L../Framework/BaseServer/ \
	-L../Framework/BaseDB/ \
	-L./Account/ \
	-L./Avatar/ \
	-L./Task/ \
	-L./Friend/ \
	-L./Trade/ \
	-L./Mail/ \
	-L./OffLineUpdate/ \
	-L./Guide/ \
	-L./Delegate/ \
	-L./Npc/ \
	-L./Pet/ \
	-L./Bag/ \
	-L./Chat/ \
	-L./StoreBag/ \
	-L./Shop/ \
	-L./Team/ \
	-L./PK/ \
	-L./Login/ \
	-L./Map/ \
	-L./RoleInfo/ \
	-L./CoreData/ \
	-L/usr/local/mysql/lib/mysql/
OBJS = MainSvc.o mainServer.o MainSvcSS.o SSClientManager.o  Account.o

SERVERFLAGFILE = ./___ServerModual___

all: $(SERVERFLAGFILE) $(BIN) $(OBJS) 

$(SERVERFLAGFILE):
	@cd ./CoreData; make
	@cd ./Account; make
	@cd ./Avatar; make
	@cd ./Login; make
	@cd ./Map; make
	@cd ./RoleInfo; make
	@cd ./Task; make
	@cd ./Npc; make
	@cd ./Bag; make
	@cd ./Pet; make
	@cd ./PK; make
	@cd ./Chat; make
	@cd ./StoreBag; make
	@cd ./Shop; make
	@cd ./Team; make
	@cd ./Friend; make
	@cd ./Trade; make
	@cd ./Mail; make
	@cd ./OffLineUpdate; make
	@cd ./Guide; make
	@cd ./Delegate; make

.cpp.o:
	$(CPPCOMPILE) -c -o $*.o $(COMPILEOPTION) $(INCLUDEDIR)  $*.cpp
	
$(BIN): $(OBJS) $(DEP_LIB)
	$(LINK) $(LINKOPTION) $(LIBDIRS) $(OBJS) $(APPENDLIB) $(INCLUDEDIR) $(APPENDOBJS)



clean:
	@cd ./CoreData; make clean
	@cd ./Avatar; make clean
	@cd ./Account; make clean
	@cd ./Login; make clean
	@cd ./Map; make clean
	@cd ./RoleInfo; make clean
	@cd ./Task; make clean
	@cd ./Npc; make clean
	@cd ./Bag; make clean
	@cd ./Pet; make clean
	@cd ./PK; make clean
	@cd ./Chat; make clean
	@cd ./StoreBag; make clean
	@cd ./Shop; make clean
	@cd ./Team; make clean
	@cd ./Friend; make clean
	@cd ./Trade; make clean
	@cd ./Mail; make clean
	@cd ./OffLineUpdate; make clean
	@cd ./Guide; make clean
	@cd ./Delegate; make clean
	
	rm -f $(OBJS)
	rm -f $(BIN)

installbin:
	make
	cp $(BIN) $(INSTALL_DIR)

install:
	make
	mkdir -p $(LOG_DIR)
	cp $(BIN) $(INSTALL_DIR) 
	cp ./mainServer.ini  $(INSTALL_DIR) 
	cp ./filters.txt  $(INSTALL_DIR) 

