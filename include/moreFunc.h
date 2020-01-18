#ifndef MOREFUNC_H
#define MOREFUNC_H

#include "datadef.h"
#include <stdio.h>

extern char *ptr_bigmalloc;
extern char *jmalloc_array[1000];
extern char *people[100];



void write_rolo (FILE *fp1, FILE *fp2);

void cathelpfile (char *filepath, char *helptopic, int clear);

void clearinit ();

void clear_the_screen ();

int read_rolodex (int fd);

int rlength (Ptr_Rolo_List rlist);

void rolo_reorder();

void print_short();

void print_people();

void interactive_rolo();

int rolo_menu_yes_no (char *prompt, int rtn_default, int help_allowed, char *helpfile,char *subject);

void any_char_to_continue ();

void save_and_exit (int rval);

void user_eof();

void display_entry(Ptr_Rolo_Entry entry);

void rolo_insert (Ptr_Rolo_List link, int (*compare)());

int nocase_compare (char *s1, int l1, char *s2, int l2);

void rolo_peruse_mode (Ptr_Rolo_List first_rlink);

void rolo_update_mode (Ptr_Rolo_List rlink);

void rolo_delete (Ptr_Rolo_List link);

void roloexit (int rval);

void rolo_search_mode (int field_index, char *field_name, char *search_string);

void rolo_add ();

int select_field_to_search_by (int *ptr_index, char **ptr_name);

void save_to_disk ();

void display_entry_for_update (Ptr_Rolo_Entry entry);

int rolo_menu_data_help_or_abort (char *prompt, char *helpfile, char *subject, char **ptr_response);

void display_field_names ();

int rolo_menu_number_help_or_abort (char *prompt, int low, int high, int *ptr_ival);

int entry_action (Ptr_Rolo_List rlink);

void display_list_of_entries (Ptr_Rolo_List rlist);

void summarize_entry_list (Ptr_Rolo_List rlist, char *ss);

#endif