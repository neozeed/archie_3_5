extern char *_archMakePath(const char *dir, const char *sep, const char *file);
extern char *_archStrChr(char *s, int c);
extern char *_archStrDup(const char *s);
extern char *_archStrNDup(const char *s, size_t n);
extern int _archAtEOF(FILE *fp);
extern int _archFileOpen(const char *dir,
                         const char *file,
                         const char *mode,
                         int must_exist,
                         FILE **fp);
extern int _archMemSprintf(char **buf, const char *fmt, ...);
extern int _archUnlinkFile(const char *dir, const char *file);
extern long _archSystemPageSize(void);
extern long _archFileSizeFP(FILE *fp);
extern int identical(char c);
