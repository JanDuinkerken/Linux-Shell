FLAGS=-Wall
SUBDIR=bin/
SUCCESS=@echo Build successful!

shell: $(SUBDIR)shell.o $(SUBDIR)cmd_delete_internal.o $(SUBDIR)cmd_list_internal.o $(SUBDIR)list.o $(SUBDIR)proc_list.o
	gcc $(SUBDIR)shell.o $(SUBDIR)cmd_delete_internal.o $(SUBDIR)cmd_list_internal.o $(SUBDIR)list.o $(SUBDIR)proc_list.o $(FLAGS) -o shell
	$(SUCCESS)

delete: $(SUBDIR)cmd_delete.o $(SUBDIR)cmd_delete_internal.o $(SUBDIR)cmd_list_internal.o
	gcc $(SUBDIR)cmd_delete.o $(SUBDIR)cmd_delete_internal.o $(SUBDIR)cmd_list_internal.o $(FLAGS) -o delete
	$(SUCCESS)

list: $(SUBDIR)cmd_list.o $(SUBDIR)cmd_list_internal.o
	gcc $(SUBDIR)cmd_list.o $(SUBDIR)cmd_list_internal.o $(FLAGS) -o list
	$(SUCCESS)

$(SUBDIR)%.o: %.c
	@mkdir -p $(SUBDIR)
	gcc $(FLAGS) -c -o $@ $<
