struct arch_lock;

extern int _archUnlock(struct arch_lock **lock);
extern struct arch_lock *_archLockExclusive(char *dir, char *file);
extern struct arch_lock *_archLockShared(char *dir, char *file);
