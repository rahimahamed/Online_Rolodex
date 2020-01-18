#include <sys/file.h>
#include <stdio.h>
#include <ctype.h>
#include <sgtty.h>
#include <signal.h>
#include <pwd.h>

#include "sys5.h"

#ifdef TMC
#include <ctools.h>
#else
#include "ctools.h"
#endif
#include "args.h"
#include "menu.h"
#include "mem.h"

#include "rolofiles.h"
#include "rolodefs.h"
#include "datadef.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "moreFunc.h"



static char rolodir[DIRPATHLEN];        /* directory where rolo data is */
static char filebuf[DIRPATHLEN];        /* stores result of homedir() */

int changed = 0;
int reorder_file = 0;
int rololocked = 0;
int in_search_mode = 0;

char *jmalloc_array[1000];
char *people[100];

static int malloc_counter = 1;


char *rolo_emalloc (size) int size;

/* error handling memory allocator */

{
  char *rval;
  if (0 == (rval = malloc(size))) {
     fprintf(stderr,"Fatal error:  out of memory\n");
     save_and_exit(-1);
  }
  memset(rval, 0, size);
  jmalloc_array[malloc_counter]=rval;
  malloc_counter++;
  return(rval);
}


char *copystr (s) char *s;

/* memory allocating string copy routine */


{
 char *copy;
 if (s == 0) return(0);
 copy = rolo_emalloc(strlen(s)+1);
 strcpy(copy,s);
 return(copy);
}


char *timestring ()

/* returns a string timestamp */

{
  char *s;
  long timeval;
  time(&timeval);
  s = ctime(&timeval);
  s[strlen(s) - 1] = '\0';
  return(copystr(s));
}


void user_interrupt ()

/* if the user hits C-C (we assume he does it deliberately) */

{
  unlink(homedir(ROLOLOCK));
  fprintf(stderr,"\nAborting rolodex, no changes since last save recorded\n");
  exit(-1);
}


void user_eof ()

/* if the user hits C-D */

{
  unlink(homedir(ROLOLOCK));
  fprintf(stderr,"\nUnexpected EOF on terminal. Saving rolodex and exiting\n");
  save_and_exit(-1);
}


void roloexit (rval) int rval;
{
  if (rololocked) unlink(homedir(ROLOLOCK));
  exit(rval);
}


void save_to_disk ()

/* move the old rolodex to a backup, and write out the new rolodex and */
/* a copy of the new rolodex (just for safety) */

{
  FILE *tempfp,*copyfp;
  char d1[DIRPATHLEN], d2[DIRPATHLEN];
  // int r;

  tempfp = fopen(homedir(ROLOTEMP),"w");
  copyfp = fopen(homedir(ROLOCOPY),"w");


  if (tempfp == NULL || copyfp == NULL ) {
     fprintf(stderr,"Unable to write rolodex...\n");
     fprintf(stderr,"Any changes made have not been recorded\n");
     roloexit(-1);
  }
  write_rolo(tempfp,copyfp);

  free(jmalloc_array[0]);
  int r = 1;
  while(jmalloc_array[r] != 0){
      free(jmalloc_array[r]);
      r++;
  }

  fclose(tempfp);
  fclose(copyfp);
  if (rename(strcpy(d1,homedir(ROLODATA)),strcpy(d2,homedir(ROLOBAK))) ||
      rename(strcpy(d1,homedir(ROLOTEMP)),strcpy(d2,homedir(ROLODATA)))) {
     fprintf(stderr,"Rename failed.  Revised rolodex is in %s\n",ROLOCOPY);
     roloexit(-1);
  }
  printf("Rolodex saved\n");
  sleep(1);
  changed = 0;
}


void save_and_exit (rval) int rval;
{
  if (changed) save_to_disk();
  roloexit(rval);
}


extern struct passwd *getpwnam();

char *home_directory (name) char *name;
{
  struct passwd *pwentry;
  if (0 == (pwentry = getpwnam(name))) return("");
  return(pwentry -> pw_dir);
}


char *homedir (filename) char *filename;

/* e.g., given "rolodex.dat", create "/u/massar/rolodex.dat" */
/* rolodir generally the user's home directory but could be someone else's */
/* home directory if the -u option is used. */

{
  nbuffconcat(filebuf,3,rolodir,"/",filename);
  return(filebuf);
}


char *libdir (filename) char *filename;

/* return a full pathname into the rolodex library directory */
/* the string must be copied if it is to be saved! */

{
  nbuffconcat(filebuf,3,ROLOLIB,"/",filename);
  return(filebuf);
}





void rolo_main (argc,argv) int argc; char *argv[];

