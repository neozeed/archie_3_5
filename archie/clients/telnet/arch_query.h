struct arch_query;


extern int queryPerform(struct arch_query *q);
extern int querySetDomainMatches(struct arch_query *q, char *dlist);
extern int querySetKey(struct arch_query *q, char *key);
extern int querySetPathMatches(struct arch_query *q, char *plist);
extern struct arch_query *queryNew(void);
extern void queryFree(struct arch_query **query);
extern void queryPrintResults(struct arch_query *q, FILE *ofp);
extern void querySetMaxHits(struct arch_query *q, int mh, int mm, int mhpm);
extern void querySetOutputFormat(struct arch_query *q, char *ofmt);
extern void querySetSearchType(struct arch_query *q, char *srch);
extern void querySetSortOrder(struct arch_query *q, char *sort);
