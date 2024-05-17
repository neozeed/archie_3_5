#ifndef _MYSTDIO_H_
#define _MYSTDIO_H_

#ifdef __STDC__

extern	        time_t		time(time_t *);
extern	        int		getopt(int, char **, char *);
extern		int		atoi(char *);
extern	        void		exit(int);
extern		void		setbuf(FILE *, char *);
extern	        int		rename(char *, char *);
extern		char *		bsearch(char *, char *, int, int, int ());
extern	        int		dup2(int, int);
extern		int		execlp();
extern		int		fseek(FILE *,long, int);
extern	        long		ftell(FILE *);
extern	        int		fread (char *,int,int,FILE *);
extern	        int		fwrite (char *,int,int,FILE *);
extern	        int		fflush(FILE *);
extern		int		fclose(FILE *);
extern	        int		ftruncate(int, int);
extern	        int		fprintf();

#else

extern	        time_t		time();
extern	        int		getopt();
extern		int		atoi();
extern	        void		exit();
extern		void		setbuf();
extern	        int		rename();
extern		char *		bsearch();
extern	        int		dup2();
extern		int		execlp();
extern		int		fseek();
extern	        long		ftell();
extern	        int		fread();
extern	        int		fwrite();
extern	        int		fflush();
extern		int		fclose();
extern	        int		ftruncate();

#endif

#endif
