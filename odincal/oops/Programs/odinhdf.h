#ifndef ODINHDF_H
#define ODINHDF_H

/* $Id$ */

/*****************************************************************
**
** SMR structures to HDF software. Version 0.3
**
** Created by:
**   Frank Merino
**
** The following software was written to write an OdinScan
** structure to a HDF file using the Vdata model interface.
** The format that the data takes within the Vdata are based
** on Nick Lloyd's software ONYX.
**
*****************************************************************/

/* number of fields in the ODIN_SMR_STRUCT                  */
#define NUM_FIELDS                 47
/* number of fields when the ODIN_SMR_STRUCT is expanded    */
#define NUM_FIELDS_EXPANDED       100
/* number of different Vdata/Containers available           */
#define NUM_VDATA_TYPES             5     
 
/********************************************************
**              Container information                  **
********************************************************/
 
#define CONT_SMR_HEADER      10000

#define SIZE_FBA_DATA            3
#define SIZE_SMR_DATA          448
#define SIZE_AOS_1728         1728

/*---------------------------------------------------------
--    HOPTR information and structure                    --
---------------------------------------------------------*/
/* used when calling VSsetfields for the HOPTR    */
#define HOPTR_FIELDS            "crosscheck,Array"   
/* used when calling VSdefine for the HOPTR       */
#define HOPTR_FIELD1            "crosscheck"             
/* used when calling VSdefine for the HOPTR       */
#define HOPTR_FIELD2            "Array"              
/* used for allocating memory for the HOPTR       */
#define HOPTR_FIELD1_SIZE       sizeof(uint32)           
/* used for allocating memory for the HOPTR       */
#define HOPTR_FIELD2_SIZE       sizeof(float32)          
/* used when calling VSdefine for the HOPTR       */
#define HOPTR_FIELD1_TYPE       DFNT_UINT32              
/* used when calling VSdefine for the HOPTR       */
#define HOPTR_FIELD2_TYPE       DFNT_FLOAT32             

#include "odinscan.h"
/*-------------------------------------------------------
-- The HOPTR structure, taken from Nick Lloyds,	       --
-- ONYX software.				       --
-------------------------------------------------------*/
#define NUM_HOPTR_ENTRY  4     /* number of HOPTR entries   */
struct HOPTR
{
  /* An array that unquely defines this exposure */
  unsigned long key[NUM_HOPTR_ENTRY];  
};

/*---------------------------------------------------------
--    ODIN_SMR_STRUCT structure                          --
---------------------------------------------------------*/
struct ODIN_SMR_STRUCT
{
  struct OdinScan            scan;  /* OdinScan structure  */
  struct HOPTR pointer;  /* ONYX Hoptr points to associated child containers */
};

typedef struct ODIN_SMR_STRUCT    Sstruct;

int32 container_records(int32 , int32 );
int32 read_smr_hdf(int32 , struct OdinScan *);
int32 write_smr_hdf(int32 , struct OdinScan *);
int32 open_HDF_vdata(char *, int32 , int16 );
void close_HDF_vdata(int32 );
int32 read_smr(int32 , Sstruct *, int32 , int32 );
int32 write_smr(int32 , Sstruct *, int32 , int32 );
char *get_field_names();
int32 write_spectra(int32 , struct HOPTR *, void *, int32 , int32 , int32 );
int32 close_containers();
int32 header_vdata(int32 , char *, char *);
int32 container_id(int32 , int32 , char *);
int32 hoptr_vdata(int32 , char *, int32 , char *);
unsigned long next_cross_checkID();
int32 read_spectra(int32 , struct HOPTR , void *, int32 );

#endif
