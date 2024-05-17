/*
   Routines to handle the manipulation of archie variables.
*/

#ifndef AIX
#include <limits.h>
#endif
#include <stdio.h>
#include <string.h>
#ifdef __STDC__
#include <stdlib.h>
#include <unistd.h>
#endif
#include "defines.h"
#include "error.h"
#include "lang.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "mode.h"
#include "strmap.h"
#include "vars.h"

#include "protos.h"

extern int set_term();


#include "vars_lang.h" /* protect from string extraction */


static enum var_type_e get_var_type PROTO((const char *varname));
static int check_value PROTO((const struct var_desc_s *var, const char *value));
static int set_var_ PROTO((struct var_desc_s *vp,
                           const char *var_name, const char *var_ptr));
static int show_all_vars PROTO((void));
static int show_var PROTO((const char *varname));
static struct var_desc_s *find PROTO((const char *name));
  

/*  
 *  Check the 'value' of 'var' against a list of possible values (if such a
 *  list exists).
 */  

static int check_value(var, value)
  const struct var_desc_s *var;
  const char *value;
{
  return mapEmpty(var->valid_list) ? 1 : !! mapStr(value, var->valid_list);
}


/*  
 *  Return a pointer to the variable specified by name.
 */  

static struct var_desc_s *find(name)
  const char *name;
{
  struct var_desc_s *ret = 0;
  struct var_desc_s *vp;

  vp = vars;
  while ( ! mapEmpty(vp->name))
  {
    if (mapStr(name, vp->name))
    {
      ret = vp;
      break;
    }
    vp++;
  }
  return ret;
}


/*  
 *  If the variable is set return its value, otherwise return a NULL pointer.
 #
 #  Return the value corresponding to the current language.
 */  

const char *get_var(name)
  const char *name;
{
  struct var_desc_s *vp = find(name);
  return vp ? mapFirstStr(&vp->val[0]) : (const char *)0;
}


/*  
 *  Return the variable type.
 */  

static enum var_type_e get_var_type(name)
  const char *name;
{
  struct var_desc_s *vp = find(name);
  return vp ? vp->type : VAR_BAD_TYPE;
}


/*  
 *  Check whether a variable is set.  Return 1 if it is, return 0 if it is
 *  not set, or does not exist.
 */  

int is_set(name)
  const char *name;
{
  struct var_desc_s *vp = find(name);
  return vp ? vp->is_set : 0;
}


/*  
 *  Really set the variable.
 *  
 *  Assumes we have a valid pointer to it.
 *  
 *  (Gotta put this first, due to static decl.)
 */  

static int set_var_(vp, var_name, var_ptr)
  struct var_desc_s *vp;
  const char *var_name;
  const char *var_ptr;
{
  /* Do some checks to see whether the variable can be set. */

  if ( ! (vp->modes & current_mode()))
  {
    printf(curr_lang[287], var_name, mode_str(current_mode()));
  }
  if ( ! vp->allow_chval && ! (current_mode() & vp->modes))
  {
    printf(curr_lang[288],
           var_name) ;
    return 0 ;
  }
  if ( ! check_value(vp, var_ptr))
  {
    printf(curr_lang[289],
           var_ptr, var_name);
    return 0;
  }

  if (vp->doit != NOFUNC && ! (*vp->doit)(var_ptr))
  {
    return 0;
  }

  switch (vp->type)
  {
  case BOOLEAN:
    vp->is_set = 1;
    return 1;

  case NUMERIC:
    {
      int val;

      if (sscanf(var_ptr, curr_lang[274], &val) != 1)
      {
        printf(curr_lang[290]);
        return 0;
      }


      if (val < vp->min_val || val > vp->max_val)
      {
        printf(curr_lang[291], vp->min_val, vp->max_val);
        return 0;
      }

      if (vp->is_set && vp->has_been_reset)
      {
        freeStrMap(vp->val);
      }

      if ( ! newStrMap(var_ptr, vp->val))
      {
        error(A_ERR, curr_lang[292], curr_lang[293]);
        return 0;
      }
      else
      {
        if (vp->is_set)
        {
          vp->has_been_reset = 1;
        }
        vp->is_set = 1;
        return 1;
      }
    }

  case STRING:
    {
      if (strlen(var_ptr) > MAX_VAR_STR_LEN) /* simple sanity check */
      {
        printf(curr_lang[294], MAX_VAR_STR_LEN);
        return 0;
      }

      if (vp->is_set && vp->has_been_reset)
      {
        freeStrMap(vp->val);
      }

      if ( ! newStrMap(var_ptr, vp->val))
      {
        error(A_ERR, curr_lang[292], curr_lang[293]);
        return 0;
      }
      else
      {
        if (vp->is_set)
        {
          vp->has_been_reset = 1;
        }
        vp->is_set = 1;
        return 1;
      }
    }

  default:
    /* This should never happen :-) */

    error(A_INTERR, curr_lang[295], curr_lang[296]);
    return 0;
  }
}


/*  
 *  Set the value of a variable.
 *  
 *  Second argument is always a pointer to the string representation of the
 *  variable, execpt in the case of BOOLEAN where it is not used.
 */  

