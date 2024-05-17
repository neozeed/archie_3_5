/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*----------------------------------------------------------------------*/

#ifndef _H_utils_Z39_50
#define _H_utils_Z39_50

#include "cdialect.h"
#include "cutil.h"
#include <pfs_threads.h>
/*----------------------------------------------------------------------*/



#define HEADER_LEN	(size_t)2 

typedef long pdu_type;

#define	initAPDU			(pdu_type)20
#define	initResponseAPDU		(pdu_type)21
#define	searchAPDU			(pdu_type)22
#define	searchResponseAPDU		(pdu_type)23
#define	presentAPDU			(pdu_type)24
#define	presentResponseAPDU		(pdu_type)25
#define	deteteAPDU			(pdu_type)26
#define	deleteResponseAPDU		(pdu_type)27
#define	accessControlAPDU		(pdu_type)28
#define	accessControlResponseAPDU	(pdu_type)29
#define	resourceControlAPDU		(pdu_type)30
#define	resourceControlResponseAPDU	(pdu_type)31

typedef struct any {	
	unsigned long	size; 
	char*			bytes;
	} any;
	
typedef any	bit_map; 	

typedef unsigned long data_tag;

#define DT_PDUType			(data_tag)1 	
#define	DT_ReferenceID			(data_tag)2
#define	DT_ProtocolVersion		(data_tag)3
#define	DT_Options			(data_tag)4
#define	DT_PreferredMessageSize		(data_tag)5
#define	DT_MaximumRecordSize		(data_tag)6
#define	DT_IDAuthentication		(data_tag)7
#define	DT_ImplementationID		(data_tag)8
#define	DT_ImplementationName		(data_tag)9
#define	DT_ImplementationVersion	(data_tag)10
#define	DT_UserInformationField		(data_tag)11
#define	DT_Result			(data_tag)12
#define	DT_SmallSetUpperBound		(data_tag)13
#define	DT_LargeSetLowerBound		(data_tag)14
#define	DT_MediumSetPresentNumber	(data_tag)15
#define	DT_ReplaceIndicator		(data_tag)16
#define	DT_ResultSetName		(data_tag)17
#define	DT_DatabaseNames		(data_tag)18
#define	DT_ElementSetNames 		(data_tag)19
#define	DT_QueryType			(data_tag)20
#define	DT_Query			(data_tag)21
#define	DT_SearchStatus			(data_tag)22
#define	DT_ResultCount			(data_tag)23
#define	DT_NumberOfRecordsReturned	(data_tag)24
#define	DT_NextResultSetPosition	(data_tag)25
#define	DT_ResultSetStatus		(data_tag)26
#define	DT_PresentStatus		(data_tag)27
#define	DT_DatabaseDiagnosticRecords	(data_tag)28
#define	DT_NumberOfRecordsRequested	(data_tag)29
#define	DT_ResultSetStartPosition	(data_tag)30
#define	DT_ResultSetID			(data_tag)31
#define	DT_DeleteOperation		(data_tag)32
#define	DT_DeleteStatus			(data_tag)33
#define	DT_NumberNotDeleted		(data_tag)34
#define	DT_BulkStatuses			(data_tag)35
#define	DT_DeleteMSG			(data_tag)36
#define	DT_SecurityChallenge		(data_tag)37
#define	DT_SecurityChallengeResponse	(data_tag)38
#define	DT_SuspendedFlag		(data_tag)39
#define	DT_ResourceReport		(data_tag)40
#define	DT_PartialResultsAvailable	(data_tag)41
#define	DT_ContinueFlag			(data_tag)42
#define	DT_ResultSetWanted		(data_tag)43

#define UNUSED	-1


#define CompressedInt1Byte	128 		
#define CompressedInt2Byte	16384 		
#define CompressedInt3Byte	2097152 	



#define QT_0	"0"	

#define	WILL_USE		TRUE
#define WILL_NOT_USE		FALSE
#define WILL_SUPPORT		TRUE
#define WILL_NOT_SUPPORT	FALSE


#define ACCEPT	TRUE
#define REJECT	FALSE


#define ON	TRUE
#define OFF	FALSE


#define	SUCCESS	0 
#define FAILURE	1


#define	SUBSET	1
#define INTERIM	2
#define NONE	3



#define PARTIAL_1	1
#define PARTIAL_2	2
#define PARTIAL_3	3
#define PARTIAL_4	4
#define PS_NONE		5 

#define DIAGNOSTIC_CODE_SIZE	(size_t)3

typedef struct diagnosticRecord 
 { boolean	SURROGATE;
   char		DIAG[DIAGNOSTIC_CODE_SIZE];
   char* 	ADDINFO;
 } diagnosticRecord;

