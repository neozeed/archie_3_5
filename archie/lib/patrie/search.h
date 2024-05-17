#ifndef _SEARCH_H
#define _SEARCH_H

struct patrie_node {
  unsigned long val;            /* skip value or start value */
  int is_skip;                  /* 1: node is a skip, 0: node is a start */
  int lev;                      /* level on which the current node resides */
  int height;                   /* height of current node (i.e. bit to check) [root at 1?] */
  int pos;                      /* bit offset (from 0), from start of page, of node's type */
  int inode;                    /* this node is the n'th internal node on this level */
  int enode;                    /* this node is the n'th external node on this level */
  int tnode;                    /* total number of nodes on this node's level */
  int pageNum;                  /* the offset of the page on which the node resides */
};

#endif
