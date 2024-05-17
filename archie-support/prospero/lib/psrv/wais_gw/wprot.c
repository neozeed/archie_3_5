/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*----------------------------------------------------------------------*/

#define _C_WAIS_protocol_

#include "wprot.h"
#include "cutil.h"
#include "panic.h"
#include <string.h> 

#define DefWAISInitResponseSize		(size_t)200
#define DefWAISSearchSize			(size_t)3000
#define DefWAISSearchResponseSize	(size_t)6000
#define DefWAISPresentSize			(size_t)1000
#define DefWAISPresentResponseSize	(size_t)6000
#define DefWAISDocHeaderSize		(size_t)500
#define DefWAISShortHeaderSize		(size_t)200
#define DefWAISLongHeaderSize		(size_t)800
#define DefWAISDocTextSize			(size_t)6000
#define DefWAISDocHeadlineSize		(size_t)500
#define DefWAISDocCodeSize			(size_t)500

#define RESERVE_SPACE_FOR_WAIS_HEADER(len)	\
     if (*len > 0)							\
     	*len -= header_len;

static char* writeUserInfoHeader _AP((data_tag tag,long infoSize,	
				      long estHeaderSize,char* buffer,
				      long* len));

static unsigned long userInfoTagSize _AP((data_tag tag,
					  unsigned long length));

/*----------------------------------------------------------------------*/

char*
writeInitInfo(init,buffer,len)
InitAPDU* init;
char* buffer;
long* len;
{
  char *userInfo, *buf;
  

  
  /*!! Clever - set it to Null, and then test it */
  userInfo = NULL;

  if(userInfo != NULL) {
    buf = writeString(userInfo, DT_UserInformationField, buffer, len);
    return(buf);
  }
  else return buffer;
}

/*----------------------------------------------------------------------*/

static char* readUserInfoHeader _AP((data_tag* tag,unsigned long* num,
				     char* buffer));

char*
readInitInfo(info,buffer)
void** info;
char* buffer;
{
  
  readString((char **)info, buffer);
  return buffer;
}

/*----------------------------------------------------------------------*/

static unsigned long
userInfoTagSize(tag,length)
data_tag tag;
unsigned long length;

{
  unsigned long size;

  
  size = writtenCompressedIntSize(tag);
  size += writtenCompressedIntSize(length);
      
  return(size);
}   

/*----------------------------------------------------------------------*/

static char*
writeUserInfoHeader(tag,infoSize,estHeaderSize,buffer,len)
data_tag tag;
long infoSize;
long estHeaderSize;
char* buffer;
long* len;

{
  long dummyLen = 100;		
  char* buf = buffer;
  long realSize = infoSize - estHeaderSize;
  long realHeaderSize = userInfoTagSize(tag,realSize);

  if (buffer == NULL || *len == 0)
    return(NULL);
  
  
  buf = writeTag(tag,buf,&dummyLen);
  
  
  if (estHeaderSize != realHeaderSize)
    {				
      CHECK_FOR_SPACE_LEFT(realHeaderSize - estHeaderSize,len);
      memmove(buffer + realHeaderSize,buffer + estHeaderSize,(size_t)(realSize));
    }
   
  
  writeCompressedInteger(realSize,buf,&dummyLen);
  
  
  return(buffer + realHeaderSize + realSize);
}

/*----------------------------------------------------------------------*/

static char*
readUserInfoHeader(tag,num,buffer)
data_tag* tag;
unsigned long* num;
char* buffer;

{
  char* buf = buffer;
  buf = readTag(tag,buf);
  buf = readCompressedInteger(num,buf);
  return(buf); 
}

/*----------------------------------------------------------------------*/

WAISInitResponse* 
makeWAISInitResponse(chunkCode,
		     chunkIDLen,
		     chunkMarker,
		     highlightMarker,
		     deHighlightMarker,
		     newLineChars)
long chunkCode;
long chunkIDLen;
char* chunkMarker;
char* highlightMarker;
char* deHighlightMarker;
char* newLineChars;

{
  WAISInitResponse* init = (WAISInitResponse*)s_malloc((size_t)sizeof(WAISInitResponse));

  init->ChunkCode = chunkCode;	
  init->ChunkIDLength = chunkIDLen;
  init->ChunkMarker = chunkMarker;
  init->HighlightMarker = highlightMarker;
  init->DeHighlightMarker = deHighlightMarker;
  init->NewlineCharacters = newLineChars;
  
  return(init);
}

/*----------------------------------------------------------------------*/

void 
freeWAISInitResponse(init)
WAISInitResponse* init;

{
  if(!init)
    return;
  s_free(init->ChunkMarker);
  s_free(init->HighlightMarker);
  s_free(init->DeHighlightMarker);
  s_free(init->NewlineCharacters);
  s_free(init);
}

/*----------------------------------------------------------------------*/

char*
writeInitResponseInfo(init,buffer,len)
InitResponseAPDU* init;
char* buffer;
long* len;

{
  unsigned long header_len = userInfoTagSize(DT_UserInformationLength,
					     DefWAISInitResponseSize);
  char* buf = buffer + header_len;
  WAISInitResponse* info = (WAISInitResponse*)init->UserInformationField;
  unsigned long size;
  
  RESERVE_SPACE_FOR_WAIS_HEADER(len);
    
  buf = writeNum(info->ChunkCode,DT_ChunkCode,buf,len);
  buf = writeNum(info->ChunkIDLength,DT_ChunkIDLength,buf,len);
  buf = writeString(info->ChunkMarker,DT_ChunkMarker,buf,len);
  buf = writeString(info->HighlightMarker,DT_HighlightMarker,buf,len);
  buf = writeString(info->DeHighlightMarker,DT_DeHighlightMarker,buf,len);
  buf = writeString(info->NewlineCharacters,DT_NewlineCharacters,buf,len);
  
  
  size = buf - buffer; 
  buf = writeUserInfoHeader(DT_UserInformationLength,size,header_len,buffer,len);
  
  return(buf);
}

/*----------------------------------------------------------------------*/

char*
readInitResponseInfo(info,buffer)
void** info;
char* buffer;

{
  char* buf = buffer;
  unsigned long size; 
  unsigned long headerSize;
  data_tag tag;
  long chunkCode,chunkIDLen;
  char* chunkMarker = NULL;
  char* highlightMarker = NULL;
  char* deHighlightMarker = NULL;
  char* newLineChars = NULL;
  
  chunkCode = chunkIDLen = UNUSED;
  
  buf = readUserInfoHeader(&tag,&size,buf);
  headerSize = buf - buffer;
    
  while (buf < (buffer + size + headerSize))
    { data_tag tag = peekTag(buf);
      switch (tag)
	{ case DT_ChunkCode:
	    buf = readNum(&chunkCode,buf);
	    break;
	  case DT_ChunkIDLength:
	    buf = readNum(&chunkIDLen,buf);
	    break;
	  case DT_ChunkMarker:
	    buf = readString(&chunkMarker,buf);
	    break;
	  case DT_HighlightMarker:
	    buf = readString(&highlightMarker,buf);
	    break;
	  case DT_DeHighlightMarker:
	    buf = readString(&deHighlightMarker,buf);
	    break;
	  case DT_NewlineCharacters:
	    buf = readString(&newLineChars,buf);
	    break;
	  default:
	    s_free(highlightMarker);
	    s_free(deHighlightMarker);
	    s_free(newLineChars);
	    REPORT_READ_ERROR(buf);	/*does a return */
	    break;
	  }
    }
  	  
  *info = (void *)makeWAISInitResponse(chunkCode,chunkIDLen,chunkMarker,
			       highlightMarker,deHighlightMarker,
			       newLineChars);
  return(buf);
}

/*----------------------------------------------------------------------*/

WAISSearch* 
makeWAISSearch(seedWords,
	       docs,
	       textList,
	       dateFactor,
	       beginDateRange,
	       endDateRange,
	       maxDocsRetrieved)
char* seedWords;
DocObj** docs;
char** textList;
long dateFactor;
char* beginDateRange;
char* endDateRange;
long maxDocsRetrieved;


{ 
  WAISSearch* query = (WAISSearch*)s_malloc((size_t)sizeof(WAISSearch));

  query->SeedWords = seedWords;	
  query->Docs = docs;		
  query->TextList = textList;	
  query->DateFactor = dateFactor;
  query->BeginDateRange = beginDateRange;
  query->EndDateRange = endDateRange;
  query->MaxDocumentsRetrieved = maxDocsRetrieved;
  
  return(query);
}

/*----------------------------------------------------------------------*/

void 
freeWAISSearch(query)
WAISSearch* query;


{
  void* ptr = NULL;
  long i;
  
  s_free(query->SeedWords);
  
  if (query->Docs != NULL)
    for (i = 0,ptr = (void *)query->Docs[i]; ptr != NULL; ptr = (void *)query->Docs[++i])
      freeDocObj((DocObj*)ptr);
  s_free(query->Docs);
   
  if (query->TextList != NULL)	
    for (i = 0,ptr = (void *)query->TextList[i]; ptr != NULL; ptr = (void *)query->TextList[++i])
      s_free(ptr);
  s_free(query->TextList);

  s_free(query->BeginDateRange);
  s_free(query->EndDateRange);
  s_free(query);
}

