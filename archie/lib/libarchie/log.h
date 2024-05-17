#ifndef _ALOG_H_
#define _ALOG_H_

typedef enum log_level_t{
	AL_INFO,
	AL_WARNING,
	AL_ERROR,
	AL_FATAL
};

#ifdef __STDC__

extern	status_t	open_alog( char *, log_level_t );
extern	void		alog();

#else

extern	status_t	open_alog();
extern	void		alog();

#endif
