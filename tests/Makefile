test:
	cd rnd_printf && $(MAKE) all && $(MAKE) test
	cd remote && $(MAKE) all && $(MAKE) test
# This uses make instead of $(MAKE) on purpose as a "portable" -j1 to work around GNU make
	cd librnd && make all && make test
	cd conf && $(MAKE) all && make test
	cd base64 && $(MAKE) all && make test
	@echo " "
	@echo "+-------------------------------------------------+"
	@echo "+  All tests passed, librnd is safe to install.   +"
	@echo "+-------------------------------------------------+"

clean:
	cd rnd_printf && $(MAKE) clean
	cd remote && $(MAKE) clean
	cd librnd && $(MAKE) clean
	cd conf && $(MAKE) clean
	cd base64 && $(MAKE) clean

distclean:
	cd librnd && $(MAKE) distclean