/*----------------------------------------------------------------------*/

DocObj* 
makeDocObjUsingWholeDocument(docID,type)		/*!!ALLOC*/
any* docID;
char* type;


{
  DocObj* doc = (DocObj*)s_malloc((size_t)sizeof(DocObj));
  doc->DocumentID = docID;		
  doc->Type = type;		
  doc->ChunkCode = CT_document;
  return(doc);
}

/*----------------------------------------------------------------------*/

DocObj* 
makeDocObjUsingLines(docID,type,start,end)		/*!!ALLOC*/
any* docID;
char* type;
long start;
long end;


{
  DocObj* doc = (DocObj*)s_malloc((size_t)sizeof(DocObj));
  doc->ChunkCode = CT_line;
  doc->DocumentID = docID;		
  doc->Type = type;		
  doc->ChunkStart.Pos = start;
  doc->ChunkEnd.Pos = end;
  return(doc);
}

/*----------------------------------------------------------------------*/


DocObj* 
makeDocObjUsingBytes(docID,type,start,end)
any* docID;
char* type;
long start;
long end;


{
  DocObj* doc = (DocObj*)s_malloc((size_t)sizeof(DocObj));
  doc->ChunkCode = CT_byte;
  doc->DocumentID = docID;		
  doc->Type = type;		
  doc->ChunkStart.Pos = start;
  doc->ChunkEnd.Pos = end;
  return(doc);
}

/*----------------------------------------------------------------------*/

DocObj* 
makeDocObjUsingParagraphs(docID,type,start,end)
any* docID;
char* type;
any* start;
any* end;


{
  DocObj* doc = (DocObj*)s_malloc((size_t)sizeof(DocObj));
  doc->ChunkCode = CT_paragraph;
  doc->DocumentID = docID;		
  doc->Type = type;
  doc->ChunkStart.ID = start; 
  doc->ChunkEnd.ID = end; 
  return(doc);
}

/*----------------------------------------------------------------------*/

void
freeDocObj(doc)
DocObj* doc;


{
  freeAny(doc->DocumentID);
  s_free(doc->Type);
  if (doc->ChunkCode == CT_paragraph)
    { freeAny(doc->ChunkStart.ID);
      freeAny(doc->ChunkEnd.ID);
    }
  s_free(doc);
}

/*----------------------------------------------------------------------*/

static char* writeDocObj _AP((DocObj* doc,char* buffer,long* len));

static char*
writeDocObj(doc,buffer,len)
DocObj* doc;
char* buffer;
long* len;


{
  char* buf = buffer;
  
  
  if (doc->ChunkCode == CT_document)
    buf = writeAny(doc->DocumentID,DT_DocumentID,buf,len);
  else
    buf = writeAny(doc->DocumentID,DT_DocumentIDChunk,buf,len);
  
  if (doc->Type != NULL)
    buf = writeString(doc->Type,DT_TYPE,buf,len);
  
  switch (doc->ChunkCode)
    { case CT_document:
	
	break;
      case CT_byte:
      case CT_line:
	buf = writeNum(doc->ChunkCode,DT_ChunkCode,buf,len);
	buf = writeNum(doc->ChunkStart.Pos,DT_ChunkStartID,buf,len);
	buf = writeNum(doc->ChunkEnd.Pos,DT_ChunkEndID,buf,len);
	break;
      case CT_paragraph:
	buf = writeNum(doc->ChunkCode,DT_ChunkCode,buf,len);
	buf = writeAny(doc->ChunkStart.ID,DT_ChunkStartID,buf,len);
	buf = writeAny(doc->ChunkEnd.ID,DT_ChunkEndID,buf,len);
	break;
      default:
	panic("Implementation error: unknown chuck type %ld",
	      doc->ChunkCode);
	break;
      }
   
  return(buf);
}

/*----------------------------------------------------------------------*/

static char* readDocObj _AP((DocObj** doc,char* buffer));

static char*
readDocObj(doc,buffer)	/*!!ALLOCS (in doc) */
DocObj** doc;
char* buffer;


{
  char* buf = buffer;
  data_tag tag;
  char *retval = NULL;
  
  *doc = (DocObj*)s_malloc((size_t)sizeof(DocObj));
  
  tag = peekTag(buf);
  buf = readAny(&((*doc)->DocumentID),buf);
  
  if (tag == DT_DocumentID)
    { (*doc)->ChunkCode = CT_document;
      tag = peekTag(buf);
      if (tag == DT_TYPE)	
	buf = readString(&((*doc)->Type),buf);
      
    }
  else if (tag == DT_DocumentIDChunk)
    { boolean readParagraphs = false; 
      tag = peekTag(buf);
      if (tag == DT_TYPE)	
	buf = readString(&((*doc)->Type),buf);
      buf = readNum(&((*doc)->ChunkCode),buf);
      switch ((*doc)->ChunkCode)
	{ case CT_byte:
	  case CT_line:
	    buf = readNum(&((*doc)->ChunkStart.Pos),buf);
	    buf = readNum(&((*doc)->ChunkEnd.Pos),buf);
	    break;
	  case CT_paragraph:
	    readParagraphs = true;
	    buf = readAny(&((*doc)->ChunkStart.ID),buf);
	    buf = readAny(&((*doc)->ChunkEnd.ID),buf);
	    break;
	  default:
	    freeAny((*doc)->DocumentID);
	    if (readParagraphs)
	      { freeAny((*doc)->ChunkStart.ID);
		freeAny((*doc)->ChunkEnd.ID);
	      }
	    s_free(doc);
      	    doc = NULL;		/* Prevent caller using bad pointer*/
	    CLEAN_REPORT_READ_ERROR(buf);	/*Returns*/
	    break;
	  }
    }
  else
    { freeAny((*doc)->DocumentID);
      s_free(*doc);
      doc = NULL;		/* Prevent caller using bad pointer*/
      CLEAN_REPORT_READ_ERROR(buf);
    }
  RETURN(buf);  

cleanup:
	return(retval);
}

/*----------------------------------------------------------------------*/

char* 
writeSearchInfo(query,buffer,len)
SearchAPDU* query;
char* buffer;
long* len;


{
  if (strcmp(query->QueryType,QT_TextRetrievalQuery) == 0)
    { return(writeAny((any*)query->Query,DT_Query,buffer,len));
    }
  else
    { unsigned long header_len = userInfoTagSize(DT_UserInformationLength,
						 DefWAISSearchSize); 
      char* buf = buffer + header_len;
      WAISSearch* info = (WAISSearch*)query->Query;
      unsigned long size;
      long i;
  
      RESERVE_SPACE_FOR_WAIS_HEADER(len);
       
      buf = writeString(info->SeedWords,DT_SeedWords,buf,len);

      if (info->Docs != NULL)
      { for (i = 0; info->Docs[i] != NULL; i++)
	  { buf = writeDocObj(info->Docs[i],buf,len);
	  }
	}
   
      
 
      buf = writeNum(info->DateFactor,DT_DateFactor,buf,len);
      buf = writeString(info->BeginDateRange,DT_BeginDateRange,buf,len);
      buf = writeString(info->EndDateRange,DT_EndDateRange,buf,len);
      buf = writeNum(info->MaxDocumentsRetrieved,DT_MaxDocumentsRetrieved,buf,len);
  
      
      size = buf - buffer; 
      buf = writeUserInfoHeader(DT_UserInformationLength,size,header_len,buffer,len);
   
      return(buf);
    }
}

/*----------------------------------------------------------------------*/

char* 
readSearchInfo(info,buffer)
void** info;
char* buffer;


{
  data_tag type = peekTag(buffer);
  if (type == DT_Query)		
    { char* buf = buffer;
      any* query = NULL;
      buf = readAny(&query,buf);
      *info = (void *)query;
      return(buf);
    }
  else				
    { char* buf = buffer;
      unsigned long size; 
      unsigned long headerSize;
      data_tag tag;
      char* seedWords = NULL;
      char* beginDateRange = NULL;
      char* endDateRange = NULL;
      long dateFactor,maxDocsRetrieved;
      char** textList = NULL; 
      DocObj** docIDs = NULL;
      DocObj* doc = NULL;
      long docs = 0;
      long i;
      void* ptr = NULL;
      char *retval;

      dateFactor = maxDocsRetrieved = UNUSED;
  
      buf = readUserInfoHeader(&tag,&size,buf);
      headerSize = buf - buffer;
  
      while (buf < (buffer + size + headerSize))
	{ data_tag tag = peekTag(buf);
	  switch (tag)
	    { case DT_SeedWords:
		buf = readString(&seedWords,buf);
		break;
	      case DT_DocumentID:
	      case DT_DocumentIDChunk:
		if (docIDs == NULL) 
		  { docIDs = (DocObj**)s_malloc((size_t)sizeof(DocObj*) * 2);
		  }
		else		
		  { docIDs = (DocObj**)s_realloc((char*)docIDs,(size_t)(sizeof(DocObj*) * (docs + 2)));
		  }
		buf = readDocObj(&doc,buf);
		CLEAN_RETURN_ON_NULL(buf);
		docIDs[docs++] = doc; 
		docIDs[docs] = NULL;
		break;
	      case DT_TextList:
		break;
	      case DT_DateFactor:
		buf = readNum(&dateFactor,buf);
		break;
	      case DT_BeginDateRange:
		buf = readString(&beginDateRange,buf);
		break;
	      case DT_EndDateRange:
		buf = readString(&endDateRange,buf);
		break;
	      case DT_MaxDocumentsRetrieved:
		buf = readNum(&maxDocsRetrieved,buf);
		break;
	      default:
		CLEAN_REPORT_READ_ERROR(buf);
		break;
	      }
	}
  	  
      *info = (void *)makeWAISSearch(seedWords,docIDs,textList,
				     dateFactor,beginDateRange,endDateRange,
				     maxDocsRetrieved);
      return(buf);

cleanup:
	/* Note only cleanup on error  success returns pointers to these*/
	s_free(seedWords);
	s_free(beginDateRange);
	s_free(endDateRange);
	if (docIDs != NULL) {
	    for	(i = 0,ptr = (void *)docIDs[i]; 
		ptr != NULL; 
   		ptr = (void *)docIDs[++i])
		    freeDocObj((DocObj*)ptr);
	    s_free(docIDs);	
    }
	return(retval);
    }
/* Note cleanup is only for else part - if part cleans itself up */
}

