append /local/pcb/puller/enable {}
append /local/pcb/puller/buildin {}

put /local/pcb/mod {puller}
append /local/pcb/mod/OBJS [@ $(PLUGDIR)/puller/puller.o @]

if /local/pcb/puller/enable then
	if /local/pcb/puller/buildin then
		include {Makefile.in.mod/Buildin}
	else
		include {Makefile.in.mod/Plugin}
	end
end
