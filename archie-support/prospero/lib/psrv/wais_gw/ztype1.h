/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*----------------------------------------------------------------------*/

#ifndef _H_Type_1_Query_
#define _H_Type_1_Query_

#include "cdialect.h"
#include "zutil.h"

/*----------------------------------------------------------------------*/



#define	DT_AttributeList	(data_tag)44
#define DT_Term			(data_tag)45
#define DT_Operator		(data_tag)46

#define QT_BooleanQuery	"1"		


#define IGNORE	"ig"


#define	UV_ISBN	"ub"
#define	CORPORATE_NAME	"uc"
#define	ISSN	"us"
#define	PERSONAL_NAME	"up"
#define	SUBJECT	"uj"
#define	TITLE	"ut"
#define	GEOGRAPHIC_NAME	"ug"
#define	CODEN	"ud"
#define	SUBJECT_SUBDIVISION	"ue"
#define	SERIES_TITLE	"uf"
#define	MICROFORM_GENERATION	"uh"
#define	PLACE_OF_PUBLICATION	"ui"
#define	NUC_CODE	"uk"
#define	LANGUAGE	"ul"
#define	COMBINATION_OF_USE_VALUES	"um"
#define	SYSTEM_CONTROL_NUMBER	"un"
#define	DATE	"uo"
#define	LC_CONTROL_NUMBER	"ur"
#define	MUSIC_PUBLISHERS_NUMBER	"uu"
#define	GOVERNMENT_DOCUMENTS_NUMBER	"uv"
#define	SUBJECT_CLASSIFICATION	"uw"
#define	RECORD_TYPE	"uy"


#define	EQUAL	"re"
#define	GREATER_THAN	"rg"
#define	GREATER_THAN_OR_EQUAL	"ro"
#define	LESS_THAN	"rl"
#define	LESS_THAN_OR_EQUAL	"rp"
#define	NOT_EQUAL	"rn"


#define	FIRST_IN_FIELD	"pf"
#define	FIRST_IN_SUBFIELD	"ps"
#define	FIRST_IN_A_SUBFIELD	"pa"
#define	FIRST_IN_NOT_A_SUBFIELD	"pt"
#define	ANY_POSITION_IN_FIELD	"py"


#define	PHRASE	"sp"
#define	WORD	"sw"
#define	KEY	"sk"
#define	WORD_LIST	"sl"


#define	NO_TRUNCATION	"tn"
#define	RIGHT_TRUNCATION	"tr"
#define	PROC_NUM_INCLUDED_IN_SEARCH_ARG	"ti"


#define	INCOMPLETE_SUBFIELD	"ci"
#define	COMPLETE_SUBFIELD	"cs"
#define	COMPLETEFIELD	"cf"


#define AND	"a"
#define OR	"o"
#define AND_NOT	"n"


#define TT_Attribute		1
#define	TT_ResultSetID		2
#define	TT_Operator			3

#define ATTRIBUTE_SIZE		3
#define OPERATOR_SIZE		2

typedef struct query_term {
  
  long	TermType;
  
  char	Use[ATTRIBUTE_SIZE];
  char	Relation[ATTRIBUTE_SIZE];
  char	Position[ATTRIBUTE_SIZE];
  char	Structure[ATTRIBUTE_SIZE];
  char	Truncation[ATTRIBUTE_SIZE];
  char	Completeness[ATTRIBUTE_SIZE];
  any*	Term;
  
  any*	ResultSetID;
  
  char	Operator[OPERATOR_SIZE];
} query_term;

/*----------------------------------------------------------------------*/


#ifdef __cplusplus

extern "C"
	{
#endif 

query_term* makeAttributeTerm _AP((
        char* use,char* relation,char* position,char* structure,
	char* truncation,char* completeness,any* term));
query_term* makeResultSetTerm _AP((any* resultSet));
query_term* makeOperatorTerm _AP((char* operatorCode));
void freeTerm _AP((query_term* qt));
char* writeQueryTerm _AP((query_term* qt,char* buffer,long* len));
char* readQueryTerm _AP((query_term** qt,char* buffer));
any* writeQuery _AP((query_term** terms));
query_term** readQuery _AP((any* info));

#ifdef __cplusplus
	}
#endif 

/*----------------------------------------------------------------------*/

#endif
