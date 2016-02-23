#!/bin/sh

path=../../src_plugins

sloc()
{
	(cd "$1" && sloccount .) | awk '/^Total Phys/ { size=$9; sub(",", "", size); print size }'
}

gen_pie()
{
	local bn=$1 code_size=$2 color=$3
	echo ""
	echo "@slice"
	echo "$code_size"
	echo "@label"
	echo "$bn ($code_size)"
	if test ! -z "$color"
	then
		echo "@color"
		echo "$color"
	fi
}

echo "#autogenerated by gen.sh" > mods.pie
echo "#autogenerated by gen.sh" > after.pie

echo HIDs >&2
code_size=`sloc ../../src/hid`
gen_pie "HIDs" $code_size "orangered" >> after.pie

echo Core >&2
tmp=/tmp/pcb-mods-stat
mkdir $tmp
cp -r ../../src/*.c ../../src/*.h ../../src/Makefile* $tmp
code_size=`sloc $tmp`
gen_pie "core" $code_size "#00ff88" >> after.pie

#echo 3rd >&2
#code_size=`sloc ../../src_3rd`
#gen_pie "3rd" $code_size >> after.pie


(
cat pre.html
for n in $path/*
do
	if test -d "$n"
	then
		echo $n >&2
		bn=`basename $n`
		code_size=`sloc $n`
		total=$(($total + $code_size))
		gen_pie $bn $code_size >> mods.pie
#		case $bn in
#			gpmi) echo "@pull" >> mods.pie; echo "0.1" >> mods.pie;;
#		esac

		echo "<tr><th align=left>$bn<td>$code_size"
		awk '
			/^#/ {
				key=$1
				sub("#", "", key)
				sub("[:=]", "", key)
				$1=""
				DB[key]=$0
				next
			}
			{ desc = desc " " $0 }

			function strip(s) {
				sub("^[ \t]*", "", s)
				sub("[ \t]*$", "", s)
				return s
			}

			END {
				st = DB["state"]
				if (st ~ "partial")
					clr = "bgcolor=\"yellow\""
				else if (st ~ "works")
					clr = "bgcolor=\"lightgreen\""
				else if ((st ~ "fail") || (st ~ "disable"))
					clr = "bgcolor=\"red\""
				else
					clr=""

				clr2 = clr
				if (clr2 != "") {
					sub("bgcolor=\"", "", clr2)
					sub("\"", "", clr2)
					print "@color" >> "mods.pie"
					print clr2 >> "mods.pie"
				}

				print "<td " clr " >" st
				if (DB["lstate"] != "")
					print "<br> (" strip(DB["lstate"]) ")"

				dfl = DB["default"]
				if (dfl ~ "buildin")
					clr = "bgcolor=\"lightgreen\""
				else if (dfl ~ "plugin")
					clr = "bgcolor=\"yellow\""
				else if ((dfl ~ "fail") || (dfl ~ "disable"))
					clr = "bgcolor=\"red\""
				else
					clr=""

				print "<td " clr ">" dfl
				if (DB["ldefault"] != "")
					print "<br> (" strip(DB["ldefault"]) ")"
				print "<td>" desc
			}
		' < $n/README
	fi
done
cat post.html
gen_pie "plugins" "$total" "#0088ff" >> after.pie
) > index.html

for n in mods after
do
	animpie < $n.pie | animator -H -d $n
	pngtopnm ${n}0000.png | pnmcrop | pnmtopng > $n.png
	rm ${n}0000.png
done

