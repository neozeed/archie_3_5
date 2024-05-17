#if 0
#ifdef DEBUG
#include <stdio.h>
#include "debug.h"
#include "defs.h"
#include "error.h"
#include "misc.h"
#include "all.h"


static void fprintf_filter proto_((FILE *fp, struct filter *f, char *name));
static void fprintf_pattrib proto_((FILE *fp, struct pattrib *p, char *name));
static void fprintf_pfile proto_((FILE *fp, struct pfile *p, char *name));
static void fprintf_replicas proto_((FILE *fp, VLINK v, char *name));
static void fprintf_token proto_((FILE *fp, struct token *t, char *name));
static void fprintf_value proto_((FILE *fp, union avalue *v, int type));


static void fprintf_replicas(fp, v, name)
  FILE *fp;
  VLINK v;
  char *name;
{
  efprintf(fp, "struct replicas %s = %p\n", STR(name), (void *)v);
}


static void fprintf_token(fp, t, name)
  FILE *fp;
  struct token *t;
  char *name;
{
  if (name && *name) efprintf(fp, "\tstruct token %s = ", STR(name));
  efprintf(fp, "{\n");
  while (t)
  {
    efprintf(fp, "\t<%s>", STR(t->token));
    efprintf(fp, "%s\n", (t = t->next) ? "," : "");
  }
  efprintf(fp, "}\n");
}


static void fprintf_filter(fp, f, name)
  FILE *fp;
  struct filter *f;
  char *name;
{
  if ( ! f)
  {
    efprintf(fp, "struct filter %s = 0x0\n", STR(name));
  }
  else
  {
    efprintf(fp, "struct filter %s = ", STR(name));
    efprintf(fp, "{\n");
    efprintf(fp, "\tname: %s, type: %d, execution_location: %d, pre_or_post: %d, applied: %d\n",
            STR(f->name), f->type, f->execution_location, f->pre_or_post, f->applied);
    fprintf_token(fp, f->args, "args");
    efprintf(fp, "}\n");
  }
}


static void fprintf_pfile(fp, p, name)
  FILE *fp;
  struct pfile *p;
  char *name;
{
  if ( ! p)
  {
    efprintf(fp, "struct pfile %s = 0x0\n", STR(name));
  }
  else
  {
    efprintf(fp, "struct pfile %s = ", STR(name));
    efprintf(fp, "{\n");
    efprintf(fp, "\tversion: %d, f_magic_no: %ld, exp: %ld, ttl: %ld, last_ref: %ld",
             p->version, p->f_magic_no, p->exp, p->ttl, p->last_ref);
    efprintf(fp, "}\n");
  }
}


static void fprintf_pattrib(fp, p, name)
  FILE *fp;
  struct pattrib *p;
  char *name;
{
  efprintf(fp, "struct pattrib %s = {", STR(name));
  while (p)
  {
    efprintf(fp, "<%s,", STR(p->aname));
    fprintf_value(fp, &p->value, p->avtype);
    p = p->next;
  }
  efprintf(fp, ">\n");
}


static void fprintf_value(fp, v, type)
  FILE *fp;
  union avalue *v;
  int type;
{
  switch (type)
  {
  case ATR_FILTER:
    fprintf_filter(fp, v->filter, "");
    break;
    
  case ATR_LINK:
    efprintf(fp, "<VLINK>");
    break;
    
  case ATR_SEQUENCE:
    fprintf_token(fp, v->sequence, "");
    break;
    
  default:
    efprintf(stderr, "%s: fprintf_value: unknown type `%d'.\n", logpfx(), type);
  }
}


void fprintf_vlink(fp, v)
  FILE *fp;
  VLINK v;
{
  efprintf(fp, "name: `%s'\n", STR(v->name));
  efprintf(fp, "target: `%s'\n", STR(v->target));
  efprintf(fp, "hosttype: `%s'\n", STR(v->hosttype));
  efprintf(fp, "host: `%s'\n", STR(v->host));
  efprintf(fp, "hsonametype: `%s'\n", STR(v->hsonametype));
  efprintf(fp, "hsoname: `%s'\n", STR(v->hsoname));
  efprintf(fp,
          "dontfree: %d, linktype: %c, expanded: %d, version: %d, f_magic_no: %d, dest_exp: %ld\n",
          v->dontfree, v->linktype, v->expanded, v->version, v->f_magic_no, v->dest_exp);
  fprintf_filter(fp, v->filters, "filters");
  fprintf_replicas(fp, v->replicas, "replicas");
  fprintf_pattrib(fp, v->oid, "oid");
  fprintf_pfile(fp, v->f_info, "f_info");
  fprintf_pattrib(fp, v->lattrib, "lattrib");
}

#endif
#endif
