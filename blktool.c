#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#ifndef TRUE
#       define  TRUE    1
#endif
#ifndef FALSE
#       define  FALSE   0
#endif
#define VERSION_NUM	"0.1"
#define READFLAG     "O_RDONLY"
#define WRITEFLAG    "O_RDWR|O_TRUNC"

typedef enum
{
       FIELD_TM_CM_RW_SN_LN,        /* TIME, COMMAND, RWBS, SECTORNO, LENGTH           */
       FIELD_TM_CM_RW_SN_LN_DR,     /* TIME, COMMAND, RWBS, SECTORNO, LENGTH, DURATION */ 
       FIELD_DR,                    /* DURATION                                        */
} field_select_t;

char *help[]={
" Blktrace output parser "VERSION_NUM,
" ",
"  Usage: FILENAME [-v] [-f field_decision] INPUTFILE",
" ",
"          It parses number of blkparsed file for specific format",
"          Result file has extension of _parsed",
"          -v set verbose mode",
"          -f Choose fields to print one of following (Default = 0)",
"             (0 = All except Duration, 1 = All with Duration, 2 = Only Duration)",
""
};

void show_help()
{
        int i ;
        for (i=0; strlen(help[i]); i++)
                (void) fprintf (stdout, "%s\n", help[i]);
        return;
}

