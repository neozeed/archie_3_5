/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*----------------------------------------------------------------------*/

#define _C_utils_Z39_50_

#include <math.h>
#include "zutil.h"
#include <string.h>
#include <pfs_threads.h>

EXTERN_CHARP_DEF(readErrorPosition); 
/* Lets hope these get initialized to NULL or that noone cares*/
#define readErrorPosition p_th_arreadErrorPosition[p__th_self_num()]

/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/

diagnosticRecord* 
makeDiag(surrogate,code,addInfo)
boolean surrogate;
char* code;
char* addInfo;
{
  diagnosticRecord* diag = 
    (diagnosticRecord*)s_malloc((size_t)sizeof(diagnosticRecord));
  
  diag->SURROGATE = surrogate;
  memcpy(diag->DIAG,code,DIAGNOSTIC_CODE_SIZE);
  diag->ADDINFO = s_strdup(addInfo); 

  return(diag);
}

/*----------------------------------------------------------------------*/

void 
freeDiag(diag)
diagnosticRecord* diag;
{ 
  if (diag != NULL)
    { if (diag->ADDINFO != NULL)
	s_free(diag->ADDINFO);
	s_free(diag);
      }
}

/*----------------------------------------------------------------------*/

#define END_OF_RECORD	0x1D

char* 
writeDiag(diag,buffer,len)
diagnosticRecord* diag;
char* buffer;
long* len;

{
  char* buf = buffer;
  long  length;
  
  if (diag == NULL)		
    return(buf);

  buf = writeTag(DT_DatabaseDiagnosticRecords,buf,len);
  CHECK_FOR_SPACE_LEFT(0L,len);
  
  length = 3; 
  if (diag->ADDINFO != NULL)
    length += strlen(diag->ADDINFO);
    
  if (length >= 0xFFFF )	
    { length = 0xFFFF - 1;
      diag->ADDINFO[0xFFFF - 3 - 1] = '\0';
    }
   
  buf = writeBinaryInteger(length,2L,buf,len);

  CHECK_FOR_SPACE_LEFT(1L,len);
  buf[0] = diag->DIAG[0]; 
  buf++;
  
  CHECK_FOR_SPACE_LEFT(1L,len);
  buf[0] = diag->DIAG[1];
  buf++;
  
  if (length > 3)
    { CHECK_FOR_SPACE_LEFT(3L,len);
      memcpy(buf,diag->ADDINFO,(size_t)length - 3);
      buf += length - 3;
    }
   
  CHECK_FOR_SPACE_LEFT(1L,len);
  buf[0] = diag->SURROGATE;
  buf++;
  
  CHECK_FOR_SPACE_LEFT(1L,len);
  buf[0] = END_OF_RECORD;
  buf++;

  return(buf);
}

/*----------------------------------------------------------------------*/

char* 
readDiag(diag,buffer)	/*!!ALLOCS - diag */
diagnosticRecord** diag;
char* buffer;
{
  char* buf = buffer;
  diagnosticRecord* d 
    = (diagnosticRecord*)s_malloc((size_t)sizeof(diagnosticRecord));
  data_tag tag;
  long len;
  
  buf = readTag(&tag,buf);
  
  buf = readBinaryInteger(&len,2L,buf);
  
  d->DIAG[0] = buf[0];
  d->DIAG[1] = buf[1];
  d->DIAG[2] = '\0';
    
  if (len > 3)
    { d->ADDINFO = (char*)s_malloc((size_t)(len - 3 + 1));
      memcpy(d->ADDINFO,(char*)(buf + 2),(size_t)(len - 3));
      d->ADDINFO[len - 3] = '\0';
    }
  else
    d->ADDINFO = NULL;
    
  d->SURROGATE = buf[len - 1];
  
  *diag = d;

  return(buf + len + 1);
}

/*----------------------------------------------------------------------*/

#define continueBit	0x80
#define dataMask	0x7F
#define dataBits	7

char*
writeCompressedInteger(num,buf,len)
unsigned long num;
char* buf;
long* len;

{
  char aByte;
  long i;
  unsigned long size;
  
  size = writtenCompressedIntSize(num);
  CHECK_FOR_SPACE_LEFT(size,len);
  
  for (i = size - 1; i >= 0; i--)
    { aByte = num & dataMask;
      if (i != (size-1))	
	aByte = (char)(aByte | continueBit);
      buf[i] = aByte;
      num = num >> dataBits;	
    }
   
  return(buf + size);
} 

/*----------------------------------------------------------------------*/

char*
readCompressedInteger(num,buf)
unsigned long *num;
char* buf;

{
  long i = 0;
  unsigned char aByte;

  *num = 0;
  
  do 
    { aByte = buf[i++];
      *num = *num << dataBits;
      *num += (aByte & dataMask);
    }
  while (aByte & continueBit);

  return(buf + i);
} 

