#ifndef EXCERPT_H
#define EXCERPT_H

#define TEXT_SIZE 512
#define TITLE_SIZE 128

typedef struct {
  char title[TITLE_SIZE];
  char text[TEXT_SIZE];
} excerpt_t;

#endif
