/*============================================================================
frsc_read.c: S.W. Ellingson, Virginia Tech, 2014 Jan 25
Show output of frsc when written to file
---
COMPILE: (see makefile)
---
COMMAND LINE SYNTAX, INPUT, OUTPUT: 
  ra_show_file <infile> <ch>
  <infile>:  path/name of a frsc output file
  <ch>:      if specified, info specific to channel <ch> contained in eType=1 reports is written to "frsc_read.dat"
             valid values are [1..nCh]; values of 0 or less are ignored
---
REQUIRES
  Nothing special

See end of this file for history.

Ideas for Future Feature-adds:
---
============================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* for struct timeval, malloc() */
#include <time.h>

#include "ra_format.c"         /* output format definition */
#include "ra_format_defines.h" /* macro defines for field values in ra_format.c */

#define RA_MAX_FILENAME_LENGTH 1024

/*************************************************************************/
/*** main() **************************************************************/
/*************************************************************************/

main ( int narg, char *argv[] ) {

  /*=================*/
  /*=== Variables ===*/
  /*=================*/

  /* command line parameters */ 
  char infile[RA_MAX_FILENAME_LENGTH]; /* name of jobfile */

  struct ra_header_struct header; /* output report header; contains parameters that define operation */
  struct ra_td td;                /* this is what gets written as body of report */
  FILE *fp;
  FILE *fpo;

  int i;
  int n;
  int ch;

  int bFirst;

  /*======================================*/
  /*=== Acquire Command Line Arguments ===*/
  /*======================================*/

  /* read <jobfile> */
  memset(infile,'\0',RA_MAX_FILENAME_LENGTH);  /* just in case */
  if (narg>=2) {
      sscanf( argv[1], "%s", infile );
    } else {
      printf("FATAL: main(): <infile> not specified\n");
      return;
    } 
  printf("<infile>='%s'\n",infile);

  ch=0;
  if (narg>=3) {
    sscanf( argv[2], "%d", &ch );
    } 
  printf("<ch>=%d\n",ch);

  //return;

  /* open the input file */
  if (!(fp = fopen(infile,"rb"))) {
    printf("FATAL: main(): infile='%s' not found\n",infile);
    return;
    }

  /* open output file, if necessary */
  if (ch>0) {
    if (!(fpo = fopen("frsc_read.dat","w"))) {
      printf("FATAL: main(): couldn't fopen() output file\n");
      return;
      }
    }

  /* read the header */
  fread( &header, sizeof(struct ra_header_struct), 1, fp ); 

  bFirst = 1;
  while ( !feof(fp) ) {

    /* if this is the first header we've seen, show it: */
    if (bFirst) {
      bFirst = 0;

      printf("Contents of first report's header:\n");
      printf("  header.eType=%d\n",header.eType);
      printf("  header.err=%ld\n",header.err);
      printf("  header.iReportVersion=%hd\n",header.iReportVersion);
      printf("  header.iRAVersion=%hd\n",header.iRAVersion);
      printf("  header.eSource=%d\n",header.eSource);
      printf("  header.sInfo='%s'\n",header.sInfo);       
      printf("  header.tvStart: UTC %s",asctime(gmtime(&(header.tvStart.tv_sec))));
      printf("  header.nCh=%ld\n",header.nCh);
      printf("  header.bw=%lf\n",header.bw);
      printf("  header.fc=%lf\n",header.fc);
      printf("  header.fs=%lf\n",header.fs);
      printf("  header.tflags=0x%02x\n",(0xFF) & header.tflags);
      printf("  header.fflags=0x%02x\n",(0xFF) & header.fflags);
      printf("  header.T0=%lf\n",header.T0);
      printf("  header.T1=%lf\n",header.T1);
      printf("  header.T2=%lf\n",header.T2);
      printf("  header.bChIn:   ");
        for (i=RA_MAX_CH_DIV64-1;i>=0;i--) { printf("0x%016lx ", header.bChIn[i]); }
      printf("\n");
      printf("  header.bChInCh: ");
        for (i=RA_MAX_CH_DIV64-1;i>=0;i--) { printf("0x%016lx ", header.bChInCh[i]); }
      printf("\n");
      printf("  header.nSubCh=%ld\n",header.nSubCh);
      printf("  header.eSubChMethod=%d\n",header.eSubChMethod);
      printf("  header.eTBL_Method=%d\n",header.eTBL_Method);
      printf("  header.nTBL_Order=%d\n",header.nTBL_Order);
      printf("  header.eTBL_Units=%d\n",header.eTBL_Units);
      printf("  header.nfft=%d\n",header.nfft);
      printf("  header.nfch=%d\n",header.nfch);
      printf("  header.eFBL_Method=%d\n",header.eFBL_Method);
      printf("  header.nFBL_Order=%d\n",header.nFBL_Order);
      printf("  header.eFBL_Units=%d\n",header.eFBL_Units);
      printf("  header.iSeqNo=%ld\n",header.iSeqNo);
      printf("  header.fStart=%lf\n",header.fStart);

      printf("Now summarizing all reports found, one line per report in order received:\n");

      } /* if (bFirst) */

    printf("  ");
    printf("header.eType=%d ",header.eType);
    printf("header.err=%ld ",header.err);
    printf("header.iSeqNo=%ld ",header.iSeqNo);
    printf("header.fStart=%lf",header.fStart);
    printf("\n");

    /* read rest of report */
    switch (header.eType) {

      case RA_H_ETYPE_NULL:
        /* nothing else to do */
        break;

      case RA_H_ETYPE_TF0:
      case RA_H_ETYPE_TF1:

        /* remaining data is in a "struct ra_td"; read that */
        fread( &td, sizeof(struct ra_td), 1, fp );

        /* save data to file */
        fprintf(fpo, "%ld", header.iSeqNo);              // col 1
        fprintf(fpo, " %lf", header.fStart);              // col 2
        fprintf(fpo, " %ld %ld",td.clips.x,td.clips.y);  // col 3..4

        if (ch>0) {
          n = ch-1;

          fprintf(fpo, " %f %f %f %f %f %f %f %f %f %f", // col 5..14
            td.tdac[n].xi.mean, td.tdac[n].xi.max, td.tdac[n].xi.rms, td.tdac[n].xi.s, td.tdac[n].xi.k,
            td.tdac[n].xq.mean, td.tdac[n].xq.max, td.tdac[n].xq.rms, td.tdac[n].xq.s, td.tdac[n].xq.k
            );
          fprintf(fpo, " %f %f %f %f %f %f %f %f %f %f", // col 15..24
            td.tdac[n].yi.mean, td.tdac[n].yi.max, td.tdac[n].yi.rms, td.tdac[n].yi.s, td.tdac[n].yi.k,
            td.tdac[n].yq.mean, td.tdac[n].yq.max, td.tdac[n].yq.rms, td.tdac[n].yq.s, td.tdac[n].yq.k
            );
          fprintf(fpo, " %f %f %f %f %f %f %f %f %f %f", // col 25..34
            td.tdac[n].xm2.mean, td.tdac[n].xm2.max, td.tdac[n].xm2.rms, td.tdac[n].xm2.s, td.tdac[n].xm2.k,
            td.tdac[n].ym2.mean, td.tdac[n].ym2.max, td.tdac[n].ym2.rms, td.tdac[n].ym2.s, td.tdac[n].ym2.k
            );
          fprintf(fpo, " %f %f %f %f %f %f %f %f %f %f", // col 35..44
            td.tdac[n].u.mean, td.tdac[n].u.max, td.tdac[n].u.rms, td.tdac[n].u.s, td.tdac[n].u.k,
            td.tdac[n].v.mean, td.tdac[n].v.max, td.tdac[n].v.rms, td.tdac[n].v.s, td.tdac[n].v.k
            );
          fprintf(fpo,"\n");        

          } /* if (ch>0) */

        break;

      default:
        /* TODO */
        break;

      } /* switch (header.eType) */

    /* attempt to read the next header */
    fread( &header, sizeof(struct ra_header_struct), 1, fp ); 

    } /* while ( !feof(fp) ) */

  fclose(fp); 
  if (ch>0) { fclose(fpo); }

  return;
  } /* main() */

//==================================================================================
//=== HISTORY ======================================================================
//==================================================================================
// frsc_read.c: S.W. Ellingson, Virginia Tech, 2014 Jan 25
//   .1: changed name, making improvements
// ra_show.c: S.W. Ellingson, Virginia Tech, 2013 Dec 14
//   .1: Initial version
// ra_show_output.c: S.W. Ellingson, Virginia Tech, 2013 Nov 25
//   .1: initial version

//==================================================================================
//=== BELOW THIS LINE IS SCRATCH ===================================================
//==================================================================================

