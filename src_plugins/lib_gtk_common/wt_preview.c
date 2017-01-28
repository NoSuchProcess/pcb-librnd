/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file copied and modified by Peter Clifton, starting from
 * gui-pinout-window.c, written by Bill Wilson for the PCB Gtk port
 * then got a major refactoring by Tibor 'Igor2' Palinkas in pcb-rnd
 */

#include "config.h"
#include "conf_core.h"

#include "wt_preview.h"

#include "copy.h"
#include "data.h"
#include "draw.h"
#include "move.h"
#include "rotate.h"
#include "obj_all.h"
#include "macro.h"

#include "in_mouse.h"

/* Just define a sensible scale, lets say (for example), 100 pixel per 150 mil */
#define SENSIBLE_VIEW_SCALE  (100. / PCB_MIL_TO_COORD (150.))
static void pinout_set_view(pcb_gtk_preview_t * pinout)
{
	float scale = SENSIBLE_VIEW_SCALE;
	pcb_element_t *elem = &pinout->element;

#warning switch for .kind here and do a zoom-to-extend on layer

	pinout->x_max = elem->BoundingBox.X2 + conf_core.appearance.pinout.offset_x;
	pinout->y_max = elem->BoundingBox.Y2 + conf_core.appearance.pinout.offset_y;
	pinout->w_pixels = scale * (elem->BoundingBox.X2 - elem->BoundingBox.X1);
	pinout->h_pixels = scale * (elem->BoundingBox.Y2 - elem->BoundingBox.Y1);
}


static void pinout_set_data(pcb_gtk_preview_t * pinout, pcb_element_t * element)
{
	if (element == NULL) {
		pcb_element_destroy(&pinout->element);
		pinout->w_pixels = 0;
		pinout->h_pixels = 0;
		return;
	}

	/* 
	 * copy element data 
	 * enable output of pin and padnames
	 * move element to a 5% offset from zero position
	 * set all package lines/arcs to zero width
	 */
	pcb_element_copy(NULL, &pinout->element, element, FALSE, 0, 0);
	PCB_PIN_LOOP(&pinout->element);
	{
		PCB_FLAG_SET(PCB_FLAG_DISPLAYNAME, pin);
	}
	PCB_END_LOOP;

	PCB_PAD_LOOP(&pinout->element);
	{
		PCB_FLAG_SET(PCB_FLAG_DISPLAYNAME, pad);
	}
	PCB_END_LOOP;


	pcb_element_move(NULL, &pinout->element,
											conf_core.appearance.pinout.offset_x -
											pinout->element.BoundingBox.X1, conf_core.appearance.pinout.offset_y - pinout->element.BoundingBox.Y1);

	pinout_set_view(pinout);

	PCB_ELEMENT_PCB_LINE_LOOP(&pinout->element);
	{
		line->Thickness = 0;
	}
	PCB_END_LOOP;

	PCB_ARC_LOOP(&pinout->element);
	{
		/* 
		 * for whatever reason setting a thickness of 0 causes the arcs to
		 * not display so pick 1 which does display but is still quite
		 * thin.
		 */
		arc->Thickness = 1;
	}
	PCB_END_LOOP;
}


enum {
	PROP_ELEMENT_DATA = 1,
	PROP_GPORT = 2,
	PROP_INIT_WIDGET = 3,
	PROP_EXPOSE = 4,
	PROP_KIND = 5,
	PROP_LAYER = 6,
};


static GObjectClass *ghid_pinout_preview_parent_class = NULL;


/*! \brief GObject constructed
 *
 *  \par Function Description
 *  Initialise the pinout preview object once it is constructed.
 *  Chain up in case the parent class wants to do anything too.
 *
 *  \param [in] object  The pinout preview object
 */
static void ghid_pinout_preview_constructed(GObject * object)
{

	/* chain up to the parent class */
	if (G_OBJECT_CLASS(ghid_pinout_preview_parent_class)->constructed != NULL)
		G_OBJECT_CLASS(ghid_pinout_preview_parent_class)->constructed(object);

}



