/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.

   This is part of the shell user-interface tools for the WAIS software.
*/

/*---------------------------------------------------------------------------*/

#define _C_SOURCE

#include <string_with_strcasecmp.h>
#include <stdio.h>
#include "cutil.h"
#include "futil.h"
#include "irfileio.h"
#include "source.h"
#include <list_macros.h>		/* For TH_APPEND_LIST*/
#include <mitra_macros.h>		/* For TH_FIND_LIST */
#include <psrv.h>               /* includes global declarations of
                                   waissource_count and waissource_max to be
                                   read. */ 

#define DESC_SIZE 65535

extern int alphasort();
#if !defined(IN_RMG) && !defined(PFS_THREADS)
SList Sources;

/* I'm not sure why this is all commented out, looks like its been
   superceeded by "SList Sources" */
char **Source_items = NULL;
#else

WAISSOURCE WaisSources = NULL;

#endif /*!IN_RMG && !PFS_THREADS*/

char *sourcepath = NULL;

#if !defined(IN_RMG) && !defined(PFS_THREADS)
void
freeSourceID(sid)
SourceID sid;
{
  if (sid != NULL) {
    if (sid->filename != NULL) s_free(sid->filename);
    s_free(sid);
  }
}

SourceID
copysourceID(sid)
SourceID sid;
{
  SourceID result = NULL;
  if(sid != NULL) {
    if((result = (SourceID) s_malloc(sizeof(_SourceID))) != NULL) {
      result->filename = s_strdup(sid->filename);
    }
  }
  return result;
}

char **
buildSourceItemList(sourcelist)
SourceList sourcelist;
{
  char **result;
  int num, i;
  SourceList source;

  /* find the length of the sidlist in the question */

  for (num = 0, source = sourcelist; 
       source != NULL && source->thisSource != NULL;
       num++, source = source->nextSource);

  result = (char**)s_malloc((1+num)*sizeof(char*));
  if(num > 0)
    for(i =0, source = sourcelist; i<num; i++, source = source->nextSource)
      result[i] = source->thisSource->filename;
  result[num] = NULL;
  return(result);
}

char **
buildSItemList(sourcelist)
SList sourcelist;
{
  char **result;
  int num, i;
  SList source;

  /* find the length of the sidlist in the question */

  for(num = 0, source = sourcelist; 
      source != NULL;
      num++, source = source->nextSource);

  result = (char**) s_malloc((1+num)*sizeof(char*));
  if(num > 0)
    for(i =0, source = sourcelist; i<num; i++, source = source->nextSource)
      if(source->thisSource != NULL)
	result[i] = source->thisSource->name;
  result[num] = NULL;
  return(result);
}

static short
ReadSourceID(file, sid)
FILE *file;
SourceID sid;
{
  char temp_string[MAX_SYMBOL_SIZE];
  char filename[MAX_SYMBOL_SIZE];
  short check_result;

  check_result = CheckStartOfStruct("source-id", file);
  filename[0] = '\0';
  if(FALSE == check_result){ 
    return(false);
  }
  if(END_OF_STRUCT_OR_LIST == check_result)
    {
      return(FALSE);
    }
    
  /* read the slots: */
  while(TRUE){

    short check_result = ReadSymbol(temp_string, file, MAX_SYMBOL_SIZE);
    if(END_OF_STRUCT_OR_LIST == check_result) break;
    if(FALSE == check_result){
      return(false);
    } 
    if(0 == strcmp(temp_string, ":filename")) {
      if (FALSE == ReadString(filename, file, MAX_SYMBOL_SIZE))
	return(false);
      sid->filename = s_strdup(filename);
    }
    else
      SkipObject(file);
  }
  return(TRUE);
}

