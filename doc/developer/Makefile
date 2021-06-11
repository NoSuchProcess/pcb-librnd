ROOT=../..
DEVDIR=$(DOCDIR)/developer

all:

install_all:
	$(SCCBOX) mkdir -p $(DEVDIR)/hid_remote
	$(SCCBOX) $(HOW) hid_remote/*.html hid_remote/*.svg $(DEVDIR)/hid_remote/
	$(SCCBOX) $(HOWF) packaging/librnd_packages.sh $(DATADIR)/librnd_packages.sh

install:
	$(MAKE) install_all HOW="install -f -d" HOWF="install -f"

linstall:
	$(MAKE) install_all HOW="install -f -l -d" HOWF="install -f -l"

uninstall:
	$(MAKE) install_all HOW="install -u" HOWF="install -u"

clean:

distclean:


include $(ROOT)/Makefile.conf