/*----------------------------------------------------------------------*/

#define pad	128 

char*
writeCompressedIntWithPadding(num,size,buffer,len)
unsigned long num;
unsigned long size;
char* buffer;
long* len;

{
  char* buf = buffer;
  unsigned long needed,padding;
  long i;
    
  CHECK_FOR_SPACE_LEFT(size,len);
  
  needed = writtenCompressedIntSize(num);
  padding = size - needed;
  i = padding - 1;

  for (i = padding - 1;i >= 0;i--)
    { buf[i] = pad;
    }
  
  buf = writeCompressedInteger(num,buf + padding,len);
  
  return(buf);
} 

/*----------------------------------------------------------------------*/

unsigned long
writtenCompressedIntSize(num)
unsigned long num;

{
  if (num < CompressedInt1Byte) 
    return(1);
  else if (num < CompressedInt2Byte) 
    return(2);
  else if (num < CompressedInt3Byte)
    return(3);
  else
    return(4);    
}

/*----------------------------------------------------------------------*/

char*
writeTag(tag,buf,len)
data_tag tag;
char* buf;
long* len;

{ 
  return(writeCompressedInteger(tag,buf,len));
} 

/*----------------------------------------------------------------------*/

char*
readTag(tag,buf)
data_tag* tag;
char* buf;

{ 
  return(readCompressedInteger(tag,buf));
} 

/*----------------------------------------------------------------------*/

unsigned long 
writtenTagSize(tag)
data_tag tag;
{ 
  return(writtenCompressedIntSize(tag));
}

/*----------------------------------------------------------------------*/

data_tag
peekTag(buf)
char* buf;

{
  data_tag tag;
  readTag(&tag,buf);
  return(tag);
} 

/*----------------------------------------------------------------------*/

any* 
makeAny(size,data)
unsigned long size;
char* data;
{
  any* a = (any*)s_malloc((size_t)sizeof(any));
  a->size = size;
  a->bytes = data;
  return(a);
}

/*----------------------------------------------------------------------*/

void
freeAny(a)
any* a;

{
  if (a != NULL)
    { if (a->bytes != NULL)
	s_free(a->bytes);
      s_free(a);
    }
}

/*----------------------------------------------------------------------*/

any* 
duplicateAny(a)
any* a;
{
  any* copy = NULL;

  if (a == NULL)
    return(NULL);

  copy = (any*)s_malloc((size_t)sizeof(any));
  copy->size = a->size;
  if (a->bytes == NULL)
    copy->bytes = NULL;
  else
    { copy->bytes = (char*)s_malloc((size_t)copy->size);
      memcpy(copy->bytes,a->bytes,(size_t)copy->size);
    }
  return(copy);
}

/*----------------------------------------------------------------------*/

char* 
writeAny(a,tag,buffer,len)
any* a;
data_tag tag;
char* buffer;
long* len;

{
  char* buf = buffer;

  if (a == NULL)		
    return(buf);
  
  
  buf = writeTag(tag,buf,len);
  buf = writeCompressedInteger(a->size,buf,len);

  
  CHECK_FOR_SPACE_LEFT(a->size,len);
  memcpy(buf,a->bytes,(size_t)a->size);

  return(buf+a->size);
}

/*----------------------------------------------------------------------*/

char* 
readAny(anAny,buffer)	/*!!ALLOCS at anAny */
any** anAny;
char* buffer;

{
  char* buf = buffer;
  any* a = (any*)s_malloc((size_t)sizeof(any));
  data_tag tag;
  
  buf = readTag(&tag,buf);
  buf = readCompressedInteger(&a->size,buf);
  
  a->bytes = (char*)s_malloc((size_t)a->size);
  memcpy(a->bytes,buf,(size_t)a->size);
  *anAny = a;
  return(buf+a->size);
}

/*----------------------------------------------------------------------*/

unsigned long 
writtenAnySize(tag,a)
data_tag tag;
any* a;
{
  unsigned long size;

  if (a == NULL)
    return(0);

  size = writtenTagSize(tag);
  size += writtenCompressedIntSize(a->size);
  size += a->size;
  return(size);
}

/*----------------------------------------------------------------------*/

any*
stringToAny(s)
char* s;
{
  any* a = NULL;
  
  if (s == NULL)
    return(NULL);
    
  a = (any*)s_malloc((size_t)sizeof(any));
  a->size = strlen(s);
  a->bytes = (char*)s_malloc((size_t)a->size);
  memcpy(a->bytes,s,(size_t)a->size);
  return(a);
}

/*----------------------------------------------------------------------*/

