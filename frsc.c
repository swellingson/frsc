/*============================================================================
frsc.c: S.W. Ellingson, Virginia Tech, 2014 Jan 26
(Front-end for RF Signal Characterization) 
---
COMPILE: (see makefile)
---
COMMAND LINE SYNTAX, INPUT, OUTPUT: 
  frsc <job_file> 
  <job_file>:   ASCII file defining what we're supposed to do. See ra_read_jobfile() for format.

See end of this file for history.

============================================================================*/
#define RA_H_RA_VERSION 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* for struct timeval, malloc() */
#include <time.h>
#include <math.h>

#include "ra_aux.c"            /* auxilliary (support) code, put here to avoid cluttering up this file */
#include "ra_format.c"         /* output format definition */
#include "ra_format_defines.h" /* macro defines for field values in ra_format.c */
#include "ra_read_jobfile.c"   /* code that reads jobfile */
#include "ra_guppi_file.c"     /* code that reads GUPPI raw data file */
#include "ra_analyze.c"        /* analysis; called from ra_swallow() */
#include "ra_swallow.c"        /* copies data from raw sample blocks into rate-T0 and -T1 buffers, launches analysis as needed */

#define RA_MAX_FILENAME_LENGTH 1024

/*************************************************************************/
/*** main() **************************************************************/
/*************************************************************************/

