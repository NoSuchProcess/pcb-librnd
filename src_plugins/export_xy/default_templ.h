static const char *templ_xy_hdr =
	"# $Id$\n"
	"# PcbXY Version 1.0\n"
	"# Date: %UTC%\n"
	"# Author: %author%\n"
	"# Title: %title% - PCB X-Y\n"
	"# RefDes, Description, Value, X, Y, rotation, top/bottom\n"
	"# X,Y in %suffix%.  rotation in degrees.\n"
	"# --------------------------------------------\n";

static const char *templ_xy_elem =
	"%elem.name%,\"%elem.descr%\",\"%elem.value%\",%elem.x%,%elem.y%,%elem.rot%,%elem.side%\n";


static const char *templ_gxyrs_hdr =
	"#gxyrs version 1.0\n"
	"#Date: %UTC%\n"
	"#Author: %author%\n"
	"#Title: %title% - pcb-rnd gxyrs\n"
	"#Placement data:\n"
	"#Designator X-Loc Y-Loc Rotation Side Type X-Size Y-Size Value Footprint\n"
	"# --------------------------------------------\n";

static const char *templ_gxyrs_elem =
	"%elem.name%,%elem.x%,%elem.y%,%elem.rot%,%elem.side%,%elem.descr%\n";

static const char *templ_TM220A_hdr = 
	"#pcb-rnd TM220A xyrs version 1.0\n"
	"#Date: %UTC%\n"
	"#Author: %author%\n"
	"#Title: %title% - pcb-rnd gxyrs\n"
	"#Placement data:\n"
	"#RefDes, top/bottom, X, Y, rotation\n"
	"# X,Y in %suffix%.  rotation in degrees.\n"
        "# --------------------------------------------\n";

static const char *templ_TM220A_elem =
	"%elem.name%,%elem.side%,%elem.x%,%elem.y%,%elem.rot%\n";
