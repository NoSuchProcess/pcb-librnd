#include <gtk/gtk.h>
#include <librnd/core/compat_misc.h>
#include "gtkc_trunc_label.h"


struct GtkcTruncLabel_s {
	GtkWidget parent_instance;
	PangoLayout *layout;
	char *text;
	PangoAttrList *attrs;
	int max_char_width;  /* in cairo units */
	int max_char_height; /* in pixels */
	int rotated;
	int notruncate;
};

typedef struct {
	GtkWidgetClass parent_class;
} GtkcTruncLabelClass;

G_DEFINE_TYPE(GtkcTruncLabel, gtkc_trunc_label, GTK_TYPE_WIDGET)


static void gtkc_trunc_label_invalidate(GtkcTruncLabel *tl)
{
	if (tl->layout != NULL) {
		g_object_unref(tl->layout);
		tl->layout = NULL;
	}
}

static void gtkc_trunc_label_dispose(GObject *object)
{
	GtkcTruncLabel *tl = GTKC_TRUNC_LABEL(object);

	free(tl->text);
	tl->text = NULL;

	gtkc_trunc_label_invalidate(tl);
	if (tl->attrs != NULL)
		g_object_unref(tl->attrs);

	G_OBJECT_CLASS(gtkc_trunc_label_parent_class)->dispose(object);
}

static void gtkc_trunc_label_init(GtkcTruncLabel *tl)
{
	tl->text = NULL;
	tl->layout = NULL;
}


static void gtkc_trunc_label_ensure_layout(GtkcTruncLabel *self)
{
	PangoAlignment align;
	gboolean rtl;

	if (self->layout)
		return;

	rtl = gtk_widget_get_direction(GTK_WIDGET(self)) == GTK_TEXT_DIR_RTL;
	self->layout = gtk_widget_create_pango_layout(GTK_WIDGET(self), self->text);

	pango_layout_set_attributes(self->layout, self->attrs);

	/* always left aligned */
	align = rtl ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT;

	pango_layout_set_alignment(self->layout, align);
	pango_layout_set_ellipsize(self->layout, 0);
/*	pango_layout_set_wrap(self->layout, self->wrap_mode);*/
	pango_layout_set_single_paragraph_mode(self->layout, 1);

/*	if (self->lines > 0)
		pango_layout_set_height (self->layout, - self->lines);*/

	{
		PangoContext *context;
		PangoFontMetrics *metrics;
		int char_width, digit_width, chr_height;

		context = pango_layout_get_context(self->layout);
		metrics = pango_context_get_metrics(context, NULL, NULL);
		char_width = pango_font_metrics_get_approximate_char_width(metrics);
		digit_width = pango_font_metrics_get_approximate_digit_width(metrics);
		pango_layout_get_pixel_size(self->layout, NULL, &chr_height);
		pango_font_metrics_unref(metrics);

		self->max_char_width = MAX(char_width, digit_width);
		self->max_char_height = chr_height;
	}
}


static void gtkc_trunc_label_snapshot(GtkWidget *widget, GtkSnapshot *snapshot)
{
	GtkcTruncLabel *self = GTKC_TRUNC_LABEL(widget);
	int wh, ww, cliph, clipw;

	GtkStyleContext *context;

	wh = gtk_widget_get_height(widget);
	ww = gtk_widget_get_width(widget);

	gtkc_trunc_label_ensure_layout(self);

	context = gtk_widget_get_style_context (widget);

	if (self->rotated) {
		GskTransform *tr;

		cliph = ww;
		clipw = wh;
		tr = gsk_transform_new();
		tr = gsk_transform_translate(tr, &GRAPHENE_POINT_INIT(0, gtk_widget_get_height(widget)));
		tr = gsk_transform_rotate(tr, -90);
		gtk_snapshot_transform(snapshot, tr);
		gsk_transform_unref(tr);
	}
	else {
		cliph = wh;
		clipw = ww;
	}

	gtk_snapshot_push_clip(snapshot, &GRAPHENE_RECT_INIT(0, 0, clipw, cliph));
	gtk_snapshot_render_layout (snapshot, context, 0, 0, self->layout);
	gtk_snapshot_pop(snapshot);
}

static int get_text_len(GtkcTruncLabel *self)
{
	PangoRectangle rect;
	pango_layout_get_extents(self->layout, NULL, &rect);
	return rect.width;
}

static void gtkc_trunc_label_measure(GtkWidget *widget, GtkOrientation orientation, int for_size, int *minimum, int *natural, int *minimum_baseline, int *natural_baseline)
{
	GtkcTruncLabel *self = GTKC_TRUNC_LABEL(widget);

	gtkc_trunc_label_ensure_layout(self);

	if (orientation == GTK_ORIENTATION_VERTICAL) { /* vertical on screen */
		if (!self->rotated) {
			*minimum = *natural = self->max_char_height;
		}
		else { /* 90 deg rotation */
			if (self->notruncate)
				*minimum = *natural = PANGO_PIXELS_CEIL(get_text_len(self));
			else
				*minimum = *natural = PANGO_PIXELS_CEIL(self->max_char_width);
		}
	}
	else { /* horizontal on screen */
		if (!self->rotated) {
			if (self->notruncate)
				*minimum = *natural = PANGO_PIXELS_CEIL(get_text_len(self));
			else
				*minimum = *natural = PANGO_PIXELS_CEIL(self->max_char_width);
		}
		else /* 90 deg rotation */
			*minimum = *natural = self->max_char_height;
	}

	*minimum_baseline = *natural_baseline = -1;
}


static void gtkc_trunc_label_class_init(GtkcTruncLabelClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

	object_class->dispose = gtkc_trunc_label_dispose;
	widget_class->snapshot = gtkc_trunc_label_snapshot;
	widget_class->measure = gtkc_trunc_label_measure;
}


GtkWidget *gtkc_trunc_label_new(const char *text)
{
	GtkcTruncLabel *tl = g_object_new(gtkc_trunc_label_get_type(), NULL);
	tl->text = rnd_strdup(text);
	return GTK_WIDGET(tl);
}

void gtkc_trunc_set_rotated(GtkcTruncLabel *self, gboolean rotated)
{
	if (self->rotated != rotated) {
		self->rotated = rotated;
		gtkc_trunc_label_invalidate(self);
	}
}


void gtkc_trunc_set_notruncate(GtkcTruncLabel *self, gboolean notruncate)
{
	if (self->notruncate != notruncate) {
		self->notruncate = notruncate;
		gtkc_trunc_label_invalidate(self);
	}
}

