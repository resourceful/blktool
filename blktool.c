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
#define VERSION_NUM	"0.2"
#define READFLAG     "O_RDONLY"
#define WRITEFLAG    "O_RDWR|O_TRUNC"

typedef enum
{
        FIELD_TM_CM_RW_SN_LN,        /* TIME, COMMAND, RWBS, SECTORNO, LENGTH           */
        FIELD_TM_CM_RW_SN_LN_DR,     /* TIME, COMMAND, RWBS, SECTORNO, LENGTH, DURATION */ 
        FIELD_DR,                    /* DURATION                                        */
        FIELD_TM_SN,                 /* DURATION                                        */
} field_select_t;

typedef enum
{
        RWBS_WRITE,                   /* Write                                          */
        RWBS_READ,                    /* READ                                           */
        RWBS_RD_WR,                   /* READ and Write                                 */
}  RWBS_select_t;

typedef enum
{
        CMD_COMPLETE,                 /* Complete                                      */
} CMD_select_t;

typedef enum
{
        CDF_TIME,                    /* Time                                            */
        CDF_SECTOR,                  /* Sector No                                       */ 
        CDF_LENGTH,                  /* Length                                          */
        CDF_DURATION,                /* Duration                                        */
} CDF_select_t;


/* global variable */
int     FieldSelectNo = 0;      /* field selector default 0     */
char    row_of_fields[250];     /* temp line before printing    */
char    TIME[20];               /* field names                  */
char    COMMAND[5];             /* field names                  */
char    RWBS[5];                /* field names                  */
char    SECTORNO[15];           /* field names                  */
char    LENGTH[10];             /* field names                  */
char    DURATION[20];           /* field names                  */
int     vopt = FALSE;           /* verbose option               */
int     CDFopt = FALSE;         /* CDF option                   */
int     RWBSSelect  = 0;        /* command selector default 0   */
int     CDFSelect  = 0;         /* CDF column default 0         */
char    separator[] = " \n";    /* token separator              */
char    out_file[250];          /* output file                  */
char    *out_ext = "_parsed";   /* output file extension        */
char    *RWBS_ext;              /* read_write extension         */

char *help[]={
" Blktrace output parser "VERSION_NUM,
" ",
"  Usage: FILENAME [-v] [-c] [-f field_decision] [-r Read_Write] in_file",
" ",
"          It parses number of blkparsed file for specific format",
"          Result file has extension of _parsed",
"          ",
"          -v Set verbose mode",
"          -c Set CDF mode",
"          -f Choose fields to print one of following (Default = 0)",
"             Deafault output is Time, Command, RWBS, Sectorno, Length.",
"             (0 = All except Duration, 1 = All with Duration, 2 = Only Duration)",
"             (3 = TIME and Sector No)", 
"          -r Choose commnads from read and write (Default = 0)", 
"             (0 = Write, 1 = Read, 2 = Read and Write )",
""
};

void show_help()
{
        int i ;
        for (i=0; strlen(help[i]); i++)
                (void) fprintf (stdout, "%s\n", help[i]);
        return;
}

void FieldSelection()
{
        if (FieldSelectNo ==  FIELD_TM_CM_RW_SN_LN)
        {
        snprintf(row_of_fields, sizeof row_of_fields, 
                "%s\t%s\t%s\t%s\t%s", 
        TIME, COMMAND, RWBS, SECTORNO, LENGTH);
        }
        else if (FieldSelectNo == FIELD_TM_CM_RW_SN_LN_DR )
        {
               snprintf(row_of_fields, sizeof row_of_fields, 
                           "%s\t%s\t%s\t%s\t%s\t%s", 
                           TIME, COMMAND, RWBS, SECTORNO, LENGTH, DURATION);
        }
        else if (FieldSelectNo == FIELD_DR )
        {
               snprintf(row_of_fields, sizeof row_of_fields, 
                           "%s", DURATION); 
        }
        else if (FieldSelectNo == FIELD_TM_SN )
        {
               snprintf(row_of_fields, sizeof row_of_fields, 
                           "%s\t%s", TIME, SECTORNO); 
        }

        if (vopt) (void) fprintf (stdout, "%s \n", row_of_fields); 
}

