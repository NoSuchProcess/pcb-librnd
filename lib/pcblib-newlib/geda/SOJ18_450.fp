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
Element[0x00000000 "Small outline J-leaded package (450 mil)" "" "SOJ18_450" 0 0 -2000 -6000 0 100 0x00000000]
(
# 
# Pad[x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags]
        Pad[   -24803 -20000 
			 -18503 -20000 
			2362 1000 3362 "1" "1" 0x00000100]
        Pad[   -24803 -15000 
			 -18503 -15000 
			2362 1000 3362 "2" "2" 0x00000100]
        Pad[   -24803 -10000 
			 -18503 -10000 
			2362 1000 3362 "3" "3" 0x00000100]
        Pad[   -24803 -5000 
			 -18503 -5000 
			2362 1000 3362 "4" "4" 0x00000100]
        Pad[   -24803 0 
			 -18503 0 
			2362 1000 3362 "5" "5" 0x00000100]
        Pad[   -24803 5000 
			 -18503 5000 
			2362 1000 3362 "6" "6" 0x00000100]
        Pad[   -24803 10000 
			 -18503 10000 
			2362 1000 3362 "7" "7" 0x00000100]
        Pad[   -24803 15000 
			 -18503 15000 
			2362 1000 3362 "8" "8" 0x00000100]
        Pad[   -24803 20000 
			 -18503 20000 
			2362 1000 3362 "9" "9" 0x00000100]
        Pad[   24803 20000 
			 18503 20000 
			2362 1000 3362 "10" "10" 0x00000100]
        Pad[   24803 15000 
			 18503 15000 
			2362 1000 3362 "11" "11" 0x00000100]
        Pad[   24803 10000 
			 18503 10000 
			2362 1000 3362 "12" "12" 0x00000100]
        Pad[   24803 5000 
			 18503 5000 
			2362 1000 3362 "13" "13" 0x00000100]
        Pad[   24803 0 
			 18503 0 
			2362 1000 3362 "14" "14" 0x00000100]
        Pad[   24803 -5000 
			 18503 -5000 
			2362 1000 3362 "15" "15" 0x00000100]
        Pad[   24803 -10000 
			 18503 -10000 
			2362 1000 3362 "16" "16" 0x00000100]
        Pad[   24803 -15000 
			 18503 -15000 
			2362 1000 3362 "17" "17" 0x00000100]
        Pad[   24803 -20000 
			 18503 -20000 
			2362 1000 3362 "18" "18" 0x00000100]
	ElementLine[-26984 -22181 -26984  22181 1000]
	ElementLine[-26984  22181  26984  22181 1000]
	ElementLine[ 26984  22181  26984 -22181 1000]
	ElementLine[-26984 -22181 -2500 -22181 1000]
	ElementLine[ 26984 -22181  2500 -22181 1000]
	# punt on the arc on small parts as it can cover the pads
	ElementArc[0 -22181 2500 2500 0 180 1000]
)
