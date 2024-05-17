/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*---------------------------------------------------------------------------*/

/********************************************************
 *  Writing and reading structures to files.		*
 *							*
 *  These use the Lisp printer format with the		*
 *  lisp naming conventions.  You ask: "Why would	*
 *  you want to use that?".  Well, we need an		*
 *  easily readable data syntax that can handle		*
 *  a large number of different data types.		*
 *  Further, we need it to be tagged so that		*
 *  run time tagged languages can read it and 		*
 *  it is flexible.  We need one that supports		*
 *  optional fields so that the format can 		*
 *  grow backcompatibly.  And (the kicker),		*
 *  it must be read from many languages since		*
 *  user interfaces may be written in anything		*
 *  from smalltalk to hypercard.			*
 * 							*
 *  -brewster 5/10/90					*
 ********************************************************/

#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pfs.h>
#include <perrno.h>

#include "irfileio.h"
#include "cutil.h"

#define INDENT_SPACES (2L)
#define MAX_INDENT    (40L)
#ifdef NOTUSEDANDNOTTHREADSAFE
static long indent_level;  
#endif /*NOTUSEDANDNOTTHREADSAFE

#define RETURN(val)	{retval = val; goto cleanup; }

/**********************/

/**********************/


#ifdef NOTUSEDANDNOTTHREADSAFE
static void indent(FILE* file);

static void indent(file)
FILE* file;

{
  long i;
  for(i = 0; i <= MIN(MAX_INDENT, MAX(0L, indent_level * INDENT_SPACES)); i++){
    putc(' ', file);
  }
}

long WriteStartOfList(file)
FILE* file;
{
  indent_level++;
  return(fprintf(file, " ( "));
}

long WriteEndOfList(file)
FILE* file;
{
  indent_level--;
  return(fprintf(file, " ) "));
}

long WriteStartOfStruct(name,file)
char* name;
FILE* file;
{
  indent_level++;
  return(fprintf(file, " (:%s ", name));
}

long WriteEndOfStruct(file)
FILE* file;
{
  indent_level--;
  return(fprintf(file, " ) "));
}

long WriteSymbol(name,file)
char* name;
FILE* file;
{
  return(fprintf(file, " %s ", name));
}

long WriteNewline(file)
FILE* file;
{
  long return_value = fprintf(file, "\n");
  indent(file);
  return(return_value);
}

long WriteLong(number,file)
long number;
FILE* file;
{
  return(fprintf(file, " %ld ", number));  
}

long WriteDouble(number,file)
double number;
FILE* file;
{
  return(fprintf(file, " %f ", number));
}

long WriteString(string,file)
char* string;
FILE* file;
{
  long i;
  putc('\"', file);
  for(i = 0; i < strlen(string); i++){
    if(string[i] == '\\' || string[i] == '\"')
      putc('\\', file);		
    putc(string[i], file);
  }
  putc('\"', file);
  return(1);
}

long WriteAny(value,file)
any* value;
FILE* file;
{
  WriteStartOfStruct("any", file);
  WriteSymbol(":size", file); WriteLong(value->size, file);
  WriteSymbol(":bytes", file);
  Write8BitArray(value->size, value->bytes, file);
  return(WriteEndOfStruct(file));
}

long Write8BitArray(length,array,file)
long length;
char* array;
FILE* file;
{
  long i;
  fprintf(file, " #( ");
  for(i=0; i<length; i++){
    WriteLong((long)array[i], file);
  }
  return(fprintf(file, " ) "));
}


long WriteTM(atime,file)
struct tm* atime;
FILE* file;
{
  WriteStartOfStruct("tm", file);
  WriteSymbol(":tm-sec", file); WriteLong(atime->tm_sec, file);
  WriteSymbol(":tm-min", file); WriteLong(atime->tm_min, file);
  WriteSymbol(":tm-hour", file); WriteLong(atime->tm_hour, file);
  WriteSymbol(":tm-mday", file); WriteLong(atime->tm_mday, file);
  WriteSymbol(":tm-mon", file); WriteLong(atime->tm_mon, file);
  WriteNewline(file);
  WriteSymbol(":tm-year", file); WriteLong(atime->tm_year, file);
  WriteSymbol(":tm-wday", file); WriteLong(atime->tm_wday, file);
  WriteNewline(file);
  WriteSymbol(":tm-yday", file); WriteLong(atime->tm_yday, file);
  WriteSymbol(":tm-isdst", file); WriteLong(atime->tm_isdst, file);
  WriteEndOfStruct(file);
  return(WriteNewline(file));
}

