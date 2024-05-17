/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * The menu API and client were written by Kwynn Buess (buess@isi.edu)
 * Extensive modifications by Steven Augart (swa@ISI.EDU)
 */

#include <usc-license.h>

#include <usc-copyr.h>


#include <pfs.h>
#include <string.h>
#include "menu.h"
#include "p_menu.h"

struct q_arg_list {
  char *name;
  char *value;
  
  struct q_arg_list *next;
};
typedef struct q_arg_list *QAL;

void display_query_doc(char*,PATTRIB);
char *return_arg_default(PATTRIB);
static TOKEN replace_args(TOKEN,QAL);
static char *replace_arg(TOKEN,QAL);

/* Returns a linked list of VLINKs.  In case of error, display a message and
   return null. */
VLINK
open_search(VLINK vl) 
{ 
    TOKEN q_tok = get_token(vl,"QUERY-METHOD"); 
    QAL alist;
    QAL get_query_args(TOKEN);
    void fill_query_args(QAL,PATTRIB);
    VDIR_ST dir_st;
    VDIR dir = &dir_st;
    TOKEN acomp;
    VLINK retval;                  /* value to return. */
    int tmp;                      /* return from p_get_dir() */

    m_error = NULL;               /* no error right now. */
    if(q_tok && (q_tok->token)) display_query_doc(q_tok->token,vl->lattrib);

    alist = get_query_args(q_tok);
    fill_query_args(alist,vl->lattrib);

    acomp = replace_args((q_tok->next ? q_tok->next->next : NULL),alist);

    vdir_init(dir);
    tmp = p_get_dir(vl,replace_arg(q_tok->next,alist),dir,0,&acomp);
    if (tmp) {
        perrmesg("Couldn't complete search:", 0, (char *) NULL);
        printf("\n\nRepeating previous menu\n");
        vdir_freelinks(dir);
        retval = NULL;
        goto cleanup;
    }
    retval = dir->links; dir->links = NULL;
    vdir_freelinks(dir);
    /* alist is a list of QAL's not freed on exit !!*/
 cleanup:
    tklfree(acomp);	/* Was a nasty memory leak */
    return retval;
}


static TOKEN 
replace_args(TOKEN fill, QAL alist) 
{ 
    TOKEN	ttok;
    char	*tstr;
    if(!fill) return(NULL);
    tstr = replace_arg(fill, alist);
    ttok = tkalloc(NULL);
    ttok->token = tstr;
    ttok->next = replace_args(fill->next, alist);
    return(ttok);
}


static char *
replace_arg(TOKEN fill, QAL alist) 
{ 
  TOKEN temp_fill_next;

  QAL temp_al;
  int size = 0;
  char *the_arg;
  int cnt_the,cnt_fill;
  int done = 0;
  char *close_br;
  char *temp_value;
  char *get_value(char *,QAL);
  TOKEN new_token;
  char *copy, *startcopy;
  char *head;

  if (fill == NULL) return(NULL);
  if (fill->token == NULL) return(NULL);

  startcopy = copy = stcopy(fill->token);
  
  temp_al = alist;

  while (temp_al != NULL) { 
    size += strlen(temp_al->value) + 1;
    temp_al = temp_al->next;
  }
  
  size += strlen(copy);
  
  head = the_arg = (char *)stalloc(sizeof(char) * size);
  cnt_the = cnt_fill = 0;

  while (!done) { 
    if (*copy == '$') {
      if (*(copy +cnt_fill + 1) == '{' 
	  && *(copy + cnt_fill - 1) != '$') 
	{
	  copy += 2;
	  close_br = strchr(copy,'}');
	  *close_br = '\0';
	  temp_value = get_value(copy,alist);
	  *the_arg = '\0';
	  strcat(the_arg,temp_value);
	  the_arg += strlen(temp_value);
	  copy = close_br + 1;
	  continue;
	}
      else {
	*the_arg++ = *copy++;
	if (*(the_arg - 1) == '\0') done = 1;
      }
    }
    else {
      *the_arg++ = *copy++;
      if (*(the_arg - 1) == '\0') done = 1;
    }
  }

  /* Nasty memory leak - copy is at the end of the string*/
  stfree(startcopy);	
  return(head);

}

char *
get_value(char *to_cmp, QAL list) 
{ 
    QAL temp = list;

    while (temp != NULL) { 
        if (!strcmp(to_cmp,temp->name)) return temp->value;
        temp = temp->next;
    }
    return NULL;
}


