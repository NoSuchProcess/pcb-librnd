PkgLoad("pcb-rnd-gpmi/actions", 0);
PkgLoad("pcb-rnd-gpmi/dialogs", 0);

def ev_action(id, name, argc, x, y)
	dialog_report("Greeting window", "Hello world!");
end

Bind("ACTE_action", "ev_action");
action_register("hello", "", "log hello world", "hello()");
