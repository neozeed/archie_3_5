/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * The menu API and client were written by Kwynn Buess (buess@isi.edu)
 */

#include <usc-copyr.h>


#include <stdio.h>
int 
get_top_level_answer()
{
    int temp = 'A';
    char digits_buf[10];
    int cnt;
    int q_or_u;
    int temp_digit;
    void really_quit(int);


    while (isspace(temp = getchar())) ;
    if (temp == EOF) {
	really_quit(temp);
	return -1;
    } else if (temp == 'q' || temp == 'u') {
	q_or_u = temp;
	while (!(temp == '\n' || temp == EOF))
	    temp = getchar();
	if (temp == EOF) {
	    clearerr(stdin);
	    return -1;
	}
	if (q_or_u == 'q') {
	    really_quit(q_or_u);
	    return -1;
	} else
	    return 0;

    } else if (!isdigit(temp)) {
	while (!(temp == '\n' || temp == EOF))
	    temp = getchar();
	if (temp == EOF)
	    clearerr(stdin);
	return -1;
    }
    digits_buf[0] = temp;
    cnt = 1;
    while (isdigit(temp = getchar()) && cnt < 10)
	digits_buf[cnt++] = temp;

    while (!(temp == '\n' || temp == EOF))
	temp = getchar();

    if (temp == EOF) {
	clearerr(stdin);
	return -1;
    }
    digits_buf[cnt] = '\0';
    temp_digit = atoi(digits_buf);

    if (temp_digit == 0)
	return -1;
    return temp_digit;
}

int 
query_save_data()
{
    char option = ' ';
    char temp;

    fputs("\n\nViewing, mailing, and printing of many additional file types will\n\
come in later versions of this program.  For now, the only thing we can do\n\
with this data file is save it to a local file.  Would you like to save (y/n) ?",
	  stdout);
    fflush(stdout);

    while (isspace(option) && option != '\n')
	option = getchar();
    temp = option;

    while (!(temp == '\n' || temp == EOF))
	temp = getchar();

    if (temp == EOF) {
	clearerr(stdin);
	return 0;	/* Must mean no */
    }
    if (option == 'y')
	return 1;
    else
	return 0;	/* anything else means no */
}




void 
really_quit(int quit_char)
{
    char really_quit_ch;
    char temp = 'a';

    if (quit_char == EOF)
	clearerr(stdin);
    printf("\nReally quit (y/n) ? ");
    really_quit_ch = getchar();
    while (!(temp == '\n' || temp == EOF))
	temp = getchar();
    if (temp == EOF)
	clearerr(stdin);
    if (really_quit_ch == 'y' || really_quit_ch == EOF) {
	if (really_quit_ch == EOF)
	    printf("\n");
	exit(0);
    }
    return;
}

int 
get_option()
{
    int option = ' ';
    int temp = 'a';

    while (isspace(option) && option != '\n')
	option = getchar();
    temp = option;
    while (!(temp == '\n' || temp == EOF))
	temp = getchar();

    if (temp == EOF) {
	clearerr(stdin);
	return -1;
    }
    if (!(option == 'p' || option == 'm' || option == 's'))
	return -1;
    else
	return option;
}
