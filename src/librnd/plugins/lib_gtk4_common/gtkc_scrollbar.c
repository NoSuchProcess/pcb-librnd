/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include <gtk/gtk.h>
#include "gtkc_scrollbar.h"


struct _GtkcScrollbar {
	GtkWidget parent_instance;
	GtkOrientation orientation;
	double min, max, win, val;

	/* normalized values */
	double nmin, nmax, nwin, nval;

	/* drag-move state */
	unsigned dragging:1;
	double drag_offs;
};

/*typedef struct {
	GtkWidgetClass parent_class;
} GtkcScrollbarClass;*/

G_DEFINE_TYPE(GtkcScrollbar, gtkc_scrollbar, GTK_TYPE_WIDGET)

enum {
	VALUE_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


/*
#warning TODO: remove this if there is no local fields to free
static void gtkc_scrollbar_dispose(GObject *object)
{
	GtkcScrollbar *scb = GTKC_SCROLLBAR(object);

	G_OBJECT_CLASS(gtkc_scrollbar_parent_class)->dispose(object);
}
*/

/* Normalize min/max/val/win to between 0..1 and return 1 if values are valid */
static int gtkc_scrollbar_normalize(GtkcScrollbar *scb)
{
	double val, min, max, win, cw;

	if (scb->min >= scb->max) {
		scb->nmin = scb->nmax = scb->nval = scb->nwin = 0;
		return 0;
	}

	min = scb->min;
	max = scb->max;
	win = scb->win;
	val = scb->val;
	cw = max - min;

	/* normalize to 0..max */
	max -= min;
	val -= min;
	min = 0;

	/* normalize to 0..1 */
	max /= cw;
	val /= cw;
	win /= cw;

	/* clamp win and val */
	if (win > 1.0)
		win = 1.0;
	if (val < 0.0)
		val = 0.0;
	if (val > 1.0 - win)
		val = 1.0 - win;

	scb->nmin = min;
	scb->nmax = max;
	scb->nval = val;
	scb->nwin = win;

	return 1;
}

static double gtkc_scrollbar_screen2normal(GtkcScrollbar *scb, double widget_x, double widget_y)
{
	double size;

	switch(scb->orientation) {
		case GTK_ORIENTATION_HORIZONTAL:
			size = gtk_widget_get_width(GTK_WIDGET(scb));
			return widget_x / size;
		case GTK_ORIENTATION_VERTICAL:
			size = gtk_widget_get_height(GTK_WIDGET(scb));
			return widget_y / size;
		default:;
	}
	return 0;
}


static void gtkc_scrollbar_init(GtkcScrollbar *scb)
{
	scb->min = 0;
	scb->max = 1;
	scb->win = 0.1;
	scb->val = 0.45;
	gtkc_scrollbar_normalize(scb);
}



static void gtkc_scrollbar_snapshot(GtkWidget *widget, GtkSnapshot *snapshot)
{
	GtkcScrollbar *scb = GTKC_SCROLLBAR(widget);
	int w = gtk_widget_get_width(widget), h = gtk_widget_get_height(widget);
/*	GtkStyleContext *context= gtk_widget_get_style_context(widget);*/
	GdkRGBA fg;

	/* background is drawn from the scrollbar's css;
	   foreground: can't extract "scrollbar > range > trough > slider" from css
	   (in the original implementation the slider is a separate widget and it is
	   drawn by css background) */
	gdk_rgba_parse(&fg, "#777777");

	if (gtkc_scrollbar_normalize(scb)) {
		switch(scb->orientation) {
			case GTK_ORIENTATION_HORIZONTAL:
				gtk_snapshot_append_color(snapshot, &fg, &GRAPHENE_RECT_INIT(scb->nval * w, 2, scb->nwin * w, h-4));
				break;
			case GTK_ORIENTATION_VERTICAL:
				gtk_snapshot_append_color(snapshot, &fg, &GRAPHENE_RECT_INIT(2, scb->nval * h, w - 4, scb->nwin * h));
				break;
			default:;
		}
	}
}

static void gtkc_scrollbar_measure(GtkWidget *widget, GtkOrientation orientation, int for_size, int *minimum, int *natural, int *minimum_baseline, int *natural_baseline)
{
	*minimum = *natural = 10;
	*minimum_baseline = *natural_baseline = -1;
}

static void gtkc_scrollbar_class_init(GtkcScrollbarClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

/*	object_class->dispose = gtkc_scrollbar_dispose;*/
	widget_class->snapshot = gtkc_scrollbar_snapshot;
	widget_class->measure = gtkc_scrollbar_measure;

	/* this gets the theme-correct background */
	gtk_widget_class_set_css_name(widget_class, g_intern_static_string("scrollbar"));

	signals[VALUE_CHANGED] = g_signal_new("value-changed",
		G_TYPE_FROM_CLASS(object_class),
		G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL,
		G_TYPE_NONE, 0);
}

static void scb_motion_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer user_data)
{
	GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));
	GtkcScrollbar *scb = GTKC_SCROLLBAR(widget);
	double cval, target;

	if (!scb->dragging)
		return;

	cval = gtkc_scrollbar_screen2normal(scb, x, y);
	target = cval - scb->drag_offs;
	if (target < 0.0) target = 0.0;
	if (target > 1.0) target = 1.0;

	gtkc_scrollbar_set_val_normal(scb, target);

