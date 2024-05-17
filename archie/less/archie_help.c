#include "less.h"

extern int screen_trashed;
extern IFILE curr_ifile;


/*
  A replacement help() when less is used for archie (so we can get rid
  of lsystem() entirely).

  We use the tty mode save/restore code.
*/

void help()
{
	register int inp;
	register char *shell;
	register char *p;
	register char *curr_filename;

	/*
	 * Close the current input file.
	 */
	curr_filename = get_filename(curr_ifile);
	(void) edit(NULL, 0);

	/*
	 * De-initialize the terminal and take out of raw mode.
	 */
	deinit();
	flush();	/* Make sure the deinit chars get out */
	raw_mode(0);

	/*
	 * Restore signals to their defaults.
	 */
	init_signals(0);

	/*
	 * Force standard input to be the user's terminal
	 * (the normal standard input), even if less's standard input 
	 * is coming from a pipe.
	 */
	inp = dup(0);
	close(0);
	if (open("/dev/tty", 0) < 0)
		dup(inp);


        /*************************************************************/

#define LESS_OPTS "-m",  "-H", "-+E", "-+s", \
        "-PmHELP -- ?eEND -- Press g to see it again:Press RETURN for more., or q when done "

        switch (fork())
        {
        case -1: /* failed */
          return;

        case 0: /* child */
          execl("bin/less", "less", LESS_OPTS, "bin/less.hlp", (char *)0);
          exit(1);

        default: /* parent */
          wait((int *)0);
        }

        /*************************************************************/

              
	/*
	 * Restore standard input, reset signals, raw mode, etc.
	 */
	close(0);
	dup(inp);
	close(inp);

	init_signals(1);
	raw_mode(1);
	init();
	screen_trashed = 1;

	/*
	 * Reopen the current input file.
	 */
	(void) edit(curr_filename, 0);

#if defined(SIGWINCH) || defined(SIGWIND)
	/*
	 * Since we were ignoring window change signals while we executed
	 * the system command, we must assume the window changed.
	 * Warning: this leaves a signal pending (in "sigs"),
	 * so psignals() should be called soon after lsystem().
	 */
	winch();
#endif
}
