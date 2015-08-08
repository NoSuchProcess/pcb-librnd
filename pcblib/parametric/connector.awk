BEGIN {
	set_arg(P, "?spacing", 100)
	set_arg(P, "?silkmark", "square")
	set_arg(P, "?sequence", "normal")

	proc_args(P, "nx,ny,spacing,silkmark,eshift,etrunc", "nx,ny")


	step = parse_dim(P["spacing"])

	if (pin_ringdia > step*0.9)
		pin_ringdia = step*0.9

	if (pin_drill > pin_ringdia*0.9)
		pin_drill = pin_ringdia*0.9

	half=step/2

	P["nx"] = int(P["nx"])
	P["ny"] = int(P["ny"])

	eshift=tolower(P["eshift"])
	etrunc=tobool(P["etrunc"])
	if ((eshift != "x") && (eshift != "y") && (eshift != "") && (eshift != "none"))
		error("eshift must be x or y or none (got: " eshift ")");

	element_begin("", "CONN1", P["nx"] "*" P["ny"]    ,0,0, 0, -step)

	for(x = 0; x < P["nx"]; x++) {
		if ((eshift == "x") && ((x % 2) == 1))
			yo = step/2
		else
			yo = 0
		for(y = 0; y < P["ny"]; y++) {
			if ((eshift == "y") && ((y % 2) == 1)) {
				xo = step/2
				if ((etrunc) && (x == P["nx"]-1))
					continue
			}
			else {
				xo = 0
				if ((etrunc) && (y == P["ny"]-1) && (yo != 0))
					continue
			}
			if (P["sequence"] == "normal") {
				pinno++
			}
			else if (P["sequence"] == "pivot") {
				pinno = y * P["nx"] + x + 1
			}
			else if (P["sequence"] == "zigzag") {
				if (x % 2) {
					pinno = (x+1) * P["ny"] - y
					if ((etrunc) && (eshift == "x"))
						pinno -= int(x/2)+1
					else if ((etrunc) && (eshift == "y"))
						pinno += int(x/2)
				}
				else {
					pinno = x * P["ny"] + y + 1
					if ((etrunc) && (eshift == "x"))
						pinno -= x/2
					else if ((etrunc) && (eshift == "y"))
						pinno += x/2-1
				}
			}
			element_pin(x * step + xo, y * step + yo, pinno)
		}
	}

	xo = 0
	yo = 0
	if (!etrunc) {
		if (eshift == "y")
			xo = step/2
		if (eshift == "x")
			yo = step/2
	}

	element_rectangle(-half, -half, P["nx"] * step - half + xo, P["ny"] * step - half + yo)

	if (P["silkmark"] == "angled") {
		element_line(0, -half,  -half, 0)
	}
	else if (P["silkmark"] == "square") {
		element_line(-half, half,  half, half)
		element_line(half, -half,  half, half)
	}
	else if ((P["silkmark"] == "external") || (P["silkmark"] == "externalx")) {
		element_line(-half, 0,        -step, -half/2)
		element_line(-half, 0,        -step, +half/2)
		element_line(-step, -half/2,  -step, +half/2)
	}
	else if (P["silkmark"] == "externaly") {
		element_line(0, -half,        -half/2, -step)
		element_line(0, -half,        +half/2, -step)
		element_line(-half/2, -step,  +half/2, -step)
	}
	else if ((P["silkmark"] != "none") && (P["silkmark"] != "")) {
		error("invalid silkmark parameter: " P["silkmark"])
	}
	
	element_end()
}