SourceList ReadListOfSources(file)
FILE *file;
{
  short check_result;
  SourceID sid = NULL;
  SourceList result, this, last;
          
  /* initialize */
  this = last = result = NULL;

  if(ReadStartOfList(file) == FALSE)
    return(NULL);

  while(TRUE) {
    sid = (SourceID)s_malloc(sizeof(_SourceID));
    check_result = ReadSourceID(file, sid);
    if(check_result == END_OF_STRUCT_OR_LIST) {
      s_free(sid);
      return(result);
    }
    else if(check_result == FALSE)
      return(result);

    else if(check_result == TRUE) {
      if(result == NULL)
	result = this = (SourceList)s_malloc(sizeof(_SourceList));
      else
	this = (SourceList)s_malloc(sizeof(_SourceList));
      this->thisSource = sid;
      if(last != NULL)
	last->nextSource = this;
      last = this;
    }
  }
}

#endif /*!defined(IN_RMG) && !defined(PFS_THREADS)*/

/* from util.c */
void find_value(source, key, value, value_size)
char *source, *key, *value;
int value_size;
{
  char ch;
  long position = 0;  /* position in value */
  char *pos =(char*)strstr(source, key); /* address into source */

  value[0] = '\0';		/* initialize to nothing */

  if(NULL == pos)
    return;

  pos = pos + strlen(key);
  ch = *pos;
  /* skip leading quotes and spaces */
  while((ch == '\"') || (ch == ' ')) {
    pos++; ch = *pos;
  }
  for(position = 0; pos < source + strlen(source); pos++){
    if((ch = *pos) == ' ') {
      value[position] = '\0';
      return;
    }
    value[position] = ch;
    position++;
    if(position >= value_size){
      value[value_size - 1] = '\0';
      return;
    }
  }
  value[position] = '\0';
}

static boolean 
ReadSource(WAISSOURCE source, FILE *file)
{
  char temp_string[MAX_SYMBOL_SIZE];
  char desc_string[DESC_SIZE];
  short check_result;
  long port;

  long version;

  /* make sure it's a Source */
  
  check_result = CheckStartOfStruct("source", file);
  if(FALSE == check_result){ 
    return(false);
  }
  if(END_OF_STRUCT_OR_LIST == check_result)
    {
      return(FALSE);
    }
    
  strcpy(source->server, "");
  strcpy(source->service, "");

  /* read the slots: */
  while(TRUE){

    short check_result = ReadSymbol(temp_string, file, MAX_SYMBOL_SIZE);
    if((END_OF_STRUCT_OR_LIST == check_result)  || 
       (EOF == check_result))
      break;
    if(FALSE == check_result){
      return(false);
    } 
    if(0 == strcmp(temp_string, ":version")) {
      if(FALSE == ReadLong(file, &version))
	return(false);
    }
    else if(0 == strcmp(temp_string, ":ip-name")) {
      if(FALSE == ReadString(temp_string, file, MAX_SYMBOL_SIZE))
	return(false);
      strcpy(source->server, temp_string);
    }
    else if(0 == strcmp(temp_string, ":ip-address")) {
      if(FALSE == ReadString(temp_string, file, MAX_SYMBOL_SIZE))
	return(false);
      strcpy(source->server, temp_string);
    }
    else if(0 == strcmp(temp_string, ":configuration")) {
      if(FALSE == ReadString(temp_string, file, MAX_SYMBOL_SIZE))
	return(false);
      find_value(temp_string, "IPAddress", source->server, STRINGSIZE);
      find_value(temp_string, "RemotePort", source->service, STRINGSIZE);
    }
    else if(0 == strcmp(temp_string, ":tcp-port")) {
      if(FALSE == ReadLong(file, &port))
	return(false);
      sprintf(source->service,"%d", port);
    }
    else if(0 == strcmp(temp_string, ":port")) {
      if(FALSE == ReadLong(file, &port))
	return(false);
      sprintf(source->service,"%d", port);
    }
    else if(0 == strcmp(temp_string, ":maintainer")) {
      if(FALSE == ReadString(temp_string, file, MAX_SYMBOL_SIZE))
	return(false);
      if(source->maintainer != NULL) s_free(source->maintainer);
      source->maintainer = s_strdup(temp_string);
    }
    else if(0 == strcmp(temp_string, ":database-name")) {
      if(FALSE == ReadString(temp_string, file, MAX_SYMBOL_SIZE))
	return(false);
      strcpy(source->database, temp_string);
    }
    else if(0 == strcmp(temp_string, ":cost")) {
      double cost;
      if(FALSE == ReadDouble(file, &cost))
	return(false);
      sprintf(source->cost, "%.2f", cost);
    }
    else if(0 == strcmp(temp_string, ":cost-unit")) {
      if(FALSE == ReadSymbol(temp_string, file, MAX_SYMBOL_SIZE))
	return(false);
      strcpy(source->units, temp_string);
    }
    else if(0 == strcmp(temp_string, ":subjects")) {
      if(FALSE == ReadString(desc_string, file, DESC_SIZE))
	return(false);
      if(source->subjects != NULL) s_free(source->subjects);
      source->subjects = s_strdup(desc_string);
    }
    else if(0 == strcmp(temp_string, ":description")) {
      if(FALSE == ReadString(desc_string, file, DESC_SIZE))
	return(false);
      if(source->description != NULL) s_free(source->description);
      source->description = s_strdup(desc_string);
    }
    else if(0 == strcmp(temp_string, ":update-time")) {
      if(EOF == SkipObject(file)) break;
    }
    else
      if(EOF == SkipObject(file)) break; /* we don't know the key, so we don't know how
			   to interpret the value, skip it */
  }

  return(TRUE);
}

