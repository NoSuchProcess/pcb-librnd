extern const char rnd_acts_ExportDialog[];
extern const char rnd_acth_ExportDialog[];
fgw_error_t rnd_act_ExportDialog(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

extern const char rnd_acts_PrintDialog[];
extern const char rnd_acth_PrintDialog[];
fgw_error_t rnd_act_PrintDialog(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

/* Creates and runs the non-modal dialog. Useful for implementing alternative
   actioins, e.g. for exporting a whole project */
void rnd_dlg_export(const char *title, int exporters, int printers, rnd_design_t *dsg, void *appspec);


void rnd_dialog_export_close(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);

