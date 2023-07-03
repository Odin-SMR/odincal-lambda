/* Id */

/*****************************************************************
**
** SMR structures to HDF software. Version 0.3
**
** Created by:
**   Frank Merino
**
** The following software was written to write an OdinScan
** an OdinFilter structure to a HDF file using the Vdata model interface.
** The format that the data takes within the Vdata are based
** on Nick Lloyd's software ONYX.
**
*****************************************************************/
/* number of fields in the ODIN_SMR_STRUCT                  */
#define NUM_FIELDS                 48
/* number of fileds in the ODIN_FILTER_STRUCT structure     */
#define NUM_FILTER_FIELDS          32
/* number of fields when the ODIN_SMR_STRUCT is expanded    */
#define NUM_FIELDS_EXPANDED       104
/* number of fields when the ODIN_FILTER_STRUCT is expanded */
#define NUM_FILTER_FIELDS_EXPANDED 51 
/* number of different Vdata/Containers available           */
#define NUM_VDATA_TYPES             5     
 
/********************************************************
**              Container information                  **
********************************************************/
 
#define CONT_SMR_HEADER      10000
#define CONT_FILTER_DATA     20000

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
