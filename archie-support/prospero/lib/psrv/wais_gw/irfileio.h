/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*---------------------------------------------------------------------------*/

#ifndef IRCFILEIO_H
#define IRCFILEIO_H

#include "cdialect.h"
#include "futil.h"
#include "zprot.h"

#define MAX_SYMBOL_SIZE       (255L)
#define END_OF_STRUCT_OR_LIST   (6L)

long SkipObject(FILE* file);
long ReadLong(FILE* file,long* num);
long ReadDouble(FILE* file,double* num);
long ReadSymbol(char* string, FILE* file, long string_size);
long ReadString(char* string, FILE* file, long string_size);
long CheckStartOfStruct(char* name, FILE* file);
long ReadAny(any* destination, FILE* file);
long ReadTM(struct tm* /* time */, FILE* file);
long Read8BitArray(char* destination, FILE* file, long /* length */);
long ReadEndOfListOrStruct(FILE* file);
long ReadStartOfStruct(char* name, FILE* file);
boolean ReadStartOfList(FILE* file);
boolean ReadEndOfList(FILE* file);
boolean	readAbsoluteTime(struct tm* /* time */,FILE* file);
long WriteStartOfStruct(char* name, FILE* file);
long WriteEndOfStruct(FILE* file);
long WriteSymbol(char* name, FILE* file);
long WriteString(char* string, FILE* file);
long WriteNewline(FILE* file);
long WriteLong(long number, FILE* file);
long WriteDouble(double number, FILE* file);
long WriteAny(any* value, FILE* file);
long Write8BitArray(long /* length */, char* array, FILE* file);
long WriteTM(struct tm* /* time */, FILE* file);
long WriteStartOfList(FILE* file);
long WriteEndOfList(FILE* file);
boolean	writeAbsoluteTime(struct tm* /* time */,FILE* file); 

#endif 