boolean  
writeAbsoluteTime(atime,file)
struct tm* atime;
FILE* file;
{
  WriteStartOfStruct("absolute-time",file);
  WriteNewline(file);
  WriteSymbol(":year",file); WriteLong((long)atime->tm_year,file);
  WriteNewline(file);
  WriteSymbol(":month",file); WriteLong((long)atime->tm_mon,file);
  WriteNewline(file);
  WriteSymbol(":mday",file); WriteLong((long)atime->tm_mday,file);
  WriteNewline(file);
  WriteSymbol(":hour",file); WriteLong((long)atime->tm_hour,file);
  WriteNewline(file);
  WriteSymbol(":minute",file); WriteLong((long)atime->tm_min,file);
  WriteNewline(file);
  WriteSymbol(":second",file); WriteLong((long)atime->tm_sec,file);
  WriteNewline(file);
  return(WriteEndOfStruct(file));
}
#endif /*NOTUSEDANDNOTTHREADSAFE*/



/************************/

/************************/



#define BEFORE (1L)
#define DURING (2L)
#define HASH   (3L)
#define S      (4L)
#define QUOTE  (5L)

#if !defined(IN_RMG)
boolean ReadStartOfList(file)
FILE* file;
{
  long ch;
  while(TRUE){
    ch = getc(file);
    if(ch == '(') 
      return(TRUE);
    if(!isspace(ch)){
      
      if(ch == 'N' || ch == 'n'){
	ch = getc(file);
	if(ch == 'I' || ch == 'i'){
	  ch = getc(file);
	  if(ch == 'L' || ch == 'l'){
	    ungetc(')', file);
	    return(TRUE);
	  }
	}
      }
      return(FALSE); 
    }
  }
}


boolean ReadEndOfList(file)
FILE* file;
{
  long ch;
  while(TRUE){
    ch = getc(file);
    if(ch == ')') 
      return(TRUE);
    if(!isspace(ch)) 
      return(FALSE);
  }
}
#endif /*!IN_RMG*/

#define STRING_ESC '\\'

long 
SkipObject(file)
FILE* file;

{
  long ch;

  while (true)	
    { ch = getc(file);
      if (ch == EOF)
	return (EOF); 
      else
	{ if (isspace(ch))
	    continue; 
	  else if (ch == '"') 
	    { long escapeCount = 0;
	      while (true)
		{ ch = getc(file);
		  if (ch == EOF)
		    return (EOF);
		  else
		    { if (ch == STRING_ESC)
			{ escapeCount++;
			  escapeCount = escapeCount % 2;
			}
		      if (ch == '"' && escapeCount == 0)
			break; 
                    }
		}
	      break; 
            }
	  else if ((isdigit(ch) || ch == '-' || ch == '.') || 
		   (ch == ':')) 
	    { while (!isspace(ch)) 
		{ ch = getc(file);
		  if (ch == EOF)
		    return(EOF);
		}
	      break; 
	    }
	  else if ((ch == '#') || 
		   (ch == '(')) 
	    { long parenCount = 1;
	      if (ch == '#')	
		ch = getc(file); 
	      while (parenCount > 0)
		{ ch = getc(file);
		  if (ch == EOF)
		    return(EOF);
		  else if (ch == '"')
		    { 
		      ungetc(ch,file);
		      SkipObject(file);
		    }
		  else if (ch == '(') 	
		    parenCount++;
		  else if (ch == ')') 
		    parenCount--;
		}
	      break; 
	    }
	}
    }

  return(true);
}

long ReadLong(file,answer)
FILE* file;
long* answer;

{
  long ch;
  long state = BEFORE;
  boolean isNegative = false;
  long count = 0;
  
  *answer = 0;
  
  while(TRUE){
    ch = getc(file);
    if (ch == EOF){
      break;			
    }
    else if (isdigit(ch)){
      if(state == BEFORE){
	state = DURING;
      }
      count++;
      if(count == 12){
	
	return(false);
      }
      *answer = *answer * 10 + (ch - '0');
    }
    else if (ch == '-') {
      if (isNegative)
	
	return(false);
      if (state == BEFORE) {
	
	isNegative = true;
	state = DURING;
      }
      else {
	ungetc(ch,file);
	break;			
      }
    }
    else if(ch == ')' && (state == DURING)){
      ungetc(ch, file);
      return(true);		
    }
    else if(!isspace(ch)){
      
      return(false);
    }
    
    else if(state == DURING){
      ungetc(ch, file);
      break;			
    }
    
  }
  
  if (isNegative)
    *answer *= -1;
  return(true);
}

