/*===============================================================
ra_read_jobfile.c: S.W. Ellingson, Virginia Tech, 2014 Jan 19
reads jobfile
================================================================*/

#define RA_MAX_LINE_LENGTH 4096  /* for text files read in */

/*==============================================================*/
/*=== iswhitespace() ===========================================*/
/*==============================================================*/

int iswhitespace( char c ) {
  int bResult=0;
  if (c==0x09) bResult=0x09; /* horizontal tab */
  if (c==0x0a) bResult=0x0a; /* linefeed */
  if (c==0x0b) bResult=0x0b; /* vertical tab */
  if (c==0x0c) bResult=0x0c; /* form feed */
  if (c==0x0d) bResult=0x0d; /* carriage return */
  if (c==0x20) bResult=0x20; /* space */
  return bResult;
  }

/*==============================================================*/
/*=== ra_read_jobfile() ======================================*/
/*==============================================================*/

int ra_read_jobfile( char* jobfile,                    /* [in] name of job file */
                     struct ra_header_struct *header,  /* [out] report output header */
                     char* infile                      /* [out] name of data file */   
                    ) {

  FILE *fp;
  char line[RA_MAX_LINE_LENGTH];
  int i;
  char keyword[RA_MAX_LINE_LENGTH];
  int bFoundKeyword=0;
  char scratch_string[RA_MAX_LINE_LENGTH];
  int iCh;

  int string_length;

  int index_arr;
  int index_bit;
  unsigned long int uli;

  int temp_char;

  /* initialize the header */
  header->eType = RA_H_ETYPE_NULL;
  header->err = 0;
  header->iReportVersion = RA_H_REPORT_VERSION; 
  header->iRAVersion     = RA_H_RA_VERSION;         
  header->eSource        = RA_H_ESOURCE_GUPPI_FILE; /* only option implemented so far... */                         
  sprintf(header->sInfo,"%s",jobfile);          /* use jobfile name as default for this */
  //header->tvStart

  header->nCh = 0;
  header->bw = 0.0;
  header->fc = 0.0;
  header->fs = 0.0;

  header->tflags = 0;
  header->fflags = 0;

  header->T0 =  1.0;
  header->T1 = -1;
  header->T2 = -1;

  for (i=0;i<RA_MAX_CH_DIV64;i++) { header->bChIn[i]=0; }
  for (i=0;i<RA_MAX_CH_DIV64;i++) { header->bChInCh[i]=0; }

  header->nSubCh = 0;
  header->eSubChMethod = RA_H_ESUBCHMETHOD_FFT;

  header->eTBL_Method = RA_H_ETBL_METHOD_SIMPLE;
  header->nTBL_Order = 1;
  header->eTBL_Units = RA_H_ETBL_UNITS_NATURAL;

  header->nfft = 16;
  header->nfch = (header->nfft)*3/4;
  header->eFBL_Method = RA_H_EFBL_METHOD_SIMPLE;
  header->nFBL_Order = 1;
  header->eFBL_Units = RA_H_EFBL_UNITS_NATURAL;

  header->iSeqNo = 0;
  header->fStart = 0.0;

  /* open the jobfile */
  if (!(fp=fopen(jobfile,"r"))) {
      printf("FATAL: In ra_read_jobfile(), unable to open '%s'\n",jobfile);
      return 1;
    } 

  /* read line-by-line */
  while (fgets(line,RA_MAX_LINE_LENGTH,fp)>0) {
    //printf("%s",line);

    /* find first non-whitespace character */
    i = 0;
    while ( iswhitespace( line[i] ) && (i<(strlen(line))-1) ) i++;

    bFoundKeyword = 0;    
    if ( (!iswhitespace( line[i] )) && ( line[i] != '#' ) ) { /* if no content or a '#', we're done with this line */
      //printf("%s",&(line[i]));
     
      sscanf(&(line[i]),"%s",keyword);  //printf("'%s'\n",keyword);
      
      if (strncmp(keyword,"SOURCE",6)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&temp_char);
        header->eSource = temp_char;
        } 
      /* TODO error checking */

      if (strncmp(keyword,"INFILE",6)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %s",keyword,infile);
        memset(header->sInfo,'\0',RA_MAX_SINFO_LENGTH); /* fill with null terminators */
        string_length = strlen(infile); if (string_length>(RA_MAX_SINFO_LENGTH-1)) { string_length=RA_MAX_SINFO_LENGTH-1; }
        memcpy(header->sInfo,infile,string_length);
        } 

      if (strncmp(keyword,"TFLAGS",6)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&temp_char);
        header->tflags = temp_char;
        } 

      if (strncmp(keyword,"FFLAGS",6)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&temp_char);
        header->fflags = temp_char;
        } 

      if (strncmp(keyword,"T0",6)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %lf",keyword,&(header->T0));
        } 

      if (strncmp(keyword,"T1",6)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %lf",keyword,&(header->T1));
        } 

      if (strncmp(keyword,"T2",6)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %lf",keyword,&(header->T2));
        } 

      if (strncmp(keyword,"EXCLUDE",7)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&iCh);
        if (iCh>(RA_MAX_CH_DIV64*64)) {
          printf("FATAL: In ra_read_jobfile(), iCh=%d is > RA_MAX_CH_DIV64*64 = %d\n",iCh,RA_MAX_CH_DIV64*64);
          return 1;        
          }
        index_arr = (iCh-1)/64;                          /* this is the 0-based array index containing bit to be set */
        index_bit = (iCh-1) - index_arr*64;              /* this is the 0-based bit index to be set (0=LSB) */
        uli = 1;
        header->bChIn  [index_arr] += ( uli << index_bit); /* set the bit */
        header->bChInCh[index_arr] += ( uli << index_bit); /* set the bit */
        } 
    
      if (strncmp(keyword,"INCLUDE",7)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&iCh);
        if (iCh>(RA_MAX_CH_DIV64*64)) {
          printf("FATAL: In ra_read_jobfile(), iCh=%d is > RA_MAX_CH_DIV64*64 = %d\n",iCh,RA_MAX_CH_DIV64*64);
          return 1;        
          }
        index_arr = (iCh-1)/64;                          /* this is the 0-based array index containing bit to be set */
        index_bit = (iCh-1) - index_arr*64;              /* this is the 0-based bit index to be set (0=LSB) */
        uli = 1;
        header->bChInCh[index_arr] = header->bChInCh[index_arr] ^ ( uli << index_bit); /* unset the bit */
        } 

      if (strncmp(keyword,"N_SUB_CH",8)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %ld",keyword,&(header->nSubCh));
        } 

      if (strncmp(keyword,"SUB_CH_METHOD",15)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&temp_char);
        header->eSubChMethod = temp_char;
        } 

      if (strncmp(keyword,"TBL_METHOD",10)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&temp_char);
        header->eTBL_Method = temp_char;
        } 

      if (strncmp(keyword,"TBL_ORDER",9)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&temp_char);
        header->nTBL_Order = temp_char;
        } 

      if (strncmp(keyword,"TBL_UNITS",9)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&temp_char);
        header->eTBL_Units = temp_char;
        } 

      if (strncmp(keyword,"NFFT",4)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&(header->nfft));
        } 

      if (strncmp(keyword,"NFCH",4)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&(header->nfch));
        } 

      if (strncmp(keyword,"FBL_METHOD",10)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&temp_char);
        header->eFBL_Method = temp_char;
        } 

      if (strncmp(keyword,"FBL_ORDER",9)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&(header->nFBL_Order));
        } 

      if (strncmp(keyword,"FBL_UNITS",9)==0) {
        bFoundKeyword=1;
        sscanf(&(line[i]),"%s %d",keyword,&temp_char);
        header->eFBL_Units = temp_char;
        } 

      if (!bFoundKeyword) {
        printf("FATAL: In ra_read_jobfile(), keyword '%s' not recognized\n",keyword);
        fclose(fp);
        return 1;
        }

      } /* ( line[i] != '#' ) */

    } /* while (fgets */

  /* close the jobfile */
  fclose(fp);

  return 0;
  }

//==================================================================================
//=== HISTORY ======================================================================
//==================================================================================
// ra_read_jobfile.c: S.W. Ellingson, Virginia Tech, 2014 Jan 19
// -- removed oflags
// ra_read_jobfile.c: S.W. Ellingson, Virginia Tech, 2014 Jan 18
// -- added oflags
// ra_read_jobfile.c: S.W. Ellingson, Virginia Tech, 2013 Nov 25
// -- initial version