char*
anyToString(a)
any* a;
{
  char* s = NULL;
  
  if (a == NULL)
    return(NULL);
    
  s = s_malloc((size_t)(a->size + 1));
  memcpy(s,a->bytes,(size_t)a->size);
  s[a->size] = '\0';
  return(s);
}

/*----------------------------------------------------------------------*/

char* 
writeString(s,tag,buffer,len)
char* s;
data_tag tag;
char* buffer;
long* len;

{
  char* buf = buffer;
  any* data = NULL;
  if (s == NULL)
    return(buffer);		
  data = (any*)s_malloc((size_t)sizeof(any)); 
  data->size = strlen(s);
  data->bytes = s;		
  buf = writeAny(data,tag,buf,len);
  s_free(data);			
  return(buf);
}

/*----------------------------------------------------------------------*/

char* 
readString(s ,buffer)
char** s ;
char* buffer;

{
  any* data = NULL;
  char* buf = readAny(&data,buffer);
  *s = anyToString(data);
  freeAny(data);
  return(buf);
}

/*----------------------------------------------------------------------*/

unsigned long 
writtenStringSize(tag,s)
data_tag tag;
char* s;
{
  unsigned long size;

  if (s == NULL)
   return(0);

  size = writtenTagSize(tag);
  size += writtenCompressedIntSize(size);
  size += strlen(s);
  return(size);
}

/*----------------------------------------------------------------------*/

any* 
longToAny(num)
long num;

{
  char s[40];

  sprintf(s,"%ld",num);
  
  return(stringToAny(s));
}

/*----------------------------------------------------------------------*/

long
anyToLong(a)
any* a;

{
  long num;
  char* str = NULL;
  str = anyToString(a);
  sscanf(str,"%ld",&num);	
  s_free(str);
  return(num);
}
 
/*----------------------------------------------------------------------*/

#define bitsPerByte	8

bit_map*
makeBitMap(unsigned long numBits,...)

{
  va_list ap;
  long i,j;
  bit_map* bm = NULL;

  va_start(ap,numBits);
  
  bm = (bit_map*)s_malloc((size_t)sizeof(bit_map));
  bm->size = (unsigned long)ceil((double)numBits / bitsPerByte); 
  bm->bytes = (char*)s_malloc((size_t)bm->size);
  
  
  for (i = 0; i < bm->size; i++) 
    { char aByte = 0;
      for (j = 0; j < bitsPerByte; j++) 
	{ if ((i * bitsPerByte + j) < numBits)
	    { boolean bit = false;
	      bit = (boolean)va_arg(ap,boolean); 
	      if (bit)
	        { aByte = aByte | (1 << (bitsPerByte - j - 1));
	        }
	    }
	  }
      bm->bytes[i] = aByte;
    }

  va_end(ap);
  return(bm);
}


/*----------------------------------------------------------------------*/

void
freeBitMap(bm)
bit_map* bm;

{
  s_free(bm->bytes);
  s_free(bm);
}

/*----------------------------------------------------------------------*/



boolean
bitAtPos(pos,bm)
long pos;
bit_map* bm;
{
  if (pos > bm->size*bitsPerByte)
    return false;
  else
    return((bm->bytes[(pos / bitsPerByte)] & 
	    (0x80>>(pos % bitsPerByte))) ?
	   true : false);
}

/*----------------------------------------------------------------------*/

char*
writeBitMap(bm,tag,buffer,len)
bit_map* bm;
data_tag tag;
char* buffer;
long* len;

{ 
  return(writeAny((any*)bm,tag,buffer,len));
}

/*----------------------------------------------------------------------*/

char*
readBitMap(bm,buffer)
bit_map** bm;
char* buffer;

{ 
  return(readAny((any**)bm,buffer));
}

/*----------------------------------------------------------------------*/

char* 
writeByte(aByte,buf,len)
unsigned long aByte;
char* buf;
long* len;
{
  CHECK_FOR_SPACE_LEFT(1L,len);
  buf[0] = aByte & 0xFF; 
  return(buf + 1);
}

/*----------------------------------------------------------------------*/

char* 
readByte(aByte,buf)
unsigned char* aByte;
char* buf;
{
  *aByte = buf[0];
  return(buf + 1);
}

/*----------------------------------------------------------------------*/

char* 
writeBoolean(flag,buf,len)
boolean flag;
char* buf;
long* len;
{
  return(writeByte(flag,buf,len));
}

/*----------------------------------------------------------------------*/

char* 
readBoolean(flag,buffer)
boolean* flag;
char* buffer;
{
  unsigned char aByte;
  char* buf = readByte(&aByte,buffer);
  *flag = (aByte == true) ? true : false;
  return(buf);
}

/*----------------------------------------------------------------------*/

char*
writePDUType(pduType,buf,len)
pdu_type pduType;
char* buf;
long* len;