long ReadDouble(file,answer)
FILE* file;
double* answer;
{
  
  long ch;
  long state = BEFORE;
  long count = 0;
  long decimal_count = 0;
  
  *answer = 0.0;
  
  while(TRUE){	
    ch = getc(file);
    if (ch == EOF){
      return(true);
    }
    else if (ch == '.'){
      decimal_count ++;
    }
    else if (isdigit(ch)){
      if(state == BEFORE){
	state = DURING;
      }
      count++;
      if(count == 12){
	
	return(false);
      }
      if (decimal_count == 0){
	*answer = *answer * 10 + (ch - '0');
      }
      else{			
	double fraction = (ch - '0');
	long internal_count;
	for(internal_count = 0; internal_count < decimal_count; 
	    internal_count++){
	  fraction = fraction / 10.0;
	}
	*answer = *answer + fraction;
	decimal_count++;
      }
    }
    else if(!isspace(ch)){
      
      return(false);
    }
    
    else if(state == DURING){
      ungetc(ch, file);
      return(true);		
    }
    
  }
}

static boolean issymbolchar(long ch);

static 
boolean issymbolchar(ch)
long ch;

{
  return(!( isspace(ch) || ch == ')' || ch == '(' || ch == EOF));
}


long ReadSymbol(string,file,string_size)
char* string;
FILE* file;
long string_size;
{
  long ch;
  long state = BEFORE;
  long position = 0;
  
  while(TRUE){
    ch = getc(file);
    if((state == BEFORE) && (ch == ')'))
      return(END_OF_STRUCT_OR_LIST);
    if(issymbolchar((long)ch)){	
      if(state == BEFORE)
	state = DURING;
      string[position] = ch;
      position++;
      if(position >= string_size){
	string[string_size - 1] = '\0';
	return(FALSE);
      }
    }
    
    else if((state == DURING) || ch == EOF){
      if(ch != EOF) ungetc(ch, file);
      string[position] = '\0';
      return(TRUE);		
    }
    
  }
}
#if !defined(IN_RMG)
long ReadEndOfListOrStruct(file)
FILE* file;
{
  long ch;
  while(TRUE){
    ch = getc(file);
    if (EOF == ch) 
      return(FALSE);
    else if(')' == ch) 
      return(TRUE);
    else if(!isspace(ch)) 
      return(FALSE);
  }
}
#endif

long ReadString(string,file,string_size)
char* string;
FILE* file;
long string_size;
{
  long ch;
  long state = BEFORE;
  long position = 0;
  string[0] = '\0';  
  
  while(TRUE){
    ch = getc(file);
    if((state == BEFORE) && (ch == '\"'))
      state = DURING;
    else if (EOF == ch){
      string[position] = '\0';
      return(FALSE);
    }
    else if ((state == BEFORE) && (ch == ')'))
      return(END_OF_STRUCT_OR_LIST);
    else if ((state == DURING) && (ch == '\\'))
      state = QUOTE; 
    else if ((state == DURING) && (ch == '"')){	
      string[position] = '\0';
      return(TRUE);
    }
    else if ((state == QUOTE) || (state == DURING)){
			if(state == QUOTE)
				state = DURING;
			string[position] = ch;
			position++;
			if(position >= string_size){
				string[string_size - 1] = '\0';
				return(FALSE);
			}
		}
		
	}
}

long ReadStartOfStruct(name,file)
char* name;
FILE* file;
{
  int ch;
  long state = BEFORE;
	
  name[0] = '\0';
	
  while(TRUE){
    ch = getc(file);
    if((state == BEFORE) && (ch == '#'))
      state = HASH;
    if((state == BEFORE) && (ch == '('))
      state = DURING;
    else if((state == BEFORE) && (ch == ')'))
      return(END_OF_STRUCT_OR_LIST);
    else if((state == BEFORE) && !isspace(ch))
      return(FALSE);		
    else if(state == HASH){
      if (ch == 's')
	state = S;
      else{
	fprintf(stderr,"Expected an 's' but got an %c\n", ch);
	return(FALSE);
      }
    }
    else if(state == S){
      if (ch == '(')
	state = DURING;
      else{
	fprintf(stderr,"Expected an '(' but got an an %c\n",ch);
	return(FALSE);
      }
    }
    else if(state == DURING){
      return(ReadSymbol(name, file, MAX_SYMBOL_SIZE));
    }
  }
}

long CheckStartOfStruct(name,file)
char* name;
FILE* file;
{
  char temp_string[MAX_SYMBOL_SIZE];
  long result = ReadStartOfStruct(temp_string, file);
  if(result == END_OF_STRUCT_OR_LIST)
    return(END_OF_STRUCT_OR_LIST);
  else if(result == FALSE)
    return(FALSE);
  else if(0 == strcmp(temp_string, name))
    return(TRUE);
  else 
    return(FALSE);
}

