#ifndef SIGNALS_H
#define SIGNALS_H

typedef void Sigfunc PROTO((int));


extern Sigfunc *ppc_signal PROTO((int sig, Sigfunc *fn));
extern void catch_alarm PROTO((int sig));
extern void child_sigs PROTO((void));
extern void ctl_c PROTO((int sig));
extern void parent_sigs PROTO((void));
extern void waiting PROTO((int bool));

#endif
