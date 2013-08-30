	# number of pads
	# pad width in 1/1000 mil
	# pad length in 1/1000 mil
	# pad pitch 1/1000 mil
	# seperation between pads on opposite sides 1/1000 mil
	# X coordinates for the right hand column of pads (1/100 mils)
	# pad clearance to plane layer in 1/100 mil
	# pad soldermask width in 1/100 mil
	# silk screen width (1/100 mils)
	# figure out if we have an even or odd number of pins per side
	# silk bounding box is -XMAX,-YMAX, XMAX,YMAX (1/100 mils)
# element_flags, description, pcb-name, value, mark_x, mark_y,
# text_x, text_y, text_direction, text_scale, text_flags
Element[0x00000000 "Small outline J-leaded package (300 mil)" "" "SOJ32_300" 0 0 -2000 -6000 0 100 0x00000000]
(
# 
# Pad[x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags]
        Pad[   -17322 -37500 
			 -11023 -37500 
			2362 1000 3362 "1" "1" 0x00000100]
        Pad[   -17322 -32500 
			 -11023 -32500 
			2362 1000 3362 "2" "2" 0x00000100]
        Pad[   -17322 -27500 
			 -11023 -27500 
			2362 1000 3362 "3" "3" 0x00000100]
        Pad[   -17322 -22500 
			 -11023 -22500 
			2362 1000 3362 "4" "4" 0x00000100]
        Pad[   -17322 -17500 
			 -11023 -17500 
			2362 1000 3362 "5" "5" 0x00000100]
        Pad[   -17322 -12500 
			 -11023 -12500 
			2362 1000 3362 "6" "6" 0x00000100]
        Pad[   -17322 -7500 
			 -11023 -7500 
			2362 1000 3362 "7" "7" 0x00000100]
        Pad[   -17322 -2500 
			 -11023 -2500 
			2362 1000 3362 "8" "8" 0x00000100]
        Pad[   -17322 2500 
			 -11023 2500 
			2362 1000 3362 "9" "9" 0x00000100]
        Pad[   -17322 7500 
			 -11023 7500 
			2362 1000 3362 "10" "10" 0x00000100]
        Pad[   -17322 12500 
			 -11023 12500 
			2362 1000 3362 "11" "11" 0x00000100]
        Pad[   -17322 17500 
			 -11023 17500 
			2362 1000 3362 "12" "12" 0x00000100]
        Pad[   -17322 22500 
			 -11023 22500 
			2362 1000 3362 "13" "13" 0x00000100]
        Pad[   -17322 27500 
			 -11023 27500 
			2362 1000 3362 "14" "14" 0x00000100]
        Pad[   -17322 32500 
			 -11023 32500 
			2362 1000 3362 "15" "15" 0x00000100]
        Pad[   -17322 37500 
			 -11023 37500 
			2362 1000 3362 "16" "16" 0x00000100]
        Pad[   17322 37500 
			 11023 37500 
			2362 1000 3362 "17" "17" 0x00000100]
        Pad[   17322 32500 
			 11023 32500 
			2362 1000 3362 "18" "18" 0x00000100]
        Pad[   17322 27500 
			 11023 27500 
			2362 1000 3362 "19" "19" 0x00000100]
        Pad[   17322 22500 
			 11023 22500 
			2362 1000 3362 "20" "20" 0x00000100]
        Pad[   17322 17500 
			 11023 17500 
			2362 1000 3362 "21" "21" 0x00000100]
        Pad[   17322 12500 
			 11023 12500 
			2362 1000 3362 "22" "22" 0x00000100]
        Pad[   17322 7500 
			 11023 7500 
			2362 1000 3362 "23" "23" 0x00000100]
        Pad[   17322 2500 
			 11023 2500 
			2362 1000 3362 "24" "24" 0x00000100]
        Pad[   17322 -2500 
			 11023 -2500 
			2362 1000 3362 "25" "25" 0x00000100]
        Pad[   17322 -7500 
			 11023 -7500 
			2362 1000 3362 "26" "26" 0x00000100]
        Pad[   17322 -12500 
			 11023 -12500 
			2362 1000 3362 "27" "27" 0x00000100]
        Pad[   17322 -17500 
			 11023 -17500 
			2362 1000 3362 "28" "28" 0x00000100]
        Pad[   17322 -22500 
			 11023 -22500 
			2362 1000 3362 "29" "29" 0x00000100]
        Pad[   17322 -27500 
			 11023 -27500 
			2362 1000 3362 "30" "30" 0x00000100]
        Pad[   17322 -32500 
			 11023 -32500 
			2362 1000 3362 "31" "31" 0x00000100]
        Pad[   17322 -37500 
			 11023 -37500 
			2362 1000 3362 "32" "32" 0x00000100]
	ElementLine[-19503 -39681 -19503  39681 1000]
	ElementLine[-19503  39681  19503  39681 1000]
	ElementLine[ 19503  39681  19503 -39681 1000]
	ElementLine[-19503 -39681 -2500 -39681 1000]
	ElementLine[ 19503 -39681  2500 -39681 1000]
	# punt on the arc on small parts as it can cover the pads
	ElementArc[0 -39681 2500 2500 0 180 1000]
)