#if !defined(IN_RMG)

long ReadAny(destination,file)
any* destination;
FILE* file;
{
  char temp_string[MAX_SYMBOL_SIZE];
  long retval;
	
  destination->size = 0; 
  if(FALSE == CheckStartOfStruct("any", file)){
    fprintf(stderr,"An 'any' structure was not read from the disk");
    return(FALSE);
  }
	
  while(TRUE){
    long check_result;
    check_result = ReadSymbol(temp_string, file, MAX_SYMBOL_SIZE);
    if(FALSE == check_result) 
      return(FALSE);
    if(END_OF_STRUCT_OR_LIST == check_result) 
      RETURN(TRUE);
		
    if(0 == strcmp(temp_string, ":size")) {
      long	size;
      ReadLong(file,&size);
      destination->size = (unsigned long)size;
    }
    else if(0 == strcmp(temp_string, ":bytes")){
      long result;
      
      destination->bytes = (char*)s_malloc(destination->size);
      if(NULL == destination->bytes){
	/* May never get here, s_malloc may abort if no space */
	p_err_string = qsprintf_stcopyr(p_err_string,
		"Error on reading file. Malloc couldnt allocate %d bytes",
		destination->size );
	RETURN(FALSE);
      }
      result = Read8BitArray(destination->bytes, file, destination->size);
      if(FALSE == result)
	RETURN(FALSE);
    }
    else{
      p_err_string = qsprintf_stcopyr(p_err_string,
		"Unknown keyword for ANY %s\n", temp_string);
      RETURN(FALSE);
    }
  } /*while*/
cleanup:
	/* For now, not freeing destination->bytes on error, let caller do it*/
	return(retval);
}


long 
Read8BitArray(char *destination, FILE *file,long len)
{
  
  int ch;                       /* don't use a long, because %c conversion
                                   isn't defined for longs. */
  long state = BEFORE;
  while(TRUE){
    ch = getc(file);
    if((state == BEFORE) && ((ch == '#') || (ch == '('))) {
      if (ch == '(') state = DURING;
      else state = HASH;
    }
    else if((state == BEFORE) && !isspace(ch)){
      fprintf(stderr,"error in reading array.  Expected # and got %c", ch);
      return(FALSE);
    }
    else if(state == HASH){
      if (ch == '(')
	state = DURING;
      else{
	fprintf(stderr,"Expected an '(' but got an %c\n", ch);
	return(FALSE);
      }
    }
    else if(state == DURING){
      long i;
      ungetc(ch, file);
      for(i = 0; i < len; i++){
	long value;
	if(ReadLong(file,&value) == false){ 
	  fprintf(stderr,"Error in reading a number from the file.");
	  return(FALSE);
	}
	if(value > 255){	
	  fprintf(stderr,"Error in reading file.  Expected a byte in an ANY, but got %ld", value);
	  return(FALSE);
	}
	destination[i] = (char)value;
      }
      if(FALSE == ReadEndOfListOrStruct(file)){
	fprintf(stderr,"array was wrong length");
	return(FALSE);
      }
      return(TRUE);
    }
  }
}
			

boolean
readAbsoluteTime(atime,file)
struct tm* atime;
FILE* file;
{
  if (CheckStartOfStruct("absolute-time",file) == FALSE)
    return(false);
		  
  while (true)
    { long result;
      long val;
      char temp_string[MAX_SYMBOL_SIZE + 1];
     
      result = ReadSymbol(temp_string,file,MAX_SYMBOL_SIZE);
     
      if (result == END_OF_STRUCT_OR_LIST)
	break;
      else if (result == false)
	return(false);
	   	       
      if (strcmp(temp_string,":second") == 0)
	{ if (ReadLong(file,&val) == false)
	    return(false);
	  atime->tm_sec = val;
	}

      else if (strcmp(temp_string,":minute") == 0)
	{ if (ReadLong(file,&val) == false)
	    return(false);
	  atime->tm_min = val;
	}

      else if (strcmp(temp_string,":hour") == 0)
	{ if (ReadLong(file,&val) == false)
	    return(false);
	  atime->tm_hour = val;
	}

      else if (strcmp(temp_string,":mday") == 0)
	{ if (ReadLong(file,&val) == false)
	    return(false);
	  atime->tm_mday = val;
	}

      else if (strcmp(temp_string,":month") == 0)
	{ if (ReadLong(file,&val) == false)
	    return(false);
	  atime->tm_mon = val;
	}

      else if (strcmp(temp_string,":year") == 0)
	{ if (ReadLong(file,&val) == false)
	    return(false);
	  atime->tm_year = val;
	}
      
      else
	SkipObject(file);

    }

  return(true);
}
#endif
