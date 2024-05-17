/*
 * Copyright (c) 1992, 1993, 1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

/*
 * Changed by swa, Jan 1994:
 * This file will now only support the DIRECTORY, VERSION, AUTHENTICATOR,
 * and LIST commands.  These are all that is needed to support old Archie
 * clients. 
 */
/* Changed by swa, Mar, 1994: Fixed prototypes to be posix. */

#include <usc-license.h>

#include <netdb.h>
#include <sgtty.h>
#include <posix_signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>          /* needed by SCO */
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <pmachine.h>
#include <pserver.h>
#ifdef SERVER_SUPPORT_V1        /* This entire file exists only so that the
                                   server can support Version 1 of the Prospero
                                   Protocol. */
#include <ardp.h>
#include <pfs.h>
#include <psrv.h>
#include <plog.h>
#include <pprot.h>
#include <perrno.h>


#include "dirsrv.h"


/* PPROT.H used to include this definition, but it doesn't any more.  It's now
   moved. */
/* This macro is now officially vestigial.  */

/* Replacement for strtok that doesn't keep state.  Both the variable  */
/* S and the variable S_next must be defined.  To initialize, assign   */
/* the string to be stepped through to S_next, then call get_token on  */
/* S.  The first token will be in S, and S_next will point to the next */
/* token.  Like strtok, this macro does modify the string passed to it */
#define get_token(S,C) \
  do { \
    S = S##_next; \
    if(S) { \
     while(*S == C) S++; \
     S##_next = strchr(S,C); \
     if(S##_next) *(S##_next++) = '\0'; \
     if(!*S) S = NULL; \
    } \
  } while (0)

static char *quote();
static char	*v1_check_nfs();
static int v1_externalize(VLINK vl);
static int reply_v1(RREQ req, char *buf);
static void flush_pending_v1_reply(void);
static int creply_v1(RREQ req, char *buf);
static int replyf_v1(RREQ req, char *format, ...);
static int creplyf_v1(RREQ req, char *format, ...);
static int vreplyf_v1(RREQ req, char *format, va_list ap);
static int error_reply_v1(RREQ req, char *format, ...);
static int haswhite(char *s);
static int hasnl(char *s);


VLINK	check_fwd();
static	v1_cmd_lookup();
extern char *unquote(), *unquoten();
static TOKEN tokenize(char *);

    
/* link TARGET of SYMBOLIC must be changed to SYM-LINK for V1 protocol. */
#define symlinkify(foo) (((foo)[0] == 'S') ? "SYM-LINK" : (foo))


/* This will be called by dirsrv() to process a Protocol Version 1 format
   packet.  It is old code which can have its internal buffers easily
   over-written, but that's life.   It also won't return all of the information
   that the newer versions of the protocol will.  Tough noogies. */
    
int
dirsrv_v1(RREQ req, char *command_next)
{
    long 	client_host;

    /* The following are set in one line and used by subsequent */
    /* lines of the same message                                */

    int	client_version = MAX_VERSION;   /* Protocol version nbr   */
    char	authent[160];           	/* Authentication data    */
    char	client_id[160];         	/* Authenticated username */
    char	client_dir[MAXPATHLEN]; 	/* Current directory      */
    long	dir_version = -1;       	/* Directory version nbr  */
    long	dir_magic_no = -1;      	/* Directory magic number */
#ifdef ARCHIE
    int	max_list_commands = 5;		/* Max lists in request   */
#endif /* ARCHIE */

    /* The following are used while processing the current line       */

    char		*command;          /* The current line            */
    /* command_next is The next line            */
    int		cmd_code;          /* Operation code              */

    PFILE		fi;                /* individual lines            */

    VLINK		fl; 	           /* List of forwarding pointers */
    VLINK		fp; 	           /* The current fp              */

    char *this_comp;             /*  component being worked on*/
    TOKEN additional_comps;      /* additional components to be processed. */
#if 0
    AUTOSTAT_TYPEP(TOKEN,tokenlist_to_freep);
#endif
#if 0
    char		*components;	   /* Components to be processed  */
    char		*remcomp;	   /* Remaining components        */
#endif
    char		localexp;	   /* OK to exp ul for remcomp    */
    VLINK		uexp;		   /* Current link being expanded */
    char		attribfl;	   /* Send back atribues in list  */
    int		item_count = 0;    /* Count of returned items     */

    /* Temporaries */

    char		dir_type[40];     /* Type or dir name (ASCII)     */
    char		*amarg;           /* Arguments for access method  */
    VLINK		clink;            /* For stepping through links   */
    VLINK		crep;            /* For stepping through replicas*/
    FILTER		cfil;             /* For stepping through filters */
    PATTRIB		ca;		  /* Current Attribute            */

    char		*suffix;	  /* Trailing component(s)        */
    int		rsinfo_ret;       /* Ret Val from dsrfinfo        */
    int		verify_dir;       /* Only verifying the directory */
    int		retval;
    int		tmp;
    int		i;
    int		dsdb_options = 0; 

    int		lpriv;		  /* LPRIV option for CREATE-DIR  */
    ACL		wacl;		  /* Working access control list  */
    int		laclchkl;         /* Cached ACL check             */
    int		daclchkl;         /* Cached ACL check             */
    int		laclchkr;         /* Cached ACL check             */
    int		daclchkr;         /* Cached ACL check             */
    int		aclchk;	          /* Cached ACL check             */
    ACL		nacl;		  /* New ACL entry                */

    /* Temporaries for use by sscanf */
    char	t_ltype;
    char 	t_name[MAX_DIR_LINESIZE];
    char	t_type[MAX_DIR_LINESIZE];
    char 	t_htype[MAX_DIR_LINESIZE];
    char 	t_host[MAX_DIR_LINESIZE];
    char 	t_ntype[MAX_DIR_LINESIZE];
    char 	t_fname[MAX_DIR_LINESIZE];
    char	t_options[MAX_DIR_LINESIZE];
    char 	t_acetype[MAX_DIR_LINESIZE];
    char 	t_atype[MAX_DIR_LINESIZE];
    char 	t_rights[MAX_DIR_LINESIZE];
    char	t_principals[MAX_DIR_LINESIZE];
    int	t_num;
    int	n_options;

    char	insrights[MAX_DIR_LINESIZE];

    char	qatype[MAX_DIR_LINESIZE];
    char	qrights[MAX_DIR_LINESIZE];
    struct dsrobject_list_options listopts;
    P_OBJECT ob = NULL;         /* Used in LIST only. */

    if(req->peer_ardp_version > -1) req->peer_ardp_version = -2;

    client_host = req->peer.sin_addr.s_addr;

    *client_dir = '\0';
    strcpy(authent,"NONE");
    strcpy(client_id,"");

    get_token(command,'\n');       /* Defined above */

    while(command) 	{

        cmd_code = v1_cmd_lookup(command);
        switch(cmd_code) {

        case VERSION:
            tmp = sscanf(command,"VERSION %d",&client_version);
            if(tmp != 1) {
                replyf_v1(req,"VERSION %d %s\n", MAX_VERSION,PFS_SW_ID);
            } else {
                if (client_version != 1)
                    creplyf_v1(req,"ERROR Cannot specify two different \
versions at the same time.\n",command);
                plog(L_DIR_ERR, req, "Received VERSION 1 and VERSION %d \
messages in same Prospero request.", client_version);
                RETURNPFAILURE;
            }
            break;

        case AUTHENTICATOR: {
            char	auth_type[40];    /* Type of Authentication */ 
            PAUTH patmp;

            tmp = sscanf(command,"AUTHENTICATOR %s %s",auth_type,authent);
            if(tmp != 2) {
                creplyf_v1(req,"ERROR Invalid arguments: %s\n",command);
                plog(L_DIR_PERR, req,
                     "Invalid AUTHENTICATOR command: %s", command);
                RETURNPFAILURE;
            }
            if(strcmp(auth_type,"UNAUTHENTICATED")) {
                creplyf_v1(req,"ERROR authentication type %s not supported\n",
                       auth_type);
                plog(L_DIR_ERR,req,"Invalid auth-type %s: %s", 
                     auth_type,command);
                RETURNPFAILURE;
            }

            /* This memory freed by rdgram transmit() function. */
            patmp = paalloc();

            patmp->next = req->auth_info;
            req->auth_info = patmp;

            strcpy(client_id,authent);
            req->client_name = stcopyr(authent,req->client_name);
            patmp->principals = tkalloc(client_id);
            patmp->ainfo_type = PFSA_UNAUTHENTICATED;

        }
            break;


        case DIRECTORY:
            dir_version = 0;
            dir_magic_no = 0;
            tmp = sscanf(command,"DIRECTORY %s %s %ld %ld",
                         dir_type,client_dir,&dir_version,&dir_magic_no);

            if(tmp < 2) {
                creplyf_v1(req,"ERROR Invalid arguments: %s\n",command);
                plog(L_DIR_PERR,req,
                     "Invalid DIRECTORY command: %s",command);
                RETURNPFAILURE;
            }

            if(strcmp(dir_type,"ASCII")) {
                creplyf_v1(req,"ERROR id-type %s not supported\n",dir_type);
                plog(L_DIR_ERR,req,"Invalid id-type: %s",
                     command);
                RETURNPFAILURE;
            }

            if(check_handle(client_dir) == FALSE) {
                creply_v1(req,"FAILURE NOT-AUTHORIZED\n");
                plog(L_AUTH_ERR,req,
                     "Invalid directory name: %s",client_dir);
                RETURNPFAILURE;
            }

            break;

            /* This is actually part of the LIST command. */
            /* This is part we don't bother really making work. */
        more_comps:
            /* Set the directory for the next component */

            /* At this point, clink contains the link for the next */
            /* directory, and the directory itself is still filled */
            /* in.  We should save away the directory information, */
            /* then free what remains                              */
            dir_version = clink->version;
            dir_magic_no = clink->f_magic_no;

            strcpy(dir_type,clink->hsonametype);
            strcpy(client_dir,clink->hsoname);

            if(strcmp(dir_type,"ASCII")) {
                creplyf_v1(req,"ERROR id-type %s not supported\n",dir_type);
                plog(L_DIR_ERR,req,"Invalid id-type: %s",
                     command);
                obfree(ob);
                ob = NULL;
                RETURNPFAILURE;
            }

            if(check_handle(client_dir) == FALSE) {
                creply_v1(req,"FAILURE NOT-AUTHORIZED\n");
                plog(L_AUTH_ERR,req,
                     "Invalid directory name: %s",client_dir);
                obfree(ob);
                ob = NULL;
                RETURNPFAILURE;
            }

            assert(additional_comps && additional_comps->token);
            this_comp = additional_comps->token;
            additional_comps = additional_comps->next;
            
            obfree(ob);
            ob = oballoc();     /* reinitialize a clean object to continue
                                 with. */
            goto continue_list;

        case LIST: 
            list_count++;
#ifdef ARCHIE
            if(max_list_commands-- <= 0) {
                creply_v1(req,"FAILURE NOT-AUTHORIZED Too many list commands in a single request\n");
                plog(L_AUTH_ERR,req,"Too many list commands");
                RETURNPFAILURE;
            }
#endif /* ARCHIE */

            if (ob) {
                obfree(ob);
            }
            ob = oballoc();
            tmp = sscanf(command,"LIST %s COMPONENTS %[^\n]",
                         t_options, t_name);

            if (tmp < 1) {
                plog(L_DIR_PERR,req,
                     "Misformatted V1 LIST command: %s",command);
                creplyf_v1(req,"ERROR mis-formatted LIST command: %s\n",command);
                obfree(ob);
                ob = NULL;
                RETURNPFAILURE;
            }
            /* Now we know that t_options were set. */
            
            /* If no options, parse again */
            if(strcmp(t_options,"COMPONENTS")==0) {
                tmp = 1 + sscanf(command,"LIST COMPONENTS %[^\n]", t_name);
            }
            /* t_name now will contain a meaningful components list IFF 
               (tmp == 2) */
            if((tmp < 2)
               || ((additional_comps = p__slashpath2tkl(t_name))
                   == NULL)) {
                this_comp = "*";
                additional_comps = NULL;
                strcpy(t_name, "*"); /* do this for the sake of the log message
                                        below.  */
            } else {
                /* Will be freed upon receiving the next new LIST command in
                   this particular thread.  This thereby stops any potential
                   memory leak.  */
#ifdef PFS_THREADS
                if (*tokenlist_to_freep) tklfree(*tokenlist_to_freep);
                *tokenlist_to_freep = additional_comps;
#endif                
                /* additional_comps must be non-NULL, by what's above. */
                this_comp = additional_comps->token;
                additional_comps = additional_comps->next;
            }
            /* t_name now contains a meaningful name, for the log message
               below. */

            if(strstr(t_options,"VERIFY")) verify_dir = 1;
            else verify_dir = 0;

#ifndef DONTSUPPORTOLD
            if(strequal(this_comp,"%#$PRobably_nOn_existaNT$#%")) {
                this_comp = "*";
                verify_dir = 1;
            }
#endif

            /* If EXPAND specified, remeber that fact */
            if(strstr(t_options,"EXPAND") || strstr(t_options,"LEXPAND")) 
                localexp = 2;
            else localexp = 0;

            if(strstr(t_options,"ATTRIBUTES")) attribfl = DSRD_ATTRIBUTES;
            else attribfl = 0;

            plog(L_DIR_REQUEST, req, "L%s %s %s",
                 (verify_dir ? "V" : " "), client_dir, t_name);

            /* Here's where we start to resolve additional components */
        continue_list:
            uexp = NULL;

            /* If only expanding last component, clear the flag */
            if(localexp == 1) localexp = 0;

            /* If remaining components, expand for this component only */
            if(additional_comps && !localexp) localexp = 1;

        exp_ulink:

            p_clear_errors();

            /* From new V5 list.c */
            listopts.thiscompp = &this_comp;
            listopts.remcompp = &additional_comps;
            listopts.requested_attrs = "#INTERESTING";
            listopts.filters = NULL; /* no server-side filters in V1 */
            retval = dsrobject(req, dir_type, client_dir, 0, dir_magic_no,
                               verify_dir? DRO_VERIFY : 0, &listopts, ob);
            
            /* If forwarded, just say so and go on.  No need to continue. */
            if (retval == DSRFINFO_FORWARDED) {
                replyf_v1(req,"FORWARDED\n");
                break;
#if 0
                if (remcomp && *remcomp) {
                    fl = dir->f_info->forward; dir->f_info->forward = NULL;
                    fp = check_fwd(fl,client_dir,dir_magic_no);
                    vdir_freelinks(dir);
                    if(fp) {
                        if (haswhite(fp->hosttype)
                            || haswhite(fp->host) ||
                            haswhite(fp->hsonametype) ||
                            haswhite(fp->hsoname)) {
                            flush_pending_v1_reply();
                            creply_v1(req, "FAILURE OUT-OF-DATE Cannot \
represent place object was forwarded to in v1 of Prospero protocol.\n");
                            RETURNPFAILURE;
                        } else

                            replyf_v1(req,"LINK L DIRECTORY %s %s %s %s %s %d %d\n",
                                      quote(components),
                                      fp->hosttype,fp->host,fp->hsonametype,
                                      fp->hsoname, fp->version,fp->f_magic_no);
                        p__tkl_back_2slashpath(additional_comps, t_name)
                        replyf_v1(req, "UNRESOLVED %s/%s\n", this_comp,
                               t_name); 
                    }
                    else replyf_v1(req,"FORWARDED\n");
                    vllfree(fl);

                    break;
                } else {
                    goto dforwarded;
                }
#endif
            }

            /* If not a directory, say so */
            if(retval == DSRDIR_NOT_A_DIRECTORY) {
                creply_v1(req,"FAILURE NOT-A-DIRECTORY\n");
                obfree(ob);
                ob = NULL;
                RETURNPFAILURE;
            }

            /* If unauthorized, say so */
            if(retval == DIRSRV_NOT_AUTHORIZED) {
                if(p_err_string && *p_err_string) 
                    creplyf_v1(req,"FAILURE NOT-AUTHORIZED %s\n",p_err_string);
                else creplyf_v1(req,"FAILURE NOT-AUTHORIZED\n");
                obfree(ob);
                ob = NULL;

                RETURNPFAILURE;
            }

            /* If some other failure, say so */
            if(retval) {
                creplyf_v1(req,"FAILURE SERVER-FAILED\n");
                obfree(ob);
                ob = NULL;
                RETURNPFAILURE;
            }

            /* Cache the default answers for ACL checks */
            daclchkl = srv_check_acl(ob->acl, ob->acl, req, "l",
                         SCA_LINKDIR,client_dir,NULL);
            daclchkr = srv_check_acl(ob->acl, ob->acl, req, "r",
                         SCA_LINKDIR,client_dir,NULL);

            /* Here we must send back the links, excluding those that do */
            /* not match the component name. For each link, we must also */
            /* send back any replicas or links with conflicting names    */
            clink = ob->links;
            while(clink) {
              crep = clink;
              while(crep) {
                /* If ->expanded set means we already returned it */
                if(crep->expanded) {
                    crep = crep->next;
                    continue;
                }
                /* Check individual ACL only if necessary */
                laclchkl = daclchkl; laclchkr = daclchkr;
                if(crep->acl) {
                    laclchkl = srv_check_acl(crep->acl,ob->acl,req,"l",
                                             SCA_LINK,NULL,NULL);
                    laclchkr = srv_check_acl(crep->acl,ob->acl,req,"r",
                                             SCA_LINK,NULL,NULL);
                }
                if(!verify_dir && wcmatch(crep->name,this_comp) &&
                   (laclchkl || (laclchkr && 
                                 (strcmp(crep->name,this_comp)==0)))) {
                    if(laclchkr) {
#if 0
                        /* servers not required to resolve multi-comp names. */
                        if(remcomp && !strcmp(crep->host,hostwport) &&
                           !(crep->filters) && !item_count) {
                            /* If components remain on this host    */
                            /* don't reply, but continue searching  */
                            goto more_comps;
                        }
#endif
                        /* Only display this link if it can be converted to
                           V1 EXTERNAL format. */  
                        if((*crep->target == 'E' 
                            &&  v1_externalize(crep) != PSUCCESS) ||
                           haswhite(crep->target) ||
                           haswhite(crep->name) ||
                           haswhite(crep->hosttype) ||
                           haswhite(crep->host) ||
                           haswhite(crep->hsonametype) ||
                           haswhite(crep->hsoname)) {
                            flush_pending_v1_reply();
                            goto next_crep;
                        } 
                        replyf_v1(req,"LINK L %s %s %s %s %s %s %d %d\n",
                                  symlinkify(crep->target), 
                                  quote(crep->name),
                                  crep->hosttype, crep->host,
                                  crep->hsonametype, crep->hsoname,
                                  crep->version,crep->f_magic_no);
                        item_count++;
                    } else {
                        if (haswhite(crep->name)) {
                            flush_pending_v1_reply();
                            goto next_crep;
                        } 
                        replyf_v1(req, "LINK L NULL %s NULL NULL NULL NULL 0 0\n", quote(crep->name));
                        item_count++;
                    }
                    /* Using ->expanded to indicate returned */
                    crep->expanded = TRUE;
                    /* If link attributes are to be returned, do so */
                    /* For now, only link attributes returned       */
                    ca = crep->lattrib;
                    while(ca && attribfl) {
                     /* For now return all attributes. To be done: */
                     /* return only those requested                */
                        TOKEN tk;
                        if(ca->avtype == ATR_SEQUENCE 
                           && ca->value.sequence
                           /* skip access-method attribute, since it's
                              not used. */
                           && !strequal(ca->aname, "ACCESS-METHOD")) {
                            if (haswhite(ca->aname)) {
                                flush_pending_v1_reply();
                                goto listnextat;
                            }
                            else
                                replyf_v1(req,"LINK-INFO %s %s ASCII",
                                    ((ca->precedence==ATR_PREC_LINK) ? "LINK":
                                     ((ca->precedence==ATR_PREC_REPLACE)? "REPLACEMENT":
                                      ((ca->precedence==ATR_PREC_ADD) ? "ADDITIONAL":
                                       "CACHED"))),
                                    ca->aname);
                            for (tk = ca->value.sequence; tk; tk = tk->next)
                                if (hasnl(tk->token)) {
                                    flush_pending_v1_reply();
                                    goto listnextat;
                                } else
                                    replyf_v1(req, " %s", tk->token);
                            reply_v1(req, "\n");
                        }
                    listnextat:
                        ca = ca->next;
                    }
#if 0                           /* We don't need to support this for backwards
                               compatability.  */
                    /* if there are any filters, send them back too */ 
                    cfil = crep->filters;
                    while(cfil && laclchkr) {
                        filter_reply_v1(req, cfil, 0);
                        cfil = cfil->next;
                    }
#endif
                }
                /* Replicas are linked through next, not replicas */
                /* But the primary link is linked to the replica  */
                /* list through replicas                          */
            next_crep:
                if(crep == clink) crep = crep->replicas;
                else crep = crep->next;
              }
              clink = clink->next;
            }
            /* here we must send back the unexpanded union links */
            clink = ob->ulinks;
            while(clink && !verify_dir) {
                if(!clink->expanded &&
                   srv_check_acl(clink->acl,ob->acl,req,"r",
                                 SCA_LINK,NULL,NULL)) {
                    if(localexp && !(clink->filters) &&
                       !strcmp(clink->host,hostwport)) {
                        /* Set the directory for the next component   */
                        /* At this point, clink contains the link     */
                        /* for the next directory                     */
                        dir_version = clink->version;
                        dir_magic_no = clink->f_magic_no;

                        strcpy(dir_type,clink->hsonametype);
                        strcpy(client_dir,clink->hsoname);

                        if(strcmp(dir_type,"ASCII")) {
                            creplyf_v1(req,"ERROR id-type %s not supported\n",
                                   dir_type);
                            plog(L_DIR_ERR,req,
                                 "Invalid id-type: %s", command);
                            obfree(ob);
                            ob = NULL;
                            RETURNPFAILURE;
                        }

                        if(check_handle(client_dir) == FALSE) {
                            creplyf_v1(req,"FAILURE NOT-AUTHORIZED\n");
                            plog(L_AUTH_ERR,req,
                                 "Invalid directory name: %s",client_dir);
                            obfree(ob);
                            ob = NULL;
                            RETURNPFAILURE;
                        }
                        clink->expanded = TRUE;
                        uexp = clink;
                        goto exp_ulink;
                    }
                    /* Don't do any more expanding */
                    localexp = 0;
                    /* Union links; don't need to v1_externalize. */

                    if (haswhite(clink->target) ||
                        haswhite(clink->name) ||
                        haswhite(clink->hosttype) ||
                        haswhite(clink->host) ||
                        haswhite(clink->hsonametype) ||
                        haswhite(clink->hsoname)) {
                        flush_pending_v1_reply();
                        goto listnextulink;
                    } else {
                        replyf_v1(req,"LINK %c %s %s %s %s %s %s %d %d\n",
                           clink->linktype,
                           symlinkify(clink->target), quote(clink->name),
                           clink->hosttype, clink->host,
                           clink->hsonametype,clink->hsoname,
                           clink->version,clink->f_magic_no);
                        item_count++;
                    }
                    /* if there are any filters */
                    /* V5 filters not representable in V1 format. */
                }
            listnextulink:
                clink = clink->next;
            }

            /* If none, match, say so */
            if(!item_count)
                replyf_v1(req,"NONE-FOUND\n");
            /* Otherwise, if components remain say so */
            else if(additional_comps) {
                p__tkl_back_2slashpath(additional_comps, t_name);
                replyf_v1(req, "UNRESOLVED %s\n", t_name);
            }
            obfree(ob);
            ob = NULL;
            break;


        case OBSOLETE:
            creplyf_v1(req, "FAILURE OUT-OF-DATE Please update your clients \
to Prospero version 5.\n");
            RETURNPFAILURE;
            break;

        default: 
            plog(L_DIR_PERR,req,
                 "Unknown message: %s",command);
            creplyf_v1(req,"FAILURE UNIMPLEMENTED %s\n",command);
            RETURNPFAILURE;
        }

        get_token(command,'\n');       /* Defined above */
        continue;

    dforwarded:
    forwarded:
        replyf_v1(req,"FORWARDED\n");
        /* Forwarding is not used in the current ARCHIE clients, so skip the
           fancy stuff. . */
#if 0
    dforwarded:
        fl = dir->f_info->forward; dir->f_info->forward = NULL;
        fp = check_fwd(fl,client_dir,dir_magic_no);
        vdir_freelinks(dir);

    forwarded:

        if(fp) {
            if (haswhite(fp->hosttype) || haswhite(fp->host) ||
                haswhite(fp->hsonametype) || haswhite(fp->hsoname)) {
                creply_v1(req, "FAILURE OUT-OF-DATE Cannot \
represent place object was forwarded to in v1 of Prospero protocol.\n");
                            RETURNPFAILURE;
            }
            replyf_v1(req,"FORWARDED %s %s %s %s %d %d\n",
                   fp->hosttype,fp->host,fp->hsonametype,fp->hsoname,
                   fp->version,fp->f_magic_no);
        }
        else replyf_v1(req,"FORWARDED\n");
        vllfree(fl);

#endif
        get_token(command,'\n');       /* Defined above */
    }
    creply_v1(req,NULL);
    return(PSUCCESS);
}

/*
 * v1_cmd_lookup - lookup the command name and return integer
 *
 *    V1_CMD_LOOKUP takes a pointer to a string containing a command.
 *    It then looks up the first word found in the string and
 *    returns an int that can be used in a switch to dispatch
 *    to the correct routines.
 *
 *    This has been optimzed for the swerver side of the VFS protocol.
 */
static 
int
v1_cmd_lookup(char *cmd)
{
    switch(*cmd) {
    case 'A':
        if(!strncmp(cmd,"AUTHENTICATOR",13))
            return(AUTHENTICATOR);
        else return(UNIMPLEMENTED);
#ifndef PSRV_READ_ONLY
    case 'C':
        if(!strncmp(cmd,"CREATE-OBJECT",11))
            return OBSOLETE;
        else if(!strncmp(cmd,"CREATE-LINK",11))
            return OBSOLETE;
        else if(!strncmp(cmd,"CREATE-DIRECTORY",16))
            return OBSOLETE;
        else return(UNIMPLEMENTED);
#endif
    case 'D':
        if(!strncmp(cmd,"DELETE-LINK",11))
#ifndef PSRV_READ_ONLY
            return OBSOLETE;
#else
            return(UNIMPLEMENTED);
#endif
        else if(!strncmp(cmd,"DIRECTORY",9))
            return(DIRECTORY);
        else return(UNIMPLEMENTED);
    case 'E':
        if(!strncmp(cmd,"EDIT-OBJECT-INFO",14))
            return OBSOLETE;
        else return(UNIMPLEMENTED);
    case 'G':
        if(!strncmp(cmd,"GET-OBJECT-INFO",13))
            return OBSOLETE;
        else return(UNIMPLEMENTED);
    case 'L':
        if(!strncmp(cmd,"LIST-ACL",8))
            return OBSOLETE;
        else if(!strncmp(cmd,"LIST",4))
            return(LIST);
        else return(UNIMPLEMENTED);
    case 'M':
        if(!strncmp(cmd,"MODIFY-ACL",10))
            return OBSOLETE;
        else if(!strncmp(cmd,"MODIFY-LINK",11))
            return OBSOLETE;
        else return(UNIMPLEMENTED);
    case 'P':
        if(!strncmp(cmd,"PACKET",1))
            return(PACKET);
        else return(UNIMPLEMENTED);
    case 'R':
        if(!strncmp(cmd,"RESTART",7))
            return OBSOLETE;
        else return(UNIMPLEMENTED);
    case 'S':
        if(!strncmp(cmd,"STATUS",6))
            return OBSOLETE;
        else return(UNIMPLEMENTED);
    case 'T':
        if(!strncmp(cmd,"TERMINATE",9))
            return OBSOLETE;
        else return(UNIMPLEMENTED);
    case 'U':
        if(!strncmp(cmd,"UPDATE",6))
            return OBSOLETE;
        else return(UNIMPLEMENTED);
    case 'V':
        if(!strncmp(cmd,"VERSION",7))
            return(VERSION);
        else return(UNIMPLEMENTED);
    default:
        return(UNIMPLEMENTED);
    }
}


/* This is the old library file quote.c.  It is vestigial except for
   dirsrv_v1(). */ 
/*
 * quote - quote string if necessary
 *
 *	      QUOTE takes a string and quotes it if it contains special
 *            characters.  The null string is also quoted.
 *
 *    ARGS:   s - string to be quoted
 *            
 * RETURNS:   The quoted string.  If quoting is required, the string
 *	      appears in static storage, and must be copied if 
 *            it is to last beyond the next call to quote.
 *            If s is null, or the null string, '' is returned.
 *
 *    BUGS:   Still needs to check for additional illegal embedded characters
 */

static
char *
quote(char *s 		/* String to be quoted */)
{
#ifdef PFS_THREADS
    AUTOSTAT_CHARPP(quotedp);
#else
    static char *quoted;
#endif
    char		*qp;
    char		*sp = s;
    int		needsquoting = 0;

#ifdef PFS_THREADS
    if (p__bstsize(*quotedp) == 0) *quotedp = stalloc(200);
    qp = *quotedp;
#else
    if (p__bstsize(quoted) == 0) quoted = stalloc(200);
    qp = quoted;
#endif
    *qp++ = '\'';

    if(!sp) sp = "";

    while(*sp) {
        if ((*sp < '#') || (*sp > '~')) needsquoting++;
        if (*sp == '\'') *(qp++) = '\'';
        *(qp++) = *sp++;
#ifdef PFS_THREADS
        if (qp > *quotedp + p__bstsize(*quotedp) - 2) {
            int newsize = p__bstsize(*quotedp) + 200;
            stfree(*quotedp);
            *quotedp = stalloc(newsize);
            return quote(s);
        }
#else
        if (qp > quoted + p__bstsize(quoted) - 2) {
            int newsize = p__bstsize(quoted) + 200;
            stfree(quoted);
            quoted = stalloc(newsize);
            return quote(s);
        }
#endif
    }

    *(qp++) = '\'';
    *(qp++) = '\0';
#ifdef PFS_THREADS
    if((qp == *quotedp + 3) || needsquoting) return(*quotedp);
#else
    if((qp == quoted + 3) || needsquoting) return(quoted);
#endif
    return(s);
}


/* Convert an external link from Version 5 link format to Version 1 format. */
static int
v1_externalize(VLINK vl)
{
    PATTRIB at;
    TOKEN am;                   /* access method itself. */
    if (vl->target && strequal(vl->target, "EXTERNAL")) {
        /* Look for an AFTP access method. */
        for (at = vl->lattrib; at; at = at->next) {
            if (strequal(at->aname, "ACCESS-METHOD")
                && length((am = at->value.sequence)) == 6
                && strequal(am->token, "AFTP")) {
                /* Make sure hosttype, etc, are all correctly set. */
                if (*elt(am, 1))
                    vl->hosttype = stcopyr(elt(am,1), vl->hosttype);
                if (*elt(am,2))
                    vl->host = stcopyr(elt(am,2), vl->host);
                if (*elt(am,3))
                    vl->hsonametype = stcopyr(elt(am,3), vl->hsonametype);
                if (*elt(am,4))
                    vl->hsoname = stcopyr(elt(am,4), vl->hsoname);
                if(strequal(elt(am,5), "BINARY")) {
                    vl->target = stcopyr("EXTERNAL(AFTP,BINARY)", vl->target);
                    return PSUCCESS;
                } else if (strequal(elt(am,5), "TEXT")) {
                    vl->target = stcopyr("EXTERNAL(AFTP,TEXT)", vl->target);
                    return PSUCCESS;
                } else {
                    /* That's really weird.  Must be BINARY or TEXT. Oh well,
                       we're stuck now; go on. */
                    continue;
                }
            }
        } /* for each attribute */
        RETURNPFAILURE;        /* couldn't find an access-method.  How odd. */
    } else
        return PSUCCESS;
}

/* copied from shadowv1v5cv5.c.  Really should go into a separate v1compat.c
   file.  */
static TOKEN
tokenize(s)
    char *s;
{
    TOKEN retval;
    char buf[MAX_DIR_LINESIZE];

    int tmp = qsscanf(s, "%!!s %r", buf, sizeof buf, &s);
    if (tmp < 0)
        internal_error("Buffer overflow!");
    if (tmp == 0)
        return NULL;
    retval = tkalloc(buf);
    if (tmp == 2)
        retval->next = tokenize(s);
    return retval;
}


/* These reply functions make sure that we don't split lines across packets
   in replies to old clients, since the older version of the client library
   could not handle lines split across packets.
   If a line is too long, they set the warning message final_warning.
   Then, when the reply is completed, the warning message is sent, if possible.
   */ 
static char reply_buf[ARDP_PTXT_LEN + 1]; /* buffer. */
static int reply_buflen = 0;    /* length starts at 0. */
                                /* Set to -1 if line was too long & has already
                                   been exceeded, but waiting for final
                                   newline. */ 
/* NEWLINE NULL terminated string if a warning message present; otherwise just
   a NULL in the buffer.  Note that warnings were limited to 100 bytes in
   Beta.4.2e client; so we can't send a longer one than that. */
static char final_warning[100] = "";

/* This code assumes that if a newline is present in buf, it will be the last
   character of buf. */
static int
reply_v1(RREQ req, char *buf)
{
    if (!buf) return PSUCCESS;
    assert(!strchr(buf, '\n') || strchr(buf, '\n')[1] == '\0');
    if (reply_buflen == -1) {
        if (strchr(buf, '\n'))
            flush_pending_v1_reply();
        return PSUCCESS;
    }
    while (*buf) {
        reply_buf[reply_buflen++] = *buf++;
        if (reply_buflen > ARDP_PTXT_LEN) {
            reply_buflen = -1;
            reply_buf[ARDP_PTXT_LEN] = '\0'; /* for the debugger's convenience.
                                                */ 
            return PSUCCESS;
        }
    } 
    reply_buf[reply_buflen] = '\0'; /* cap it off nicely; always
                                       null-terminated  for debugger's
                                       convenience and for our own at end.*/ 
   if (reply_buflen > 0 && reply_buf[reply_buflen - 1] == '\n') {
        int tmp;
        /* Just need ARDP_A2R_NOSPLITL, but this compensates for a bug. */
        tmp = ardp_reply(req, ARDP_R_INCOMPLETE| ARDP_A2R_NOSPLITBL, 
                          reply_buf);
        reply_buflen = 0;
        return tmp;
    }
    return PSUCCESS;
}


/* Flush the pending v1 reply; set the warning message */
/* With or without trailing newline is ok; special code in creply_v1 handles
   this. */
static
void
flush_pending_v1_reply(void)
{
    reply_buf[reply_buflen = 0] = '\0';
    /* Keep it to < 100 characters! */
    qsprintf(final_warning, sizeof final_warning, 
             "WARNING OUT-OF-DATE Some information you wanted can't be sent. \
Upgrade to Prospero v5.\n");
}

static int
creply_v1(RREQ req, char *buf)
{
    reply_v1(req, buf);
    /* There should be no incomplete line pending. */
    /* Just in case... */
    if (reply_buflen) reply_v1(req, "\n");
    assert(!reply_buflen);
    /* Just need ARDP_A2R_NOSPLITL, but this compensates for a bug. */
    if (*final_warning) {
        int tmp =  ardp_reply(req, ARDP_A2R_NOSPLITBL | ARDP_R_COMPLETE, 
                          final_warning);
        *final_warning = '\0';
        return tmp;
    }
    return ardp_reply(req, ARDP_A2R_NOSPLITBL | ARDP_R_COMPLETE, 
                      (char *) NULL);
}



static int
replyf_v1(RREQ req, char *format, ...)
{
    va_list ap;
    int retval;

    va_start(ap, format);
    retval = vreplyf_v1(req, format, ap);
    va_end(ap);
    return retval;
}


static int
creplyf_v1(RREQ req, char *format, ...)
{
    va_list ap;
    int retval;

    va_start(ap, format);
    retval = vreplyf_v1(req, format, ap);
    va_end(ap);
    if (!retval) creply_v1(req, (char *) NULL); /* complete the request */
    return retval;
}

static int
vreplyf_v1(RREQ req, char *format, va_list ap)
{
#ifdef PFS_THREADS
    AUTOSTAT_CHARPP(bufp);
    AUTOSTAT_INTP(bufsizp);
#else
    static char *buf = NULL;
    static int bufsiz = 0;
#endif
    int n;                      /* # of characters possibly written.
                                   # of characters in string in buf */
#ifdef PFS_THREADS
    *bufp = vqsprintf_stcopyr(*bufp, format, ap);
    /* Perform the reply and pass through any error codes */
    return reply_v1(req, *bufp);
#else
    buf = vqsprintf_stcopyr(buf, format, ap);
    /* Perform the reply and pass through any error codes */
    return reply_v1(req, buf);
#endif
}

/* Lifted from lib/psrv/error_reply.c */
static int
error_reply_v1(RREQ req, char *format, ...)
{
    va_list ap;
    char *bufp;
    
    va_start(ap, format);
    
    
    bufp = vplog(L_DIR_PERR, req, format, ap); /* return formatted string */

    reply_v1(req, "ERROR ");
    reply_v1(req, bufp);
    creply_v1(req, "\n");
    va_end(ap);
    RETURNPFAILURE;
}

#include <ctype.h>

static int
haswhite(char *s)
{
    for ( ; *s; ++s) {
        if (isspace(*s)) return 1;
    }
    return 0;
}

static int
hasnl(char *s)
{
    for ( ; *s; ++s) {
        if (*s == '\n') return 1;
    }
    return 0;
}

#endif /* SERVER_SUPPORT_V1 */