/*! \brief GObject finalise handler
 *
 *  \par Function Description
 *  Just before the pcb_gtk_preview_t GObject is finalized, free our
 *  allocated data, and then chain up to the parent's finalize handler.
 *
 *  \param [in] widget  The GObject being finalized.
 */
static void ghid_pinout_preview_finalize(GObject * object)
{
	pcb_gtk_preview_t *pinout = GHID_PINOUT_PREVIEW(object);

	/* Passing NULL for element data will free the old memory */
	pinout_set_data(pinout, NULL);

	G_OBJECT_CLASS(ghid_pinout_preview_parent_class)->finalize(object);
}


/*! \brief GObject property setter function
 *
 *  \par Function Description
 *  Setter function for pcb_gtk_preview_t's GObject properties,
 *  "settings-name" and "toplevel".
 *
 *  \param [in]  object       The GObject whose properties we are setting
 *  \param [in]  property_id  The numeric id. under which the property was
 *                            registered with g_object_class_install_property()
 *  \param [in]  value        The GValue the property is being set from
 *  \param [in]  pspec        A GParamSpec describing the property being set
 */
static void ghid_pinout_preview_set_property(GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	pcb_gtk_preview_t *pinout = GHID_PINOUT_PREVIEW(object);
	GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(pinout));

	switch (property_id) {
	case PROP_ELEMENT_DATA:
		pinout->kind = PCB_GTK_PREVIEW_PINOUT;
		pinout_set_data(pinout, (pcb_element_t *) g_value_get_pointer(value));
		pinout->expose_data.content.elem = &pinout->element;
		if (window != NULL)
			gdk_window_invalidate_rect(window, NULL, FALSE);
		break;
	case PROP_GPORT:
		pinout->gport = (void *)g_value_get_pointer(value);
		break;
	case PROP_INIT_WIDGET:
		pinout->init_drawing_widget = (void *)g_value_get_pointer(value);
		break;
	case PROP_EXPOSE:
		pinout->expose = (void *)g_value_get_pointer(value);
		break;
	case PROP_KIND:
		pinout->kind = g_value_get_int(value);
		break;
	case PROP_LAYER:
		pinout->kind = PCB_GTK_PREVIEW_LAYER;
		pinout->expose_data.content.layer_id = g_value_get_long(value);
		if (window != NULL)
			gdk_window_invalidate_rect(window, NULL, FALSE);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}

}


/*! \brief GObject property getter function
 *
 *  \par Function Description
 *  Getter function for pcb_gtk_preview_t's GObject properties,
 *  "settings-name" and "toplevel".
 *
 *  \param [in]  object       The GObject whose properties we are getting
 *  \param [in]  property_id  The numeric id. under which the property was
 *                            registered with g_object_class_install_property()
 *  \param [out] value        The GValue in which to return the value of the property
 *  \param [in]  pspec        A GParamSpec describing the property being got
 */
static void ghid_pinout_preview_get_property(GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}

}

/* Converter: set up a pinout expose and use the generic preview expose call */
static gboolean ghid_preview_expose(GtkWidget * widget, GdkEventExpose * ev)
{
	pcb_gtk_preview_t *pinout = GHID_PINOUT_PREVIEW(widget);

	switch(pinout->kind) {
		case PCB_GTK_PREVIEW_PINOUT:
			pinout->expose_data.view.X1 = 0;
			pinout->expose_data.view.Y1 = 0;
			pinout->expose_data.view.X2 = pinout->x_max;
			pinout->expose_data.view.Y2 = pinout->y_max;
			return pinout->expose(widget, ev, pcb_hid_expose_pinout, &pinout->expose_data);

		case PCB_GTK_PREVIEW_LAYER:
			return pinout->expose(widget, ev, pcb_hid_expose_layer, &pinout->expose_data);

		case PCB_GTK_PREVIEW_INVALID:
		case PCB_GTK_PREVIEW_kind_max:
			return FALSE;
	}

	return FALSE;
}

