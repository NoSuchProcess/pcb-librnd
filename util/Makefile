# plain old hand crafted Makefile
ROOT=..

all:

include $(ROOT)/Makefile.conf

DEVHLP=$(LIBDIR)/devhelpers

clean:

distclean:

install_all:
	$(SCCBOX) mkdir -p  "$(DEVHLP)"
	$(SCCBOX) $(HOW) devhelpers/awk_on_formats.sh "$(DEVHLP)/awk_on_formats.sh"
	$(SCCBOX) $(HOW) devhelpers/list_dialogs.sh "$(DEVHLP)/list_dialogs.sh"
	$(SCCBOX) $(HOW) devhelpers/font2c.sh "$(DEVHLP)/font2c.sh"

install:
	$(MAKE) install_all HOW="install -f"

linstall:
	$(MAKE) install_all HOW="linstall -f"

uninstall:
	$(MAKE) install_all HOW="uninstall"