int     main    (int argc, char *argv [])
{
        char    buf [BUFSIZ];           /* a buffer                     */
        char    *tokptr, *strptr = buf; /* a pointer to the buffer      */
        int     c;                      /* general-purpose              */
        int     idx;                    /* token index                  */
        int     vopt = FALSE;           /* verbose option               */
        int     FieldSelectNo = 0;      /* field selector default 0     */
        char    separator[] = " ";      /* token separator              */
        char    row_of_fields[250];     /* temp line before printing    */
        char    TIME[20];               /* field names                  */
        char    COMMAND[5];             /* field names                  */
        char    RWBS[5];                /* field names                  */
        char    SECTORNO[15];           /* field names                  */
        char    LENGTH[10];             /* field names                  */
        char    DURATION[20];           /* field names                  */
        char    out_file[250];          /* output file                  */
        char    *out_ext = "_parsed";   /* output file extension        */
        FILE    *infp;                  /* input file                   */
        FILE    *outfp;                 /* output file                  */

        /*     Check for input file                                     */
        if (argc < 2)   show_help();

        /*      process the command line arguments                      */
        while ((c = getopt (argc, argv, "?vf:")) != EOF) {
                switch (c) {
                        case '?':
                                show_help();
                                break;
                        case 'v':
                                vopt = TRUE;
                                break;
                        case 'f':
                                FieldSelectNo = atoi(optarg);
                                break;
                        default:
                                show_help();
                                return (0);
                                break;
                }
        }
        /*      now process any arguments supplied...   */
        while (optind != argc) {
                if (vopt) {
                        (void) fprintf (stderr, "Processing %s...\n", argv [optind]);
                }
                /*      open the input file             */
                if ((infp = fopen (argv [optind], "r")) == (FILE *) NULL) {
                        (void) fprintf (stderr, "%s:\tcan't open %s\n", 
                                                            argv [0], argv [optind]);
                        exit (EXIT_FAILURE);
                }
                snprintf(out_file, sizeof out_file, "%s%s", argv [optind], out_ext);
                if ((outfp = fopen (out_file, "w")) == (FILE *) NULL){
                        (void) fprintf (stderr, "%s:\tcan't open %s\n", 
                                                            argv [0], out_file);
                }
                /*      get a line from the input file  */
                while (fgets (buf, sizeof (buf), infp) != (char *) NULL) {
                        /*      we have to point to buf */
                        strptr = buf;
                        /*      take the line apart     */
                        idx = 1; 
                        while ((tokptr = strtok (strptr, separator)) != (char *) NULL) {
                               if (idx == 1)  // Dev Maj/Min No 
                               {
                                      // (void) fprintf (stdout, "%s\t", tokptr); 
                                       idx++;
                               } 
                               else if (idx == 2) // CPU No
                               {
                                      // (void) fprintf (stdout, "%s\t", tokptr); 
                                       idx++;
                               } 
                               else if (idx == 3) // Sequence No
                               {
                                      // (void) fprintf (stdout, "%s\t", tokptr); 
                                       idx++; 
                               } 
                               else if (idx == 4) // TIME 
                               {
                                       if (vopt) (void) fprintf (stdout, "%s\t", tokptr); 
                                       //fprintf (outfp, "%s\t", tokptr);
                                       strcpy (TIME, tokptr);
                                       //strcpy (row_of_fields, tokptr);
                                       idx++; 
                               } 
                               else if (idx == 5) // Process No
                               {
                                      // (void) fprintf (stdout, "%s\t", tokptr); 
                                       idx++; 
                               }
                               else if (idx == 6) // Command
                               {
                                       if (vopt) (void) fprintf (stdout, "%s\t", tokptr); 
                                       //fprintf (outfp, "%s\t", tokptr);
                                       strcpy (COMMAND, tokptr);
                                       idx++; 
                               }
                               else if (idx == 7) // RWBS
                               {
                                       if (vopt) (void) fprintf (stdout, "%s\t", tokptr); 
                                       //fprintf (outfp, "%s\t", tokptr);
                                       strcpy (RWBS, tokptr);
                                       idx++; 
                               }
                               else if (idx == 8) // Sector No
                               {
                                       if (vopt) (void) fprintf (stdout, "%s\t", tokptr); 
                                       //fprintf (outfp, "%s\t", tokptr);
                                       strcpy (SECTORNO, tokptr);
                                       idx++; 
                               }
                               else if (idx == 9) // + Separator but check for [0]
                               {
                                      // (void) fprintf (stdout, "%s\t", tokptr); 
                                       idx++; 
                               }
                               else if (idx == 10) // Length 
                               {
                                       if (vopt) (void) fprintf (stdout, "%s \t", tokptr); 
                                       //fprintf (outfp, "%s\t", tokptr);
                                       strcpy (LENGTH, tokptr);
                                       idx++; 
                               }
                               else if (idx == 11) // ( separator or [0]
                               {
                                      // (void) fprintf (stdout, "%s\t", tokptr); 
                                       idx++; 
                               }
                               else if (idx == 12) // Duration
                               {
                                       if (vopt) (void) fprintf (stdout, "%s \n", tokptr); 
                                       //fprintf (outfp, "%s\n", tokptr);
                                       strcpy (DURATION, tokptr);
                                       idx++; 
                               }
                               /*      null the pointer        */
                                strptr = (char *) NULL;
                        }
                        if ( FieldSelectNo == 0 )
                        {
                               snprintf(row_of_fields, sizeof row_of_fields, 
                                           "%s\t%s\t%s\t%s\t%s", 
                                           TIME, COMMAND, RWBS, SECTORNO, LENGTH);
                        }
                        else if ( FieldSelectNo == 1 )
                        {
                               snprintf(row_of_fields, sizeof row_of_fields, 
                                           "%s\t%s\t%s\t%s\t%s\t%s", 
                                           TIME, COMMAND, RWBS, SECTORNO, LENGTH, DURATION);
                        }
                        else if ( FieldSelectNo == 2 )
                        {
                               snprintf(row_of_fields, sizeof row_of_fields, 
                                           "%s", DURATION); 
                        }
                        (void) fprintf (outfp, "%s\n", row_of_fields);
                }
                /*      close the input file            */
                if (fclose (infp))
                        (void) fprintf (stderr, "%s:\tcan't close %s\n", 
                                                            argv [0], argv [optind]);
                if (fclose (outfp))
                        (void) fprintf (stderr, "%s:\tcan't close %s\n",
                                                            argv [0], out_file);
                /*      bumpt the counter for next file */
                optind++;
        }
        return (0);
}
