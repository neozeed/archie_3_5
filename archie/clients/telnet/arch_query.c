#include <fcntl.h>
#include <netdb.h> /* don't ask... */
#include <stdlib.h>
#include <time.h>
#include "typedef.h"
#include "archie_dns.h"
#include "archie_strings.h"
#include "extern.h"
#include "core_entry.h"
#include "files.h"
#include "get-info.h"
#include "archstridx.h"
#include "db_ops.h"
#include "debug.h"
#include "domain.h"
#include "error.h"
#include "master.h"
#include "search.h"
#include "start_db.h"
#include "times.h"
#include "arch_query.h"


/*
 *  The type of our comparison functions.
 */
typedef int (*CmpFn)(query_result_t *, query_result_t *);

/*
 *  The type of the comparison function required by `qsort'.
 */
typedef int (*QsortCmpFn)(const void *, const void *);

/*
 *  The type of our result printing functions.
 */
typedef void (*PrintFn)(struct arch_query *, FILE *);


struct arch_query {
  struct arch_stridx_handle *strhan;

  file_info_t *startsDB;
  file_info_t *hostauxDB;
  file_info_t *hostbyaddrDB;
  
  char *key;

  int srchType;                 /* one of EXACT, REGEX or SUB */
  int caseSens;                 /* 0: insensitive; 1: sensitive */

  CmpFn cmpFn;                  /* sort comparison function */
  PrintFn printFn;              /* results printing function */

  int maxHits;
  int maxMatches;
  int maxHitsPerMatch;

  char **matchPaths;
  int matchPathsCnt;
  int pathLogicalPath;

  domain_t matchDomains[MAX_NO_DOMAINS];
  int matchDomainsCnt;

  query_result_t *result;
  int resultCnt;
};


static void verbose_results(struct arch_query *q, FILE *ofp);


static int cmp_filename(query_result_t *a, query_result_t *b) {
  return strcmp(a->qrystr, b->qrystr);
}


static int cmp_rfilename(query_result_t *a, query_result_t *b) {
  return -cmp_filename(a, b);
}


static int cmp_filesize(query_result_t *a, query_result_t *b) {
  file_size_t sa = a->details.type.file.size, sb = b->details.type.file.size;
  return sa > sb ? -1 : (sa < sb ? 1 : 0);
}


static int cmp_rfilesize(query_result_t *a, query_result_t *b) {
  return -cmp_filesize(a, b);
}


/*
 *  The `str' field is prefixed by the host name.
 */
static int cmp_hostname(query_result_t *a, query_result_t *b) {
  return strcmp(a->str, b->str);
}


static int cmp_rhostname(query_result_t *a, query_result_t *b) {
  return -cmp_hostname(a, b);
}


static int cmp_modtime(query_result_t *a, query_result_t *b) {
  date_time_t da = a->details.type.file.date, db = b->details.type.file.date;
  return da > db ? -1 : (da < db ? 1 : 0);
}


static int cmp_rmodtime(query_result_t *a, query_result_t *b) {
  return -cmp_modtime(a, b);
}


static void sort_results(struct arch_query *q)
{
  if (q->cmpFn) {
    qsort(q->result, q->resultCnt, sizeof *q->result, (QsortCmpFn)q->cmpFn);
  }
}


void querySetSortOrder(struct arch_query *q, char *sort)
{
  if ( ! sort) return;

  if      (strcmp(sort, "filename")  == 0) q->cmpFn = cmp_filename;
  else if (strcmp(sort, "hostname")  == 0) q->cmpFn = cmp_hostname;
  else if (strcmp(sort, "none")      == 0) q->cmpFn = 0;
  else if (strcmp(sort, "nothing")   == 0) q->cmpFn = 0;
  else if (strcmp(sort, "rfilename") == 0) q->cmpFn = cmp_rfilename;
  else if (strcmp(sort, "rhostname") == 0) q->cmpFn = cmp_rhostname;
  else if (strcmp(sort, "rnothing")  == 0) q->cmpFn = 0;
  else if (strcmp(sort, "rsize")     == 0) q->cmpFn = cmp_rfilesize;
  else if (strcmp(sort, "rtime")     == 0) q->cmpFn = cmp_rmodtime;
  else if (strcmp(sort, "size")      == 0) q->cmpFn = cmp_filesize;
  else if (strcmp(sort, "time")      == 0) q->cmpFn = cmp_modtime;
}