/*! \brief GType class initialiser for pcb_gtk_preview_t
 *
 *  \par Function Description
 *  GType class initialiser for pcb_gtk_preview_t. We override our parent
 *  virtual class methods as needed and register our GObject properties.
 *
 *  \param [in]  klass       The pcb_gtk_preview_class_t we are initialising
 */
static void ghid_pinout_preview_class_init(pcb_gtk_preview_class_t * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *gtk_widget_class = GTK_WIDGET_CLASS(klass);

	gobject_class->finalize = ghid_pinout_preview_finalize;
	gobject_class->set_property = ghid_pinout_preview_set_property;
	gobject_class->get_property = ghid_pinout_preview_get_property;
	gobject_class->constructed = ghid_pinout_preview_constructed;

	gtk_widget_class->expose_event = ghid_preview_expose;

	ghid_pinout_preview_parent_class = (GObjectClass *) g_type_class_peek_parent(klass);

	g_object_class_install_property(gobject_class, PROP_ELEMENT_DATA,
		g_param_spec_pointer("element-data", "", "", G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_GPORT,
		g_param_spec_pointer("gport", "", "", G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_INIT_WIDGET,
		g_param_spec_pointer("init-widget", "", "", G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_EXPOSE,
		g_param_spec_pointer("expose", "", "", G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_KIND,
		g_param_spec_int("kind", "", "", 0, PCB_GTK_PREVIEW_kind_max-1, 0, G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_LAYER,
		g_param_spec_long("layer", "", "", -(1UL<<31), (1UL<<31)-1, -1, G_PARAM_WRITABLE));
}


/*! \brief Function to retrieve pcb_gtk_preview_t's GType identifier.
 *
 *  \par Function Description
 *  Function to retrieve pcb_gtk_preview_t's GType identifier.
 *  Upon first call, this registers the pcb_gtk_preview_t in the GType system.
 *  Subsequently it returns the saved value from its first execution.
 *
 *  \return the GType identifier associated with pcb_gtk_preview_t.
 */
GType pcb_gtk_preview_get_type()
{
	static GType ghid_pinout_preview_type = 0;

	if (!ghid_pinout_preview_type) {
		static const GTypeInfo ghid_pinout_preview_info = {
			sizeof(pcb_gtk_preview_class_t),
			NULL,											/* base_init */
			NULL,											/* base_finalize */
			(GClassInitFunc) ghid_pinout_preview_class_init,
			NULL,											/* class_finalize */
			NULL,											/* class_data */
			sizeof(pcb_gtk_preview_t),
			0,												/* n_preallocs */
			NULL,											/* instance_init */
		};

		ghid_pinout_preview_type =
			g_type_register_static(GTK_TYPE_DRAWING_AREA, "pcb_gtk_preview_t", &ghid_pinout_preview_info, (GTypeFlags) 0);
	}

	return ghid_pinout_preview_type;
}


GtkWidget *pcb_gtk_preview_pinout_new(void *gport, pcb_gtk_init_drawing_widget_t init_widget, pcb_gtk_preview_expose_t expose, pcb_element_t *element)
{
	pcb_gtk_preview_t *pinout_preview;

	pinout_preview = (pcb_gtk_preview_t *) g_object_new(GHID_TYPE_PINOUT_PREVIEW,
		"element-data", element,
		"gport", gport,
		"init-widget", init_widget,
		"expose", expose,
		"kind", PCB_GTK_PREVIEW_PINOUT,
		NULL);

	pinout_preview->init_drawing_widget(GTK_WIDGET(pinout_preview), pinout_preview->gport);

	return GTK_WIDGET(pinout_preview);
}

static void update_expose_data(pcb_gtk_preview_t *prv)
{
	pcb_gtk_zoom_post(&prv->view);
	prv->expose_data.view.X1 = prv->view.x0;
	prv->expose_data.view.Y1 = prv->view.y0;
	prv->expose_data.view.X2 = prv->view.x0 + prv->view.width;
	prv->expose_data.view.Y2 = prv->view.y0 + prv->view.height;

/*	pcb_printf("EXPOSE_DATA: %$mm %$mm - %$mm %$mm (%f %$mm)\n",
		prv->expose_data.view.X1, prv->expose_data.view.Y1,
		prv->expose_data.view.X2, prv->expose_data.view.Y2,
		prv->view.coord_per_px, prv->view.x0);*/
}

static gboolean preview_configure_event_cb(GtkWidget *w, GdkEventConfigure * ev, void *tmp)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *)w;
	preview->win_w = ev->width;
	preview->win_h = ev->height;

	preview->view.canvas_width = ev->width;
	preview->view.canvas_height = ev->height;

	update_expose_data(preview);
	return TRUE;
}

static void get_ptr(pcb_gtk_preview_t *preview, pcb_coord_t *cx, pcb_coord_t *cy, gint *xp, gint *yp)
{
	gdk_window_get_pointer(gtk_widget_get_window(GTK_WIDGET(preview)), xp, yp, NULL);
#undef SIDE_X
#undef SIDE_Y
#define SIDE_X(x) x
#define SIDE_Y(y) y
	*cx = EVENT_TO_PCB_X(&preview->view, *xp);
	*cy = EVENT_TO_PCB_Y(&preview->view, *yp);
#undef SIDE_X
#undef SIDE_Y
}

static gboolean button_press(GtkWidget *w, pcb_hid_cfg_mod_t btn)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *)w;
	pcb_coord_t cx, cy;
	gint wx, wy;
	get_ptr(preview, &cx, &cy, &wx, &wy);

	switch(btn) {
		case PCB_MB_LEFT:
			if (preview->mouse_cb != NULL) {
/*				pcb_printf("bp %mm %mm\n", cx, cy); */
				if (preview->mouse_cb(w, PCB_HID_MOUSE_PRESS, cx, cy))
					ghid_preview_expose(w, NULL);
			}
			break;
		case PCB_MB_RIGHT:
			preview->view.panning = 1;
			preview->grabx = cx;
			preview->graby = cy;
			break;
		case PCB_MB_SCROLL_UP:
			pcb_gtk_zoom_view_rel(&preview->view, 0, 0, 0.8);
			goto do_zoom;
		case PCB_MB_SCROLL_DOWN:
			pcb_gtk_zoom_view_rel(&preview->view, 0, 0, 1.25);
			goto do_zoom;
		default:
			return FALSE;
	}
	return FALSE;

	do_zoom:;
	preview->view.x0 = cx - (preview->view.canvas_width / 2) * preview->view.coord_per_px;
	preview->view.y0 = cy - (preview->view.canvas_height / 2) * preview->view.coord_per_px;
	update_expose_data(preview);
	ghid_preview_expose(w, NULL);

	return FALSE;
}

static gboolean preview_button_press_cb(GtkWidget *w, GdkEventButton * ev, gpointer data)
{
	return button_press(w, ghid_mouse_button(ev->button));
}

static gboolean preview_scroll_cb(GtkWidget *w, GdkEventScroll *ev, gpointer data)
{
	switch (ev->direction) {
		case GDK_SCROLL_UP:    return button_press(w, PCB_MB_SCROLL_UP);
		case GDK_SCROLL_DOWN:  return button_press(w, PCB_MB_SCROLL_DOWN);
		default:;
	}
	return FALSE;
}

static gboolean preview_button_release_cb(GtkWidget *w, GdkEventButton * ev, gpointer data)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *)w;

	switch(ghid_mouse_button(ev->button)) {
		case PCB_MB_RIGHT:
			preview->view.panning = 0;
			break;
		case PCB_MB_LEFT:
			if (preview->mouse_cb != NULL) {
				pcb_coord_t cx, cy;
				gint wx, wy;
				get_ptr(preview, &cx, &cy, &wx, &wy);
/*				pcb_printf("br %mm %mm\n", cx, cy); */
				if (preview->mouse_cb(w, PCB_HID_MOUSE_RELEASE, cx, cy))
					ghid_preview_expose(w, NULL);
			}
			break;
		default: ;
	}
	return FALSE;
}