static boolean 
ReadSourceFile(WAISSOURCE asource, char *filename, char *directory)
{
  FILE *fp;
  char pathname[MAX_FILENAME_LEN+1];
  boolean result;

  strncpy(pathname, directory, MAX_FILENAME_LEN);
  strncat(pathname, filename, MAX_FILENAME_LEN);

  if((fp = locked_fopen(pathname, "r")) == NULL)
    return FALSE;

  asource->name = s_strdup(filename);
  asource->directory = s_strdup(directory);

  result = ReadSource(asource, fp);
  locked_fclose_A(fp,pathname,TRUE);
  return(result);
}


/* Read sourcefile "name" from one of the directories in "sourcepath"
   and add to Sources (creating that if neccessary). Return source found
   to make it quicker next time */
static WAISSOURCE
loadSource(name, sourcepath)
char *name;
char *sourcepath;
{
  char *i, *p, source_dir[MAX_FILENAME_LEN];
  WAISSOURCE source = waissource_alloc();

/*
  if(sourcepath == NULL || sourcepath[0] == 0) {
    if((sourcepath = (char*)getenv("WAISSOURCEPATH")) == NULL)
      return NULL;
  }
*/

  for (p = sourcepath, i = p;
       i != NULL;
       p = i+1) {
    if((i = (char*)strchr(p, ':')) == NULL)
      strcpy(source_dir, p);
    else {
      strncpy(source_dir, p, i-p);
      source_dir[i-p] = 0;
    }

    if(ReadSourceFile(source, name, source_dir)) {
      set_connection(source);
      TH_APPEND_ITEM(source,WaisSources,WAISSOURCE);
      return (source);
    }
  }
  s_free(source);
  source = NULL;
  return (source);
}

static int 
match_connection(WAISSOURCE asource, WAISSOURCE bsource)
{
	return (!strcmp(asource->server,bsource->server) &&
		!strcmp(asource->service,bsource->service));
}

/* Try and copy parameters from another source we've already looked at */
static void 
set_connection(WAISSOURCE source)
{
  WAISSOURCE s = WaisSources;
  TH_FIND_OBJFNCTN_LIST(s,source,match_connection,WAISSOURCE)
  if (s) {
	source->connection = s->connection;
	source->buffer_length = s->buffer_length;
	source->initp = s->initp;
  }
}

