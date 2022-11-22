#!/bin/sh
#   action list dump - collates the pcb-rnd action table into a html doc page
#   Copyright (C) 2017,2018 Tibor 'Igor2' Palinkas
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#   http://repo.hu/projects/librnd

#shell lib; configuration:
# $asrc  path to action sources
# $lsrc  path to librnd action sources

# $1 is app name
print_hdr() {
echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">
<html>
<head>
<title> $1 user manual </title>
<meta http-equiv=\"Content-Type\" content=\"text/html;charset=us-ascii\">
<link rel=\"stylesheet\" type=\"text/css\" href=\" ../default.css\">
</head>
<body>
<p>
<h1> $1 User Manual: Appendix </h1>
<p>
<h2> Action Reference</h2>
<table border=1>"
echo "<caption>\n" "<b>"
echo $pcb_rnd_ver ", " $pcb_rnd_rev
echo "</b>"
echo  "<th> Action <th> Description <th> Syntax <th> Plugin"
}

gen() {
 awk '

function flush_sd()
{
		if ( a != "" ||  s != "" || d != "" ) {
			sub("^<br>", "", a)
			sub("^<br>", "", d)
			sub("^<br>", "", s)
			sub("^<br>", "", c)
			print  "<tr><act>" a "</act>" "<td>" d "</td>" "<td>" s "</td>" "<td>" c "</td>"
			}
		
	a=""
	s=""
	d=""
	c=""
}


/^A/ {
	flush_sd()
	sub("^A", "", $0)
	a = a "<br>" $0
	next
}

/^D/ {
	sub("^D", "", $0)
	d = d "<br>" $0
	next
}

/^S/ {
	sub("^S", "", $0)
	s = s "<br>" $0
	next
}

/^C/ {
	sub("^C", "", $0)
	c = c "<br>" $0
	next
}

' | sort -fu | awk -v "asrc=$asrc" -v "lsrc=$lsrc" '
# insert links around actions where applicable
	BEGIN {
		q = "\""
	}
	/<act>/ {
		pre = $0
		sub("<act>.*", "", pre)
		post = $0
		sub(".*</act>", "", post)
		act = $0
		sub(".*<act>", "", act)
		sub("</act>.*", "", act)
		loact = tolower(act)
		fn = asrc "/" loact ".html"
		lfn = lsrc "/" loact ".html"
		if ((getline < fn) == 1)
			print pre "<td><a href=" q "action_details.html#" loact q ">" act "</a></td>" post
		else if ((getline < lfn) == 1)
			print pre "<td><a href=" q "action_details.html#" loact q ">" act "</a> <sup><a href=\"#ringdove\">(RND)</a></sup></td>" post
		else
			print pre "<td>" act "</td>" post
		close(fn)
		close(lfn)
		next
	}

	{ print $0 }
	
	END {
		print "</table>"
		print "<p id=\"ringdove\">RND: this action comes from librnd and is common to all ringdove applications."
		print "</body>"
		print "</html>"
	}
'
}