main ( int narg, char *argv[] ) {

  /*=================*/
  /*=== Variables ===*/
  /*=================*/

  /* command line parameters */ 
  char jobfile[RA_MAX_FILENAME_LENGTH]; /* name of jobfile */

  
  struct ra_header_struct header0; /* output report header; contains parameters that define operation; prototype for all headers created later */
  char infile[RA_MAX_FILENAME_LENGTH]; /* name of input data file (used when raw data file mode selected) */

  FILE *fp_out;
  FILE *fp_in;             

  char rg_header[RG_MAX_HEADER_LENGTH];
  signed char *blk;                     /* memory for contiguous block of raw data: this is explicitly allocated below */

  int bDone=0;       /* run out of data from input file? */
  long int fpos=0;   /* position within file; e.g., first byte: 1, second byte: 2, etc. */
  long int nblock=0;

  struct timeval pe1_tv; /* used to remember when code started running */
  struct timeval pe2_tv; /* used to remember when code stopped running */

  /* profiling */
  struct timeval tv1; 
  double time1 = 0;   /* [s] accumulated time spent duing Activity 1 */
  struct timeval tv2; 
  double time2 = 0;   /* [s] accumulated time spent duing Activity 2 */

  int overlap=0;
  float obsfreq;            /* OBSFREQ */
  float obsbw;              /* OBSBW */
  float chan_bw;            /* CHAN_BW */
  int obsnchan;             /* OBSNCHAN */
  double fs;                /* 1/TBIN */

  signed char *blk0; /* allocated below */
  long int nT0;
  long int blk0_ptr;
  double fstart0 = 0; 

  signed char *blk1; /* allocated below */
  long int nT1;
  long int blk1_ptr;
  double fstart1 = 0; 

  /* scratch variables */
  int eStatus; /* used for returned error codes */
  long int l;
  long int m;
  //signed char x_temp;      

  /*=====================*/
  /*=== Say Hello =======*/
  /*=====================*/

  printf("This is frsc v.%d\n",RA_H_RA_VERSION);
  printf("  A 'struct ra_header_struct' is %ld bytes\n",sizeof(struct ra_header_struct));
  printf("  A 'struct ra_td' is %ld bytes\n",sizeof(struct ra_td));

  gettimeofday( &pe1_tv, NULL );
  printf("Execution begins: UTC %s",asctime(gmtime(&pe1_tv.tv_sec))); 

  /*======================================*/
  /*=== Acquire Command Line Arguments ===*/
  /*======================================*/

  /* read <jobfile> */
  memset(jobfile,'\0',RA_MAX_FILENAME_LENGTH);  /* just in case */
  if (narg>=2) {
      sscanf( argv[1], "%s", jobfile );
    } else {
      printf("FATAL: main(): <jobfile> not specified\n");
      return;
    } 
  printf("<jobfile>='%s'\n",jobfile);

  /* read the jobfile, initialize header */
  if ( eStatus = ra_read_jobfile( jobfile, &header0, infile ) ) {
    printf("FATAL: main(): ra_read_jobfile() failed with code %d\n",eStatus);
    return;
    }

  printf("Here are some things I learned from the jobfile:\n");
  printf("  header0.esource = %d\n",header0.eSource);

  /*==================*/
  /*=== Initialize ===*/
  /*==================*/

  /* open output file */
  system("rm out.dat"); /* just in case */
  fp_out = fopen("out.dat","wb");
 
  /* attempt to open input file */
  if (!(fp_in = fopen(infile,"rb"))) {
    printf("FATAL: main(): couldn't open '%s'\n",infile);
    return;
    }

  /* allocate memory for the input raw data block */
  if ( (blk = malloc( RG_BLK_SIZE * sizeof(*blk) ) ) == NULL ) {
    printf("FATAL: main(): malloc() of blk failed\n"); 
    return;
    }

  /* read GUPPI header */
  rg_read_header(fp_in,&fpos,rg_header);
  //printf("Header is %d bytes\n",(int)strlen(rg_header));
  //printf("End of header is at byte %ld (counting from 1)\n",fpos);

  /* parse/interpret header; load up metadata; identify possible issues */
  printf("Analyzing header...\n");
  if (rg_analyze_header(rg_header,&overlap,&obsfreq,&obsbw,&chan_bw,&obsnchan,&fs)>0) {
    printf("FATAL: rg_analyze_header() failed.  Writing out header as diagnostic:\n");
    printf("%s\n",rg_header);
    return;
    }

  printf("Here is what I learned from the header:\n");
  printf("  rg_analyze_header() says OVERLAP = %d samples\n",overlap);
  printf("  rg_analyze_header() says OBSFREQ = %f MHz (center freq of observation)\n",obsfreq);
  printf("  rg_analyze_header() says OBSBW = %f MHz (BW of bandpass incl. all channels)\n",obsbw);
  printf("  rg_analyze_header() says CHAN_BW = %f MHz (BW of a single channel)\n",chan_bw);
  printf("  rg_analyze_header() says OBSNCHAN = %d (# channels in dataset)\n",obsnchan);
  printf("  rg_analyze_header() says fs (1/TBIN) = %e samples/s\n",fs);

  /* update static fields in report header structure based on what we extracted from GUPPI file header */
  header0.tvStart.tv_sec  = 0;     /* have no better way to set these, so initialized to start of UNIX epoch */
  header0.tvStart.tv_usec = 0;
  header0.nCh = obsnchan;          /* number of channels */
  header0.bw  = obsbw*(1.0e+6);    /* [Hz] "full bandwidth" = OBSBW*(1e+6) (may be negative) */
  header0.fc  = obsfreq*(1.0e+6);  /* [Hz] Center frequency for "full bandwidth" = OBSFREQ*(1e+6) */
  header0.fs  = fs;                /* [Hz] Sample rate per-channel = 1/TBIN */

  /* Write a header and flush stream */
  fwrite( &header0, sizeof(struct ra_header_struct), 1, fp_out ); fflush(fp_out);

  ///* DIAGNOSTIC: Checking channel bits */
  //for (l=1;l<=obsnchan;l++) { /* note..starting from 1 here! */
  //  printf("l = %ld, ra_isChBitSet(header.bChIn,l) = %d\n",l,ra_isChBitSet(header.bChIn,l));
  //  }
  //return;  

  /* Allocating sample buffer memory */
  nT0 = ( header0.T0 * header0.fs );          /* number of samples/channel in time T0 */
  header0.T0 = (((double) nT0)) / header0.fs; /* recompute T0 so that it is an integer number of samples */
  printf("nT0 = %ld; header0.T0 recomputed, now %le. blk0 (buffer) is %f MB\n",nT0,header0.T0,((double)nT0*obsnchan*RG_NPOL)/(1024.0*1024.0)); 
  if ( (blk0 = malloc( nT0 * obsnchan * RG_NPOL * sizeof(*blk0) ) ) == NULL ) { /* a single block of length nT0 for all channels and both pols */
    printf("FATAL: main(): malloc() of blk0 failed\n"); 
    return;
    }

  nT1 = ( header0.T1 * header0.fs );          /* number of samples/channel in time T1 */
  header0.T1 = (((double) nT1)) / header0.fs; /* recompute T1 so that it is an integer number of samples */
  printf("nT1 = %ld; header0.T1 recomputed, now %le. blk1 (buffer) is %f MB\n",nT1,header0.T1,((double)nT1*obsnchan*RG_NPOL)/(1024.0*1024.0)); 
  if ( (blk1 = malloc( nT1 * obsnchan * RG_NPOL * sizeof(*blk1) ) ) == NULL ) { /* a single block of length nT1 for all channels and both pols */
    printf("FATAL: main(): malloc() of blk1 failed\n"); 
    return;
    }

  bDone = 0;
  nblock = 0;
  blk0_ptr = 0;

  /*****************/
  /*** Main Loop ***/
  /*****************/
  printf("Beginning main loop...\n"); 

  while (!bDone) {

    /* read sample block */
    gettimeofday(&tv1,NULL);            /* PROFILING */
      fread( blk, RG_BLK_SIZE, 1, fp_in);
      time1 += ra_timer(tv1);           /* PROFILING */
    fpos += RG_BLK_SIZE;
    nblock++;  
    //printf("main(): nblock=%ld (sample block read)\n",nblock); //if (nblock==2) { bDone=1; }   

    /* swallow -- T0-rate processing */
    gettimeofday(&tv2,NULL);  /* PROFILING */
      ra_swallow(blk,                        /* the data */
                 &header0,                   /* the instructions */
                 blk0,                       /* the current T0 buffer */
                 &blk0_ptr,                  /* pointer within current T0 buffer */  
                 nT0,                        /* the length of the T0 buffer in samples (1 sample = RG_NPOL bytes) */
                 obsnchan, chan_bw, overlap, /* stuff learned from GUPPI header */ 
                 fp_out,                     /* where output should go */
                 &fstart0                    /* keeping track of absolute time relative to start of run */
                );
      time2 += ra_timer(tv2); /* PROFILING */

    ///* swallow -- T1-rate processing */
    //gettimeofday(&tv2,NULL);  /* PROFILING */
    //  ra_swallow(blk,                        /* the data */
    //             &header0,                   /* the instructions */
    //             blk1,                       /* the current T1 buffer */
    //             &blk1_ptr,                  /* pointer within current T1 buffer */  
    //             nT1,                        /* the length of the T1 buffer in samples (1 sample = RG_NPOL bytes) */
    //             obsnchan, chan_bw, overlap, /* stuff learned from GUPPI header */
    //             fp_out                      /* where output should go */
    //           );
    //  time2 += ra_timer(tv2); /* PROFILING */

    /* read header of next block */
    eStatus = rg_read_header(fp_in,&fpos,rg_header);
    if (eStatus>0) {
      printf("rg_read_header() returned 1... end-of-file garbage? Setting bDone=1\n");
      bDone=1;
      }
    printf("main(): nblock=%ld (header read), fstart0=%lf [s]\n",nblock+1,fstart0); //if (nblock==2) { bDone=1; }   

    /* see if we've exhausted the file */
    if (feof(fp_in)) { 
      printf("In main(), feof(fp_in) is TRUE.  Setting bDone=1\n");
      bDone=1; 
      } 

    //printf("main(): time1 = %lf s, time2 = %lf\n",time1,time2); fflush(stdout);

    } /* while (!bDone) */

  /************************/
  /*** End of Main Loop ***/
  /************************/
  printf("End of main loop.\n");

  /*====================*/
  /*=== Winding Down ===*/
  /*====================*/

  /* close files */
  fclose(fp_out);
  fclose(fp_in);

  /* free data block memory */
  free(blk);  blk  = NULL;
  free(blk0); blk0 = NULL;
  free(blk1); blk1 = NULL;

  /* free block memory (allocated in ra_analyze.c) */
  if (raa_bAllocSS) {
    free(raa_xi); raa_xi = NULL;
    free(raa_xq); raa_xq = NULL;
    free(raa_yi); raa_yi = NULL;
    free(raa_yq); raa_yq = NULL;
    free(raa_xx); raa_xx = NULL;
    free(raa_yy); raa_yy = NULL;
    free(raa_xyi); raa_xyi = NULL; 
    free(raa_xyq); raa_xyq = NULL; 
    raa_bAllocSS = 0;
    }

  printf("Program execution began: UTC %s",asctime(gmtime(&pe1_tv.tv_sec))); 
  gettimeofday( &pe2_tv, NULL );
  printf("Program execution ended: UTC %s",asctime(gmtime(&pe2_tv.tv_sec))); 
  printf("Program excution took %ld s = %f h\n",pe2_tv.tv_sec-pe1_tv.tv_sec,(pe2_tv.tv_sec-pe1_tv.tv_sec)/3600.0);

  /* PROFILING */
  printf("Elapsed time spent on Activity 1 (reading file) = %lf s\n",time1);
  printf("Elapsed time spent on Activity 2 (swallow())    = %lf s\n",time2);

  printf("Bye.\n"); 

  return;
  } /* main() */

//==================================================================================
//=== HISTORY ======================================================================
//==================================================================================
// frsc.c: S.W. Ellingson, Virginia Tech, 2014 Jan 26
// -- some diagnostic printf's commented out
// frsc.c: S.W. Ellingson, Virginia Tech, 2014 Jan 25
// -- cleaning up
// frsc.c: S.W. Ellingson, Virginia Tech, 2014 Jan 20
// -- renamed "frsc.c"
// ra.c: S.W. Ellingson, Virginia Tech, 2013 Dec 14
// -- continuing development
// ra.c: S.W. Ellingson, Virginia Tech, 2013 Dec 04
// -- continuing development
// ra.c: S.W. Ellingson, Virginia Tech, 2013 Dec 02
// -- continuing development
// ra.c: S.W. Ellingson, Virginia Tech, 2013 Nov 26
// -- continuing development
// ra.c: S.W. Ellingson, Virginia Tech, 2013 Nov 25
// -- initial version

//==================================================================================
//=== BELOW THIS LINE IS SCRATCH ===================================================
//==================================================================================

