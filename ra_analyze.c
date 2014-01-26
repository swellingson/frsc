/*===============================================================
ra_analyze.c: S.W. Ellingson, Virginia Tech, 2014 Jan 26
analyzes a block of data
================================================================*/

/* This is globally-defined scratch space for calculations within ra_analyze() */
/* This space is malloc'ed the first time ra_analyze() is called */
/* This space is freed at the end of main() */
int raa_bAllocSS = 0; /* has scratch space been allocated? */
float *raa_xi; /* scratch space for xi */
float *raa_xq; /* scratch space for xq */
float *raa_yi; /* scratch space for yi */
float *raa_yq; /* scratch space for yq */
float *raa_xx; /* scratch space for |x|^2 */
float *raa_yy; /* scratch space for |y|^2 */
float *raa_xyi; /* scratch space for real(x*cong(y)) */
float *raa_xyq; /* scratch space for imag(x*cong(y)) */
long int raa_nSamplesPerChannel = 0;

/*=======================================================*/
/*=== ra_analyze() ======================================*/
/*=======================================================*/
/* called from ra_swallow(); look there for info on input data format */

int ra_analyze( 
                struct ra_header_struct *header0, /* [in] prototype report output header; defines which analyses are done */
                signed char *blk,                 /* [in/out] data to be analyzed */
                long int nSamplesPerChannel,      /* [in] the length of the block in samples (1 sample = RG_NPOL bytes) */
                FILE *fp_out,                     /* [in] where output should go */
                double fstart                     /* [in] keeping track of absolute time relative to start of run */	
                //int obsnchan,                   /* [in] OBSNCHAN */
                //float chan_bw                   /* [in] CHAN_BW */
                ) {

    struct ra_header_struct header; /* this is what gets written as header of report */
    struct ra_td td;                /* this is what gets written as body of report */
    long int l;

    long int n;
    float xi,xq;
    float yi,yq;
    float q,q2,q3,q4;

    long int mev2;

    /* allocate scratch space, if this hasn't been done already */
    /* this space is unallocated at the end of ra.c */
    if (!raa_bAllocSS) {

        if ( (raa_xi = malloc( nSamplesPerChannel * sizeof(*raa_xi) ) ) == NULL ) { 
          printf("FATAL: ra_analyze(): malloc() of raa_xi failed\n"); 
          return;
          }
        if ( (raa_xq = malloc( nSamplesPerChannel * sizeof(*raa_xq) ) ) == NULL ) { 
          printf("FATAL: ra_analyze(): malloc() of raa_xq failed\n"); 
          return;
          }
        if ( (raa_yi = malloc( nSamplesPerChannel * sizeof(*raa_yi) ) ) == NULL ) { 
          printf("FATAL: ra_analyze(): malloc() of raa_yi failed\n"); 
          return;
          }
        if ( (raa_yq = malloc( nSamplesPerChannel * sizeof(*raa_yq) ) ) == NULL ) { 
          printf("FATAL: ra_analyze(): malloc() of raa_yq failed\n"); 
          return;
          }
        if ( (raa_xx = malloc( nSamplesPerChannel * sizeof(*raa_xx) ) ) == NULL ) { 
          printf("FATAL: ra_analyze(): malloc() of raa_xx failed\n"); 
          return;
          }
        if ( (raa_yy = malloc( nSamplesPerChannel * sizeof(*raa_yy) ) ) == NULL ) { 
          printf("FATAL: ra_analyze(): malloc() of raa_yy failed\n"); 
          return;
          }
        if ( (raa_xyi = malloc( nSamplesPerChannel * sizeof(*raa_xyi) ) ) == NULL ) { 
          printf("FATAL: ra_analyze(): malloc() of raa_xyi failed\n"); 
          return;
          }
        if ( (raa_xyq = malloc( nSamplesPerChannel * sizeof(*raa_xyq) ) ) == NULL ) { 
          printf("FATAL: ra_analyze(): malloc() of raa_xyq failed\n"); 
          return;
          }
        raa_nSamplesPerChannel = nSamplesPerChannel;
        raa_bAllocSS = 1;

      } else { /* make sure this hasn't changed */

        if (raa_nSamplesPerChannel!=nSamplesPerChannel) {
          printf("FATAL: ra_analyze(): raa_nSamplesPerChannel != nSamplesPerChannel (%ld != %ld)\n",raa_nSamplesPerChannel,nSamplesPerChannel); 
          return;
          } 

      } /* if (raa_bAllocSS) */

    //printf("ra_analyze(): tflags=%c\n",header0->tflags);
    //if ( header0->tflags & RA_H_TFLAGS_TC ) {
    //  printf("ra_analyze(): I think RA_H_TFLAGS_TC is asserted\n");
    //  }

    /* TODO: This is where selection of type of analysis (based on "tflags" and "fflags") would normally get done */
    /* For now, only "time-domain analysis for channels" is implemented.  Anything else will be ignored */ 
    if ( (header0->tflags) & RA_H_TFLAGS_TC ) { /* START CODEBLOCK A */

    /* Loop over channels */
    for (l=1;l<=header0->nCh;l++) { /* note..starting from 1 here! */
      if (!ra_isChBitSet(header0->bChIn,l)) { /* if channel bit is not set, then we do this one */    
 
        //printf("ra_analyze(): Doing channel %ld\n",l);
        //printf("xi(0) = %f\n",(float) blk[ (l-1)*RG_NDIM*RG_NPOL + 0 + RG_NPOL*0]);

        /* intialize clip counters */
        td.clips.x = 0;
        td.clips.y = 0;
        switch (header0->eSource) { 
          case RA_H_ESOURCE_GUPPI_FILE:
          case RA_H_ESOURCE_GUPPI_RT:
            mev2 = 127*127; 
            break;
          default:
            printf("FATAL: ra_analyze(): I don't recongnize header0->eSource=%d\n",header0->eSource); 
            return;
            break;
          }

        /* pass 1: Get |x|^2, |y|^2, & xy* (sample-by-sample) since we know we need these multiple times */
        /*         also collect clipping info */
        for ( n=0; n<nSamplesPerChannel; n++ ) {
          xi = (float) blk[ (l-1)*RG_NDIM*RG_NPOL + 0 + RG_NPOL*n];
          xq = (float) blk[ (l-1)*RG_NDIM*RG_NPOL + 1 + RG_NPOL*n ];  
          yi = (float) blk[ (l-1)*RG_NDIM*RG_NPOL + 2 + RG_NPOL*n ];
          yq = (float) blk[ (l-1)*RG_NDIM*RG_NPOL + 3 + RG_NPOL*n ]; 
          raa_xi[n] = xi;
          raa_xq[n] = xq;
          raa_yi[n] = yi;
          raa_yq[n] = yq;
          raa_xx[n] = xi*xi + xq*xq; 
          raa_yy[n] = yi*yi + yq*yq; 
          raa_xyi[n] = xi*yi + xq*yq; 
          raa_xyq[n] = xq*yi - xi*yq; 
          if (raa_xx[n]>=mev2) { td.clips.x++; }
          if (raa_yy[n]>=mev2) { td.clips.y++; }
          }

        /* pass 2: get mean/max statistics, computing Stokes parameters along the way */
        td.tdac[l-1].xi.mean = 0; /* initialize */
        td.tdac[l-1].xi.max  = 0; /* initialize */
        td.tdac[l-1].xq.mean = 0; /* initialize */
        td.tdac[l-1].xq.max  = 0; /* initialize */
        td.tdac[l-1].yi.mean = 0; /* initialize */
        td.tdac[l-1].yi.max  = 0; /* initialize */
        td.tdac[l-1].yq.mean = 0; /* initialize */
        td.tdac[l-1].yq.max  = 0; /* initialize */
        td.tdac[l-1].xm2.mean = 0; /* initialize */
        td.tdac[l-1].xm2.max  = 0; /* initialize */
        td.tdac[l-1].ym2.mean = 0; /* initialize */
        td.tdac[l-1].ym2.max  = 0; /* initialize */
        td.tdac[l-1].ym2.mean = 0; /* initialize */
        td.tdac[l-1].u.mean   = 0; /* initialize */
        td.tdac[l-1].u.max    = 0; /* initialize */
        td.tdac[l-1].v.mean   = 0; /* initialize */
        td.tdac[l-1].v.max    = 0; /* initialize */
        for ( n=0; n<nSamplesPerChannel; n++ ) {
          td.tdac[l-1].xi.mean += raa_xi[n];  /* mean (gets finished up below) */
          td.tdac[l-1].xq.mean += raa_xq[n];
          td.tdac[l-1].yi.mean += raa_yi[n];  
          td.tdac[l-1].yq.mean += raa_yq[n];
          td.tdac[l-1].xm2.mean += raa_xx[n]; 
          td.tdac[l-1].ym2.mean += raa_yy[n];
          td.tdac[l-1].u.mean   += raa_xyi[n]; 
          td.tdac[l-1].v.mean   += raa_xyq[n];
          if ( raa_xi[n]  > td.tdac[l-1].xi.max ) { td.tdac[l-1].xi.max = raa_xi[n]; }
          if ( raa_xq[n]  > td.tdac[l-1].xq.max ) { td.tdac[l-1].xq.max = raa_xq[n]; }
          if ( raa_yi[n]  > td.tdac[l-1].yi.max ) { td.tdac[l-1].yi.max = raa_yi[n]; }
          if ( raa_yq[n]  > td.tdac[l-1].yq.max ) { td.tdac[l-1].yq.max = raa_yq[n]; }
          if ( raa_xx[n]  > td.tdac[l-1].xm2.max ) { td.tdac[l-1].xm2.max = raa_xx[n]; }
          if ( raa_yy[n]  > td.tdac[l-1].ym2.max ) { td.tdac[l-1].ym2.max = raa_yy[n]; }
          if ( raa_xyi[n] > td.tdac[l-1].u.max   ) { td.tdac[l-1].u.max = raa_xyi[n]; }
          if ( raa_xyq[n] > td.tdac[l-1].v.max   ) { td.tdac[l-1].v.max = raa_xyq[n]; }
          }
        td.tdac[l-1].xi.mean /=       nSamplesPerChannel;  /* finish up */
        td.tdac[l-1].xq.mean /=       nSamplesPerChannel;
        td.tdac[l-1].yi.mean /=       nSamplesPerChannel; 
        td.tdac[l-1].yq.mean /=       nSamplesPerChannel;
        td.tdac[l-1].xm2.mean /=       nSamplesPerChannel;  
        td.tdac[l-1].ym2.mean /=       nSamplesPerChannel;
        td.tdac[l-1].u.mean   /= (+0.5*nSamplesPerChannel);
        td.tdac[l-1].v.mean   /= (-0.5*nSamplesPerChannel);
        td.tdac[l-1].u.max *= (+2.0);
        td.tdac[l-1].v.max *= (-2.0);

        /* pass 3: rms, skewness, kurtosis */
        td.tdac[l-1].xi.rms  = 0; td.tdac[l-1].xi.s  = 0; td.tdac[l-1].xi.k  = 0; /* initialize */
        td.tdac[l-1].xq.rms  = 0; td.tdac[l-1].xq.s  = 0; td.tdac[l-1].xq.k  = 0; /* initialize */
        td.tdac[l-1].yi.rms  = 0; td.tdac[l-1].yi.s  = 0; td.tdac[l-1].yi.k  = 0; /* initialize */
        td.tdac[l-1].yq.rms  = 0; td.tdac[l-1].yq.s  = 0; td.tdac[l-1].yq.k  = 0; /* initialize */
        td.tdac[l-1].xm2.rms  = 0; td.tdac[l-1].xm2.s  = 0; td.tdac[l-1].xm2.k  = 0;  /* initialize */
        td.tdac[l-1].ym2.rms  = 0; td.tdac[l-1].ym2.s  = 0; td.tdac[l-1].ym2.k  = 0;  /* initialize */
        td.tdac[l-1].u.rms    = 0; td.tdac[l-1].u.s  = 0; td.tdac[l-1].u.k  = 0;  /* initialize */
        td.tdac[l-1].v.rms    = 0; td.tdac[l-1].v.s  = 0; td.tdac[l-1].v.k  = 0;  /* initialize */
        for ( n=0; n<nSamplesPerChannel; n++ ) {
          q =      raa_xi[n]  - td.tdac[l-1].xi.mean; q2 = q *q; td.tdac[l-1].xi.rms += q2; 
                                                      q3 = q2*q; td.tdac[l-1].xi.s   += q3; 
                                                      q4 = q3*q; td.tdac[l-1].xi.k   += q4; 
          q =      raa_xq[n]  - td.tdac[l-1].xq.mean; q2 = q *q; td.tdac[l-1].xq.rms += q2; 
                                                      q3 = q2*q; td.tdac[l-1].xq.s   += q3; 
                                                      q4 = q3*q; td.tdac[l-1].xq.k   += q4;
          q =      raa_yi[n]  - td.tdac[l-1].yi.mean; q2 = q *q; td.tdac[l-1].yi.rms += q2; 
                                                      q3 = q2*q; td.tdac[l-1].yi.s   += q3; 
                                                      q4 = q3*q; td.tdac[l-1].yi.k   += q4; 
          q =      raa_yq[n]  - td.tdac[l-1].yq.mean; q2 = q *q; td.tdac[l-1].yq.rms += q2; 
                                                      q3 = q2*q; td.tdac[l-1].yq.s   += q3; 
                                                      q4 = q3*q; td.tdac[l-1].yq.k   += q4; 
          q =      raa_xx[n]  - td.tdac[l-1].xm2.mean; q2 = q *q; td.tdac[l-1].xm2.rms += q2; 
                                                       q3 = q2*q; td.tdac[l-1].xm2.s   += q3; 
                                                       q4 = q3*q; td.tdac[l-1].xm2.k   += q4; 
          q =      raa_yy[n]  - td.tdac[l-1].ym2.mean; q2 = q *q; td.tdac[l-1].ym2.rms += q2; 
                                                       q3 = q2*q; td.tdac[l-1].ym2.s   += q3; 
                                                       q4 = q3*q; td.tdac[l-1].ym2.k   += q4; 
          q =  2.0*raa_xyi[n] - td.tdac[l-1].u.mean;   q2 = q *q; td.tdac[l-1].u.rms += q2; 
                                                       q3 = q2*q; td.tdac[l-1].u.s   += q3; 
                                                       q4 = q3*q; td.tdac[l-1].u.k   += q4;
          q = -2.0*raa_xyq[n] - td.tdac[l-1].v.mean;   q2 = q *q; td.tdac[l-1].v.rms += q2; 
                                                       q3 = q2*q; td.tdac[l-1].v.s   += q3; 
                                                       q4 = q3*q; td.tdac[l-1].v.k   += q4;  
          }
        td.tdac[l-1].xi.rms  = sqrt( td.tdac[l-1].xi.rms / nSamplesPerChannel );
        td.tdac[l-1].xi.s    =     ( td.tdac[l-1].xi.s   / nSamplesPerChannel ) / ( td.tdac[l-1].xi.rms * td.tdac[l-1].xi.rms * td.tdac[l-1].xi.rms);
        td.tdac[l-1].xi.k    =     ( td.tdac[l-1].xi.k   / nSamplesPerChannel ) / ( td.tdac[l-1].xi.rms * td.tdac[l-1].xi.rms * td.tdac[l-1].xi.rms * td.tdac[l-1].xi.rms ) - 3.0;
        td.tdac[l-1].xq.rms  = sqrt( td.tdac[l-1].xq.rms / nSamplesPerChannel );
        td.tdac[l-1].xq.s    =     ( td.tdac[l-1].xq.s   / nSamplesPerChannel ) / ( td.tdac[l-1].xq.rms * td.tdac[l-1].xq.rms * td.tdac[l-1].xq.rms);
        td.tdac[l-1].xq.k    =     ( td.tdac[l-1].xq.k   / nSamplesPerChannel ) / ( td.tdac[l-1].xq.rms * td.tdac[l-1].xq.rms * td.tdac[l-1].xq.rms * td.tdac[l-1].xq.rms ) - 3.0;
        td.tdac[l-1].yi.rms  = sqrt( td.tdac[l-1].yi.rms / nSamplesPerChannel );
        td.tdac[l-1].yi.s    =     ( td.tdac[l-1].yi.s   / nSamplesPerChannel ) / ( td.tdac[l-1].yi.rms * td.tdac[l-1].yi.rms * td.tdac[l-1].yi.rms);
        td.tdac[l-1].yi.k    =     ( td.tdac[l-1].yi.k   / nSamplesPerChannel ) / ( td.tdac[l-1].yi.rms * td.tdac[l-1].yi.rms * td.tdac[l-1].yi.rms * td.tdac[l-1].yi.rms ) - 3.0;
        td.tdac[l-1].yq.rms  = sqrt( td.tdac[l-1].yq.rms / nSamplesPerChannel );
        td.tdac[l-1].yq.s    =     ( td.tdac[l-1].yq.s   / nSamplesPerChannel ) / ( td.tdac[l-1].yq.rms * td.tdac[l-1].yq.rms * td.tdac[l-1].yq.rms);
        td.tdac[l-1].yq.k    =     ( td.tdac[l-1].yq.k   / nSamplesPerChannel ) / ( td.tdac[l-1].yq.rms * td.tdac[l-1].yq.rms * td.tdac[l-1].yq.rms * td.tdac[l-1].yq.rms ) - 3.0;
        td.tdac[l-1].xm2.rms  = sqrt( td.tdac[l-1].xm2.rms / nSamplesPerChannel );
        td.tdac[l-1].xm2.s    =     ( td.tdac[l-1].xm2.s   / nSamplesPerChannel ) / ( td.tdac[l-1].xm2.rms * td.tdac[l-1].xm2.rms * td.tdac[l-1].xm2.rms);
        td.tdac[l-1].xm2.k    =     ( td.tdac[l-1].xm2.k   / nSamplesPerChannel ) / ( td.tdac[l-1].xm2.rms * td.tdac[l-1].xm2.rms * td.tdac[l-1].xm2.rms * td.tdac[l-1].xm2.rms ) - 3.0;
        td.tdac[l-1].ym2.rms  = sqrt( td.tdac[l-1].ym2.rms / nSamplesPerChannel );
        td.tdac[l-1].ym2.s    =     ( td.tdac[l-1].ym2.s   / nSamplesPerChannel ) / ( td.tdac[l-1].ym2.rms * td.tdac[l-1].ym2.rms * td.tdac[l-1].ym2.rms);
        td.tdac[l-1].ym2.k    =     ( td.tdac[l-1].ym2.k   / nSamplesPerChannel ) / ( td.tdac[l-1].ym2.rms * td.tdac[l-1].ym2.rms * td.tdac[l-1].ym2.rms * td.tdac[l-1].ym2.rms ) - 3.0;
        td.tdac[l-1].u.rms    = sqrt( td.tdac[l-1].u.rms   / nSamplesPerChannel );
        td.tdac[l-1].u.s      =     ( td.tdac[l-1].u.s     / nSamplesPerChannel ) / ( td.tdac[l-1].u.rms * td.tdac[l-1].u.rms * td.tdac[l-1].u.rms);
        td.tdac[l-1].u.k      =     ( td.tdac[l-1].u.k     / nSamplesPerChannel ) / ( td.tdac[l-1].u.rms * td.tdac[l-1].u.rms * td.tdac[l-1].u.rms * td.tdac[l-1].u.rms ) - 3.0;
        td.tdac[l-1].v.rms    = sqrt( td.tdac[l-1].v.rms   / nSamplesPerChannel );
        td.tdac[l-1].v.s      =     ( td.tdac[l-1].v.s     / nSamplesPerChannel ) / ( td.tdac[l-1].v.rms * td.tdac[l-1].v.rms * td.tdac[l-1].v.rms);
        td.tdac[l-1].v.k      =     ( td.tdac[l-1].v.k     / nSamplesPerChannel ) / ( td.tdac[l-1].v.rms * td.tdac[l-1].v.rms * td.tdac[l-1].v.rms * td.tdac[l-1].v.rms ) - 3.0;

        } /* if (!ra_isChBitSet(header0->bChIn,l)) */
      } /* for l */

    /* DIAG FIXME */
    //printf("*** %f %f %f %f %f\n", td.tdac[10].xi.mean, td.tdac[10].xi.max, td.tdac[10].xi.rms, td.tdac[10].xi.s, td.tdac[10].xi.k);

    /* update prototype header */
    (header0->iSeqNo)++;
    //header0.fStart +=

    /* copy prototype header into current header */
    memcpy( &header, header0, sizeof(struct ra_header_struct) ); 

    /* update the header to be written */
    header.eType = RA_H_ETYPE_TF0;   /* indicate type of packet */
    header.err   = 0;                /* indicate error status */
    header.fStart = fstart;         

    /* write the report */
    fwrite( &header, sizeof(struct ra_header_struct), 1, fp_out );
    fwrite( &td,     sizeof(struct ra_td),            1, fp_out );
    fflush(fp_out); /* make sure this doesn't get stalled in buffer somewhere (useful especially if there is a crash...) */

    } /* END CODEBLOCK A */

    return 0;
    }

//==================================================================================
//=== HISTORY ======================================================================
//==================================================================================
// ra_analyze.c: S.W. Ellingson, Virginia Tech, 2014 Jan 26
// -- commented out diagnostic printf's
// ra_analyze.c: S.W. Ellingson, Virginia Tech, 2014 Jan 19
// -- added clip counters
// ra_analyze.c: S.W. Ellingson, Virginia Tech, 2013 Dec 12
// -- continuing development
// ra_analyze.c: S.W. Ellingson, Virginia Tech, 2013 Dec 06
// -- initial version
