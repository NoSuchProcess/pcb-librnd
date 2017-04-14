#!/bin/sh

# Run this script after changing or adding .pup files.
# WARNING: this script requires a fair amount of tools - it is intended to
#          run on developers' machines, not on users'

export LANG=C
PUPLUG=../src_3rd/puplug/util/puplug


# generate scconfig's 3 state plugin list
$PUPLUG findpups . '%$class$|%N|%3|%A|%$short$\n' | sed '
s/^lib/1|lib/
s/^feature/2|feature/
s/^fp/3|fp/
s/^import/4|import/
s/^export/5|export/
s/^io/6|io/
s/^hid/7|hid/
' | sort | awk -F "[|]" '
BEGIN {
	HDR["lib"] = "Library plugins"
	HDR["feature"] = "Feature plugins"
	HDR["fp"] = "Footprint backends"
	HDR["import"] = "Import plugins"
	HDR["export"] = "Import plugins"
	HDR["io"] = "IO plugins (file formats)"
	HDR["hid"] = "HID plugins"
	print "/******************************************************************************"
	print " Auto-generated by trunk/src_plugins/map_plugins.sh - do NOT edit,"
	print " run make map_plugins in trunk/src/ - to change any of the data below,"
	print " edit trunk/src_plugins/PLUGIN/PLUGIN.pup"
	print "******************************************************************************/"
}

function q(s) { return "\"" s "\"," }

($2 != last) {
	print "\nplugin_header(\"\\n" HDR[$2] ":\\n\")"
	last = $2
}

{
	printf("plugin_def(%-20s%-35s%-10s%s)\n", q($3), q($6), $4 "," , $5)
}

END { print "\n" }

' > ../scconfig/plugins.h

$PUPLUG findpups . "" 'plugin_dep("%N", "%m")\n' >> ../scconfig/plugins.h


# Generate the plugin list
echo "# List of all plugins - generated by make map_plugins - do NOT edit" > plugins_ALL.tmpasm
$PUPLUG findpups . "include {../src_plugins/%D/Plug.tmpasm}\n" | sort >> plugins_ALL.tmpasm