/*----------------------------------------------------------------------*/

WAISDocumentHeader*
makeWAISDocumentHeader(docID,
		       versionNumber,
		       score,
		       bestMatch,
		       docLen,
		       lines,
		       types,
		       source,
		       theDate,
		       headline,
		       originCity)
any* docID;
long versionNumber;
long score;
long bestMatch;
long docLen;
long lines;
char** types;
char* source;
char* theDate;
char* headline;
char* originCity;


{
  WAISDocumentHeader* header = 
    (WAISDocumentHeader*)s_malloc((size_t)sizeof(WAISDocumentHeader));

  header->DocumentID = docID;
  header->VersionNumber = versionNumber;
  header->Score = score;
  header->BestMatch = bestMatch;
  header->DocumentLength = docLen;
  header->Lines = lines;
  header->Types = types;
  header->Source = source;
  header->Date = theDate;
  header->Headline = headline;
  header->OriginCity = originCity;
  
  return(header);
}

/*----------------------------------------------------------------------*/

void
freeWAISDocumentHeader(header)
WAISDocumentHeader* header;

{
  freeAny(header->DocumentID);
  doList((void**)header->Types,fs_free); 
  s_free(header->Types);
  s_free(header->Source);
  s_free(header->Date);
  s_free(header->Headline);
  s_free(header->OriginCity);
  s_free(header);
}

/*----------------------------------------------------------------------*/

char*
writeWAISDocumentHeader(header,buffer,len)
WAISDocumentHeader* header;
char* buffer;
long* len;
{
  unsigned long header_len = userInfoTagSize(DT_DocumentHeaderGroup ,
					     DefWAISDocHeaderSize);
  char* buf = buffer + header_len;
  unsigned long size;
  
  RESERVE_SPACE_FOR_WAIS_HEADER(len);
   
  buf = writeAny(header->DocumentID,DT_DocumentID,buf,len);
  buf = writeNum(header->VersionNumber,DT_VersionNumber,buf,len);
  buf = writeNum(header->Score,DT_Score,buf,len);
  buf = writeNum(header->BestMatch,DT_BestMatch,buf,len);
  buf = writeNum(header->DocumentLength,DT_DocumentLength,buf,len);
  buf = writeNum(header->Lines,DT_Lines,buf,len);
  if (header->Types != NULL)
    { long size;
      char* ptr = NULL;
      long i;
      buf = writeTag(DT_TYPE_BLOCK,buf,len);
      for (i = 0,size = 0,ptr = header->Types[i]; ptr != NULL; ptr = header->Types[++i])
	{ long typeSize = strlen(ptr);
	  size += writtenTagSize(DT_TYPE);
	  size += writtenCompressedIntSize(typeSize);
	  size += typeSize; 
	}
      buf = writeCompressedInteger((unsigned long)size,buf,len);
      for (i = 0,ptr = header->Types[i]; ptr != NULL; ptr = header->Types[++i])
	buf = writeString(ptr,DT_TYPE,buf,len);
    }
  buf = writeString(header->Source,DT_Source,buf,len);
  buf = writeString(header->Date,DT_Date,buf,len);
  buf = writeString(header->Headline,DT_Headline,buf,len);
  buf = writeString(header->OriginCity,DT_OriginCity,buf,len);
  
  
  size = buf - buffer; 
  buf = writeUserInfoHeader(DT_DocumentHeaderGroup,size,header_len,buffer,len);

  return(buf);
}

/*----------------------------------------------------------------------*/

char*
readWAISDocumentHeader(header,buffer)
WAISDocumentHeader** header;
char* buffer;
{
  char* buf = buffer;
  unsigned long size; 
  unsigned long headerSize;
  data_tag tag;
  any* docID = NULL;
  long versionNumber,score,bestMatch,docLength,lines;
  char** types = NULL;
  char *source = NULL;
  char *theDate = NULL;
  char *headline = NULL;
  char *originCity = NULL;
  char *retval = NULL;
  
  versionNumber = score = bestMatch = docLength = lines = UNUSED;
  
  buf = readUserInfoHeader(&tag,&size,buf);
  headerSize = buf - buffer;
    
  while (buf < (buffer + size + headerSize))
    { data_tag tag = peekTag(buf);
      switch (tag)
	{ case DT_DocumentID:
	    buf = readAny(&docID,buf);
	    break;
	  case DT_VersionNumber:
	    buf = readNum(&versionNumber,buf);
	    break;
	  case DT_Score:
	    buf = readNum(&score,buf);
	    break;
	  case DT_BestMatch:
	    buf = readNum(&bestMatch,buf);
	    break;
	  case DT_DocumentLength:
	    buf = readNum(&docLength,buf);
	    break;
	  case DT_Lines:
	    buf = readNum(&lines,buf);
	    break;
	  case DT_TYPE_BLOCK:
	    { unsigned long size = -1;
	      long numTypes = 0;
	      buf = readTag(&tag,buf);
	      buf = readCompressedInteger(&size,buf);
	      while (size > 0)
		{ char* type = NULL;
		  char* originalBuf = buf;
		  buf = readString(&type,buf);
		  types = (char**)s_realloc(types,(size_t)(sizeof(char*) * (numTypes + 2)));
		  types[numTypes++] = type;
		  types[numTypes] = NULL;
		  size -= (buf - originalBuf);
		}
	    }
	  case DT_Source:
	    buf = readString(&source,buf);
	    break;
	  case DT_Date:
	    buf = readString(&theDate,buf);
	    break;
	  case DT_Headline:
	    buf = readString(&headline,buf);
	    break;
	  case DT_OriginCity:
	    buf = readString(&originCity,buf);
	    break;
	  default:
	    CLEAN_REPORT_READ_ERROR(buf);
	    break;
	  }
    }
  	  
  *header = makeWAISDocumentHeader(docID,versionNumber,score,bestMatch,
				   docLength,lines,types,source,theDate,headline,
				   originCity);
  return(buf);

cleanup:
	/* Only comes this way on error */
	freeAny(docID);
	s_free(source);
	s_free(theDate);
	s_free(headline);
	s_free(originCity);
	return(retval);
}

/*----------------------------------------------------------------------*/

WAISDocumentShortHeader*
makeWAISDocumentShortHeader(docID,
			    versionNumber,
			    score,
			    bestMatch,
			    docLen,
			    lines)
any* docID;
long versionNumber;
long score;
long bestMatch;
long docLen;
long lines;

{
  WAISDocumentShortHeader* header = 
    (WAISDocumentShortHeader*)s_malloc((size_t)sizeof(WAISDocumentShortHeader));

  header->DocumentID = docID;
  header->VersionNumber = versionNumber;
  header->Score = score;
  header->BestMatch = bestMatch;
  header->DocumentLength = docLen;
  header->Lines = lines;
  
  return(header);
}

/*----------------------------------------------------------------------*/

void
freeWAISDocumentShortHeader(header)
WAISDocumentShortHeader* header;
{
  freeAny(header->DocumentID);
  s_free(header);
}

/*----------------------------------------------------------------------*/

char*
writeWAISDocumentShortHeader(header,buffer,len)
WAISDocumentShortHeader* header;
char* buffer;
long* len;
{
  unsigned long header_len = userInfoTagSize(DT_DocumentShortHeaderGroup ,
					     DefWAISShortHeaderSize);
  char* buf = buffer + header_len;
  unsigned long size;
  
  RESERVE_SPACE_FOR_WAIS_HEADER(len);
   
  buf = writeAny(header->DocumentID,DT_DocumentID,buf,len);
  buf = writeNum(header->VersionNumber,DT_VersionNumber,buf,len);
  buf = writeNum(header->Score,DT_Score,buf,len);
  buf = writeNum(header->BestMatch,DT_BestMatch,buf,len);
  buf = writeNum(header->DocumentLength,DT_DocumentLength,buf,len);
  buf = writeNum(header->Lines,DT_Lines,buf,len);
  
  
  size = buf - buffer; 
  buf = writeUserInfoHeader(DT_DocumentShortHeaderGroup,size,header_len,buffer,len);

  return(buf);
}

