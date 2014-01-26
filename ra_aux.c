/* ra_aux.h */
/* S. Ellingson VT 2013 Dec 03 */
/* auxilliary (support) code for ra.c; put here to avoid cluttering up ra.c */

/*************************************************************************/
/*** ra_isChBitSet() ****************************************************/
/*************************************************************************/
/* Returns >0 if indicated bit is set; otherwise zero */

int ra_isChBitSet( 
                   unsigned long int *bCh,   /* Input array has length RA_MAX_CH_DIV64 */
                   long int l                /* Channel (bit) to check, 1 .. RA_MAX_CH_DIV64*64  */
                   ) {
  
  int index_arr;
  int index_bit;
  unsigned long int uli;

  index_arr = (l-1)/64;             /* this is the 0-based array index containing bit to be checked */
  index_bit = (l-1) - index_arr*64; /* this is the 0-based bit index to be checked (0=LSB) */
  uli = 1;
  
  if ( ( bCh[index_arr] & ( uli << index_bit ) ) != 0 ) { /* check the bit */
      return 1;
    } else {
      return 0;
    }

  }


/*************************************************************************/
/*** ra_timer() **********************************************************/
/*************************************************************************/
/* used in profiling */

double ra_timer(
                 struct timeval t1 /* [in] start time */
                 ) {

  struct timeval t2; /* end time */
  double dt;

  gettimeofday(&t2,NULL);
  dt  =   t2.tv_sec  + ((double)t2.tv_usec)/(1.0e+6)  ;
  dt -= ( t1.tv_sec  + ((double)t1.tv_usec)/(1.0e+6) );  

  return dt;
  }