#if !defined(IN_RMG) && !defined(PFS_THREADS)
#ifdef NEVERDEFINED
boolean newSourcep(name)
char *name;
{
  SList s;

  for (s = Sources; s != NULL; s = s->nextSource)
    if((s->thisSource != NULL) &&
       !strcmp(name, s->thisSource->name))
      return FALSE;

  return TRUE;
}
#endif

boolean is_source(name, test)
char *name;
boolean test;
{
  char lastchar;

  lastchar = name[strlen(name)-1];
  if(test) 
    return ((strlen(name) > 4) &&
	  strstr(name, ".src") &&
	  (!strcmp(".src", strstr(name, ".src"))));
  else 
    return (lastchar != '~' &&
	    lastchar != '#' &&
	    strcmp(name, ".") &&
	    strcmp(name, ".."));
}

#ifdef USINGSOURCEITEMS
static boolean newSource(name)
char *name;
{
  int i;

  for(i =0; i < NumSources; i++)
    if(!strcmp(name, Source_items[i]))
      return FALSE;

  return TRUE;
}

static int
issfile(dp)
struct dirent *dp;
{
  return(is_source(dp->d_name, TRUE) &&
	 newSource(dp->d_name));
}

void SortSourceNames(n)
int n;
{
  boolean Changed = TRUE;
  int i;
  char *qi;

  while(Changed) {
    Changed = FALSE;
    for(i = 0; i < n-1; i++)
      if(0 < strcasecmp(Source_items[i], Source_items[i+1])) {
	Changed = TRUE;
	qi = Source_items[i];
	Source_items[i] = Source_items[i+1];
	Source_items[i+1] = qi;
      }
  }
}

void
GetSourceNames(directory)
char *directory;
{
  /* Note result is returned in global Source_items */
  struct dirent **list;
  int i, j;
  assert(P_IS_THIS_THREAD_MASTER());
  if ((j = scandir(directory, &list, issfile, alphasort)) < 0) {
      PrintStatus(STATUS_INFO, STATUS_HIGH, "Error on open of source directory: %s.\n", directory);
      return;
    }

  if(NumSources > 0)
    Source_items = (char**) s_realloc(Source_items, (NumSources+j+1) * sizeof(char*));
  else {
    if(Source_items != NULL) {
      for (i = 0; Source_items[i] != NULL; i++) s_free(Source_items[i]);
      s_free(Source_items);
    }
    Source_items = (char**) s_malloc((j+1) * sizeof(char*));
  }

  for (i = 0; i < j; i++) {
    Source_items[i+NumSources] = s_strdup(list[i]->d_name);
    s_free(list[i]);
  }

  NumSources+=j;
  SortSourceNames(NumSources);
  Source_items[NumSources] = NULL;

  s_free(list);
}
#endif /*USINGSOURCEITEMS*/
/* read all the sources from a directory.  If test is true, only files ending
   in .src are valid
   */

/* Not used */
#if !defined(IN_RMG)
static void
ReadSourceDirectory(directory, test)
char *directory;
boolean test;
{
  char filename[MAX_FILENAME_LEN];
  FILE *fp;
  int i, j , newNumSources;
  SList Last;
  Source source;
  struct dirent **list;

  assert(P_IS_THIS_THREAD_MASTER());
  if ((j = scandir(directory, &list, NULL, NULL)) < 0) {
    return;
  }

  if(Sources == NULL)
    Sources = makeSList(NULL, NULL);

  for(Last = Sources; Last->nextSource != NULL; Last = Last->nextSource);

  for (i = 0; i < j; i++) {
    if (is_source(list[i]->d_name, test)) {
      if(newSourcep(list[i]->d_name)) {
	strcpy(filename, directory);
	strcat(filename, list[i]->d_name);
	if ((fp = locked_fopen(filename, "r")) != NULL) {
	  source = (Source)s_malloc(sizeof(_Source));
	  memset(source, 0, sizeof(_Source));
	  source->initp = FALSE;
	  source->name = s_strdup(list[i]->d_name);
	  source->directory = s_strdup(directory);
	  ReadSource(source, fp);
	  locked_fclose_A(fp,filename,TRUE);
	  if(Last->thisSource == NULL)
	    Last->thisSource = source;
	  else {
	    Last->nextSource = makeSList(source, NULL);
	    Last = Last->nextSource;
	  }
	  NumSources++;
	}
      }
    }
  }
  free((char *)list);
}
#endif