/*----------------------------------------------------------------------*/

char*
readWAISDocumentShortHeader(header,buffer)
WAISDocumentShortHeader** header;
char* buffer;
{
  char* buf = buffer;
  unsigned long size; 
  unsigned long headerSize;
  data_tag tag;
  any* docID = NULL;
  long versionNumber,score,bestMatch,docLength,lines;
  
  versionNumber = score = bestMatch = docLength = lines = UNUSED;
  
  buf = readUserInfoHeader(&tag,&size,buf);
  headerSize = buf - buffer;
    
  while (buf < (buffer + size + headerSize))
    { data_tag tag = peekTag(buf);
      switch (tag)
	{ case DT_DocumentID:
	    buf = readAny(&docID,buf);
	    break;
	  case DT_VersionNumber:
	    buf = readNum(&versionNumber,buf);
	    break;
	  case DT_Score:
	    buf = readNum(&score,buf);
	    break;
	  case DT_BestMatch:
	    buf = readNum(&bestMatch,buf);
	    break;
	  case DT_DocumentLength:
	    buf = readNum(&docLength,buf);
	    break;
	  case DT_Lines:
	    buf = readNum(&lines,buf);
	    break;
	  default:
	    freeAny(docID);
	    REPORT_READ_ERROR(buf);
	    break;
	  }
    }
  	  
  *header = makeWAISDocumentShortHeader(docID,versionNumber,score,bestMatch,
					docLength,lines);
  return(buf);
}

/*----------------------------------------------------------------------*/

WAISDocumentLongHeader*
makeWAISDocumentLongHeader(docID,
			   versionNumber,
			   score,
			   bestMatch,
			   docLen,
			   lines,
			   types,
			   source,
			   theDate,
			   headline,
			   originCity,
			   stockCodes,
			   companyCodes,
			   industryCodes)
any* docID;
long versionNumber;
long score;
long bestMatch;
long docLen;
long lines;
char** types;
char* source;
char* theDate;
char* headline;
char* originCity;
char* stockCodes;
char* companyCodes;
char* industryCodes;

{
  WAISDocumentLongHeader* header = 
    (WAISDocumentLongHeader*)s_malloc((size_t)sizeof(WAISDocumentLongHeader));

  header->DocumentID = docID;
  header->VersionNumber = versionNumber;
  header->Score = score;
  header->BestMatch = bestMatch;
  header->DocumentLength = docLen;
  header->Lines = lines;
  header->Types = types;
  header->Source = source;
  header->Date = theDate;
  header->Headline = headline;
  header->OriginCity = originCity;
  header->StockCodes = stockCodes;
  header->CompanyCodes = companyCodes;
  header->IndustryCodes = industryCodes;
  
  return(header);
}

/*----------------------------------------------------------------------*/

void
freeWAISDocumentLongHeader(header)
WAISDocumentLongHeader* header;
{
  freeAny(header->DocumentID);
  doList((void**)header->Types,fs_free); 
  s_free(header->Source);
  s_free(header->Date);
  s_free(header->Headline);
  s_free(header->OriginCity);
  s_free(header->StockCodes);
  s_free(header->CompanyCodes);
  s_free(header->IndustryCodes);
  s_free(header);
}

/*----------------------------------------------------------------------*/

char*
writeWAISDocumentLongHeader(header,buffer,len)
WAISDocumentLongHeader* header;
char* buffer;
long* len;
{
  unsigned long header_len = userInfoTagSize(DT_DocumentLongHeaderGroup ,
					     DefWAISLongHeaderSize);
  char* buf = buffer + header_len;
  unsigned long size;
  
  RESERVE_SPACE_FOR_WAIS_HEADER(len);
   
  buf = writeAny(header->DocumentID,DT_DocumentID,buf,len);
  buf = writeNum(header->VersionNumber,DT_VersionNumber,buf,len);
  buf = writeNum(header->Score,DT_Score,buf,len);
  buf = writeNum(header->BestMatch,DT_BestMatch,buf,len);
  buf = writeNum(header->DocumentLength,DT_DocumentLength,buf,len);
  buf = writeNum(header->Lines,DT_Lines,buf,len);
  if (header->Types != NULL)
    { long size;
      char* ptr = NULL;
      long i;
      buf = writeTag(DT_TYPE_BLOCK,buf,len);
      for (i = 0,size = 0,ptr = header->Types[i]; ptr != NULL; ptr = header->Types[++i])
	{ long typeSize = strlen(ptr);
	  size += writtenTagSize(DT_TYPE);
	  size += writtenCompressedIntSize(typeSize);
	  size += typeSize; 
	}
      buf = writeCompressedInteger((unsigned long)size,buf,len);
      for (i = 0,ptr = header->Types[i]; ptr != NULL; ptr = header->Types[++i])
	buf = writeString(ptr,DT_TYPE,buf,len);
    }
  buf = writeString(header->Source,DT_Source,buf,len);
  buf = writeString(header->Date,DT_Date,buf,len);
  buf = writeString(header->Headline,DT_Headline,buf,len);
  buf = writeString(header->OriginCity,DT_OriginCity,buf,len);
  buf = writeString(header->StockCodes,DT_StockCodes,buf,len);
  buf = writeString(header->CompanyCodes,DT_CompanyCodes,buf,len);
  buf = writeString(header->IndustryCodes,DT_IndustryCodes,buf,len);
  
  
  size = buf - buffer; 
  buf = writeUserInfoHeader(DT_DocumentLongHeaderGroup,size,header_len,buffer,len);

  return(buf);
}

/*----------------------------------------------------------------------*/
char*
readWAISDocumentLongHeader(header,buffer)
WAISDocumentLongHeader** header;
char* buffer;
{
  char* buf = buffer;
  unsigned long size; 
  unsigned long headerSize;
  data_tag tag;
  any* docID;
  long versionNumber,score,bestMatch,docLength,lines;
  char **types;
  char *source,*theDate,*headline,*originCity,*stockCodes,*companyCodes,*industryCodes;
  
  docID = NULL;
  versionNumber = score = bestMatch = docLength = lines = UNUSED;
  types = NULL;
  source = theDate = headline = originCity = stockCodes = companyCodes = industryCodes = NULL;
  
  buf = readUserInfoHeader(&tag,&size,buf);
  headerSize = buf - buffer;
    
  while (buf < (buffer + size + headerSize))
    { data_tag tag = peekTag(buf);
      switch (tag)
	{ case DT_DocumentID:
	    buf = readAny(&docID,buf);
	    break;
	  case DT_VersionNumber:
	    buf = readNum(&versionNumber,buf);
	    break;
	  case DT_Score:
	    buf = readNum(&score,buf);
	    break;
	  case DT_BestMatch:
	    buf = readNum(&bestMatch,buf);
	    break;
	  case DT_DocumentLength:
	    buf = readNum(&docLength,buf);
	    break;
	  case DT_Lines:
	    buf = readNum(&lines,buf);
	    break;
	  case DT_TYPE_BLOCK:
	    { unsigned long size = -1;
	      long numTypes = 0;
	      buf = readTag(&tag,buf);
	      readCompressedInteger(&size,buf);
	      while (size > 0)
		{ char* type = NULL;
		  char* originalBuf = buf;
		  buf = readString(&type,buf);
		  types = (char**)s_realloc(types,(size_t)(sizeof(char*) * (numTypes + 2)));
		  types[numTypes++] = type;
		  types[numTypes] = NULL;
		  size -= (buf - originalBuf);
		}
	    }
	  case DT_Source:
	    buf = readString(&source,buf);
	    break;
	  case DT_Date:
	    buf = readString(&theDate,buf);
	    break;
	  case DT_Headline:
	    buf = readString(&headline,buf);
	    break;
	  case DT_OriginCity:
	    buf = readString(&originCity,buf);
	    break;
	  case DT_StockCodes:
	    buf = readString(&stockCodes,buf);
	    break;
	  case DT_CompanyCodes:
	    buf = readString(&companyCodes,buf);
	    break;
	  case DT_IndustryCodes:
	    buf = readString(&industryCodes,buf);
	    break;
	  default:
	    freeAny(docID);
	    s_free(source);
	    s_free(theDate);
	    s_free(headline);
	    s_free(originCity);
	    s_free(stockCodes);
	    s_free(companyCodes);
	    s_free(industryCodes);
	    REPORT_READ_ERROR(buf);
	    break;
	  }
    }
  	  
  *header = makeWAISDocumentLongHeader(docID,versionNumber,score,bestMatch,
				       docLength,lines,types,source,theDate,headline,
				       originCity,stockCodes,companyCodes,
				       industryCodes);
  return(buf);
}
/*----------------------------------------------------------------------*/

WAISSearchResponse*
makeWAISSearchResponse(seedWordsUsed,
		       docHeaders,
		       shortHeaders,
		       longHeaders,
		       text,
		       headlines,
		       codes,
		       diagnostics)