void queryFree(struct arch_query **query)
{
  struct arch_query *q = *query;
  
  if (q->strhan) {
    archCloseStrIdx(q->strhan);
    archFreeStrIdx(&q->strhan);
  }

  if (q->startsDB) {
    close_start_dbs(q->startsDB, 0);
    destroy_finfo(q->startsDB);
  }

  if (q->hostauxDB) {
    close_host_dbs(0, 0, 0, q->hostauxDB);
    destroy_finfo(q->hostauxDB);
  }

  if (q->hostbyaddrDB) {
    close_host_dbs(q->hostbyaddrDB, 0, 0, 0);
    destroy_finfo(q->hostbyaddrDB);
  }

  if (q->key) free(q->key);

  if (q->matchPaths) free(q->matchPaths);
  if (q->result) free(q->result);

  free(q);
  *query = 0;
}


static int query_init(struct arch_query *q)
{
  char nullStr[1] = "";

  q->strhan = 0;

  q->startsDB = 0;

  q->key = 0;

  q->srchType = SUB;
  q->caseSens = 0;

  q->cmpFn = 0;
  q->printFn = verbose_results;

  q->maxHits = 30;
  q->maxMatches = 30;
  q->maxHitsPerMatch = 30;

  q->matchPaths = 0;
  q->matchPathsCnt = 0;
  q->pathLogicalPath = PATH_OR;

  q->matchDomainsCnt = 0;

  q->result = 0;
  q->resultCnt = 0;

  if ( ! set_master_db_dir(nullStr)) {
    error(A_ERR, "queryNew", "error setting the master database directory");
    return 0;
  }
  
  if ( ! set_files_db_dir(nullStr)) {
    error(A_ERR, "queryNew", "error setting the anonftp database directory");
    return 0;
  }

  if( ! set_start_db_dir(nullStr, ANONFTP_DB_NAME)) {
    error(A_ERR, "queryNew", "error setting the starts database directory");
    return 0;
  }

  if ( ! set_host_db_dir(nullStr)) {
    error(A_ERR, "queryNew", "Error setting the host database directory");
    return 0;
  }
  
  if ( ! (q->startsDB = create_finfo())) {
    return 0;
  }
  
  if (open_start_dbs(q->startsDB, 0, O_RDONLY) == ERROR) {
    error(A_ERR, "queryNew", "error opening the starts database");
    return 0;
  }

  if ( ! (q->hostbyaddrDB = create_finfo()) ||
       ! (q->hostauxDB = create_finfo())) {
    return 0;
  }
  
  if (open_host_dbs(q->hostbyaddrDB, 0, 0, q->hostauxDB, O_RDONLY) == ERROR) {
    error(A_ERR, "queryNew", "error opening the hosts databases");
    return 0;
  }

  if ( ! (q->strhan = archNewStrIdx())) {
    error(A_ERR, "queryNew", "can't create handle for searching the database");
    return 0;
  }

  {
    char *dir;

    if ( ! (dir = get_files_db_dir())) {
      return 0;
    }
    
    if ( ! archOpenStrIdx(q->strhan, dir, ARCH_STRIDX_SEARCH)) {
      error(A_ERR, "queryNew", "can't open strings index files");
      free(dir);
      return 0;
    }

    free(dir);
  }
  
  return 1;
}


struct arch_query *queryNew(void)
{
  struct arch_query *q;
  
  d1fprintf(stdout, "euid is %lu, ruid is %lu\n",
            (unsigned long)geteuid(), (unsigned long)getuid()); fflush(stdout);
  d1fprintf(stdout, "egid is %lu, rgid is %lu\n",
            (unsigned long)getegid(), (unsigned long)getgid()); fflush(stdout);

  if ( ! (q = malloc(sizeof *q))) {
    error(A_SYSERR, "queryNew", "can't allocate %ul bytes for query",
          (unsigned long)sizeof *q);
    return 0;
  }