void WriteSource(directory, source, overwrite)
     char *directory;
     Source source;
     boolean overwrite;
{
  char filename[MAX_FILENAME_LEN];
  FILE *fp;
  
  strcpy(filename, directory);
  strcat(filename, source->name);
  
  if (overwrite == FALSE) 
    if ((fp = locked_fopen(filename, "r")) != NULL) {
      PrintStatus(STATUS_INFO, STATUS_HIGH, 
		  "File %s exists, click again to overwrite.\n", filename);
      locked_fclose_A(fp,filename,TRUE);
      return;
    }
  
  if ((fp = locked_fopen(filename, "w")) == NULL) {
    PrintStatus(STATUS_INFO, STATUS_HIGH, "Error opening %s.\n", filename);
    return;
  }
  
  fprintf(fp, "(:source\n :version 3\n");
  if(source->server != NULL) 
    if(source->server[0] != 0)
      if(isdigit(source->server[0]))
	fprintf(fp, " :ip-address \"%s\"\n", source->server);
      else
	fprintf(fp, " :ip-name \"%s\"\n", source->server);

  if(source->service != NULL) 
    if(source->service[0] != 0)
      fprintf(fp, " :tcp-port %s\n", source->service);

  fprintf(fp, " :database-name \"%s\"\n", source->database);
  if(source->cost != NULL) 
    if(source->cost[0] != 0)
      fprintf(fp, " :cost %s \n", source->cost);
  else
      fprintf(fp, " :cost 0.00 \n");

  if(source->units != NULL) 
    if(source->units[0] != 0)
      fprintf(fp, " :cost-unit %s \n", source->units);
  else
    fprintf(fp, " :cost-unit :free \n");
  
  if(source->maintainer != NULL) 
    if(source->maintainer[0] != 0)
      fprintf(fp, " :maintainer \"%s\"\n", 
	      source->maintainer);
  else
      fprintf(fp, " :maintainer \"%s\"\n", 
	      current_user_name());

  if(source->description != NULL) 
    if(source->description[0] != 0) {
      fprintf(fp, " :description ");
      WriteString(source->description, fp);
    }
  else
    fprintf(fp, " :description \"Created with %s by %s on %s.\"\n",
	    command_name, current_user_name(), printable_time());

  fprintf(fp, "\n)");
  locked_fclose_A(fp,filename,FALSE);
}
SourceList
makeSourceList(source, rest)		
SourceID source;
SourceList rest;
{
  SourceList result;
  if((result = (SourceList)s_malloc(sizeof(_SourceList))) != NULL) {
    result->thisSource = source;
    result->nextSource = rest;
  }
  return(result);
}

SList
makeSList(source, rest)		
Source source;
SList rest;
{
  SList result;
  if((result = (SList)s_malloc(sizeof(_SList))) != NULL) {
    result->thisSource = source;
    result->nextSource = rest;
  }
  return(result);
}

void FreeSource(source)
Source source;
{
  if (source != NULL) {
    if(source->name != NULL)
      s_free (source->name);
    if(source->directory != NULL)
      s_free (source->directory);
    if(source->description != NULL)
      s_free (source->description);
    if(source->maintainer != NULL)
      s_free (source->maintainer);
    s_free(source);
  }
}
#endif /*!IN_RMG && !PFS_THREADS*/

/* Find the source name, in either Sources, or it not found try and load it
   from anywhere in sourcepath */
WAISSOURCE
findsource(char *name, char *sourcepath)
{
  WAISSOURCE asource = WaisSources;
  TH_FIND_STRING_LIST(asource,name,name,WAISSOURCE);
  if (asource) 
	return asource;
  return (loadSource(name, sourcepath));
}