{
  return(writeBinaryInteger((long)pduType,(unsigned long)1,buf,len));
} 

/*----------------------------------------------------------------------*/

char*
readPDUType(pduType,buf)
pdu_type* pduType;
char* buf;

{
  return(readBinaryInteger((long*)pduType,(unsigned long)1,buf));
} 

/*----------------------------------------------------------------------*/

pdu_type
peekPDUType(buf)
char* buf;

{
  pdu_type pdu;
  readPDUType(&pdu,buf + HEADER_LEN);
  return(pdu);
}

/*----------------------------------------------------------------------*/

#define BINARY_INTEGER_BYTES	sizeof(long) 
/* Writes an integer num into the buf (which has len bytes left) in a width
   of size, returns buff, and updates len */
char*
writeBinaryInteger(num,size,buf,len)
long num;
unsigned long size;
char* buf;
long* len;

{
  long i;
  char aByte;

  if (size < 1 || size > BINARY_INTEGER_BYTES)
    return(NULL);		

  CHECK_FOR_SPACE_LEFT(size,len);

  for (i = size - 1; i >= 0; i--)
    { aByte = (char)(num & 255);
      buf[i] = aByte;
      num = num >> bitsPerByte; 
    }

  return(buf + size);
}

/*----------------------------------------------------------------------*/

char*
readBinaryInteger(num,size,buf)
long* num;
unsigned long size;
char* buf;

{
  long i;
  unsigned char aByte;
  if (size < 1 || size > BINARY_INTEGER_BYTES)
    return(buf);		
  *num = 0;
  for (i = 0; i < size; i++)
    { aByte = buf[i];
      *num = *num << bitsPerByte;
      *num += aByte;
    }
  return(buf + size);
}

/*----------------------------------------------------------------------*/

unsigned long 
writtenCompressedBinIntSize(num)
long num;

{
  if (num < 0L)
    return(4);
  else if (num < 256L)		
    return(1);
  else if (num < 65536L)	
    return(2);
  else if (num < 16777216L)	
    return(3);
  else
    return(4);
}
 
/*----------------------------------------------------------------------*/

char*
writeNum(num,tag,buffer,len)
long num;
data_tag tag;
char* buffer;
long* len;

{
  char* buf = buffer;
  long size = writtenCompressedBinIntSize(num);
  
  if (num == UNUSED)
    return(buffer);
    
  buf = writeTag(tag,buf,len);
  buf = writeCompressedInteger(size,buf,len); 
  buf = writeBinaryInteger(num,(unsigned long)size,buf,len); 
  return(buf);
}

/*----------------------------------------------------------------------*/

char*
readNum(num,buffer)
long* num;
char* buffer;

{
  char* buf = buffer;
  data_tag tag;
  unsigned long size;
  unsigned long val;
  
  buf = readTag(&tag,buf);
  buf = readCompressedInteger(&val,buf);
  size = (unsigned long)val;
  buf = readBinaryInteger(num,size,buf);
  return(buf);
}

/*----------------------------------------------------------------------*/

unsigned long 
writtenNumSize(tag,num)
data_tag tag;
long num;
{
  long dataSize = writtenCompressedBinIntSize(num);
  long size;
  
  size = writtenTagSize(tag); 
  size += writtenCompressedIntSize(dataSize); 
  size += dataSize; 
  
  return(size);
}

/*----------------------------------------------------------------------*/

typedef void (voidfunc)();

void
doList(theList,func)
void** theList;
voidfunc *func;

{
  register long i;
  register void* ptr = NULL;
  if (theList == NULL)
    return;
  for (i = 0,ptr = theList[i]; ptr != NULL; ptr = theList[++i])
    (*func)(ptr);
}

/*----------------------------------------------------------------------*/

char* 
writeProtocolVersion(buf,len)
char* buf;
long* len;

{
  static bit_map* version = NULL;

  if (version == NULL)
	/* While this only NEEDS doing once, dont really care if done twice
	   simultaneously by different threads*/
   { version = makeBitMap((unsigned long)1,true); 
   }
    
  return(writeBitMap(version,DT_ProtocolVersion,buf,len));
}

/*----------------------------------------------------------------------*/

char*
defaultImplementationID()
{
  static char	ImplementationID[] = "WAIS Inc";
  return(ImplementationID);
}

/*----------------------------------------------------------------------*/

char*
defaultImplementationName()
{
  static char ImplementationName[] = "Wide Area Information Servers Inc Z39.50";
  return(ImplementationName);
}

/*----------------------------------------------------------------------*/

char*
defaultImplementationVersion()
{
  static char	ImplementationVersion[] = "2.0A";
  return(ImplementationVersion);
}

/*----------------------------------------------------------------------*/