/* Checks if a given string is in a string, with wild card support    *
 * Returns TRUE only if the match is found.                         *
*/
int match (char *SRC, char *DEST)
{
       /* Reached the end of both strings                             */
       if (*SRC == '\0' && *DEST == '\0')
                return TRUE; 
       /* Checks if the character after '*' is present in DEST.       *
        * Assume that there is no '**' in the SRC.                    *
       */
       if (*SRC == '*' && *(SRC + 1) != '\0' && *DEST == '\0')
                return FALSE;
       /* If the SRC contains '?', or current characters of SRC and   *
        * DEST match, keep on matching                                *
       */
       if (*SRC == '?' || *SRC == *DEST)
                return match(SRC+1, DEST+1);
       /* There are two possibilities when '*' is in SRC:             *
        * 1) Look for current character of DEST                       *
        * 2) Ignore current character in DEST                         *
       */
       if (*SRC == '*')
                return match(SRC+1, DEST) || match(SRC, DEST+1);
       return FALSE;
}

void calculate_cdf(char *in_file, int col_idx)
{
       int     idx = 1;                /* token index                  */
       char    out_file[250];          /* output file name             */
       char    tmp_file[250];          /* tmp output file name         */
       char    *cdf_ext = "_cdf";      /* extension for CDF result     */
       char    buf [BUFSIZ];           /* a buffer                     */
       char    command[250];           /* command line for system      */
       char    *tokptr, *strptr = buf; /* a pointer to the buffer      */
       FILE    *infp;                  /* input file                   */
       FILE    *outfp;                 /* input file                   */
       double  cumsum;                 /* cumulative sum               */ 

       /* open input file              */
       if ((infp = fopen (in_file, "r")) == (FILE *) NULL) {
               (void) fprintf (stderr, "can't open input %s\n", in_file);
               exit (EXIT_FAILURE);
       }
       snprintf(out_file, sizeof out_file, "%s%s", in_file, cdf_ext);
       if ((outfp = fopen (out_file, "wt")) == (FILE *) NULL) {
               (void) fprintf (stderr, "can't open output %s\n", out_file);
               exit (EXIT_FAILURE);
       }

       snprintf(tmp_file, sizeof tmp_file, "%s%s", in_file, "_temp");

       snprintf (command, sizeof command, "%s %d,%dn %s > %s",
                             "sort --parallel=10 --buffer-size=5G -k",
                             col_idx, col_idx, in_file, tmp_file);
       if (vopt) (void) fprintf (stderr, "Command is : %s\n", command);
       system (command);

       /*     read the file for cumulative sum */
       while (fgets (buf, sizeof (buf), infp) != (char *) NULL) {
               /*      we have to point to buf */
               strptr = buf;
               /*      take the line apart     */
               idx = 1 ;
               /* only first column is used for cumulative sum for now */
               while ((tokptr = strtok (strptr, separator)) != (char *) NULL) {
                      if (idx == 1)  // only first column for now 
                      {
                             idx++;
                             cumsum += atof (tokptr); 
                      } 
               strptr = (char *) NULL;
               }
       }

       /*     rewind and calcuate cdf          */
       if (vopt) (void) fprintf (stderr, "calculating CDF: \n");
       rewind (infp);
       while (fgets (buf, sizeof (buf), infp) != (char *) NULL) {
               /*      we have to point to buf */
               strptr = buf;
               /*      take the line apart     */
               idx = 1 ;
               /* only first column is used for cumulative sum for now */
               while ((tokptr = strtok (strptr, separator)) != (char *) NULL) {
                      if (idx == 1)  // only first column for now 
                      {
                             idx++;
                             if (vopt) (void) fprintf (stderr, "%s\t%f\n", 
                                    tokptr, (double) atof (tokptr)/cumsum*100);
                             (void) fprintf (outfp, "%s\t%f\n", 
                                    tokptr, (double) atof (tokptr)/cumsum*100);
                      } 
               strptr = (char *) NULL;
               }
       }

       /*      close the input file            */
       if (fclose (infp)) {
               (void) fprintf (stderr, "can't close %s\n", in_file);
               exit (EXIT_FAILURE);
       }
}