char* seedWordsUsed;
WAISDocumentHeader** docHeaders;
WAISDocumentShortHeader** shortHeaders;
WAISDocumentLongHeader** longHeaders;
WAISDocumentText** text;
WAISDocumentHeadlines** headlines;
WAISDocumentCodes** codes;
diagnosticRecord** diagnostics;
{
  WAISSearchResponse* response = (WAISSearchResponse*)s_malloc((size_t)sizeof(WAISSearchResponse));
  
  response->SeedWordsUsed = seedWordsUsed;
  response->DocHeaders = docHeaders;
  response->ShortHeaders = shortHeaders;
  response->LongHeaders = longHeaders;
  response->Text = text;
  response->Headlines = headlines;
  response->Codes = codes;
  response->Diagnostics = diagnostics;
  
  return(response);
}

/*----------------------------------------------------------------------*/

void
freeWAISSearchResponse(response)
WAISSearchResponse* response;
{
  void* ptr = NULL;
  long i;

  s_free(response->SeedWordsUsed);

  if (response->DocHeaders != NULL)
    for (i = 0,ptr = (void *)response->DocHeaders[i]; ptr != NULL; ptr = (void *)response->DocHeaders[++i])
      freeWAISDocumentHeader((WAISDocumentHeader*)ptr);
  s_free(response->DocHeaders);
   
  if (response->ShortHeaders != NULL)
    for (i = 0,ptr = (void *)response->ShortHeaders[i]; ptr != NULL; ptr = (void *)response->ShortHeaders[++i])
      freeWAISDocumentShortHeader((WAISDocumentShortHeader*)ptr);
  s_free(response->ShortHeaders);
   
  if (response->LongHeaders != NULL)
    for (i = 0,ptr = (void *)response->LongHeaders[i]; ptr != NULL; ptr = (void *)response->LongHeaders[++i])
      freeWAISDocumentLongHeader((WAISDocumentLongHeader*)ptr);
  s_free(response->LongHeaders);
   
  if (response->Text != NULL)
    for (i = 0,ptr = (void *)response->Text[i]; ptr != NULL; ptr = (void *)response->Text[++i])
      freeWAISDocumentText((WAISDocumentText*)ptr);
  s_free(response->Text);
   
  if (response->Headlines != NULL)
    for (i = 0,ptr = (void *)response->Headlines[i]; ptr != NULL; ptr = (void *)response->Headlines[++i])
      freeWAISDocumentHeadlines((WAISDocumentHeadlines*)ptr);
  s_free(response->Headlines);
   
  if (response->Codes != NULL)
    for (i = 0,ptr = (void *)response->Codes[i]; ptr != NULL; ptr = (void *)response->Codes[++i])
      freeWAISDocumentCodes((WAISDocumentCodes*)ptr);
  s_free(response->Codes);
   
  if (response->Diagnostics != NULL)
    for (i = 0,ptr = (void *)response->Diagnostics[i]; ptr != NULL; ptr = (void *)response->Diagnostics[++i])
      freeDiag((diagnosticRecord*)ptr);
  s_free(response->Diagnostics);
  
  s_free(response);
}

/*----------------------------------------------------------------------*/

char* 
writeSearchResponseInfo(query,buffer,len)
SearchResponseAPDU* query;
char* buffer;
long* len;
{
  unsigned long header_len = userInfoTagSize(DT_UserInformationLength,
					     DefWAISSearchResponseSize);
  char* buf = buffer + header_len;
  WAISSearchResponse* info = (WAISSearchResponse*)query->DatabaseDiagnosticRecords;
  unsigned long size;
  void* header = NULL;
  long i;
  
  RESERVE_SPACE_FOR_WAIS_HEADER(len);
  
  buf = writeString(info->SeedWordsUsed,DT_SeedWordsUsed,buf,len);
  
  
  if (info->DocHeaders != NULL)
    { for (i = 0,header = (void *)info->DocHeaders[i]; header != NULL; header = (void *)info->DocHeaders[++i])
	buf = writeWAISDocumentHeader((WAISDocumentHeader*)header,buf,len);
      }
   
  if (info->ShortHeaders != NULL)
    { for (i = 0,header = (void *)info->ShortHeaders[i]; header != NULL; header = (void *)info->ShortHeaders[++i])
	buf = writeWAISDocumentShortHeader((WAISDocumentShortHeader*)header,buf,len);
      }

  if (info->LongHeaders != NULL)
    { for (i = 0,header = (void *)info->LongHeaders[i]; header != NULL; header = (void *)info->LongHeaders[++i])
	buf = writeWAISDocumentLongHeader((WAISDocumentLongHeader*)header,buf,len);
      }

  if (info->Text != NULL)
    { for (i = 0,header = (void *)info->Text[i]; header != NULL; header = (void *)info->Text[++i])
	buf = writeWAISDocumentText((WAISDocumentText*)header,buf,len);
      }

  if (info->Headlines != NULL)
    { for (i = 0,header = (void *)info->Headlines[i]; header != NULL; header = (void *)info->Headlines[++i])
	buf = writeWAISDocumentHeadlines((WAISDocumentHeadlines*)header,buf,len);
      }

  if (info->Codes != NULL)
    { for (i = 0,header = (void *)info->Codes[i]; header != NULL;header = (void *)info->Codes[++i])
	buf = writeWAISDocumentCodes((WAISDocumentCodes*)header,buf,len);
      }

  if (info->Diagnostics != NULL)
    { for (i = 0, header = (void *)info->Diagnostics[i]; header != NULL; header = (void *)info->Diagnostics[++i])
	buf = writeDiag((diagnosticRecord*)header,buf,len);
      }
   
  
  size = buf - buffer; 
  buf = writeUserInfoHeader(DT_UserInformationLength,size,header_len,buffer,len);
  
  return(buf);
}

/*----------------------------------------------------------------------*/

static void
cleanUpWaisSearchResponse _AP((char* buf,char* seedWordsUsed,
			       WAISDocumentHeader** docHeaders,
			       WAISDocumentShortHeader** shortHeaders,
			       WAISDocumentLongHeader** longHeaders,
			       WAISDocumentText** text,
			       WAISDocumentHeadlines** headlines,
			       WAISDocumentCodes** codes,
			       diagnosticRecord**diags));

static void
cleanUpWaisSearchResponse (buf,seedWordsUsed,docHeaders,shortHeaders,
			   longHeaders,text,headlines,codes,diags)
char* buf;
char* seedWordsUsed;
WAISDocumentHeader** docHeaders;
WAISDocumentShortHeader** shortHeaders;
WAISDocumentLongHeader** longHeaders;
WAISDocumentText** text;
WAISDocumentHeadlines** headlines;
WAISDocumentCodes** codes;
diagnosticRecord** diags;

{
  void* ptr = NULL;
  long i;

  if (buf == NULL)						
   { s_free(seedWordsUsed);				
     if (docHeaders != NULL)				
       for (i = 0,ptr = (void *)docHeaders[i]; ptr != NULL; 
	    ptr = (void *)docHeaders[++i])		
	 freeWAISDocumentHeader((WAISDocumentHeader*)ptr);	
     s_free(docHeaders);				
     if (shortHeaders != NULL)	
       for (i = 0,ptr = (void *)shortHeaders[i]; ptr != NULL;
	    ptr = (void *)shortHeaders[++i])	
	 freeWAISDocumentShortHeader((WAISDocumentShortHeader*)ptr);
     s_free(shortHeaders);						
     if (longHeaders != NULL)				
       for (i = 0,ptr = (void *)longHeaders[i]; ptr != NULL; 
	    ptr = (void *)longHeaders[++i])	
	 freeWAISDocumentLongHeader((WAISDocumentLongHeader*)ptr);
     s_free(longHeaders);				
     if (text != NULL)					
       for (i = 0,ptr = (void *)text[i]; ptr != NULL; ptr = (void *)text[++i])
	 freeWAISDocumentText((WAISDocumentText*)ptr);	
     s_free(text);					
     if (headlines != NULL)					
       for (i = 0,ptr = (void *)headlines[i]; ptr != NULL;
	    ptr = (void *)headlines[++i])		
	 freeWAISDocumentHeadlines((WAISDocumentHeadlines*)ptr);	
     s_free(headlines);						
     if (codes != NULL)				     
       for (i = 0,ptr = (void *)codes[i]; ptr != NULL; 
	    ptr = (void *)codes[++i])				
	 freeWAISDocumentCodes((WAISDocumentCodes*)ptr);	 
     s_free(codes);					
     if (diags != NULL)				      	
       for (i = 0,ptr = (void *)diags[i]; ptr != NULL; 
	    ptr = (void *)diags[++i])	 
	 freeDiag((diagnosticRecord*)ptr);	     
     s_free(diags);
   }
}

/*----------------------------------------------------------------------*/

