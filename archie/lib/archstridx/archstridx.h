#define ARCH_INDEX_CHARS 1
#define ARCH_INDEX_WORDS 2

#define ARCH_STRIDX_APPEND  0
#define ARCH_STRIDX_BUILD   1
#define ARCH_STRIDX_SEARCH  2


struct arch_stridx_handle;


extern int archAppendKey(struct arch_stridx_handle *h, const char *key, unsigned long *start);
extern int archBuildStrIdx(struct arch_stridx_handle *h);
extern int archCreateStrIdx(struct arch_stridx_handle *h, const char *dbdir, int indexType);
extern int archGetIndexedSize(struct arch_stridx_handle *h, long *sz);
extern int archGetMoreMatches(struct arch_stridx_handle *h,
                              unsigned long maxhits,
                              unsigned long *nhits,
                              unsigned long start[]);
extern int archGetStateString(struct arch_stridx_handle *h, char **str);
extern int archGetString(struct arch_stridx_handle *h, long offset, char **str);
extern int archGetStringsFileSize(struct arch_stridx_handle *h, long *sz);
extern int archOpenStrIdx(struct arch_stridx_handle *h, const char *dbdir, int mode);
extern int archSearchExact(struct arch_stridx_handle *h,
                           const char *key,
                           int case_sens,
                           unsigned long maxhits,
                           unsigned long *nhits,
                           unsigned long starts[]);
extern int archSearchRegex(struct arch_stridx_handle *h,
                           const char *key,
                           int case_sens,
                           unsigned long maxhits,
                           unsigned long *nhits,
                           unsigned long starts[]);
extern int archSearchSub(struct arch_stridx_handle *h,
                         const char *key,
                         int case_sens,
                         unsigned long maxhits,
                         unsigned long *nhits,
                         unsigned long starts[]);
extern int archSetBuildMaxMem(struct arch_stridx_handle *h, size_t mem);
extern int archSetStateFromString(struct arch_stridx_handle *h, const char *str);
extern int archSetTempDir(struct arch_stridx_handle *h, const char *newdir);
extern int archStridxExists(const char *dbdir, int *ans);
extern int archUpdateStrIdx(struct arch_stridx_handle *h);
extern struct arch_stridx_handle *archNewStrIdx(void);
extern void archCloseStrIdx(struct arch_stridx_handle *h);
extern void archFreeStrIdx(struct arch_stridx_handle **h);
extern void archPrintStats(struct arch_stridx_handle *h);
extern void archSetHashLoadFactor(struct arch_stridx_handle *h, float lf);