int     main    (int argc, char *argv [])
{
        char    buf [BUFSIZ];           /* a buffer                     */
        char    *tokptr, *strptr = buf; /* a pointer to the buffer      */
        int     c;                      /* general-purpose              */
        int     SkipLine = 0;           /* pattern match checker        */
        int     idx;                    /* token index                  */
        FILE    *infp;                  /* input file                   */
        FILE    *outfp;                 /* output file                  */

        /*     Check for input file                                     */
        if (argc < 2)   show_help();

        /*      process the command line arguments                      */
        while ((c = getopt (argc, argv, "?vc:r:f:")) != EOF) {
                switch (c) {
                        case '?':
                                show_help();
                                break;
                        case 'v':
                                vopt = TRUE;
                                break;
                        case 'c':
                                CDFopt = TRUE;
                                CDFSelect = atoi(optarg);
                                break;
                        case 'r':
                                RWBSSelect = atoi(optarg);
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

        if (RWBSSelect == RWBS_WRITE)
        {
               RWBS_ext = "_W"; 
        }
        else if (RWBSSelect == RWBS_READ)
        {
               RWBS_ext = "_R";
        }
        else if (RWBSSelect == RWBS_RD_WR)
        {
               RWBS_ext = "_RW";
        } 

        /*      now process any arguments supplied...   */
        while (optind != argc) {
                if (vopt) {
                        (void) fprintf (stderr, "Processing %s...\n", argv [optind]);
                }
                if (CDFopt){
                        calculate_cdf(argv [optind], CDFSelect);
                        break; 
                        return 0;
                }
                /*      open the input file             */
                if ((infp = fopen (argv [optind], "r")) == (FILE *) NULL) {
                        (void) fprintf (stderr, "%s:\tcan't open %s\n", 
                                                            argv [0], argv [optind]);
                        exit (EXIT_FAILURE);
                }
                snprintf(out_file, sizeof out_file, "%s%s%s", argv [optind], RWBS_ext,  out_ext);
                if ((outfp = fopen (out_file, "w")) == (FILE *) NULL){
                        (void) fprintf (stderr, "%s:\tcan't open %s\n", 
                                                            argv [0], out_file);
                        exit (EXIT_FAILURE);
                }
                /*      get a line from the input file  */
                while (fgets (buf, sizeof (buf), infp) != (char *) NULL) {
                        /*      we have to point to buf */
                        strptr = buf;
                        /* set SkipLine to zero for next iteration */
                        SkipLine = 0;
                        /*      take the line apart     */
                        idx = 1; 
                        while ((tokptr = strtok (strptr, separator)) != (char *) NULL) {
                               if (idx == 1)  // Dev Maj/Min No 
                               {
                                       idx++;
                               } 
                               else if (idx == 2) // CPU No
                               {
                                       idx++;
                               } 
                               else if (idx == 3) // Sequence No
                               {
                                       idx++; 
                               } 
                               else if (idx == 4) // TIME 
                               {
                                       strcpy (TIME, tokptr);
                                       idx++; 
                               } 
                               else if (idx == 5) // Process No
                               {
                                       idx++; 
                               }
                               else if (idx == 6) // Command
                               {
                                       strcpy (COMMAND, tokptr);
                                       idx++; 
                               }
                               else if (idx == 7) // RWBS
                               {
                                       strcpy (RWBS, tokptr);
                                       idx++; 
                               }
                               else if (idx == 8) // Sector No
                               {
                                       strcpy (SECTORNO, tokptr);
                                       if (match ("[*", tokptr))
                                              SkipLine = 1;
                                       idx++; 
                               }
                               else if (idx == 9) // + Separator but check for [0]
                               {
                                       if (!match ("+", tokptr))
                                              SkipLine = 1;
                                       idx++; 
                               }
                               else if (idx == 10) // Length 
                               {
                                       strcpy (LENGTH, tokptr);
                                       idx++; 
                               }
                               else if (idx == 11) // ( separator or [0]
                               {
                                       if (!match ("(", tokptr))
                                              SkipLine = 1;
                                       idx++; 
                               }
                               else if (idx == 12) // Duration
                               {
                                       strcpy (DURATION, tokptr);
                                       idx++; 
                               }
                               /*      null the pointer        */
                                strptr = (char *) NULL;
                        }
                        /* printout only if the line has proper result */
                        if (SkipLine == 0 )
                        {
                                if (RWBSSelect == RWBS_WRITE && match("*W*", RWBS) 
                                          && match("*C*", COMMAND)) 
                                {
                                       FieldSelection(); 
                                }
                                else if (RWBSSelect == RWBS_READ && match("*R*", RWBS)
                                          && match("*C*", COMMAND)) 
                                {
                                       FieldSelection(); 
                                }
                                else if (RWBSSelect == RWBS_RD_WR && match("*C*", COMMAND)) 
                                {
                                       FieldSelection(); 
                                } 
                                (void) fprintf (outfp, "%s\n", row_of_fields);
                        }
                }

                /*      close the input file            */
                if (fclose (infp)) {
                        (void) fprintf (stderr, "%s:\tcan't close %s\n", 
                                                            argv [0], argv [optind]);
                        exit (EXIT_FAILURE);
                }
                if (fclose (outfp)) {
                        (void) fprintf (stderr, "%s:\tcan't close %s\n",
                                                            argv [0], out_file);
                        exit (EXIT_FAILURE);
                }
                /*      bumpt the counter for next file */
                optind++;
        }
        return (0);
}