{
    int fd,in_use,read_only,rolofd;
    Bool not_own_rolodex;
    char *user;
    FILE *tempfp;

    clearinit();
    clear_the_screen();

    /* parse the options and arguments, if any */

    int opt;
    int lflag = 0;
    int sflag = 0;
    int uflag = 0;
    int option_counter = 0;
    while((opt = getopt(argc, argv, "lsu:")) != -1){
      switch(opt){
        case 'l':
            option_counter++;
            if(lflag==0)
              lflag=1;
            else{
              fprintf(stderr,"duplicate option: %c\n",'l');
              roloexit(-1);
            }
            break;
        case 's':
            option_counter++;
            if(sflag==0)
              sflag=1;
            else{
              fprintf(stderr,"duplicate option: %c\n",'s');
              roloexit(-1);
            }
            break;
        case 'u':
            option_counter++;
            if(uflag==0)
              uflag=1;
            else{
              fprintf(stderr,"duplicate option: %c\n",'u');
              roloexit(-1);
            }
            break;
        default:
            fprintf(stderr,"illegal option\nusage: %s\n",USAGE);
            roloexit(-1);

      }
    }

    int non_opt_counter = argc -1 - option_counter;


    // char *arg_user = argv[optind];


    // switch (get_args(argc,argv,T,T)) {
    //     case ARG_ERROR :
    //       roloexit(-1);
    //     case NO_ARGS :
    //       break;
    //     case ARGS_PRESENT :
    //       if (ALL_LEGAL != legal_options(LEGAL_OPTIONS)) {
    //             fprintf(stderr,"illegal option\nusage: %s\n",USAGE);
    //             roloexit(-1);
    //       }
    // }

    /* find the directory in which the rolodex file we want to use is */

    not_own_rolodex = uflag;
    if (not_own_rolodex) {
       if (uflag == 1) {
          fprintf(stderr,"Illegal syntax using -u option\nusage: %s\n",USAGE);
          roloexit(-1);
       }
    }
    else {
       if (0 == (user = getenv("HOME"))) {
          fprintf(stderr,"Cant find your home directory, no HOME\n");
          roloexit(-1);
       }
    }

    if (not_own_rolodex) {
       strcpy(rolodir,home_directory(user));
       if (*rolodir == '\0') {
          fprintf(stderr,"No user %s is known to the system\n",user);
          roloexit(-1);
       }
    }
    else strcpy(rolodir,user);

    /* is the rolodex readable? */


    if (0 != access(homedir(ROLODATA),R_OK)) {

       /* No.  if it exists and we cant read it, that's an error */

       if (0 == access(homedir(ROLODATA),F_OK)) {
          fprintf(stderr,"Cant access rolodex data file to read\n");
          roloexit(-1);
       }

       /* if it doesn't exist, should we create one? */

       if (uflag) {
          fprintf(stderr,"No rolodex file belonging to %s found\n",user);
          roloexit(-1);
       }

       /* try to create it */

       if (-1 == (fd = creat(homedir(ROLODATA),0644))) {
          fprintf(stderr,"couldnt create rolodex in your home directory\n");
          roloexit(-1);
       }

       else {
          close(fd);
          fprintf(stderr,"Creating empty rolodex...\n");
       }

    }


    /* see if someone else is using it */

    in_use = (0 == access(homedir(ROLOLOCK),F_OK));

    /* are we going to access the rolodex only for reading? */

    if (!(read_only = sflag || non_opt_counter > 0)) {

       /* No.  Make sure no one else has it locked. */

       if (in_use) {
          if (uflag) {
            fprintf(stderr,"Someone else is modifying that rolodex, sorry\n");
            exit(-1);
          }
          else {
            cathelpfile(libdir("lockinfo"),"locked rolodex",0);
            exit(-1);
          }
       }

       /* create a lock file.  Catch interrupts so that we can remove */
       /* the lock file if the user decides to abort */

       if (!lflag) {
          if ((fd = open(homedir(ROLOLOCK),O_EXCL|O_CREAT,00200|00400)) < 0) {
             fprintf(stderr,"unable to create lock file...\n");
	     exit(1);
	  }
          rololocked = 1;
          close(fd);
          signal(SIGINT,user_interrupt);
       }

       /* open a temporary file for writing changes to make sure we can */
       /* write into the directory */

       /* when the rolodex is saved, the old rolodex is moved to */
       /* a '~' file, the temporary is made to be the new rolodex, */
       /* and a copy of the new rolodex is made */

       if (NULL == (tempfp = fopen(homedir(ROLOTEMP),"w"))) {
           fprintf(stderr,"Can't open temporary file to write to\n");
           roloexit(-1);
       }
       fclose(tempfp);

    }

    allocate_memory_chunk(CHUNKSIZE);

    if ('\0' == (rolofd = open(homedir(ROLODATA),O_RDONLY))) {
        fprintf(stderr,"Can't open rolodex data file to read\n");
        roloexit(-1);
    }

    /* read in the rolodex from disk */
    /* It should never be out of order since it is written to disk ordered */
    /* but just in case... */

    if (!read_only) printf("Reading in rolodex from %s\n",homedir(ROLODATA));
    read_rolodex(rolofd);
    close(rolofd);
    if (!read_only) printf("%d entries listed\n",rlength(Begin_Rlist));
    if (reorder_file && !read_only) {
       fprintf(stderr,"Reordering rolodex...\n");
       rolo_reorder();
       fprintf(stderr,"Saving reordered rolodex to disk...\n");
       save_to_disk();
    }

    /* the following routines live in 'options.c' */

    /* -s option.  Prints a short listing of people and phone numbers to */
    /* standard output */

    if (sflag) {
        print_short();
        exit(0);
    }

    /* rolo <name1> <name2> ... */
    /* print out info about people whose names contain any of the arguments */

    if (non_opt_counter > 0) {
       print_people();
       exit(0);
    }

    /* regular rolodex program */

    int z=0;
    while(optind<argc){
      optind++;
      people[z] = argv[optind];
      z++;
    }
    interactive_rolo();
    exit(0);

}
