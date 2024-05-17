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

char *recursive_get_line(int *is_eof, int count) { 
  char *temp_string;
  int temp_char;

  temp_char = getchar();

  if (temp_char == EOF) {
    clearerr(stdin);
    *is_eof = 1;
    return NULL;
  }

  if (temp_char == '\n') { 
    temp_string = (char *) stalloc(sizeof(char) * (count + 1));
    temp_string[count] = '\0';
    
    return temp_string;
  }
  temp_string        = recursive_get_line(is_eof,count + 1) ; 
  if (temp_string != NULL)
    temp_string[count] = temp_char;
  else return NULL;

  return temp_string;
}

char *get_line() {
  char *temp;

  int is_eof = 0;
  temp = recursive_get_line(&is_eof,0);
  if (is_eof) {
    clearerr(stdin);
    return NULL;
  }
  return temp;
}