#warning TODO: this should go in the renderer (e.g. gdk) API .h
gboolean ghid_preview_draw(GtkWidget * widget, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);

static gboolean preview_motion_cb(GtkWidget *w, GdkEventMotion * ev, gpointer data)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *)w;
	pcb_coord_t cx, cy;
	gint wx, wy;
	get_ptr(preview, &cx, &cy, &wx, &wy);

	if (preview->view.panning) {
		preview->view.x0 = preview->grabx - wx * preview->view.coord_per_px;
		preview->view.y0 = preview->graby - wy * preview->view.coord_per_px;
		update_expose_data(preview);
		ghid_preview_expose(w, NULL);
		return FALSE;
	}



	if (preview->mouse_cb != NULL) {
/*		pcb_printf("mo %mm %mm\n", cx, cy); */
		preview->mouse_cb(w, PCB_HID_MOUSE_MOTION, cx, cy);
		if (preview->overlay_draw_cb != NULL)
			ghid_preview_draw(w, preview->overlay_draw_cb, &preview->expose_data);
	}
	return FALSE;
}


/*
static gboolean preview_key_press_cb(GtkWidget *preview, GdkEventKey * kev, gpointer data)
{
	printf("kp\n");
}

static gboolean preview_key_release_cb(GtkWidget *preview, GdkEventKey * kev, gpointer data)
{
	printf("kr\n");
}
*/

