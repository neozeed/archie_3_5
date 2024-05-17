
/*
 *  To disregrard characters with accents.  For Accent insensetive operations.
 */

extern char _patrieChLowerCase[];
extern char _patrieChNoAccent[];

extern int NoAccent(const char c);

extern int _patrieUStrNCmp(const char *s0, const char *s1, size_t n);
extern int _patrieUStrNCaseCmp(const char *s0, const char *s1, size_t n);
extern int _patrieUStrCmp(const char *s0, const char *s1);
extern int _patrieUStrCaseCmp(const char *s0, const char *s1);

extern int _patrieUStrAccentCmp(const char *s0, const char *s1);
extern int _patrieUStrCiAiCmp(const char *s0, const char *s1);
extern int _patrieUStrCiAsCmp(const char *s0, const char *s1);
extern int _patrieUStrCsAiCmp(const char *s0, const char *s1);
extern int _patrieUStrCsAsCmp(const char *s0, const char *s1);
#if 0
extern int _patrieSortComp(const void *p0, const void *p1);
extern int _patrieSortCaseComp(const void *p0, const void *p1);
#endif
extern long _patrieDiffBit(const char *str0, const char *str1);
extern long _patrieDiffBitCase(const char *str0, const char *str1);
extern void _patrieInitDiffTab(void);

extern long _patrieDiffBitAccent(const char *str0, const char *str1);
extern long _patrieDiffBitCiAi(const char *str0, const char *str1);
extern long _patrieDiffBitCiAs(const char *str0, const char *str1);
extern long _patrieDiffBitCsAi(const char *str0, const char *str1);
extern long _patrieDiffBitCsAs(const char *str0, const char *str1);
