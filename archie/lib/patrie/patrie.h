/*  
 *
 *
 *  This is the include file for all programs using the PaTrie library.
 *
 *
 */

struct patrie_config;
struct patrie_state;


extern FILE *patrieGetInfixFP(struct patrie_config *config);
extern FILE *patrieGetLevelFP(struct patrie_config *config);
extern FILE *patrieGetPagedFP(struct patrie_config *config);
extern FILE *patrieGetTextFP(struct patrie_config *config);
extern char *patrieGetStateString(struct patrie_state *state);
extern char *patrieGetSubstring(struct patrie_config *cf, long offset, const char *stop);
extern int patrieBuild(struct patrie_config *config);
extern int patrieAlloc(struct patrie_config **config);
extern int patrieAllocState(struct patrie_state **state);
extern int patrieDequoteKey(const char *qkey, char **key, char **newstr);
extern int patrieGetMoreStarts(struct patrie_config *cf,
                               unsigned long maxhits,
                               unsigned long *nhits,
                               unsigned long start[],
                               struct patrie_state *state);
extern int patrieSearchSub(struct patrie_config *config,
                           const char *key,
                           int casesens,
                           unsigned long maxhits,
                           unsigned long *nhits,
                           unsigned long starts[],
                           struct patrie_state **state);
extern int patrieSetSortTempDir(struct patrie_config *config, const char *dir);
extern int patrieSetStateFromString(const char *str, struct patrie_state **state);
extern void patrieFree(struct patrie_config **config);
extern void patrieFreeState(struct patrie_state **state);
extern int patrieQuoteKey(const char *key, char **qkey);
extern void patrieSetAssert(struct patrie_config *config, int bool);
extern void patrieSetCaseSensitive(struct patrie_config *config, int bool);
extern void patrieSetCreateInfix(struct patrie_config *config,
                                 int bool,
                                 int keepInfix,
                                 int keepInfixTemp);
extern void patrieSetCreateLevels(struct patrie_config *config, int bool);
extern void patrieSetCreatePaged(struct patrie_config *config, int bool);
extern void patrieSetDebug(struct patrie_config *config, int bool);
extern void patrieSetIndexPointsFunction(struct patrie_config *config,
                                         void (*fn)(void *run,
                                                    void (*mark)(void *run, size_t indexPoint),
                                                    size_t tlen, const char *text, void *arg),
                                         void *arg);
extern void patrieSetInfixFP(struct patrie_config *config, FILE *fp);
extern void patrieSetLevelFP(struct patrie_config *config, FILE *fp);
extern void patrieSetLevelPageSize(struct patrie_config *config, size_t lps);
extern void patrieSetLevelsPerPage(struct patrie_config *config, unsigned int lpp);
extern void patrieSetPagedFP(struct patrie_config *config, FILE *fp);
extern void patrieSetPagedPageSize(struct patrie_config *config, size_t pps);
extern void patrieSetSortDistribute(struct patrie_config *config, int bool);
extern void patrieSetSortMaxMem(struct patrie_config *config, size_t mem);
extern void patrieSetSortMerge(struct patrie_config *config, int bool);
extern void patrieSetSortPadLen(struct patrie_config *config, size_t pad);
extern void patrieSetSortUniqueLen(struct patrie_config *config, size_t ulen);
extern void patrieSetStats(struct patrie_config *config, int bool);
extern void patrieSetTextFP(struct patrie_config *config, FILE *fp);
extern void patrieSetTextStart(struct patrie_config *config, long off);
