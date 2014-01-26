/*****************************************/
/*****************************************/
/*** "RFI Analysis" (RA) Output Format ***/
/*** Format Version 1, 2014 Jan 19     ***/
/*** S. Ellingson (VT)                 ***/
/*****************************************/
/*****************************************/
#define RA_H_REPORT_VERSION 0 /* iReportVersion */

/* 
RA outputs a "report" whenever new information is available. 
A report consists of a header (struct ra_header_struct), followed by the new information. 
The format of the new information section is one of several possible structures, determined by the "eType" field in the header.
See also file ra_format_defines.h.  (Not part of this specification, but helpful.)  
*/

#define RA_MAX_SINFO_LENGTH 80 /* length of sInfo field; see below */
#define RA_MAX_CH_DIV64 16     /* maximum number of channels supported, divided by 64 */


/*********************************************************************************/
/*********************************************************************************/
/*** Here's the standard header that appears at the beginning of all reports: ****/
/*********************************************************************************/
/*********************************************************************************/

struct ra_header_struct {

  /* what type of report is this? */
  int eType; /* =0 header only; e.g., for diagnostic purposes. */
             /* =1 time domain analysis, full-bandwidth & channels, period-T0 update */
             /* =2 time domain analysis, full-bandwidth & channels, period-T1 update */
             /* =3 time domain analysis, subchannels, period-T0 update */
             /* =4 time domain analysis, subchannels, period-T1 update */
             /* =5 freq domain analysis for specified channel, period-T0 update */
             /* =6 freq domain analysis for specified channel, period-T2 update */

  /* error/status */
  long int err; /* Bits set to identify error/status; err=0 means all OK. */

  /*******************************************/
  /*** metadata that shouldn't be changing ***/
  /*******************************************/

  unsigned short int iReportVersion;   /* version of this packet format.  Code should set this equal to RA_H_REPORT_VERSION */
  unsigned short int iRAVersion;       /* version of RA that is outputting this report.  Code should set this equal to RA_H_RA_VERSION */
  unsigned char eSource;               /* 1 = GUPPI raw data file, 2 = GUPPI real-time, <future formats added here> */
  char sInfo[RA_MAX_SINFO_LENGTH];     /* human-friendly free-format string */
                                       /* specified at start-up and passed through without modification */ 

  /**********************************************************/
  /*** metadata extracted from the source data's metadata ***/
  /**********************************************************/

  struct timeval tvStart; /* [unix time] Start time for interval considered in first report. Absolute ("calendar") time */
  long int nCh;           /* Number of channels.  Meaning of "channel" depends on eSource. */
                          /* ...Generally, "channels" are defined within the data, as opposed to being the result of some additional processing. */
                          /* ..."0" means the source data is not divided into channels. nCh=1 is not allowed. */
  float  bw;              /* [Hz] Bandwidth = OBSBW*(1e+6) (may be negative) */  
  double fc;              /* [Hz] Center frequency for "full bandwidth" = OBSFREQ*(1e+6) */
  double fs;              /* [Hz] Sample rate per-channel = 1/TBIN */

  /* note for GUPPI raw data format the center frequency of channel "i" is (fc - bw/2 + (i-0.5)*bw) [Hz] */


  /***************************************************************/
  /*** parameters specified at start-up, describing operation: ***/
  /***************************************************************/

  /* Some flags determining what gets done */
  char tflags; /* b0 (LSB): Do time-domain analysis for entire available bandwidth? (1=Yes). Channels flagged in bChInFull[] will be excluded */
               /* b1:       Do time-domain analysis for channels? (1=Yes). Channels flagged in bChIn[] will not be analyzed. */
               /* b2:       Do time-domain analysis for subchannels? (1=Yes). Subchannels in channels flagged in bChIn[] will not be analyzed. */
               /* b3:       Do baseline cal for full bandwidth? (1=Yes). Channels flagged in bChInFull[] will be excluded. */
               /*           ...applies only to Stokes-I */
               /* b4:       Do baseline cal for channels? (1=Yes). Channels flagged in bChIn[] will not be baselined. */
               /*           ...applies only to Stokes-I */
               /* b5-b7:    RESERVED */
  char fflags; /* b0 (LSB): RESERVED. (Some day: Do freq-domain analysis for entire available bandwidth? (1=Yes).) */
               /* b1:       Do freq-domain analysis for channels? (1=Yes). Channels flagged in bChIn[] will not be analyzed. */
               /* b2:       Do freq-domain analysis for subchannels? (1=Yes). Subchannels in channels flagged bChIn[] will not be analyzed. */
               /* b3:       Do baseline cal for channels prior to analysis? (1=Yes). Channels flagged in bChIn[] will not be baselined. */
               /*           ...applies only to Stokes-I */
               /* b4:       Do baseline cal for subchannels prior to analysis? (1=Yes). Subchannel flagged in bChIn[] will not be baselined. */
               /*           ...applies only to Stokes-I */
               /* b5-b7:    RESERVED */

  double T0; /* [seconds] period at which this packet is updated/sent */
  double T1; /* [seconds] period > T0 at which time-domain info is updated (in addition to updates at packet rate) */
             /* < 0 means don't do this */
  double T2; /* [seconds] period > T0 at which freq-domain info is updated (in addition to updates at packet rate) */
             /* < 0 means don't do this */
 
  /* Channels to consider. These are bit-wise flags indicating channels; e.g., LSB refers to lowest-indexed channel */ 
  unsigned long int bChIn[RA_MAX_CH_DIV64];   /* channels to be processed in "channel-by-channel" & "full bandwidth" processing (1=exclude) */
  unsigned long int bChInCh[RA_MAX_CH_DIV64]; /* channels to be processed in "channel-by-channel" processing (overrides bChIn) */

