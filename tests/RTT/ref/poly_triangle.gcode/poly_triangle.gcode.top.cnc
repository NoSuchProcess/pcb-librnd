(Created by G-code exporter)
(600 dpi)
(Unit: inch)
(Board size: 0.50x0.50 inches)#100=0.002000  (safe Z)
#101=-0.000050  (cutting depth)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7 F1
G0 Z#100
(polygon 1)
G0 X0.025000 Y0.475000    (start point)
G1 Z#101
G1 X0.025000 Y0.373333
G1 X0.126667 Y0.473333
G1 X0.126667 Y0.475000
G1 X0.025000 Y0.475000
G0 Z#100
(polygon end, distance 0.35)
(polygon 2)
G0 X0.150000 Y0.475000    (start point)
G1 Z#101
G1 X0.025000 Y0.348333
G1 X0.151667 Y0.348333
G1 X0.151667 Y0.475000
G1 X0.150000 Y0.475000
G0 Z#100
(polygon end, distance 0.43)
(polygon 3)
G0 X0.300000 Y0.475000    (start point)
G1 Z#101
G1 X0.175000 Y0.348333
G1 X0.426667 Y0.348333
G1 X0.301667 Y0.475000
G1 X0.300000 Y0.475000
G0 Z#100
(polygon end, distance 0.61)
(polygon 4)
G0 X0.025000 Y0.325000    (start point)
G1 Z#101
G1 X0.025000 Y0.323333
G1 X0.050000 Y0.150000
G1 X0.051667 Y0.148333
G1 X0.301667 Y0.298333
G1 X0.290000 Y0.300000
G1 X0.025000 Y0.325000
G0 Z#100
(polygon end, distance 0.75)
(end, total distance 54.32mm = 2.14in)
M5 M9 M2
