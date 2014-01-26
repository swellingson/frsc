/*===============================================================
ra_guppi_file.c: S.W. Ellingson, Virginia Tech, 2013 Nov 26
macro defines & code pertaining to reading GUPPI raw data files
================================================================*/

#define RG_BLK_SIZE 1073545216 /* number of bytes in a contiguous data block */
#define RG_NDIM 8387072
#define RG_NPOL 4 

#define RG_MAX_HEADER_LENGTH 16384 /* presumed maximum length of a GUPPI header */


/*************************************************************************/
/*** rg_read_header() ****************************************************/
/*************************************************************************/

int rg_read_header( 
                   FILE *fp,       /* [in]     (previous opened) file */
                   long int *fpos, /* [in/out] position within file; e.g., first byte: 1, second byte: 2, etc. */
                                   /* position on arrival should be for last byte read from file stream */   
                                   /* position upon return will be last byte of header (before data block) */
                   char *header    /* header string, assumed to have max length RG_MAX_HEADER_LENGTH */
                   ) {

  int eState=0;
  char c[2];
  long int header_len = 0;

  /* this makes "c" a proper string even though we only load the first byte: */
  memset(&(c[1]),'\0',1); 

  /* initialize "header" as a proper string */
  memset(header,'\0',RG_MAX_HEADER_LENGTH);
  header_len = 0; /* keep track of position within header */

  /* state machine that reads until the string " END" is found */
  while (eState<4) {
    fread( c, 1, 1, fp); /* read one character */
    (*fpos)++;
    switch (eState) {
      case 1: if (c[0]=='E') { eState=2; } else { eState=0; } break;
      case 2: if (c[0]=='N') { eState=3; } else { eState=0; } break;
      case 3: if (c[0]=='D') { eState=4; } else { eState=0; } break;
      }
    if (c[0]==' ') { eState=1; } /* keep state machine in this state for as long as we see spaces */
    //printf("%1s",c);

    header[header_len] = c[0];
    header_len++;

    /* avoid overruning the string; this could happen at end of a file */
    if (header_len>=RG_MAX_HEADER_LENGTH) { 
      eState=4;
      return 1; 
      }

    } 

  /* advance one position, to the beginning of the run of spaces */
  fread( c, 1, 1, fp); /* read one character */
  (*fpos)++;

  /* Run out the spaces */
  while (c[0]==' ') {
    fread( c, 1, 1, fp); /* read one character */
    (*fpos)++;
    //printf("%1s",c);
    }
  //printf("Next non-space character is byte %ld\n",i);

  /* back up one position */
  fseek(fp, (*fpos)-1, SEEK_SET); 
  (*fpos)--;

  return 0;
  }


/*************************************************************************/
/*** rg_analyze_header() *************************************************/
/*************************************************************************/

