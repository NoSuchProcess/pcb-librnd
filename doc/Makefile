ROOT=..

all:
	ROOT="" ./Autostyle.sh *.html

include ../Makefile.conf

install_main:
	$(SCCBOX) $(HOW) *.html *.txt TODO $(DOCDIR)/

install:
	$(SCCBOX) mkdir -p "$(DOCDIR)"
	cd conf && $(MAKE) install
	cd developer && $(MAKE) install
	$(MAKE) install_main HOW="install -f -d"

linstall:
	cd conf && $(MAKE) linstall
	cd developer && $(MAKE) linstall
	$(MAKE) install_main HOW="install -f -l -d"

uninstall:
	cd conf && $(MAKE) uninstall
	cd developer && $(MAKE) uninstall
	$(MAKE) install_main HOW="install -f -u -d"

clean:
	cd conf && $(MAKE) clean
	cd developer && $(MAKE) clean

distclean:
	cd conf && $(MAKE) distclean
	cd developer && $(MAKE) distclean