  if ( ! query_init(q)) {
    queryFree(&q);
    return 0;
  }

  return q;
}


int queryPerform(struct arch_query *q)
{
  index_t **strings = 0;
  start_return_t sr;
         
  error(A_INFO,"REQUEST","(maxhits=%d&query=%s&database=Anonymous FTP&type=%s&case=%s)",q->maxHits, q->key,get_type(q->srchType), get_case(q->caseSens));

  if (archQuery(
                q->strhan,
                q->key,
                q->srchType,
                q->caseSens,
                q->maxHits,
                q->maxMatches,
                q->maxHitsPerMatch,
                q->matchPaths,
                0,
                PATH_OR,
                q->matchPathsCnt,
                q->matchDomains,
                q->matchDomainsCnt,
                I_ANONFTP_DB,
                q->startsDB,
                0,              /* unused in archQuery */
                &q->result,
                &strings,       /* not sure what this is for... */
                &q->resultCnt,
                &sr,            /* not sure what this is for... */
                0,
                0,
                I_FORMAT_LINKS
                ) == ERROR) {
    error(A_ERR, "queryPerform", "Archie query failed");
    return 0;
  }

  error(A_INFO,"RESULT","Found %d Hits",q->resultCnt);
  
  if (strings) free_strings(&strings, q->maxHits);

  return 1;
}


/*
 *  `dlist' is a colon separated list of domain, or pseudo-domain, names.
 *  `archQuery' seems to require domain names, so we expand them first.
 */
int querySetDomainMatches(struct arch_query *q, char *dlist)
{
  file_info_t *domainsDB;

  if ( ! (domainsDB = create_finfo())) {
    return 0;
  }

  if (open_start_dbs(0, domainsDB, O_RDONLY) == ERROR) {
    error(A_ERR, "querySetDomainMatches", "can't open domains database");
    close_start_dbs(0, domainsDB); destroy_finfo(domainsDB);
    return 0;
  }

  if (compile_domains(dlist, q->matchDomains, domainsDB, &q->matchDomainsCnt) == ERROR) {
    q->matchDomainsCnt = 0;     /* just in case */
    close_start_dbs(0, domainsDB); destroy_finfo(domainsDB);
    return 0;
  }

  close_start_dbs(0, domainsDB);
  destroy_finfo(domainsDB);

  return 1;
}


int querySetKey(struct arch_query *q, char *key)
{
  if ( ! (q->key = strdup(key))) {
    error(A_SYSERR, "querySetKey", "can't duplicate search key");
    return 0;
  }

  return 1;
}


/*
 *  `plist' is a colon separated list of names that should appear in the path.
 */
int querySetPathMatches(struct arch_query *q, char *plist)
{
  /* bug: assume there are no consecutive colons */

  if ( ! strsplit(plist, ":", &q->matchPathsCnt, &q->matchPaths)) {
    error(A_SYSERR, "querySetPathMatches", "can't split `%s' on `:'", plist);
    return 0;
  }

  return 1;
}


/*
 *  We deliberately ignore such things as the setuid bit, the sticky bit, etc.
 *  It's not clear that we can portably check for these.
 */
static char *perms_str(query_result_t res)
{
  static char p[11];
  mode_t m = res.details.type.file.perms;

  p[0] = '-';
  if      (CSE_IS_DIR(res.details))  p[0] = 'd';
  else if (CSE_IS_LINK(res.details)) p[0] = 'l';
    
  p[1] = m & S_IRUSR ? 'r' : '-';
  p[2] = m & S_IWUSR ? 'w' : '-';
  p[3] = m & S_IXUSR ? 'x' : '-';
  p[4] = m & S_IRGRP ? 'r' : '-';
  p[5] = m & S_IWGRP ? 'w' : '-';
  p[6] = m & S_IXGRP ? 'x' : '-';
  p[7] = m & S_IROTH ? 'r' : '-';
  p[8] = m & S_IWOTH ? 'w' : '-';
  p[9] = m & S_IXOTH ? 'x' : '-';

  p[10] = '\0';

  return p;
}


