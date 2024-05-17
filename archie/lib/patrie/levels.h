extern int _patrieLevelBuild(struct patrie_config *config);
extern void _patrieLevelStats(struct patrie_config *config, int nlevs, struct trie_single_level **level);
extern void _patrieFreeSingleLevel(struct trie_single_level **l);
extern void _patrieNewSingleLevel(struct patrie_config *config, struct trie_single_level **l);
