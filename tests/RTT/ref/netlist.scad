//SCAD

module line_segment_r(length, width, thickness, x, y, a, bd, c1, c2) {
	translate([x,y,0]) rotate ([0,0,a]) union() {
		if (bd) {cube ([length, width,thickness],true);}
		if (c2) {translate([length/2.,0,0]) cylinder(h=thickness, r=width/2,center=true,$fn=30);}
		if (c1) { translate([-length/2.,0,0]) cylinder(h=thickness, r=width/2,center=true,$fn=30);}
	}
}

module line_segment(length, width, thickness, x, y, a) {
	translate([x,y,0]) rotate ([0,0,a]) {
		cube ([length, width,thickness],true);
	}
}

// START_OF_LAYER: plated-drill
layer_pdrill_list=[
];


// END_OF_LAYER layer_pdrill

// START_OF_LAYER: topsilk
module layer_topsilk_body (offset) {
translate ([0, 0, offset]) union () {
	line_segment_r(5.080000,0.254000,0.037500,1.270000,-5.715000,90.000000,1,1,1);
	line_segment_r(10.160000,0.254000,0.037500,6.350000,-8.255000,0.000000,1,1,1);
	line_segment_r(5.080000,0.254000,0.037500,11.430000,-5.715000,-90.000000,1,1,1);
	line_segment_r(3.810000,0.254000,0.037500,3.175000,-3.175000,180.000000,1,1,1);
	line_segment_r(3.810000,0.254000,0.037500,9.525000,-3.175000,180.000000,1,1,1);
	line_segment_r(0.110792,0.254000,0.037500,5.082417,-3.230344,92.499641,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.092064,-3.340610,97.500259,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.111285,-3.449616,102.499977,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.139933,-3.556532,107.499977,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.177791,-3.660545,112.500046,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,5.224569,-3.760862,117.499908,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.279912,-3.856720,122.499672,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.343400,-3.947391,127.500198,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.414549,-4.032183,132.500183,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.492817,-4.110450,137.499817,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.577610,-4.181600,142.500107,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.668280,-4.245088,147.500046,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,5.764138,-4.300431,152.500092,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,5.864455,-4.347209,157.499954,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,5.968468,-4.385067,162.500031,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.075384,-4.413715,167.500031,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.184390,-4.432936,172.499741,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,6.294656,-4.442583,177.500366,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,6.405344,-4.442583,-177.500366,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.515610,-4.432936,-172.499741,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.624617,-4.413715,-167.500031,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,6.731533,-4.385067,-162.500031,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,6.835545,-4.347209,-157.499954,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,6.935862,-4.300431,-152.500092,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,7.031720,-4.245088,-147.500046,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,7.122390,-4.181600,-142.500107,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,7.207182,-4.110450,-137.499817,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,7.285450,-4.032183,-132.500183,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,7.356599,-3.947391,-127.499794,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,7.420087,-3.856721,-122.500381,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,7.475431,-3.760863,-117.499664,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,7.522210,-3.660545,-112.500046,1,0,1);
	line_segment_r(0.110794,0.254000,0.037500,7.560067,-3.556532,-107.499825,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,7.588715,-3.449615,-102.499977,1,0,1);
	line_segment_r(0.110793,0.254000,0.037500,7.607936,-3.340610,-97.500320,1,0,1);
	line_segment_r(0.110792,0.254000,0.037500,7.617583,-3.230344,-92.499641,1,1,1);
	line_segment_r(0.889000,0.177800,0.037500,2.540000,-2.349500,90.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.037500,2.603500,-2.857500,135.000000,1,1,1);
	line_segment_r(0.254000,0.177800,0.037500,2.794000,-2.921000,180.000000,1,1,1);
	line_segment_r(0.179605,0.177800,0.037500,2.984500,-2.857500,-135.000000,1,1,1);
	line_segment_r(0.889000,0.177800,0.037500,3.048000,-2.349500,90.000000,1,1,1);
	line_segment_r(0.287368,0.177800,0.037500,3.454401,-2.006600,-135.000000,1,1,1);
	line_segment_r(1.016000,0.177800,0.037500,3.556001,-2.413000,90.000000,1,1,1);
	line_segment_r(0.381000,0.177800,0.037500,3.543301,-2.921000,180.000000,1,1,1);
}
}


// END_OF_LAYER layer_topsilk

// START_OF_LAYER: bottomsilk
module layer_bottomsilk_body (offset) {
translate ([0, 0, offset]) union () {
}
}


// END_OF_LAYER layer_bottomsilk

module board_outline () {
	polygon([[0,0],[0,-12.700000],[12.700000,-12.700000],[12.700000,0]],
[[0,1,2,3]]);
}

module all_holes() {
	plating=0.017500;
	union () {
		for (i = layer_pdrill_list) {
			translate([i[1][0],i[1][1],0]) cylinder(r=i[0]+2*plating, h=1.770000, center=true, $fn=30);
		}
		for (i = layer_udrill_list) {
			translate([i[1][0],i[1][1],0]) cylinder(r=i[0], h=1.770000, center=true, $fn=30);
		}
	}
}

module board_body() {
	translate ([0, 0, -0.800000]) linear_extrude(height=1.600000) board_outline();}

/***************************************************/
/*                                                 */
/* Components                                      */
/*                                                 */
/***************************************************/
module all_components() {
}

/***************************************************/
/*                                                 */
/* Final board assembly                            */
/* Here is the complete board built from           */
/* pre-generated modules                           */
/*                                                 */
/***************************************************/
		color ([1, 1, 1])
			layer_topsilk_body(0.818750);

		color ([1, 1, 1])
			layer_bottomsilk_body(-0.818750);

		color ([0.44, 0.44, 0])
			difference() {
				board_body();
				all_holes();
			}

		all_components();
// END_OF_BOARD
