Element(0x00 "Header connector with latches" "" "DIN41651_50" 400 250 3 200 0x00)
(
	Pin(100 500 60 40 "1" 0x101)
		Pin(200 500 60 40 "2" 0x01)
	Pin(100 600 60 40 "3" 0x01)
		Pin(200 600 60 40 "4" 0x01)
	Pin(100 700 60 40 "5" 0x01)
		Pin(200 700 60 40 "6" 0x01)
	Pin(100 800 60 40 "7" 0x01)
		Pin(200 800 60 40 "8" 0x01)
	Pin(100 900 60 40 "9" 0x01)
		Pin(200 900 60 40 "10" 0x01)
	Pin(100 1000 60 40 "11" 0x01)
		Pin(200 1000 60 40 "12" 0x01)
	Pin(100 1100 60 40 "13" 0x01)
		Pin(200 1100 60 40 "14" 0x01)
	Pin(100 1200 60 40 "15" 0x01)
		Pin(200 1200 60 40 "16" 0x01)
	Pin(100 1300 60 40 "17" 0x01)
		Pin(200 1300 60 40 "18" 0x01)
	Pin(100 1400 60 40 "19" 0x01)
		Pin(200 1400 60 40 "20" 0x01)
	Pin(100 1500 60 40 "21" 0x01)
		Pin(200 1500 60 40 "22" 0x01)
	Pin(100 1600 60 40 "23" 0x01)
		Pin(200 1600 60 40 "24" 0x01)
	Pin(100 1700 60 40 "25" 0x01)
		Pin(200 1700 60 40 "26" 0x01)
	Pin(100 1800 60 40 "27" 0x01)
		Pin(200 1800 60 40 "28" 0x01)
	Pin(100 1900 60 40 "29" 0x01)
		Pin(200 1900 60 40 "30" 0x01)
	Pin(100 2000 60 40 "31" 0x01)
		Pin(200 2000 60 40 "32" 0x01)
	Pin(100 2100 60 40 "33" 0x01)
		Pin(200 2100 60 40 "34" 0x01)
	Pin(100 2200 60 40 "35" 0x01)
		Pin(200 2200 60 40 "36" 0x01)
	Pin(100 2300 60 40 "37" 0x01)
		Pin(200 2300 60 40 "38" 0x01)
	Pin(100 2400 60 40 "39" 0x01)
		Pin(200 2400 60 40 "40" 0x01)
	Pin(100 2500 60 40 "41" 0x01)
		Pin(200 2500 60 40 "42" 0x01)
	Pin(100 2600 60 40 "43" 0x01)
		Pin(200 2600 60 40 "44" 0x01)
	Pin(100 2700 60 40 "45" 0x01)
		Pin(200 2700 60 40 "46" 0x01)
	Pin(100 2800 60 40 "47" 0x01)
		Pin(200 2800 60 40 "48" 0x01)
	Pin(100 2900 60 40 "49" 0x01)
		Pin(200 2900 60 40 "50" 0x01)
	# Befestigungsbohrung
	Pin(180  270 100 80 "M1" 0x01)
	Pin(180 3130 100 80 "M2" 0x01)
	# aeusserer Rahmen
	ElementLine(80 70 335 70 20)
	ElementLine(335 70 770 200 20)
	ElementLine(770 200 770 300 20)
	ElementLine(770 300 610 390 20)
	ElementLine(610 390 610 3050 20)
	ElementLine(610 3050 770 3100 20)
	ElementLine(770 3100 770 3200 20)
	ElementLine(770 3200 335 3330 20)
	ElementLine(335 3330 80 3330 20)
	ElementLine( 80 3330 80 70 20)
	# Codieraussparung
	ElementLine(610 1625 435 1625 5)
	ElementLine(435 1625 435 1775 5)
	ElementLine(435 1775 610 1775 5)
	# Markierung Pin 1
	ElementLine(610 450 500 500 5)
	ElementLine(500 500 610 550 5)
	# Plazierungsmarkierung == Pin 1
	Mark(100 500)
)