char*
readSearchResponseInfo(info,buffer)
void** info;
char* buffer;
{
  char* buf = buffer;
  unsigned long size; 
  unsigned long headerSize;
  data_tag tag;
  void* header = NULL;
  WAISDocumentHeader** docHeaders = NULL;
  WAISDocumentShortHeader** shortHeaders = NULL;
  WAISDocumentLongHeader** longHeaders = NULL;
  WAISDocumentText** text = NULL;
  WAISDocumentHeadlines** headlines = NULL;
  WAISDocumentCodes** codes = NULL;
  long numDocHeaders,numLongHeaders,numShortHeaders,numText,numHeadlines;
  long numCodes;
  char* seedWordsUsed = NULL;
  diagnosticRecord** diags = NULL;
  diagnosticRecord* diag = NULL;
  long numDiags = 0;
  
  numDocHeaders = numLongHeaders = numShortHeaders = numText = numHeadlines = numCodes = 0;
  
  buf = readUserInfoHeader(&tag,&size,buf);
  headerSize = buf - buffer;
    
  while (buf < (buffer + size + headerSize))
   { data_tag tag = peekTag(buf);
     switch (tag)
      { case DT_SeedWordsUsed:
      	  buf = readString(&seedWordsUsed,buf);
      	  break;
      	case DT_DatabaseDiagnosticRecords:
      	  if (diags == NULL) 
      	   { diags = (diagnosticRecord**)s_malloc((size_t)sizeof(diagnosticRecord*) * 2);
      	   }
      	  else 
      	   { diags = (diagnosticRecord**)s_realloc((char*)diags,(size_t)(sizeof(diagnosticRecord*) * (numDiags + 2)));
      	   }
      	  buf = readDiag(&diag,buf);
      	  diags[numDiags++] = diag; 
      	  diags[numDiags] = NULL;
      	  break;
      	case DT_DocumentHeaderGroup:
  		  if (docHeaders == NULL) 
  		   { docHeaders = (WAISDocumentHeader**)s_malloc((size_t)sizeof(WAISDocumentHeader*) * 2);
  		   }
  		  else 
  		   { docHeaders = (WAISDocumentHeader**)s_realloc((char*)docHeaders,(size_t)(sizeof(WAISDocumentHeader*) * (numDocHeaders + 2)));
  		   }
  		  buf = readWAISDocumentHeader((WAISDocumentHeader**)&header,buf);
		  cleanUpWaisSearchResponse(buf,seedWordsUsed,docHeaders,shortHeaders,longHeaders,text,headlines,codes,diags);
  		  RETURN_ON_NULL(buf);
  		  docHeaders[numDocHeaders++] = 
		    (WAISDocumentHeader*)header; 
  		  docHeaders[numDocHeaders] = NULL;
  		  break;
  		case DT_DocumentShortHeaderGroup:
  		  if (shortHeaders == NULL) 
  		   { shortHeaders = (WAISDocumentShortHeader**)s_malloc((size_t)sizeof(WAISDocumentShortHeader*) * 2);
  		   }
  		  else 
  		   { shortHeaders = (WAISDocumentShortHeader**)s_realloc((char*)shortHeaders,(size_t)(sizeof(WAISDocumentShortHeader*) * (numShortHeaders + 2)));
  		   }
  		  buf = readWAISDocumentShortHeader((WAISDocumentShortHeader**)&header,buf);
		  cleanUpWaisSearchResponse(buf,seedWordsUsed,docHeaders,shortHeaders,longHeaders,text,headlines,codes,diags);
  		  RETURN_ON_NULL(buf);
  		  shortHeaders[numShortHeaders++] = 
		    (WAISDocumentShortHeader*)header; 
  		  shortHeaders[numShortHeaders] = NULL;
  		  break;
  		case DT_DocumentLongHeaderGroup:
  		  if (longHeaders == NULL) 
  		   { longHeaders = (WAISDocumentLongHeader**)s_malloc((size_t)sizeof(WAISDocumentLongHeader*) * 2);
  		   }
  		  else 
  		   { longHeaders = (WAISDocumentLongHeader**)s_realloc((char*)longHeaders,(size_t)(sizeof(WAISDocumentLongHeader*) * (numLongHeaders + 2)));
  		   }
  		  buf = readWAISDocumentLongHeader((WAISDocumentLongHeader**)&header,buf);
		  cleanUpWaisSearchResponse(buf,seedWordsUsed,docHeaders,shortHeaders,longHeaders,text,headlines,codes,diags);
  		  RETURN_ON_NULL(buf);
  		  longHeaders[numLongHeaders++] = 
		    (WAISDocumentLongHeader*)header; 
  		  longHeaders[numLongHeaders] = NULL;
  		  break;
        case DT_DocumentTextGroup:
  		  if (text == NULL) 
  		   { text = (WAISDocumentText**)s_malloc((size_t)sizeof(WAISDocumentText*) * 2);
  		   }
  		  else 
  		   { text = (WAISDocumentText**)s_realloc((char*)text,(size_t)(sizeof(WAISDocumentText*) * (numText + 2)));
  		   }
  		  buf = readWAISDocumentText((WAISDocumentText**)&header,buf);
		  cleanUpWaisSearchResponse(buf,seedWordsUsed,docHeaders,shortHeaders,longHeaders,text,headlines,codes,diags);
  		  RETURN_ON_NULL(buf);
  		  text[numText++] = 
		    (WAISDocumentText*)header; 
  		  text[numText] = NULL;
  		  break;
  		case DT_DocumentHeadlineGroup:
  		  if (headlines == NULL) 
  		   { headlines = (WAISDocumentHeadlines**)s_malloc((size_t)sizeof(WAISDocumentHeadlines*) * 2);
  		   }
  		  else 
  		   { headlines = (WAISDocumentHeadlines**)s_realloc((char*)headlines,(size_t)(sizeof(WAISDocumentHeadlines*) * (numHeadlines + 2)));
  		   }
  		  buf = readWAISDocumentHeadlines((WAISDocumentHeadlines**)&header,buf);
		  cleanUpWaisSearchResponse(buf,seedWordsUsed,docHeaders,shortHeaders,longHeaders,text,headlines,codes,diags);
  		  RETURN_ON_NULL(buf);
  		  headlines[numHeadlines++] = 
		    (WAISDocumentHeadlines*)header; 
  		  headlines[numHeadlines] = NULL;
  		  break;
  		case DT_DocumentCodeGroup:
  		  if (codes == NULL) 
  		   { codes = (WAISDocumentCodes**)s_malloc((size_t)sizeof(WAISDocumentCodes*) * 2);
  		   }
  		  else 
  		   { codes = (WAISDocumentCodes**)s_realloc((char*)codes,(size_t)(sizeof(WAISDocumentCodes*) * (numCodes + 2)));
  		   }
  		  buf = readWAISDocumentCodes((WAISDocumentCodes**)&header,buf);
		  cleanUpWaisSearchResponse(buf,seedWordsUsed,docHeaders,shortHeaders,longHeaders,text,headlines,codes,diags);
  		  RETURN_ON_NULL(buf);
  		  codes[numCodes++] = 
		    (WAISDocumentCodes*)header; 
  		  codes[numCodes] = NULL;
  		  break;
        default:
          cleanUpWaisSearchResponse(buf,seedWordsUsed,docHeaders,shortHeaders,longHeaders,text,headlines,codes,diags);
          REPORT_READ_ERROR(buf);
          break;
      } /*switch*/
   }/*while*/
  	  
  *info = (void *)makeWAISSearchResponse(seedWordsUsed,docHeaders,shortHeaders,
				 longHeaders,text,headlines,codes,diags);
  return(buf);
}

/*----------------------------------------------------------------------*/

WAISDocumentText*
makeWAISDocumentText(docID,versionNumber,documentText)
any* docID;
long versionNumber;
any* documentText;
{
  WAISDocumentText* docText = (WAISDocumentText*)s_malloc((size_t)sizeof(WAISDocumentText));

  docText->DocumentID = docID;
  docText->VersionNumber = versionNumber;
  docText->DocumentText = documentText;
  
  return(docText);
}

/*----------------------------------------------------------------------*/

void 
freeWAISDocumentText(docText)
WAISDocumentText* docText;
{
  freeAny(docText->DocumentID);
  freeAny(docText->DocumentText);
  s_free(docText);
}

/*----------------------------------------------------------------------*/

char* 
writeWAISDocumentText(docText,buffer,len)
WAISDocumentText* docText;
char* buffer;
long* len;
{
  unsigned long header_len = userInfoTagSize(DT_DocumentTextGroup,
											DefWAISDocTextSize);
  char* buf = buffer + header_len;
  unsigned long size;
  
  RESERVE_SPACE_FOR_WAIS_HEADER(len);

  buf = writeAny(docText->DocumentID,DT_DocumentID,buf,len);
  buf = writeNum(docText->VersionNumber,DT_VersionNumber,buf,len);
  buf = writeAny(docText->DocumentText,DT_DocumentText,buf,len);
  
  
  size = buf - buffer; 
  buf = writeUserInfoHeader(DT_DocumentTextGroup,size,header_len,buffer,len);

  return(buf);
}

/*----------------------------------------------------------------------*/

char* 
readWAISDocumentText(docText,buffer)
WAISDocumentText** docText;
char* buffer;
{
  char* buf = buffer;
  unsigned long size; 
  unsigned long headerSize;
  data_tag tag;
  any *docID,*documentText;
  long versionNumber;
  
  docID = documentText = NULL;
  versionNumber = UNUSED;
  
  buf = readUserInfoHeader(&tag,&size,buf);
  headerSize = buf - buffer;
    
  while (buf < (buffer + size + headerSize))
   { data_tag tag = peekTag(buf);
     switch (tag)
      { case DT_DocumentID:
  		  buf = readAny(&docID,buf);
  		  break;
  		case DT_VersionNumber:
  		  buf = readNum(&versionNumber,buf);
  		  break;
  		case DT_DocumentText:
  		  buf = readAny(&documentText,buf);
  		  break;
        default:
          freeAny(docID);
          freeAny(documentText);
          REPORT_READ_ERROR(buf);
          break;
      }
   }
  	  
  *docText = makeWAISDocumentText(docID,versionNumber,documentText);
  return(buf);
}

