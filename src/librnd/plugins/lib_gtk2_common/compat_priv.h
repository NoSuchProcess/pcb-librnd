/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* internals of compat.h that are not included in the public API */

gboolean gtkc_resize_dwg_cb(GtkWidget *widget, GdkEventConfigure *ev, void *rs);
gint gtkc_mouse_scroll_cb(GtkWidget *widget, GdkEventScroll *ev, void *rs);
gint gtkc_mouse_enter_cb(GtkWidget *widget, GdkEventCrossing *ev, void *rs);
gint gtkc_mouse_leave_cb(GtkWidget *widget, GdkEventCrossing *ev, void *rs);
gint gtkc_mouse_press_cb(GtkWidget *widget, GdkEventButton *ev, void *rs);
gint gtkc_mouse_release_cb(GtkWidget *widget, GdkEventButton *ev, void *rs);
gint gtkc_mouse_motion_cb(GtkWidget *widget, GdkEventMotion *ev, void *rs);
gint gtkc_key_press_cb(GtkWidget *widget, GdkEventKey *kev, void *rs);
gint gtkc_key_release_cb(GtkWidget *widget, GdkEventKey *kev, void *rs);
gint gtkc_win_resize_cb(GtkWidget *widget, GdkEventConfigure *ev, void *rs);
gint gtkc_win_destroy_cb(GtkWidget *widget, void *rs);
gint gtkc_win_delete_cb(GtkWidget *widget, GdkEvent *ev, void *rs);