int rg_analyze_header( 
                      char *header,              /* [in] header string */
                      int *overlap,              /* [out] OVERLAP */
                      float *obsfreq,            /* [out] OBSFREQ */
                      float *obsbw,              /* [out] OBSBW */
                      float *chan_bw,            /* [out] CHAN_BW */
                      int *obsnchan,             /* [out] OBSNCHAN */  
                      double *fs                 /* [out] 1/TBIN */      
                      ) {

  char *pos;
  float tbin;
  int nbits;
  int npol;
  long int blocksize;

  /* check BACKEND */
  pos = strstr(header,"BACKEND");
  pos = strstr(pos,"'");
  if (strncmp(pos,"'GUPPI",6)) {
    printf("FATAL: rg_analyze_header() says BACKEND is not 'GUPPI'\n");
    return 1;
    }

  /* check PKTFMT */
  pos = strstr(header,"PKTFMT");
  pos = strstr(pos,"'");
  if (strncmp(pos,"'1SFA",5)) {
    printf("FATAL: rg_analyze_header() says PKTFMT is not begin with '1SFA'\n");
    return 1;
    }

  /* check FD_POLN */
  pos = strstr(header,"FD_POLN");
  pos = strstr(pos,"'");
  if (strncmp(pos,"'LIN",4)) {
    printf("FATAL: rg_analyze_header() says FD_POLN does not begin with 'LIN'\n");
    return 1;
    }

  /* get/check NBITS (bits/sample) */
  pos = strstr(header,"NBITS");
  pos = strstr(pos,"=")+1;
  sscanf(pos,"%d",&nbits);
  if (nbits!=8) {
    printf("FATAL: rg_analyze_header() says NBITS is not equal to 8\n");
    return 1;
    }

  /* get/check NPOL (bits/sample) */
  /* note NPOL=4 really means 2 pols; "I" and "Q" are counted separately */
  pos = strstr(header,"NPOL");
  pos = strstr(pos,"=")+1;
  sscanf(pos,"%d",&npol);
  if (npol!=4) {
    printf("FATAL: rg_analyze_header() says NPOL is not equal to 4\n");
    return 1;
    }

  /* get/check BLOCSIZE (samples per block) */
  pos = strstr(header,"BLOCSIZE");
  pos = strstr(pos,"=")+1;
  sscanf(pos,"%ld",&blocksize);
  if (blocksize!=RG_BLK_SIZE) {
    printf("FATAL: rg_analyze_header() says BLOCSIZE=%ld is not equal to RG_BLK_SIZE=%d\n",blocksize,RG_BLK_SIZE);
    return 1;
    }

  /* get OBSFREQ (center frequency of observation) */
  pos = strstr(header,"OBSFREQ");
  pos = strstr(pos,"=")+1;
  sscanf(pos,"%f",obsfreq);
  //printf("  rg_analyze_header says OBSFREQ = %f MHz (center freq of observation)\n",*obsfreq);

  /* get OBSNCHAN (# of channels) */
  pos = strstr(header,"OBSNCHAN");
  pos = strstr(pos,"=")+1;
  sscanf(pos,"%d",obsnchan);
  //printf("  rg_analyze_header says OBSNCHAN = %d (# channels in dataset)\n",*obsnchan);
  if (*obsnchan>(RA_MAX_CH_DIV64*64)) {
    printf("FATAL: rg_analyze_header() says OBSNCHAN=%d is greater than (RA_MAX_CH_DIV64*64)=%d\n",*obsnchan,(RA_MAX_CH_DIV64*64));
    return 1;
    }

  /* get OBSBW (bandwidth of bandpass) */
  pos = strstr(header,"OBSBW");
  pos = strstr(pos,"=")+1;
  sscanf(pos,"%f",obsbw);
  //printf("  rg_analyze_header says OBSBW = %f MHz (BW of bandpass incl. all channels; '-' OK)\n",*obsbw);

  /* get CHAN_BW (bandwidth of channel) */
  pos = strstr(header,"CHAN_BW");
  pos = strstr(pos,"=")+1;
  sscanf(pos,"%f",chan_bw);
  //printf("  rg_analyze_header says CHAN_BW = %f MHz (BW of a single channel; '-' OK)\n",*chan_bw);

  /* get TBIN (sample period) */
  pos = strstr(header,"TBIN");
  pos = strstr(pos,"=")+1;
  sscanf(pos,"%f",&tbin);
  *fs = 1/tbin;
  //printf("  rg_analyze_header says TBIN = %e s (fs = %e samples/s)\n",tbin,*fs);

  /* get OVERLAP (# of samples repeated in beginning of next block) */
  pos = strstr(header,"OVERLAP");
  pos = strstr(pos,"=")+1;
  sscanf(pos,"%d",overlap);
  //printf("  rg_analyze_header says OVERLAP = %d samples\n",*overlap);

  /* Paul D says: 
     The channel ordering is monotonic, not "FFT-style".  
     Whether the freqs increase or decrease depends on the sign of the OBSBW header param; they decrease in this case.  
     The center freq of the ith channel (using 1-based indexing like you are) is OBSFREQ - OBSBW/2 + (i-0.5)*CHAN_BW.
  */
  //ldfm->fc = ((*obsfreq) - *obsbw/2 + (chan-0.5)*(*chan_bw))*(1e+6); /* [Hz] center frequency of the channel */

  return 0;
  }

