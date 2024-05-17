struct new_string_cache;


extern int _archAddKeyToStringCache(struct new_string_cache *c,
                                    const char *key,
                                    unsigned long off);
extern int _archFreeStringCache(struct new_string_cache **cache);
extern int _archInitStringCache(struct new_string_cache *c, FILE *sfp, size_t start);
extern int _archKeyInStringCache(struct new_string_cache *c,
                          const char *key,
                          int *found,
                          unsigned long *off);
extern int _archStringCacheLoadFactor(struct new_string_cache *c, float lf);
extern struct new_string_cache *_archNewStringCache(const char *dbdir);
