/* [4.1.0] Perform an export using a specific export plugin. The export plugin
   is looked up by exporter_name. */
int rnd_hid_export_using(rnd_design_t *dsg, char *exporter_name, int argc, char *args[]);

/* [4.1.0] standard export action implementation - apps need to bind it */
extern const char rnd_acts_Export[];
extern const char rnd_acth_Export[];
extern fgw_error_t rnd_act_Export(fgw_arg_t *res, int argc, fgw_arg_t *argv);
