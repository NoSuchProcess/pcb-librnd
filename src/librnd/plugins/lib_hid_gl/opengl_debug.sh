#!/bin/sh

cp /usr/include/GL/gl*.h /tmp
indent -l10000 /tmp/gl*.h


(
echo "@@@@src"
grep "gl[A-Z].*[(]" *.[chly] | sed "s/^.*gl/gl/;s/[(].*//"
echo "@@@@end"

echo "@@@@macro"
grep "^#define[ \t]*gl[A-Z]" opengl_debug.h  | sed "s/^.*#define[ \t]*//;s/[(].*//"
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

function strip(s)
{
	sub("^[ \t]*", "", s)
	sub("[ \t]*$", "", s)
	return s
}

function gen_call_args(args   ,v,n,A,s)
{
	v = split(args, A, ",")
	for(n = 1; n <= v; n++) {
		if (n > 1)
			s = s ", "
		s = s "arg" n
	}
	return s
}

function gen_call_arg_names(args   ,v,n,A,s,a)
{
	v = split(args, A, ",")
	for(n = 1; n <= v; n++) {
		if (n > 1)
			s = s ", "
		a = A[n]
		sub("^.*[*]", "", a)
		sub("[\[(].*$", "", a)
		sub("^.*[ \t]", "", a)
		s = s a "__"
	}
	return s
}

function print_local_vars(args   ,v,n,A,s)
{
	v = split(args, A, ",")
	for(n = 1; n <= v; n++) {
		print "\t\t" strip(A[n]) "__ = (arg" n "); \\"
	}
}

# print all calls that found in the source but not in macros
END {
	for(n in NEED) {
		if (n ~ "^@") continue;
		if (n in HAVE) continue;

		args = DEF[n]
		sub("^[^(]*[(]", "", args)
		sub("[)][^)]*$", "", args)
		if (args == "void")
			args = ""
		ca = gen_call_arg_names(args)

		print "#define ", n "(" gen_call_args(args) ") \\"
		print "\tdo { \\"
		print_local_vars(args)
		print "\t\t" n "(" ca "); \\"
		print "\t\tgld_print(\"" n "(...)\", " ca "); \\"
		print "\t} while(0)"
		print "----FIXME----:", DEF[n]
		print ""
	}
}

' > opengl_debug.h.new

len=`wc -l opengl_debug.h.new | awk '{print $1}'`

if test "$len" -gt 2
then
(echo "
/**** NEW CALLS - please implement them ****/
"
cat opengl_debug.h.new 
) >> opengl_debug.h
fi