int set_var(var_name, var_ptr)
  const char *var_name;
  const char *var_ptr;
{
  struct var_desc_s *vp;

  if ( ! (vp = find(var_name)) || (vp->is_hidden && ! (current_mode() & vp->modes)))
  {
    printf(curr_lang[297],
           var_name, mode_str(current_mode()));
    return 0;
  }
  return set_var_(vp, var_name, var_ptr);
}


/*  
 *  Display all variables.
 */  

static int show_all_vars()
{
  struct var_desc_s *vp;

  for (vp = vars;  ! mapEmpty(vp->name); vp++)
  {
    if ( ! vp->is_hidden)
    {
      show_var(mapFirstStr(vp->name));
    }
  }
  return 1;
}


/*  
 *  Display a variable.
 */  

static int show_var(name)
  const char *name;
{
  const char *type;
  struct var_desc_s *vp;

  if ( ! (vp = find(name)) || vp->is_hidden)
  {
    printf(curr_lang[58], name);
    return 1;
  }

  if (vp->type == BOOLEAN)
  {
    type = curr_lang[298];
    if (vp->is_set)
    {
      printf(curr_lang[299], mapLangStr(&vp->name[0]), type);
    }
    else
    {
      printf(curr_lang[300], mapLangStr(&vp->name[0]), type);
    }
    return 1;
  }
  else if (vp->type == NUMERIC)
  {
    type = curr_lang[301];
  }
  else if (vp->type == STRING)
  {
    type = curr_lang[302];
  }
  else
  {
    error(A_INTERR, curr_lang[303], curr_lang[304],
          mapLangStr(&vp->name[0]));
    return 0;
  }

  if (vp->is_set)
  {
    printf(curr_lang[305],
           mapLangStr(&vp->name[0]), type, mapLangStr(&vp->val[0]));
  }
  else
  {
    printf(curr_lang[300], mapLangStr(&vp->name[0]), type);
  }
  return 1;
}


/*  
 *  Unset a variable.
 */  

int unset_var(name)
  const char *name;
{
  struct var_desc_s *vp;

  if ( ! (vp = find(name)) || (vp->is_hidden && ! (current_mode() & vp->modes)))
  {
    printf(curr_lang[297], name, mode_str(current_mode()));
    return 0;
  }
  
  if ( ! vp->allow_unset)
  {
    printf(curr_lang[306],
           mapFirstStr(&vp->name[0]));
  }
  else
  {
    if (vp->is_set)
    {
      vp->is_set = VAR_UNSET;
      if (vp->undoit != NOFUNC)
      {
	(*vp->undoit)();
      }
      if (vp->type != BOOLEAN && vp->has_been_reset)
      {
        freeStrMap(vp->val);
      }
    }
  }
  return 1;
}


int change_lang(s)
  const char *s;
{
  char hdir[MAX_PATH_LEN];

  head(get_var(V_HELP_DIR), hdir, sizeof hdir);
  strcat(hdir, curr_lang[98]);
  strcat(hdir, s);

  if (access(hdir, R_OK | F_OK) != -1)
  {
#ifdef UQAM
    if (strcmp(s, curr_lang[307]) == 0) curr_lang = english;
    else if (strcmp(s, curr_lang[308]) == 0) curr_lang = french;
    else
    {
      printf(curr_lang[309], s);
    }
#endif
    
    return set_var(V_HELP_DIR, hdir);
  }
  else
  {
    printf(curr_lang[310], s);
    return 0;
  }
}


int set_it(ac, av)
  int ac;
  char **av;
{
  enum var_type_e vt;

  if (ac < 2)
  {
    printf(curr_lang[53], av[0]);
    return 0;
  }

  switch (vt = get_var_type(av[1]))
  {
  case BOOLEAN:
    if (ac > 2)
    {
      printf(curr_lang[54]);
      return 0;
    }
    return set_var(av[1], (const char *)0);

  case NUMERIC:
    if (ac != 3)
    {
      printf(curr_lang[55]);
      return 0;
    }
    else
    {
      return set_var(av[1], av[2]);
    }
    break;
    
  case STRING:
    if (ac == 2)
    {
      printf(curr_lang[56]);
      return 0;
    }
    else
    {
      char var_val[256];
      int i;

      var_val[0] = '\0';
      for(i = 2; i < ac; i++)
      {
        strcat(var_val, curr_lang[57]);
        strcat(var_val, av[i]);
      }
      return set_var(av[1], var_val + 1); /*bug: kludge*/
    }

  case VAR_BAD_TYPE:
    printf(curr_lang[58], av[1]);
    return 0;

  default:
    error(A_INTERR, curr_lang[59], curr_lang[60], vt);
    return 0;
  }
}


/*  
 *  Print the value of a variable (or all variables).
 */  

int show_it(ac, av)
  int ac;
  char **av;
{
  if (ac == 1)
  {
    return show_all_vars();
  }
  else
  {
    while (--ac)
    {
      if ( ! show_var(*++av))
      {
        return 0;
      }
    }
  }
  return 1;
}


/*  
 *  unset a variable.
 */  

int unset_it(ac, av)
  int ac;
  char **av;
{
  if (ac == 2)
  {
    return unset_var(av[1]);
  }
  else
  {
    printf(curr_lang[46], av[0]);
    return 0;
  }
}