  /* Subchannel defintion.  "Subchannels" are further divisions within channels for *time-domain* processing. */
  long int nSubCh;   /* number of subchannels per channel */
                     /* ..."0" means the source data is not divided into channels. nCh=1 is not allowed. */ 
  char eSubChMethod; /* method for creating subchannels. 0=FFT, no window; 1=FFT, Hamming window */

  /* Time domain processing parameters. */
  char eTBL_Method; /* When baselining is done, this determines how. Note baselines are updated at the T2 period. */
                    /* ...=1: Fit polynomial of order nBaselineOrder to last T2-period mean, then divide */
  int nTBL_Order;   /* Order of polynomial used when eBaselineMethod=1 */
  char eTBL_Units;  /* Units in which baselined spectrum are reported */
                    /* ...=0: Natural units (i.e., no change in units) */  
                    /* ...=1: Convert to standard deviations, with mean and standard deviation computed using all available bins */

  /* Frequency domain processing parameters.  Note these apply on a channel-by-channel basis */
  int nfft;         /* FFT length */
  int nfch;         /* >=nfft; This is number of channels from center considered in baselining and freq-domain analysis */
  char eFBL_Method; /* When baselining is done, this determines how. Note baselines are updated at the T2 period. */
                    /* ...=1: Fit polynomial of order nBaselineOrder to last T2-period mean, then divide */
  int nFBL_Order;   /* Order of polynomial used when eBaselineMethod=1 */
  char eFBL_Units;  /* Units in which baselined spectrum are reported */
                    /* ...=0: Natural units (i.e., no change in units) */  
                    /* ...=1: Convert to standard deviations, with mean and standard deviation computed using all available bins */  

  /************************************************/
  /*** some metadata specific to current report ***/
  /************************************************/

  long int iSeqNo; /* sequence number; = iSeqNo from last report (of any kind) + 1 */ 
  double fStart;   /* time in seconds elapsed from tvStart until start of information being described in this report */
  
  }; /* struct ra_header_struct */


/************************************************************************************/
/************************************************************************************/
/** Structures used to define format of "new information" sections for eType 1..6 ***/
/************************************************************************************/
/************************************************************************************/

struct DAstruct {
  float mean; /* mean */
  float max;  /* maximum value over interval */
  //float min;  // future feature /* minumum value over interval */
  float rms;  /* RMS (standard deviation) */
  //float median;   // future feature
  float s; /* skewness */
  float k; /* excess kurtosis */
  };

struct DAPstruct { 
  struct DAstruct xi;  /* real component of x */
  struct DAstruct xq;  /* imag component of x */
  struct DAstruct yi;  /* real component of y */
  struct DAstruct yq;  /* imag component of y */
  struct DAstruct xm2; /* X pol magnitude squared */ 
  struct DAstruct ym2; /* Y pol magnitude squared */
  /* note Stokes I is xm2+ym2, and Stokes Q is xm2-ym2 */
  /* we do xm2 and ym2 as opposed to I and Q to better sense problems specific to X and Y */
  struct DAstruct u;   /* Stokes U */
  struct DAstruct v;   /* Stokes V */
  };

struct clips_struct {
  long x; /* number of times sqrt( xi^2 + xq^2 ) >= max encodable value of xi (or xq) is seen */
  long y; /* number of times sqrt( yi^2 + yq^2 ) >= max encodable value of yi (or yq) is seen */
  };

/****************************************************************************************************************/
/****************************************************************************************************************/
/*** "New Information" section for eType=1: time domain analysis, full-bandwidth & channels, period-T0 update ***/
/*** "New Information" section for eType=2: time domain analysis, full-bandwidth & channels, period-T1 update ***/
/****************************************************************************************************************/
/****************************************************************************************************************/

struct ra_td {
  struct clips_struct clips;                 /* clip counters */
  struct DAPstruct tda;                      /* full bandwidth (all channels as one) */
  struct DAPstruct tdac[RA_MAX_CH_DIV64*64]; /* per channel */
  }; 

/****************************************************************************************************************/
/****************************************************************************************************************/
/*** "New Information" section for eType=3: time domain analysis, subchannels, period-T0 update ***/
/*** "New Information" section for eType=4: time domain analysis, subchannels, period-T1 update ***/
/****************************************************************************************************************/
/****************************************************************************************************************/

struct ra_tds {
  //struct DAPstruct tdas[RA_MAX_CH_DIV64*64][nSubCh]; /* statistics, per channel & subchannel */
  struct DAPstruct *(tdas[RA_MAX_CH_DIV64*64]); /* statistics, per channel & subchannel.  Needs to be allocated [1..nSubCh] */
  }; 

/***********************************************************************************************************/
/***********************************************************************************************************/
/*** "New Information" section for eType=5: freq domain analysis for specified channel, period-T0 update ***/
/*** "New Information" section for eType=6: freq domain analysis for specified channel, period-T2 update ***/
/***********************************************************************************************************/
/***********************************************************************************************************/

struct ra_fd {
  int ch;                     /* channel number that this applies to */
  //struct DAPstruct fda[nfch]; /* statistics */ 
  struct DAPstruct *fda; /* statistics. Needs to be allocated [1..nfch] */ 
  }; 



/*******************************/
/*******************************/
/** Notes on future features ***/
/*******************************/
/*******************************/

// some summary report of tones, pulses (3/5/10 sigma events in time/freq)
// radars (center freq, folding period, pulse width (FWHM), absolute time sync)
// some metric that can be used to indentify "flutter"


     
