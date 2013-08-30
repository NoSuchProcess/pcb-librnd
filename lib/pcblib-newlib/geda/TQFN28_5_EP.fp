	# number of pins on left/right sides (pin1 is upper pin on left side)
	# number of pins on top/bottom sides
	# pin pitch (1/1000 mil)
	# y-coordinate for upper pin on left/right sides  (1/1000 mil)
	# x-coordinate for right pin on top/bottom sides  (1/1000 mil)
	# total horizontal package width (1/1000 mil)
	# total vertical package width (1/1000 mil)
	# how much pads extend beyond the package edge (1/1000 mil) (the 25 is 0.25 mm)
	# how much pads extend inward from the package pad edge (1/1000 mil)
	# pad length/width (1/1000 mil)
	# pad width (mil/100)
	# min/max x coordinates for the pads on the left/right sides of the package (mil/100)
	# min/max y coordinates for the pads on the top/bottom sides of the package (mil/100)
	# silkscreen width (mils/100)
	# how much the silk screen is moved away from the package (1/1000 mil)
	# upper right corner for silk screen (mil/100)
	# refdes text size (mil/100)
	# x,y coordinates for refdes label (mil/100)
	# square exposed paddle size (mil/100)
   # pad clearance to polygons (1/100 mil)
   # width of the pad solder mask relief (1/100 mil). 
   # grow by 1.5 mils on each side
   # width of the paddle soldermask relief (1/100 mil)
   # grow by 200 mils on each side
# element_flags, description, pcb-name, value, mark_x, mark_y,
# text_x, text_y, text_direction, text_scale, text_flags
Element[0x00000000 "Square Quad Flat Nolead (QFN) package" "" "TQFN28_5_EP" 0 0 -11826 -12876 0 100 0x00000000]
(
# Pad[X1, Y1, X2, Y3, width, clearance,
#     soldermask, "pin name", "pin number", flags]
# left row
	Pad[-10275  -5905  -7834  -5905  1102  2000 1402 "1" "1"  0x00000100]
	Pad[-10275  -3937  -7834  -3937  1102  2000 1402 "2" "2"  0x00000100]
	Pad[-10275  -1968  -7834  -1968  1102  2000 1402 "3" "3"  0x00000100]
	Pad[-10275  0  -7834  0  1102  2000 1402 "4" "4"  0x00000100]
	Pad[-10275  1968  -7834  1968  1102  2000 1402 "5" "5"  0x00000100]
	Pad[-10275  3937  -7834  3937  1102  2000 1402 "6" "6"  0x00000100]
	Pad[-10275  5905  -7834  5905  1102  2000 1402 "7" "7"  0x00000100]
# bottom row
	Pad[-5905  10275  -5905  7834  1102 2000 1402 "8" "8"  0x00000900]
	Pad[-3937  10275  -3937  7834  1102 2000 1402 "9" "9"  0x00000900]
	Pad[-1968  10275  -1968  7834  1102 2000 1402 "10" "10"  0x00000900]
	Pad[0  10275  0  7834  1102 2000 1402 "11" "11"  0x00000900]
	Pad[1968  10275  1968  7834  1102 2000 1402 "12" "12"  0x00000900]
	Pad[3937  10275  3937  7834  1102 2000 1402 "13" "13"  0x00000900]
	Pad[5905  10275  5905  7834  1102 2000 1402 "14" "14"  0x00000900]
# right row
	Pad[10275  5905  7834  5905  1102  2000 1402 "15" "15"  0x00000100]
	Pad[10275  3937  7834  3937  1102  2000 1402 "16" "16"  0x00000100]
	Pad[10275  1968  7834  1968  1102  2000 1402 "17" "17"  0x00000100]
	Pad[10275  0  7834  0  1102  2000 1402 "18" "18"  0x00000100]
	Pad[10275  -1968  7834  -1968  1102  2000 1402 "19" "19"  0x00000100]
	Pad[10275  -3937  7834  -3937  1102  2000 1402 "20" "20"  0x00000100]
	Pad[10275  -5905  7834  -5905  1102  2000 1402 "21" "21"  0x00000100]
# top row
	Pad[5905  -10275  5905  -7834  1102 2000 1402 "22" "22" 0x00000900]
	Pad[3937  -10275  3937  -7834  1102 2000 1402 "23" "23" 0x00000900]
	Pad[1968  -10275  1968  -7834  1102 2000 1402 "24" "24" 0x00000900]
	Pad[0  -10275  0  -7834  1102 2000 1402 "25" "25" 0x00000900]
	Pad[-1968  -10275  -1968  -7834  1102 2000 1402 "26" "26" 0x00000900]
	Pad[-3937  -10275  -3937  -7834  1102 2000 1402 "27" "27" 0x00000900]
	Pad[-5905  -10275  -5905  -7834  1102 2000 1402 "28" "28" 0x00000900]
# Exposed paddle (if this is an exposed paddle part)
# Pad(X1, Y1, X2, Y3, width, clearance,
#     soldermask, "pin name", "pin number", flags)
	Pad[0 0 0 0 12795 0 13195 "29" "29" 0x00000100]
# Silk screen around package
ElementLine[ 11826  11826  11826 -11826 1000]
ElementLine[ 11826 -11826 -11826 -11826 1000]
ElementLine[-11826 -11826 -11826  11826 1000]
ElementLine[-11826  11826  11826  11826 1000]
# Pin 1 indicator
ElementLine[-11826 -11826 -13326 -13326 1000]
)