#if !defined(IN_RMG) && !defined(PFS_THREADS)

void
format_source_cost(str,source)
char *str;
Source source;
{
  sprintf(str,"Free");
  if ((source->units != NULL) && (source->cost != NULL)) {
     
     if(0 == strcmp(source->units, ":dollars-per-query"))
        sprintf(str,"$%s/query",source->cost);

     if(0 == strcmp(source->units, ":dollars-per-minute"))
        sprintf(str,"$%s/minute",source->cost);

     if(0 == strcmp(source->units, ":dollars-per-retrieval"))
        sprintf(str,"$%s/retrieval",source->cost);
 
     if(0 == strcmp(source->units, ":dollars-per-session"))
        sprintf(str,"$%s/session",source->cost);
 
     if(0 == strcmp(source->units, ":other"))
        sprintf(str,"Special",source->cost);
  }
}

void
freeSourceList(slist)
SourceList slist;
{
  SourceList sl;
  while(slist != NULL) {
    sl = slist;
    freeSourceID(sl->thisSource);
    slist = sl->nextSource;
    s_free(sl);
  }
}

/*
#include <sockets.h>
*/

/*#define FORWARDER_SERVER "quake"*/
/* send an init message to the source.  A side effect is that the 
   negotiation of buffer sizes.  The final size is put in 
   source->buffer_length 
 */
#ifdef NEVERDEFINED
boolean init_for_source(source, request, length, response)
Source source;
char *request;
long length;
char *response;
{
  char userInfo[500];
  char message[500];
  char hostname[80];
  char domain[80];

  gethostname(hostname, 80);
  getdomainname(domain, 80);
#ifdef TELL_USER
  sprintf(userInfo, "%s %s, from host: %s.%s, user: %s",
	  command_name, VERSION, hostname, domain, getenv("USER"));
#else
  sprintf(userInfo, "%s %s, from host: %s.%s",
	  command_name, VERSION, hostname, domain);
#endif

  if(source->initp == FALSE) {
    if(source->server[0] == 0)
      source->connection = NULL;
    else {
      source->connection = connect_to_server(source->server,
					     atoi(source->service));
#ifdef FORWARDER_SERVER

#ifndef FORWARDER_SERVICE
#define FORWARDER_SERVICE "210"
#endif

      if(source->connection == NULL) {
	strncat(source->database, "@", STRINGSIZE);
	strncat(source->database, source->server, STRINGSIZE);
	strncat(source->database, ":", STRINGSIZE);
	strncat(source->database, source->service, STRINGSIZE);
	strncpy(source->server, FORWARDER_SERVER, STRINGSIZE);
	strncpy(source->service, FORWARDER_SERVICE, STRINGSIZE);
	source->connection = connect_to_server(source->server,
					       atoi(source->service));
      }
#endif

      if (source->connection == NULL) {
	PrintStatus(STATUS_URGENT, STATUS_HIGH, "Bad Connection to server.");
	source->initp = FALSE;
	return source->initp;
      }
    }
    source->buffer_length = 
      init_connection(request, response,
		      length,
		      source->connection,
		      userInfo);

    if (source->buffer_length < 0) {
      PrintStatus(STATUS_URGENT, STATUS_HIGH,
		  "\nError connecting to server: %s service: %s.",
		  source->server, source->service);
      source->initp = FALSE;
    }
    else {
      SList s;

      source->initp = TRUE;

      for (s = Sources; s != NULL; s = s->nextSource) {
      	if (s->thisSource != source) {
	  if (strcmp(s->thisSource->server, source->server) == 0 &&
	      strcmp(s->thisSource->service, source->service) == 0) {
	    s->thisSource->connection = source->connection;
	    s->thisSource->buffer_length = source->buffer_length;
	    s->thisSource->initp = TRUE;
	  }
	}
      }
    }
    return source->initp;
  }
  return source->initp;
}
#endif
#endif /*!IN_RMG && PFS_THREADS*/