static int get_location_info(char *s,
                             char **hstart, int *hlen,
                             char **dstart, int *dlen,
                             char **fstart, int *flen)
{
  char *sl;

  *hstart = s;
  if ( ! (sl = strchr(s, '/'))) return 0;
  *hlen = sl - *hstart;
  if (*hlen == 0) return 0;

  *dstart = sl;
  if ( ! (sl = strrchr(s, '/'))) return 0;
  *dlen = sl - *dstart;
  if (*dlen == 0) return 0;

  *fstart = sl + 1;
  *flen = strlen(*fstart);
  if (*flen == 0) return 0;

  return 1;
}


static int is_same_string(char *s0, int l0, char *s1, int l1)
{
  if (l0 != l1) {
    return 0;
  } else {
    return strncmp(s0, s1, l0) == 0;
  }
}


/*
 *  As if by magic, we know that an `ip_addr_t' is an unsigned int containing
 *  the IP address in network byte order. bug: maybe not...
 */
static const char *ipaddr_string(ip_addr_t ip)
{
  static char ipstr[20];
  unsigned long i = ip;

  sprintf(ipstr, "%lu.%lu.%lu.%lu",
          (i >> 24) & 0xFF, (i >> 16) & 0xFF, (i >> 8) & 0xFF, (i >> 0) & 0xFF);
  return ipstr;
}


static const char *file_or_dir(details_t d)
{
  if      (CSE_IS_FILE(d)) return "FILE";
  else if (CSE_IS_DIR(d))  return "DIRECTORY";
  else                     return "FILE"; /* oh, well... */
}


/*
 *  Our crystal ball tells us that a `date_time_t' is the number of seconds
 *  since the epoch, in GMT.  We'll also pretend it's a `time_t' rather than
 *  an unsigned int.
 */
static const char *time_string(date_time_t t)
{
  static char buf[30];
  struct tm *tm;

  if ( ! (tm = localtime(&t)) ||
       strftime(buf, sizeof buf, "%H:%M %e %h %Y", tm) == 0) {
    return "<unknown time>";
  }

  return buf;
}


static const char *ztime_string(date_time_t t)
{
  static char buf[30];
  struct tm *tm;

  if ( ! (tm = gmtime(&t)) ||
       strftime(buf, sizeof buf, "%Y%m%d%H%M%SZ", tm) == 0) {
    return "<unknown time>";
  }

  return buf;
}


/*
 *  This is the one we should use when listing the update time of a host.
 */
static const char *retrieved_time(struct arch_query *q, ip_addr_t addr)
{
  AR_DNS *a;
  char *pn;
  const char *res;
  hostdb_aux_t ha;
  index_t junk;

  if ((a = ar_gethostbyaddr(addr, DNS_LOCAL_ONLY, q->hostbyaddrDB))) {
    if ((pn = get_dns_primary_name(a)) &&
        (get_hostaux_ent(pn, ANONFTP_DB_NAME, &junk, 0, 0, &ha, q->hostauxDB) != ERROR)) {
      res = time_string(ha.retrieve_time);
    }
  
    /* bug: a leak if we don't free it here... */
    if (a->h_addr_list[0]) free(a->h_addr_list[0]);
  }

  return res;
}


static void machine_results(struct arch_query *q, FILE *ofp)
{
  int i;
  
  for (i = 0; i < q->resultCnt; i++) {
    char *dstart, *fstart, *hstart;
    int dlen, flen, hlen;

    if ( ! get_location_info(q->result[i].str,
                             &hstart, &hlen, &dstart, &dlen, &fstart, &flen)) {
      /* Bad news, dude!  Skip this result. */
      continue;
    }

    fprintf(ofp, "%s %.*s %lu %s %.*s\n",
            ztime_string(q->result[i].details.type.file.date),
            hlen, hstart,
            (unsigned long)q->result[i].details.type.file.size,
            perms_str(q->result[i]),
            dlen + flen + 1, dstart
            );
  }
}


static void terse_results(struct arch_query *q, FILE *ofp)
{
  int i;
  
  for (i = 0; i < q->resultCnt; i++) {
    char *dstart, *fstart, *hstart;
    int dlen, flen, hlen;

    if ( ! get_location_info(q->result[i].str,
                             &hstart, &hlen, &dstart, &dlen, &fstart, &flen)) {
      /* Bad news, dude!  Skip this result. */
      continue;
    }

    fprintf(ofp, "%.*s  %12s %7lu %s\n",
            hlen, hstart,
            time_string(q->result[i].details.type.file.date),
            (unsigned long)q->result[i].details.type.file.size,
            dstart
            );
  }
}


