extern int _patrieInfixBuild(struct patrie_config *config);
extern void _patrieSetIndexPoints(void *run,
                                  void mark(void *run, size_t indexPoint),
                                  size_t tlen, const char *text, void *arg);
