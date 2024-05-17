
 
#ifndef docid_h
#define docid_h

#include "cdialect.h"
#include "zprot.h"

#define COPY_WITHOUT_RESTRICTION        (0L)
#define ALL_RIGHTS_RESERVED             (1L)
#define DISTRIBUTION_RESTRICTIONS_APPLY (2L)

/*---------------------------------------------------------------------------*/

typedef struct DocID
 { any* originalServer;
   any* originalDatabase;
   any* originalLocalID;
   any* distributorServer;
   any* distributorDatabase;
   any* distributorLocalID;
   long copyrightDisposition;
 } DocID;

DocID* makeDocID(void);
DocID* copyDocID(DocID* doc);
void freeDocID(DocID* doc);
any* GetServer(DocID* doc);
DocID* docIDFromAny(any* rawDocID);
any* anyFromDocID(DocID* docID);
any* GetDatabase(DocID* doc);
any* GetLocalID(DocID* doc);
long GetCopyrightDisposition(DocID* doc);
long ReadDocID(DocID* doc, FILE* file);
long WriteDocID(DocID* doc, FILE* file);
boolean cmpDocIDs(DocID* d1,DocID* d2);

/*---------------------------------------------------------------------------*/

#endif 

