
INSTALL_DIR=~/run/gameServer

all:srv

srv:
	@cd ./Framework; make
	@cd ./Gateway; make
	@cd ./MainServer; make
	@cd ./PkServer; make
		

clean:
	@cd ./Framework; make clean
	@cd ./Gateway; make clean
	@cd ./MainServer; make clean
	@cd ./PkServer; make clean
	


install:
	make srv
	@cd ./Gateway; make install
	@cd ./MainServer; make install
	@cd ./PkServer; make install
	
	cp ./Shell/*  $(INSTALL_DIR)
	dos2unix $(INSTALL_DIR)/ka $(INSTALL_DIR)/psall $(INSTALL_DIR)/sa
	dos2unix $(INSTALL_DIR)/*.ini
	
installbin:
	@cd ./Gateway; make installbin
	@cd ./MainServer; make installbin
	@cd ./PkServer; make installbin
	

dist:
	make clean
	tar -zcvf ~/bak/appsvrsrc.`date +%Y%m%d`.tar.gz ./
	du -sh
