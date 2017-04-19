pcb_hid_attribute_t *gpmi_hid_get_export_options(int *num);
pcb_hid_gc_t gpmi_hid_make_gc(void);
void gpmi_hid_destroy_gc(pcb_hid_gc_t gc);
void gpmi_hid_do_export(pcb_hid_attr_val_t * options);
void gpmi_hid_parse_arguments(int *pcbargc, char ***pcbargv);
void gpmi_hid_set_crosshair(int x, int y, int cursor_action);
int gpmi_hid_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty);
void gpmi_hid_set_color(pcb_hid_gc_t gc, const char *name);
void gpmi_hid_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style);
void gpmi_hid_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width);
void gpmi_hid_set_draw_xor(pcb_hid_gc_t gc, int xor);
void gpmi_hid_set_draw_faded(pcb_hid_gc_t gc, int faded);
void gpmi_hid_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
void gpmi_hid_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t xradius, pcb_coord_t yradius, pcb_angle_t start_angle, pcb_angle_t delta_angle);
void gpmi_hid_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
void gpmi_hid_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius);
void gpmi_hid_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y);
void gpmi_hid_fill_pcb_polygon(pcb_hid_gc_t gc, pcb_polygon_t *poly, const pcb_box_t *clip_box);
void gpmi_hid_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
void gpmi_hid_fill_pcb_pv(pcb_hid_gc_t fg_gc, pcb_hid_gc_t bg_gc, pcb_pin_t *pad, pcb_bool drawHole, pcb_bool mask);
void gpmi_hid_fill_pcb_pad(pcb_hid_gc_t gc, pcb_pad_t * pad, pcb_bool clear, pcb_bool mask);
void gpmi_hid_use_mask(pcb_mask_op_t use_it);
