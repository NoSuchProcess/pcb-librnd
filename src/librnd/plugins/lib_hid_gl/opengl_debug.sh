#!/bin/sh

cp /usr/include/GL/gl*.h /tmp
indent -l10000 /tmp/gl*.h


(
echo "@@@@src"
grep "gl[A-Z].*[(]" *.[chly] | sed "s/^.*gl/gl/;s/[(].*//"
echo "@@@@end"

echo "@@@@macro"
grep "^#define gl[A-Z]" opengl_debug.h  | sed "s/^.*#define[ \t]*//;s/[(].*//"
echo "@@@@end"

echo "@@@@glapi"
grep "GLAPI" /tmp/gl*.h | sed "s@/tmp/gl[^.]*.h:[ \t]*@@"
echo "@@@@end"

) | awk '
/^@@@@src/,/^@@@@end/   { NEED[$1]++ }
/^@@@@macro/,/^@@@@end/ { HAVE[$1]++ }
/^@@@@glapi/,/^@@@@end/ {
	match($0, "gl[A-Za-z0-9_]*");
	name = substr($0, RSTART, RLENGTH);
	if (name != NULL)
	DEF[name] = $0
#print name, "=", DEF[name] > "/dev/stderr"
}

# print all calls that found in the source but not in macros
END {
	for(n in NEED) {
		if (n ~ "^@") continue;
		if (n in HAVE) continue;
		print "#define", n, "FIXME:", DEF[n]
	}
}

' > opengl_debug.h.new


(echo "
/**** NEW CALLS - please implement them ****/
"
cat opengl_debug.h.new 
) >> opengl_debug.h
