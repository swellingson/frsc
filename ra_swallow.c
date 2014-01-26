/*===============================================================
ra_swallow.c: S.W. Ellingson, Virginia Tech, 2013 Jan 26
================================================================*/

/*=======================================================*/
/*=== ra_swallow() ======================================*/
/*=======================================================*/
/* Copies sections of raw sample block into a buffer for analysis */ 
/* Launches analysis once buffer is filled */
/* Note buffer is identical to raw sample block, except: */
/* -- unneeded channels are not copied */
/* -- overlap bytes are stripped off (so, each channel is represented by (RG_NDIM-overlap)*RG_NPOL bytes as opposed to RG_NDIM*RG_NPOL bytes */

int ra_swallow( 
                signed char *blk,                 /* [in]  data block from GUPPI raw data file (source) */
                struct ra_header_struct *header0, /* [in] prototype report output header; defines which analyses are done */
                signed char *blk0,                /* [in/out] buffer (destination) */
                long int *blk0_ptr,               /* [in/out] position within buffer (where next byte should go) FIXME: Now works like ch_ptr */
                long int nT0,                     /* [in] the length of the T0 buffer in samples (1 sample = RG_NPOL bytes) */
                int obsnchan,                     /* [in] OBSNCHAN */
                float chan_bw,                    /* [in] CHAN_BW */
                int overlap,                      /* [in] OVERLAP */
                FILE *fp_out,			  /* [in] Where output should go.  This is passed to ra_analyze() */
                double *fstart                    /* keeping track of absolute time relative to start of run */				
                ) {

    long int nBytesPerChannel_no; /* number of bytes per channel, excluding overlap */
    long int nBytesToMove;        /* number of bytes that will be moved from src to dest */
    long int ch_ptr = 0;          /* keeping track of where we are within a channel; i.e. this counts 0..RG_NDIM*RG_NPOL */
    long int blk0_n = 0;          /* total number of bytes in dest buffer */
    int bDone = 0;           
    int bBufferFull = 0;
 
    /* scratch */
    long int l;

    /* initialize */
    nBytesPerChannel_no = (RG_NDIM-overlap)*RG_NPOL; 
    ch_ptr = 0;
    blk0_n = nT0 * obsnchan * RG_NPOL;  
    bDone = 0;

    //printf("ra_swallow():\n");
    //printf("  On entry, *blk0_ptr = %ld, so buffer %f percent full\n",*blk0_ptr,100*((float)*blk0_ptr)/blk0_n);
    // //printf("nT0                =%ld [in]; this is %f s\n",nT0,nT0/header0->fs);
    // //printf("nBytesPerChannel_no=%ld\n",nBytesPerChannel_no);
    // //printf("blk0_n             =%ld (RG_BLK_SIZE=%d,obsnchan*RG_NDIM*RG_NPOL=%d)\n",blk0_n,RG_BLK_SIZE,obsnchan*RG_NDIM*RG_NPOL);

    /* loop 'til done */
    while (!bDone) { /* we're going to loop until we have exhausted the input data block */

      /* figure out how many bytes to move, if any. */
      nBytesToMove = nBytesPerChannel_no;                         /* by default, we move all bytes except overlap, channel at a time, from blk to blk0 */
      if ( ((*blk0_ptr+nBytesToMove)*obsnchan) >= blk0_n ) {    /* If this causes us to overrun the blk0 (dest) buffer, */
        nBytesToMove = blk0_n/obsnchan - *blk0_ptr;             /* ... then we move only enough samples to fill the blk0 buffer */
        }

      if ( ((ch_ptr+overlap)*RG_NPOL+nBytesToMove) > (RG_BLK_SIZE/obsnchan) ) {   /* If this causes us to overrun the blk (source) buffer, */
        nBytesToMove = RG_BLK_SIZE/obsnchan - (ch_ptr+overlap)*RG_NPOL ;           /* ... then we move only the remaining samples */            
        bDone = 1;                                                                 /* ... set flag to remember */
        }

      /* now we check to see if the dest buffer will overflow. */
      /*  We do it here because nBytesToMove may have changed in the src buffer overflow check */    
      if ( (((*blk0_ptr)+nBytesToMove)*obsnchan) >= blk0_n ) {     /* If this causes us to overrun the blk0 (dest) buffer, */
        bBufferFull = 1;                                          /* ... set flag to remember */
        }

      //printf("  Moving %ld S/ch (%f pct of input block) *blk0_ptr=%ld ch_ptr*RG_NPOL=%ld\n",nBytesToMove/RG_NPOL,100*((float)nBytesToMove*obsnchan)/(RG_BLK_SIZE-overlap*obsnchan*RG_NPOL),*blk0_ptr,ch_ptr*RG_NPOL);
      //printf("  blk0[%ld]=%d blk0[ 10*RG_NDIM*RG_NPOL + 0 + RG_NPOL*0]=%d\n",*blk0_ptr, ((int) blk0[*blk0_ptr]),(int)blk0[ 10*RG_NDIM*RG_NPOL + 0 + RG_NPOL*0]);

      /* Loop over channels, moving data from blk to blk0 */
      for (l=1;l<=obsnchan;l++) { /* note..starting from 1 here! */
        if (!ra_isChBitSet(header0->bChIn,l)) { /* if channel bit is not set, then we do this one */     
          //printf("*blk0_ptr=%ld,ch_ptr=%ld, nBytesToMove=%ld, bBufferFull=%d, bDone=%d, l=%ld",*blk0_ptr,ch_ptr,nBytesToMove,bBufferFull,bDone,l); fflush(stdout);
          //memcpy( &(blk0[*blk0_ptr]),                                /* (dest) pointer to current location in sample buffer */
          //        &(blk [ (l-1)*RG_NDIM*RG_NPOL + ch_ptr*RG_NPOL ] ), /* (src)  pointer to channel start location in sample block */ 
          //        nBytesToMove                                        /* number of samples to move */     
          //      );
          memcpy( &(blk0[ (l-1)*(RG_NDIM-overlap)*RG_NPOL + *blk0_ptr      ] ), /* (dest) pointer to current location in sample buffer */
                  &(blk [ (l-1)* RG_NDIM         *RG_NPOL + ch_ptr*RG_NPOL ] ), /* (src)  pointer to channel start location in sample block */ 
                  nBytesToMove                                        /* number of samples to move */     
                );
          //printf(".\n");  fflush(stdout);
          } /* if (!ra_isChBitSet(header0->bChIn,l)) */
        } /* for l */

      //printf("  blk0[%ld]=%d blk0[ 10*RG_NDIM*RG_NPOL + 0 + RG_NPOL*0]=%d\n",*blk0_ptr, ((int) blk0[*blk0_ptr]),(int)blk0[ 10*RG_NDIM*RG_NPOL + 0 + RG_NPOL*0]);

      /* advance block pointers */
      ch_ptr   +=  (nBytesToMove/RG_NPOL); 
      *blk0_ptr +=  nBytesToMove; 

      /* If we hit the end of the buffer, time to do analysis */
      if (bBufferFull) {

        //printf("  running ra_analyze()\n");
        // //printf("xi(0) = %f\n",(float) blk0[ 10*RG_NDIM*RG_NPOL + 0 + RG_NPOL*0]);
        ra_analyze( header0,
                    blk0,
                    nT0, 
                    fp_out,
                    *fstart
                    //obsnchan,                   /* [in] OBSNCHAN */
                    //chan_bw                     /* [in] CHAN_BW */
                  );

        /* advance the absolute time tracking variable */
        (*fstart) += ( nT0 * (1.0e-6) / fabs(chan_bw) ); 
        //printf("ra_swallow: *fstart=%f\n",*fstart);

        /* reset buffer */
        *blk0_ptr = 0;   /* reset pointer */
        bBufferFull = 0; /* reset flag */

        } /* if (bBufferFull)  */    
     
      ///* figure out if we are done with blk */
      ///* this should rarely happen, since normally bDone would be set as the result of a input block overrun (above) */
      //if ( ch_ptr >= RG_NDIM-overlap ) {
      //  bDone=1;  
      //  printf("  Found ch_ptr >= RG_NDIM-overlap (?)\n");
      //  }

      } /* while (!bDone) */

    return 0;
    }

//==================================================================================
//=== HISTORY ======================================================================
//==================================================================================
// ra_swallow.c: S.W. Ellingson, Virginia Tech, 2013 Jan 26
// -- commented out diagnostic printf's
// ra_swallow.c: S.W. Ellingson, Virginia Tech, 2013 Dec 04
// -- renamed
// ra_analyze.c: S.W. Ellingson, Virginia Tech, 2013 Dec 04
// -- initial version