static void url_results(struct arch_query *q, FILE *ofp)
{
  int i;
  
  for (i = 0; i < q->resultCnt; i++) {
    fprintf(ofp, "\n  ftp://%s\n", q->result[i].str);
    fprintf(ofp, "\t\tDate: %s      Size: %lu bytes\n",
            time_string(q->result[i].details.type.file.date),
            (unsigned long)q->result[i].details.type.file.size
            );
  }
}


static void verbose_results(struct arch_query *q, FILE *ofp)
{
  char *od = 0, *oh = 0;        /* old directory and host names */
  int odlen = -1, ohlen = -1;   /* length of old directory and host names */
  int i;
  
  for (i = 0; i < q->resultCnt; i++) {
    char *dstart, *fstart, *hstart;
    int dlen = -1, flen = -1, hlen = -1;
    
    if ( ! get_location_info(q->result[i].str,
                             &hstart, &hlen, &dstart, &dlen, &fstart, &flen)) {
      /* Bad news, dude!  Skip this result. */
      continue;
    }

    /*
     *  Print the current host name only if it differs from the previous one.
     */

    if ( ! is_same_string(hstart, hlen, oh, ohlen)) {
      fprintf(ofp, "\n\nHost %.*s\t(%s)\n",
              hlen, hstart,
              ipaddr_string(q->result[i].ipaddr));
      fprintf(ofp, "Last updated %s\n", retrieved_time(q, q->result[i].ipaddr));
    }
    
    /*
     *  Print the current directory only if it differs from the previous one.
     */

    if ( ! is_same_string(dstart, dlen, od, odlen)) {
      fprintf(ofp, "\n    Location: %.*s\n", dlen, dstart);
    }

    /* bug: time in local time, ... */

    fprintf(ofp, "      %s    %s %13lu  %12s  %s\n",
            file_or_dir(q->result[i].details),
            perms_str(q->result[i]),
            (unsigned long)q->result[i].details.type.file.size,
            time_string(q->result[i].details.type.file.date),
            q->result[i].qrystr
            );

    od = dstart; odlen = dlen;
    oh = hstart; ohlen = hlen;
  }
}


void queryPrintResults(struct arch_query *q, FILE *ofp)
{
  if (q->resultCnt <= 0) {
    fprintf(ofp, "\n# No matches were found.\n");
  } else {
    sort_results(q);
    q->printFn(q, ofp);
  }
}


void querySetMaxHits(struct arch_query *q, int mh, int mm, int mhpm)
{
  if (mh > 0) q->maxHits = mh;
  if (mm > 0) q->maxMatches = mm;
  if (mhpm > 0) q->maxHitsPerMatch = mhpm;
}


void querySetOutputFormat(struct arch_query *q, char *ofmt)
{
  if      (strcmp(ofmt, "machine") == 0) q->printFn = machine_results;
  else if (strcmp(ofmt, "silent")  == 0) q->printFn = verbose_results;
  else if (strcmp(ofmt, "terse")   == 0) q->printFn = terse_results;
  else if (strcmp(ofmt, "verbose") == 0) q->printFn = verbose_results;
  else if (strcmp(ofmt, "url")     == 0) q->printFn = url_results;
  else                                   q->printFn = verbose_results;
}


void querySetSearchType(struct arch_query *q, char *srch)
{
  if ( ! srch) {
    return;                     /* use defaults */
  }

  if        (strcmp(srch, "exact")   == 0) {
    q->srchType = EXACT;
    q->caseSens = 1;
  } else if (strcmp(srch, "sub")     == 0) {
    q->srchType = SUB;
    q->caseSens = 0;
  } else if (strcmp(srch, "subcase") == 0) {
    q->srchType = SUB;
    q->caseSens = 1;
  } else if (strcmp(srch, "regex")   == 0) {
    q->srchType = REGEX;
    q->caseSens = 1;
  }
}
