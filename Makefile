all: FORCE
	cd src && $(MAKE) ../src_3rd/puplug/util/puplug
	cd src && $(MAKE)
	cd util && $(MAKE)
#	cd doc && $(MAKE)

test: FORCE
	cd tests && $(MAKE) test

clean: FORCE
	cd src && $(MAKE) clean
	cd util && $(MAKE) clean
#	cd doc && $(MAKE) clean
	cd tests && $(MAKE) clean
	cd src_3rd/sphash && $(MAKE) clean
	cd src_3rd/genht && $(MAKE) clean
	cd src_3rd/puplug && $(MAKE) clean
	cd src_3rd/libminuid && $(MAKE) clean ; true
	cd src_3rd/libuundo && $(MAKE) clean ; true

# Note: have to distclean utils (and tests) before src because of hidlib deps
distclean: FORCE
	$(MAKE) clean ; true
#	cd doc && $(MAKE) distclean
	cd tests && $(MAKE) distclean
	cd src && $(MAKE) distclean
	cd src_3rd/genlist && $(MAKE) clean ; true
	cd src_3rd/genregex && $(MAKE) clean ; true
	cd src_3rd/genvector && $(MAKE) clean ; true
	cd src_3rd/liblihata && $(MAKE) clean ; true
	cd src_3rd/liblihata/genht && $(MAKE) clean ; true
	cd src_3rd/qparse && $(MAKE) clean ; true
	cd src_3rd/libporty_net && $(MAKE) distclean ; true
	cd scconfig && $(MAKE) distclean ; true


install: FORCE
	cd src && $(MAKE) install
	cd util && $(MAKE) install
	cd doc && $(MAKE) install

linstall: FORCE
	cd src && $(MAKE) linstall
	cd util && $(MAKE) linstall
	cd doc && $(MAKE) linstall

uninstall: FORCE
	cd src && $(MAKE) uninstall
	cd util && $(MAKE) uninstall
	cd doc && $(MAKE) uninstall

include Makefile.conf

FORCE:
