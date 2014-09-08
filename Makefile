
EXENAME = blktool

SRC = blktool.c

INSTALL = install

prefix = /usr/local
bindir = $(prefix)/bin

CFLAGS = 

all :
	gcc -o $(EXENAME) $(SRC) $(CFLAGS)

clean :
	@rm -rf $(EXENAME)

install : $(EXENAME)
	@$(INSTALL) -m 755 -d $(bindir)
	@$(INSTALL) $(EXENAME) $(bindir)