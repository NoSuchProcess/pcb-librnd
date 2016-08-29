typedef struct {
	/* property list */
	GtkWidget *tree;        /* property list widget */
	GtkListStore *props;    /* list model of properties */

	/* value entry */
	GtkWidget *entry_val;   /* text entry */
	GtkWidget *val_box;     /* combo box */
	GtkListStore *vals;     /* model of the combo box */
	int stock_val;          /* 1 if the value in the entry box is being edited from the combo */

	/* buttons */
	GtkWidget *apply, *remove, *addattr;
} ghid_propedit_dialog_t;

GtkWidget *ghid_propedit_dialog_create(ghid_propedit_dialog_t *dlg);

void ghid_propedit_prop_add(ghid_propedit_dialog_t *dlg, const char *name, const char *common, const char *min, const char *max, const char *avg);