/* Interact with user to fill arguments to query */
void fill_query_args(QAL to_fill,PATTRIB all_attribs) {
  QAL temp_tok = to_fill;	/* Note temp_tok is not a TOKEN */
  PATTRIB temp_atr = all_attribs;
  char *get_line();

  if (temp_tok == NULL) return;
  if (all_attribs == NULL) return;

  while (temp_tok != NULL) {	/* Over all the to_fill structure */
    temp_atr = all_attribs;	/* Reset to look at head of pattribs */
				/* Redundantly set at end of loop as well */
    while (temp_atr != NULL) { 
	if (!strcmp(temp_atr->aname,"QUERY-ARGUMENT")) {
	    /* If attribute is QUERT-ARGUMENT matching this temp_tok */
	    if ((temp_atr->value.sequence != NULL) &&
		(!strcmp(temp_atr->value.sequence->token,temp_tok->name))) {
	    enter_again: /* Lazy code - should be a while loop with break*/
		printf("%s: ",temp_atr->value.sequence->next->token);
		temp_tok->value = get_line();
		if(!temp_tok->value || !*(temp_tok->value)) {
		    char	*defval;
		    if(temp_tok->value) stfree(temp_tok->value);
		    /* Get default or tryagain */
		    defval = return_arg_default(temp_atr);
		    if(!defval) {
			display_query_doc(temp_atr->value.sequence->token,
					  all_attribs);
			goto enter_again;
		    }
		    temp_tok->value = stcopy(defval);
		}
		else if(strequal(temp_tok->value,"?")) {
		    display_query_doc(temp_atr->value.sequence->token,all_attribs);
		    stfree(temp_tok->value); temp_tok->value = NULL;
		    goto enter_again;
		}
		else if(*(temp_tok->value) == '\\') {
		    char *tptr;
		    tptr = stcopy((temp_tok->value)+1);
		    stfree(temp_tok->value);
		    temp_tok->value = tptr;
		}
	    }
	}
	temp_atr = temp_atr -> next;
    }

    temp_tok = temp_tok->next;
    temp_atr = all_attribs;
  }
}

void display_query_doc(char *which,PATTRIB all_attribs) 
{
    PATTRIB temp_atr = all_attribs;

    char *paren = strchr(which,'(');

    while (temp_atr != NULL) { 
	if (!strcmp(temp_atr->aname,"QUERY-DOCUMENTATION"))
	    if (temp_atr->value.sequence != NULL)
		if((paren && !strncmp(temp_atr->value.sequence->token,which,
				     paren - which)) ||
		   (!paren && !strcmp(temp_atr->value.sequence->token,which))){
		    printf("\n%s\n\n",temp_atr->value.sequence->next->token);
		}
	temp_atr = temp_atr -> next;
    }    
}

char *return_arg_default(PATTRIB qarg) 
{
  TOKEN element;
  int count = 1;

  if(!qarg) return(NULL);
  if (!strequal(qarg->aname,"QUERY-ARGUMENT")) return (NULL);
  
  element = qarg->value.sequence;

  while(element) {
      if(count++ == 6) return(element->token);
      element = element->next;
  }
  return(NULL);
}



QAL get_query_args(TOKEN seq) { 
  char *func;
  char *t_comma;
  char *t2;
  int done_loop = 0;
  int args;
  char *end_arg;
  QAL head,current,chaser;

  if (seq == NULL) return NULL;
  if (seq->token == NULL) return NULL;

  func = stcopy(seq->token);
  t_comma = strchr(func,'(');
  if (t_comma == NULL) return NULL;
  else t_comma++;

  if (strchr(t_comma,',') == NULL) { /* Determine whether there are any args */
    done_loop = 0;
    args = 0;
    t2 = t_comma;
    while (!done_loop) { 
      t2++;
      if (*t2 == '\0' || *t2 == ')' || args) done_loop = 1;
      if (!done_loop) 
	if (!(isspace(*t2) || *t2 == ')' || *t2 == '\0')) args = 1;
    }
    if (!args) return NULL;
  }


  head = (QAL) stalloc(sizeof(struct q_arg_list));
  head->next = NULL;
  head->value = NULL;
  end_arg = strchr(t_comma,',');
  if (end_arg == NULL) end_arg = strchr(t_comma,')');
  *end_arg = '\0';
  head->name = stcopy(t_comma);
  t_comma = end_arg + 1;

  chaser = head;

  while (!done_loop) { 
   end_arg = strchr(t_comma,',');
   if (end_arg == NULL) {
      end_arg = strchr(t_comma,')');
      done_loop = 1;
    }
   current = (QAL) stalloc(sizeof(struct q_arg_list));
   current->next = NULL;
   current->value = NULL;
   *end_arg = '\0';
   current->name = stcopy(t_comma);
   
   chaser->next = current;
   chaser = current;
   t_comma = end_arg + 1;
  } 
  stfree(func);
  return head;
}


