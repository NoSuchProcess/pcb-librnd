plugin_header("\nFeature plugins:\n")
plugin_def("gpmi",            "GPMI scripting",            sbuildin)
plugin_def("autoroute",       "the original autorouter",   sbuildin)
plugin_def("toporouter",      "topological autorouter",    sdisable)
plugin_def("autoplace",       "auto place components",     sbuildin)
plugin_def("vendordrill",     "vendor drill mapping",      sbuildin)
plugin_def("puller",          "puller",                    sbuildin)
plugin_def("djopt",           "djopt",                     sbuildin)
plugin_def("mincut",          "minimal cut shorts",        sbuildin)
plugin_def("renumber",        "renumber action",           sbuildin)
plugin_def("oldactions",      "old/obsolete actions",      sdisable)
plugin_def("fontmode",        "font editor",               sbuildin)
plugin_def("legacy_func",     "legacy functions",          sbuildin)
plugin_def("stroke",          "libstroke gestures",        sdisable)
plugin_def("report",          "reprot actions",            sbuildin)
plugin_def("shand_cmd",       "command shorthands",        sbuildin)

plugin_header("\nFootprint backends:\n")
plugin_def("fp_fs",           "filesystem footprints",     sbuildin)
plugin_def("fp_wget",         "web footprints",            sbuildin)

plugin_header("\nImport plugins:\n")
plugin_def("import_sch",      "import sch",                sbuildin)
plugin_def("import_edif",     "import edif",               sbuildin)

plugin_header("\nExport plugins:\n")
plugin_def("export_gcode",    "gcode exporter",            sbuildin)
plugin_def("export_nelma",    "nelma exporter",            sbuildin)
plugin_def("export_png",      "png/gif/jpg exporter",      sbuildin)
plugin_def("export_bom",      "bom exporter",              sbuildin)
plugin_def("export_gerber",   "gerber exporter",           sbuildin)
plugin_def("export_lpr",      "lpr exporter (printer)",    sbuildin)
plugin_def("export_ps",       "postscript exporter",       sbuildin)

plugin_header("\nHID plugins:\n")
plugin_def("hid_batch",       "batch process (no-gui HID)",sbuildin)
plugin_def("hid_gtk",         "the GTK gui",               sbuildin)
plugin_def("hid_lesstif",     "the lesstif gui",           sbuildin)