#define D_PermanentSystemError	       "S1"
#define D_TemporarySystemError	       "S2"
#define D_UnsupportedSearch	       "S3"
#define D_TermsOnlyStopWords	       "S5"
#define D_TooManyArgumentWords	       "S6"
#define D_TooManyBooleanOperators      "S7"
#define D_TooManyTruncatedWords	       "S8"
#define D_TooManyIncompleteSubfields   "S9"
#define D_TruncatedWordsTooShort       "SA"
#define D_InvalidFormatForRecordNumber "SB"
#define D_TooManyCharactersInSearch    "SC"
#define D_TooManyRecordsRetrieved      "SD"
#define D_PresentRequestOutOfRange     "SF"
#define D_SystemErrorInPresentRecords  "SG"
#define D_RecordNotAuthorizedToBeSent  "SH"
#define D_RecordExceedsPrefMessageSize "SI"
#define D_RecordExceedsMaxRecordSize   "SJ"
#define D_ResultSetNotSuppAsSearchTerm "SK"
#define D_OnlyOneRsltSetAsSrchTermSupp "SL"
#define D_OnlyANDingOfASnglRsltSetSupp "SM"
#define D_RsltSetExistsNoReplace       "SN"
#define D_ResultSetNamingNotSupported  "SO"
#define D_CombinationDatabasesNotSupp  "SP"
#define D_ElementSetNamesNotSupported  "SQ"
#define D_ElementSetNameNotValid       "SR"
#define D_OnlyASingleElmntSetNameSupp  "SS"
#define D_ResultSetDeletedByTarget     "ST"
#define D_ResultSetIsInUse             "SU"
#define D_DatabasesIsLocked            "SV"
#define D_TerminatedByNoContinueResp   "SW"
#define D_ResultSetDoesNotExist        "SX"
#define D_ResExNoResultsAvailable      "SY"
#define D_ResExUnpredictableResults    "SZ"
#define D_ResExValidSubsetOfResults    "T1"
#define D_AccessControlFailure         "T2"
#define D_SecurityNotIssuedReqTerm     "T3"
#define D_SecurityNotBeIssuedRecNotInc "T4"

/*----------------------------------------------------------------------*/



#ifndef _C_utils_Z39_50_
EXTERN_CHARP_DECL(readErrorPosition); 
#define readErrorPosition p_th_arreadErrorPosition[p__th_self_num()]
#endif 

#define RETURN(val)	{retval = val; goto cleanup; }
 
#define RETURN_ON_NULL(var) 					\
	if (var == NULL) 				     	\
	  return(NULL); 

#define CLEAN_RETURN_ON_NULL(var) 					\
	if (var == NULL) 				     	\
	  RETURN(NULL); 

#define REPORT_READ_ERROR(pos) 					\
	{ readErrorPosition = (pos);				\
	  return(NULL); 		\
    }

#define CLEAN_REPORT_READ_ERROR(pos) 					\
	{ readErrorPosition = (pos);				\
	  RETURN(NULL); 		\
    }

#define CHECK_FOR_SPACE_LEFT(spaceNeeded,spaceLeft)		\
	{ if (*spaceLeft >= spaceNeeded)			\
	    (*spaceLeft) -= spaceNeeded;			\
	  else							\
	   { *spaceLeft = 0; 					\
	     return(NULL);  	\
	   }							\
	}

/*----------------------------------------------------------------------*/

#ifdef __cplusplus

extern "C"
	{
#endif 

diagnosticRecord* makeDiag _AP((boolean surrogate,char* code,char* addInfo));
void freeDiag _AP((diagnosticRecord* diag));
char* writeDiag _AP((diagnosticRecord* diag,char* buffer,long* len));
char* readDiag _AP((diagnosticRecord** diag,char* buffer));

char* writeCompressedInteger _AP((unsigned long num,char* buf,long* len));
char* readCompressedInteger _AP((unsigned long *num,char* buf));
char* writeCompressedIntWithPadding _AP((unsigned long num,unsigned long size,
					 char* buffer,long* len));
unsigned long writtenCompressedIntSize _AP((unsigned long num));

char* writeTag _AP((data_tag tag,char* buf,long* len));
char* readTag _AP((data_tag* tag,char* buf));
data_tag peekTag _AP((char* buf));
unsigned long writtenTagSize _AP((data_tag tag));

any* makeAny _AP((unsigned long size,char* data));
void freeAny _AP((any* a));
any* duplicateAny _AP((any* a));
char* writeAny _AP((any* a,data_tag tag,char* buffer,long* len));
char* readAny _AP((any** anAny,char* buffer));
unsigned long writtenAnySize _AP((data_tag tag,any* a));

any* stringToAny _AP((char* s));
char* anyToString _AP((any* a));
unsigned long writtenStringSize _AP((data_tag tag,char* s));

any* longToAny _AP((long Num));
long anyToLong _AP((any* a));

char* writeString _AP((char* s,data_tag tag,char* buffer,long* len));
char* readString _AP((char** s,char* buffer));

bit_map* makeBitMap(unsigned long numBits,...);

void freeBitMap _AP((bit_map* bm));
boolean bitAtPos _AP((long pos,bit_map* bm));
char* writeBitMap _AP((bit_map* bm,data_tag tag,char* buffer,long* len));
char* readBitMap _AP((bit_map** bm,char* buffer));

char* writeByte _AP((unsigned long aByte,char* buf,long* len));
char* readByte _AP((unsigned char* aByte,char* buf));

char* writeBoolean _AP((boolean flag,char* buf,long* len));
char* readBoolean _AP((boolean* flag,char* buf));

char* writePDUType _AP((pdu_type pduType,char* buf,long* len));
char* readPDUType _AP((pdu_type* pduType,char* buf));
pdu_type peekPDUType _AP((char* buf));

char* writeBinaryInteger _AP((long num,unsigned long size,
			      char* buf,long* len));
char* readBinaryInteger _AP((long* num,unsigned long size,char* buf));
unsigned long writtenCompressedBinIntSize _AP((long num));

char* writeNum _AP((long num,data_tag tag,char* buffer,long* len));
char* readNum _AP((long* num,char* buffer));
unsigned long  writtenNumSize _AP((data_tag tag,long num));

void doList _AP((void** theList,void (*func)()));

char* writeProtocolVersion _AP((char* buf,long* len));
char* defaultImplementationID _AP((void));
char* defaultImplementationName _AP((void));
char* defaultImplementationVersion _AP((void));

#ifdef __cplusplus
	}
#endif 

/*----------------------------------------------------------------------*/

#endif 