/*----------------------------------------------------------------------*/

WAISDocumentHeadlines*
makeWAISDocumentHeadlines(docID,
			  versionNumber,
			  source,
			  theDate,
			  headline,
			  originCity)
any* docID;
long versionNumber;
char* source;
char* theDate;
char* headline;
char* originCity;
{
  WAISDocumentHeadlines* docHeadline =
    (WAISDocumentHeadlines*)s_malloc((size_t)sizeof(WAISDocumentHeadlines));

  docHeadline->DocumentID = docID;
  docHeadline->VersionNumber = versionNumber;
  docHeadline->Source = source;
  docHeadline->Date = theDate;
  docHeadline->Headline = headline;
  docHeadline->OriginCity = originCity;
  
  return(docHeadline);
}

/*----------------------------------------------------------------------*/

void 
freeWAISDocumentHeadlines(docHeadline)
WAISDocumentHeadlines* docHeadline;
{
  freeAny(docHeadline->DocumentID);
  s_free(docHeadline->Source);
  s_free(docHeadline->Date);
  s_free(docHeadline->Headline);
  s_free(docHeadline->OriginCity);
  s_free(docHeadline);
}

/*----------------------------------------------------------------------*/

char* 
writeWAISDocumentHeadlines(docHeadline,buffer,len)
WAISDocumentHeadlines* docHeadline;
char* buffer;
long* len;
{
  unsigned long header_len = userInfoTagSize(DT_DocumentHeadlineGroup,
											DefWAISDocHeadlineSize);
  char* buf = buffer + header_len;
  unsigned long size;
  
  RESERVE_SPACE_FOR_WAIS_HEADER(len);

  buf = writeAny(docHeadline->DocumentID,DT_DocumentID,buf,len);
  buf = writeNum(docHeadline->VersionNumber,DT_VersionNumber,buf,len);
  buf = writeString(docHeadline->Source,DT_Source,buf,len);
  buf = writeString(docHeadline->Date,DT_Date,buf,len);
  buf = writeString(docHeadline->Headline,DT_Headline,buf,len);
  buf = writeString(docHeadline->OriginCity,DT_OriginCity,buf,len);
  
  
  size = buf - buffer; 
  buf = writeUserInfoHeader(DT_DocumentHeadlineGroup,size,header_len,buffer,len);

  return(buf);
}

/*----------------------------------------------------------------------*/

char* 
readWAISDocumentHeadlines(docHeadline,buffer)
WAISDocumentHeadlines** docHeadline;
char* buffer;
{
  char* buf = buffer;
  unsigned long size; 
  unsigned long headerSize;
  data_tag tag;
  any* docID;
  long versionNumber;
  char *source,*theDate,*headline,*originCity;
  
  docID = NULL;
  versionNumber = UNUSED;
  source = theDate = headline = originCity = NULL;
  
  buf = readUserInfoHeader(&tag,&size,buf);
  headerSize = buf - buffer;
    
  while (buf < (buffer + size + headerSize))
   { data_tag tag = peekTag(buf);
     switch (tag)
      { case DT_DocumentID:
  		  buf = readAny(&docID,buf);
  		  break;
  		case DT_VersionNumber:
  		  buf = readNum(&versionNumber,buf);
  		  break;
  		case DT_Source:
  		  buf = readString(&source,buf);
  		  break;
  		case DT_Date:
  		  buf = readString(&theDate,buf);
  		  break;
  		case DT_Headline:
  		  buf = readString(&headline,buf);
  		  break;
  		case DT_OriginCity:
  		  buf = readString(&originCity,buf);
  		  break;
        default:
          freeAny(docID);
          s_free(source);
          s_free(theDate);
          s_free(headline);
          s_free(originCity);
          REPORT_READ_ERROR(buf);
          break;
      }
   }
  	  
  *docHeadline = makeWAISDocumentHeadlines(docID,versionNumber,source,theDate,
  									       headline,originCity);
  return(buf);
}

/*----------------------------------------------------------------------*/

WAISDocumentCodes*
makeWAISDocumentCodes(docID,
		      versionNumber,
		      stockCodes,
		      companyCodes,
		      industryCodes)
any* docID;
long versionNumber;
char* stockCodes;
char* companyCodes;
char* industryCodes;
{
  WAISDocumentCodes* docCodes = (WAISDocumentCodes*)s_malloc((size_t)sizeof(WAISDocumentCodes));

  docCodes->DocumentID = docID;
  docCodes->VersionNumber = versionNumber;
  docCodes->StockCodes = stockCodes;
  docCodes->CompanyCodes = companyCodes;
  docCodes->IndustryCodes = industryCodes;
  
  return(docCodes);
}

/*----------------------------------------------------------------------*/

void 
freeWAISDocumentCodes(docCodes)
WAISDocumentCodes* docCodes;
{
  freeAny(docCodes->DocumentID);
  s_free(docCodes->StockCodes);
  s_free(docCodes->CompanyCodes);
  s_free(docCodes->IndustryCodes);
  s_free(docCodes);
}

/*----------------------------------------------------------------------*/

char* 
writeWAISDocumentCodes(docCodes,buffer,len)
WAISDocumentCodes* docCodes;
char* buffer;
long* len;
{
  unsigned long header_len = userInfoTagSize(DT_DocumentCodeGroup ,
											DefWAISDocCodeSize);
  char* buf = buffer + header_len;
  unsigned long size;
  
  RESERVE_SPACE_FOR_WAIS_HEADER(len);

  buf = writeAny(docCodes->DocumentID,DT_DocumentID,buf,len);
  buf = writeNum(docCodes->VersionNumber,DT_VersionNumber,buf,len);
  buf = writeString(docCodes->StockCodes,DT_StockCodes,buf,len);
  buf = writeString(docCodes->CompanyCodes,DT_CompanyCodes,buf,len);
  buf = writeString(docCodes->IndustryCodes,DT_IndustryCodes,buf,len);
  
  
  size = buf - buffer; 
  buf = writeUserInfoHeader(DT_DocumentCodeGroup,size,header_len,buffer,len);

  return(buf);
}

/*----------------------------------------------------------------------*/

char* 
readWAISDocumentCodes(docCodes,buffer)
WAISDocumentCodes** docCodes;
char* buffer;
{
  char* buf = buffer;
  unsigned long size; 
  unsigned long headerSize;
  data_tag tag;
  any* docID;
  long versionNumber;
  char *stockCodes,*companyCodes,*industryCodes;
  
  docID = NULL;
  versionNumber = UNUSED;
  stockCodes = companyCodes = industryCodes = NULL;
  
  buf = readUserInfoHeader(&tag,&size,buf);
  headerSize = buf - buffer;
    
  while (buf < (buffer + size + headerSize))
   { data_tag tag = peekTag(buf);
     switch (tag)
      { case DT_DocumentID:
  		  buf = readAny(&docID,buf);
  		  break;
  		case DT_VersionNumber:
  		  buf = readNum(&versionNumber,buf);
  		  break;
  		case DT_StockCodes:
  		  buf = readString(&stockCodes,buf);
  		  break;
  		case DT_CompanyCodes:
  		  buf = readString(&companyCodes,buf);
  		  break;
  		case DT_IndustryCodes:
  		  buf = readString(&industryCodes,buf);
  		  break;
        default:
          freeAny(docID);
          s_free(stockCodes);
          s_free(companyCodes);
          s_free(industryCodes);
          REPORT_READ_ERROR(buf);
          break;
      }
   }
  	  
  *docCodes = makeWAISDocumentCodes(docID,versionNumber,stockCodes,
  									companyCodes,industryCodes);
  return(buf);
}

/*----------------------------------------------------------------------*/

char* 
writePresentInfo(present,buffer,len)
PresentAPDU* present;
char* buffer;
long* len;
{
  
  return(buffer);
}

/*----------------------------------------------------------------------*/

char* 
readPresentInfo(info,buffer)
void** info;
char* buffer;
{
  
  *info = NULL;
  return(buffer);
}

/*----------------------------------------------------------------------*/

char* 
writePresentResponseInfo(response,buffer,len)
PresentResponseAPDU* response;
char* buffer;
long* len;
{
  
  return(buffer);
}

/*----------------------------------------------------------------------*/

char* 
readPresentResponseInfo(info,buffer)
void** info;
char* buffer;
{
  
  *info = NULL;
  return(buffer);
}

/*----------------------------------------------------------------------*/




#define	BYTE		"wb"
#define	LINE		"wl"
#define	PARAGRAPH	"wp"
#define DATA_TYPE	"wt"



static query_term** makeWAISQueryTerms _AP((DocObj** docs));
   
static query_term**
makeWAISQueryTerms(docs)
DocObj** docs;