GtkWidget *pcb_gtk_preview_layer_new(void *gport, pcb_gtk_init_drawing_widget_t init_widget, pcb_gtk_preview_expose_t expose, pcb_layer_id_t layer)
{
	pcb_gtk_preview_t *prv;

	prv = (pcb_gtk_preview_t *) g_object_new(GHID_TYPE_PINOUT_PREVIEW,
		"layer", layer,
		"gport", gport,
		"init-widget", init_widget,
		"expose", expose,
		"kind", PCB_GTK_PREVIEW_LAYER,
		"width-request", 50,
		"height-request", 50,
		NULL);

#warning TODO: maybe expose these through the object API so the caller can set it up?
	memset(&prv->view, 0, sizeof(prv->view));
	prv->view.width = PCB_MM_TO_COORD(110);
	prv->view.height = PCB_MM_TO_COORD(110);
	prv->view.coord_per_px = PCB_MM_TO_COORD(0.25);

	update_expose_data(prv);

	prv->expose_data.force = 1;
	prv->init_drawing_widget(GTK_WIDGET(prv), prv->gport);

	gtk_widget_add_events(GTK_WIDGET(prv), GDK_EXPOSURE_MASK
		| GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK | GDK_BUTTON_RELEASE_MASK
		| GDK_BUTTON_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_KEY_PRESS_MASK
		| GDK_FOCUS_CHANGE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);


	g_signal_connect(G_OBJECT(prv), "button_press_event", G_CALLBACK(preview_button_press_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "button_release_event", G_CALLBACK(preview_button_release_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "scroll_event", G_CALLBACK(preview_scroll_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "configure_event", G_CALLBACK(preview_configure_event_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "motion_notify_event", G_CALLBACK(preview_motion_cb), NULL);

/*
	g_signal_connect(G_OBJECT(prv), "key_press_event", G_CALLBACK(preview_key_press_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "key_release_event", G_CALLBACK(preview_key_release_cb), NULL);
*/

	return GTK_WIDGET(prv);
}

void pcb_gtk_preview_get_natsize(pcb_gtk_preview_t * pinout, int *width, int *height)
{
	*width = pinout->w_pixels;
	*height = pinout->h_pixels;
}
