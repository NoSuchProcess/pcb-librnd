#!/bin/sh

SERVER=www.repo.hu
BASEDIR=projects
ROOT_URL=http://$SERVER/$BASEDIR
PROJECTS="librnd pcb-rnd sch-rnd camv-rnd route-rnd"

# search the official web pages for broken links
spider() {
echo "Collecting the log for $1..." >&2
wget --spider  -o 404.raw.log -r -p "$ROOT_URL/$1" -X "$BASEDIR/$1/releases"

echo "Evaluatng the log..." >&2
awk '
	function report() {
		print url
		rerorted = 1
	}

	/^--/ {
		url=$0
		sub("--.*--  *", "", url)
		sub("^[(]try[^)]*[)] *", "", url)
	}
	/^$/ { url = ""; reported = 0 }
	(reported) { next }
	/response.*404/ { report() }
	/broken link/ { report() }
' < 404.raw.log
}

for n in $PROJECTS
do
	echo ""
	echo "=== PROJECT: $n ==="
	echo ""
	spider "$n"
done | tee 404.log