{
  query_term** terms = NULL;
  long numTerms = 0;
  DocObj* doc = NULL;
  long i;

  if (docs == NULL)
    return((query_term**)NULL);

  terms = (query_term**)s_malloc((size_t)(sizeof(query_term*) * 1));
  terms[numTerms] = NULL;

  
  for (i = 0,doc = docs[i]; doc != NULL; doc = docs[++i])
    { any* type = NULL;

      if (doc->Type != NULL)
	type = stringToAny(doc->Type);

      if (doc->ChunkCode == CT_document) 
	{ terms = (query_term**)s_realloc((char*)terms,
					  (size_t)(sizeof(query_term*) * 
						   (numTerms + 3 + 1)));
	  terms[numTerms++] = makeAttributeTerm(SYSTEM_CONTROL_NUMBER,
						EQUAL,IGNORE,IGNORE,
						IGNORE,IGNORE,doc->DocumentID);
	  if (type != NULL)
	   { terms[numTerms++] = makeAttributeTerm(DATA_TYPE,EQUAL,
						   IGNORE,IGNORE,IGNORE,
						   IGNORE,type);
	     terms[numTerms++] = makeOperatorTerm(AND);
           }
	  terms[numTerms] = NULL;
	}
      else			
	{	char chunk_att[ATTRIBUTE_SIZE];
		any* startChunk = NULL;
		any* endChunk = NULL;
 
		terms = (query_term**)s_realloc((char*)terms,
						(size_t)(sizeof(query_term*) * 
							 (numTerms + 7 + 1)));

		switch (doc->ChunkCode)
		  { case CT_byte:
		    case CT_line:
		      { char start[20],end[20];
			(doc->ChunkCode == CT_byte) ?
			  strncpy(chunk_att,BYTE,ATTRIBUTE_SIZE) :
			strncpy(chunk_att,LINE,ATTRIBUTE_SIZE);	
			sprintf(start,"%ld",doc->ChunkStart.Pos);
			startChunk = stringToAny(start);
			sprintf(end,"%ld",doc->ChunkEnd.Pos);
			endChunk = stringToAny(end);
		      }
		      break;
		    case CT_paragraph:
		      strncpy(chunk_att,PARAGRAPH,ATTRIBUTE_SIZE);
		      startChunk = doc->ChunkStart.ID;
		      endChunk = doc->ChunkEnd.ID;
		      break;
		    default:
		      
		      break;
		    }

		terms[numTerms++] = makeAttributeTerm(SYSTEM_CONTROL_NUMBER,
						      EQUAL,IGNORE,IGNORE,
						      IGNORE,
						      IGNORE,doc->DocumentID);
		if (type != NULL)
		 { terms[numTerms++] = makeAttributeTerm(DATA_TYPE,EQUAL,IGNORE,
							 IGNORE,IGNORE,IGNORE,
							 type);
		   terms[numTerms++] = makeOperatorTerm(AND);
		 }
		terms[numTerms++] = makeAttributeTerm(chunk_att,
						      GREATER_THAN_OR_EQUAL,
						      IGNORE,IGNORE,IGNORE, 
						      IGNORE,
						      startChunk);
		terms[numTerms++] = makeOperatorTerm(AND);
		terms[numTerms++] = makeAttributeTerm(chunk_att,LESS_THAN,
						      IGNORE,IGNORE,IGNORE,
						      IGNORE,
						      endChunk);
		terms[numTerms++] = makeOperatorTerm(AND);
		terms[numTerms] = NULL;

		if (doc->ChunkCode == CT_byte || doc->ChunkCode == CT_line)
		  { freeAny(startChunk);
		    freeAny(endChunk);
		  }
	      }
      
      freeAny(type);
      
     if (i != 0) 
	{ terms = (query_term**)s_realloc((char*)terms,
					  (size_t)(sizeof(query_term*) * 
						   (numTerms + 1 + 1)));
	  terms[numTerms++] = makeOperatorTerm(OR);
	  terms[numTerms] = NULL;
	}
    }

  return(terms);
}

/*----------------------------------------------------------------------*/

static DocObj** makeWAISQueryDocs _AP((query_term** terms));

static DocObj** 
makeWAISQueryDocs(terms)
query_term** terms;

{
  query_term* docTerm = NULL;
  query_term* fragmentTerm = NULL;
  DocObj** docs = NULL;
  DocObj* doc = NULL;
  long docNum,termNum;

  docNum = termNum = 0;
  
  docs = (DocObj**)s_malloc((size_t)(sizeof(DocObj*) * 1));
  docs[docNum] = NULL;

  
  while (true)
    {	      
      query_term* typeTerm = NULL;
      char* type = NULL;
      long startTermOffset;

      docTerm = terms[termNum];
     
      if (docTerm == NULL)
	break;			;

      typeTerm = terms[termNum + 1]; 

      if (strcmp(typeTerm->Use,DATA_TYPE) == 0)	
       { startTermOffset = 3;	
	 type = anyToString(typeTerm->Term);
       }
      else 				   	
       { startTermOffset = 1;
	 typeTerm = NULL;
	 type = NULL;
       }

      
      docs = (DocObj**)s_realloc((char*)docs,(size_t)(sizeof(DocObj*) * 
						      (docNum + 1 + 1)));

      
      fragmentTerm = terms[termNum + startTermOffset];
      if (fragmentTerm != NULL && fragmentTerm->TermType == TT_Attribute)
	{			
	  query_term* startTerm = fragmentTerm;
	  query_term* endTerm = terms[termNum + startTermOffset + 2]; 

	  if (strcmp(startTerm->Use,BYTE) == 0) 
	    doc = makeDocObjUsingBytes(duplicateAny(docTerm->Term),
				       type,
				       anyToLong(startTerm->Term),
				       anyToLong(endTerm->Term));
	  else if (strcmp(startTerm->Use,LINE) == 0) 
	    doc = makeDocObjUsingLines(duplicateAny(docTerm->Term),
				       type,
				       anyToLong(startTerm->Term),
				       anyToLong(endTerm->Term));
	  else if (strcmp(startTerm->Use,PARAGRAPH) == 0)
	    
	    doc = makeDocObjUsingParagraphs(duplicateAny(docTerm->Term),
					    type,
					    duplicateAny(startTerm->Term),
					    duplicateAny(endTerm->Term));
	  termNum += (startTermOffset + 4);	
	}
      else			
	{ 
	  doc = makeDocObjUsingWholeDocument(duplicateAny(docTerm->Term),
					     type);
	  termNum += startTermOffset;	
	}
     
      docs[docNum++] = doc;	
	 
      docs[docNum] = NULL;	

	 
      if (terms[termNum] != NULL)
	termNum++; 
      else
	break; 
    }

  return(docs);
}

/*----------------------------------------------------------------------*/

any* 
makeWAISTextQuery(docs)
DocObj** docs;

{
  any *buf = NULL;
  query_term** terms = NULL;
  
  terms = makeWAISQueryTerms(docs);
  buf = writeQuery(terms);
  
  doList((void**)terms,freeTerm);
  s_free(terms);
  
  return(buf);
}

/*----------------------------------------------------------------------*/
#if !defined(IN_RMG) && !defined(PFS_THREADS)
DocObj** 
readWAISTextQuery(buf)
any* buf;

{
  query_term** terms = NULL;
  DocObj** docs = NULL;
  
  terms = readQuery(buf);
  docs = makeWAISQueryDocs(terms);
  
  doList((void**)terms,freeTerm);
  s_free(terms);
  
  return(docs);
}
#endif
/*----------------------------------------------------------------------*/







/*----------------------------------------------------------------------*/

void 
CSTFreeWAISInitResponse(init)
WAISInitResponse* init;

{
  s_free(init);
}

/*----------------------------------------------------------------------*/

void 
CSTFreeWAISSearch(query)
WAISSearch* query;

{ 
  s_free(query);
}

/*----------------------------------------------------------------------*/

void
CSTFreeDocObj(doc)
DocObj* doc;

{ 
    s_free(doc);
}

/*----------------------------------------------------------------------*/

void
CSTFreeWAISDocumentHeader(header)
WAISDocumentHeader* header;
{ 
    s_free(header);
}

/*----------------------------------------------------------------------*/

void
CSTFreeWAISDocumentShortHeader(header)
WAISDocumentShortHeader* header;
{ 
  s_free(header);
}
/*----------------------------------------------------------------------*/

void
CSTFreeWAISDocumentLongHeader(header)
WAISDocumentLongHeader* header;
{
  s_free(header);
}

/*----------------------------------------------------------------------*/

void
CSTFreeWAISSearchResponse(response)
WAISSearchResponse* response;
{ 
  s_free(response);
}

/*----------------------------------------------------------------------*/

void 
CSTFreeWAISDocumentText(docText)
WAISDocumentText* docText;
{ 
  s_free(docText);
}

/*----------------------------------------------------------------------*/

void 
CSTFreeWAISDocumentHeadlines(docHeadline)
WAISDocumentHeadlines* docHeadline;
{ 
  s_free(docHeadline);
}

/*----------------------------------------------------------------------*/

void 
CSTFreeWAISDocumentCodes(docCodes)
WAISDocumentCodes* docCodes;
{
  s_free(docCodes);
}

/*----------------------------------------------------------------------*/

void 
CSTFreeWAISTextQuery(query)
any* query;
{
   freeAny(query);
}

/*----------------------------------------------------------------------*/