/*	printf("drag-move: %f %f val=%f offs=%f\n", x, y, target, scb->drag_offs);*/

	g_signal_emit(scb, signals[VALUE_CHANGED], 0);
}

static gboolean scb_mouse_press_cb(GtkGestureClick *self, gint n_press, double x, double y, gpointer user_data)
{
/*	guint btn = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(self));*/
	GtkcScrollbar *scb = user_data;
	double cval = gtkc_scrollbar_screen2normal(scb, x, y);

	if ((cval < scb->nval) || (cval > scb->nval + scb->nwin))
		return TRUE; /* not grabbing the slider */

	scb->drag_offs = cval - scb->nval;
	scb->dragging = 1;

/*	printf("drag start [%d] %d %f %f\n", btn, n_press, x, y, scb->drag_offs);*/

	return TRUE;
}

static gboolean scb_mouse_release_cb(GtkGestureClick *self, gint n_press, double x, double y, gpointer user_data)
{
/*	guint btn = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(self));*/
	GtkcScrollbar *scb = user_data;

/*	if (scb->dragging) printf("drag end [%d] %d %f %f\n", btn, n_press, x, y);*/

	scb->dragging = 0;

	return TRUE;
}


GtkWidget *gtkc_scrollbar_new(GtkOrientation orientation)
{
	GtkcScrollbar *scb = g_object_new(gtkc_scrollbar_get_type(), NULL);
	GtkEventController *bctrl, *mctrl = gtk_event_controller_motion_new();
	GtkGesture *gest = gtk_gesture_click_new();

	scb->orientation = orientation;

	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gest), 1);
	bctrl = GTK_EVENT_CONTROLLER(gest);

	g_signal_connect(G_OBJECT(mctrl), "motion", G_CALLBACK(scb_motion_cb), NULL);
	g_signal_connect(G_OBJECT(bctrl), "pressed", G_CALLBACK(scb_mouse_press_cb), scb);
	g_signal_connect(G_OBJECT(bctrl), "released", G_CALLBACK(scb_mouse_release_cb), scb);

	gtk_widget_add_controller(GTK_WIDGET(scb), mctrl);
	gtk_widget_add_controller(GTK_WIDGET(scb), bctrl);

	return GTK_WIDGET(scb);
}

void gtkc_scrollbar_set_range(GtkcScrollbar *scb, double min, double max, double win)
{
	if ((scb->min != min) || (scb->max != max) || (scb->win != win)) {
		scb->min = min;
		scb->max = max;
		scb->win = win;
		gtkc_scrollbar_normalize(scb);
/*printf("[%.2f..%.2f w%.2f v=%.2f] [%.2f..%.2f w%.2f v=%.2f]\n", scb->min, scb->max, scb->win, scb->val, scb->nmin, scb->nmax, scb->nwin, scb->nval);*/
		gtk_widget_queue_draw(GTK_WIDGET(scb));
	}
}

void gtkc_scrollbar_set_val(GtkcScrollbar *scb, double val)
{
	if (scb->val != val) {
		scb->val = val;
		gtkc_scrollbar_normalize(scb);
		gtk_widget_queue_draw(GTK_WIDGET(scb));
	}
}

void gtkc_scrollbar_set_val_normal(GtkcScrollbar *scb, double val01)
{
	double val, cw = scb->max - scb->min;

	if (val01 < 0.0) val01 = 0.0;
	if (val01 > 1.0 - scb->nwin) val01 = 1.0 - scb->nwin;

	val = val01 * cw + scb->min;

	if (scb->val != val) {
		scb->val = val;
		scb->nval = val01;
		gtk_widget_queue_draw(GTK_WIDGET(scb));
	}
}

double gtkc_scrollbar_get_val(GtkcScrollbar *scb)
{
	return scb->val;
}

double gtkc_scrollbar_get_val_normal(GtkcScrollbar *scb)
{
	return scb->nval;
}

void gtkc_scrollbar_get_range(GtkcScrollbar *scb, double *min, double *max, double *win)
{
	if (min != NULL) *min = scb->min;
	if (max != NULL) *max = scb->max;
	if (win != NULL) *win = scb->win;
}

