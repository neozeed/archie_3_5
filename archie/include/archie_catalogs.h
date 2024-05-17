#ifndef _ARCHIE_CATALOGS_H_
#define _ARCHIE_CATALOGS_H_

#define	 MAX_CATALOGS	      64

#define	 DEFAULT_CATALOGS_FILENAME  "catalogs.cf"

#define	 MAX_CAT_NAME_LENGTH  64
#define	 MAX_USERSTRING	      64

#define  MAX_AUX_REC	      200

#define	 MASTER_DIR_LINE      "!masterdir"
#define	 HOST_DIR_LINE	      "!hostdir"

typedef	 char  cat_name_t[MAX_CAT_NAME_LENGTH];
typedef	 pathname_t cat_db_t;

typedef enum{
#define ARS_NO_REGEX "noregex"
   AR_NO_REGEX = 0x1,
#define ARS_NO_SUB   "nosub"
   AR_NO_SUB = 0x2,
#define ARS_NO_SUBCASE   "nosubcase"
   AR_NO_SUBCASE = 0x4,
#define ARS_NO_EXACT   "noexact"
   AR_NO_EXACT = 0x8
} search_vector_t;

typedef enum{
   CAT_NOTYPE = 0,
#define CATS_TEMPLATE	"template"
   CAT_TEMPLATE,
#define CATS_ARCHIE	"archie"
   CAT_ARCHIE,
#define CATS_FREETEXT	"freetext"
   CAT_FREETEXT
} cat_type_t;

typedef enum{
   CATA_NOACCESS = 0,
#define CATAS_WAIS	"wais"
   CATA_WAIS,
#define CATAS_ANONFTP	"anonftp"
   CATA_ANONFTP,
#define CATAS_GOPHERINDEX	"gopherindex"
   CATA_GOPHERINDEX,
#define CATAS_WEBINDEX	"webindex"
   CATA_WEBINDEX
} cat_atype_t;


#define	 MAX_ATTRIBUTE_LENGTH	 256

typedef struct{
#if 0
   char	    name[MAX_ATTRIBUTE_LENGTH];
#else
   char	    *name;
#endif
   int	    tindex;
   int	    isheader;
#if 0
   char	    userstring[MAX_USERSTRING];
#else
   char	    *userstring;
#endif
} template_aux_rec;

typedef struct {
   pathname_t ctemplate_aux;
   int	      expand;
   int	      lmodtime;
   template_aux_rec recs[MAX_AUX_REC];
} cat_type_template;

typedef struct{
   file_info_t	*strings_idx;
   file_info_t	*strings;
   file_info_t	*strings_hash;
#ifdef GOPHERINDEX_SUPPORT
   file_info_t	*sel_finfo;
   file_info_t	*host_finfo;
   search_vector_t search_vector;
#endif
} cat_type_archie;

typedef union{
   cat_type_template template;
   cat_type_archie   archie;
}cat_ainfo_t;

typedef struct{
   cat_name_t  cat_name;
   int	       initialized;
   cat_db_t    cat_db;
   cat_type_t  cat_type;
   cat_atype_t cat_access;
   cat_ainfo_t cat_ainfo;
#ifdef BUNYIP_AUTHENTICATION
   int	       restricted;
   char	       *classes;
#endif
} catalogs_list_t;


extern catalogs_list_t *find_in_catalogs PROTO((char *, catalogs_list_t *));
extern status_t initialize_databases PROTO((catalogs_list_t *, char *errst));
extern status_t initialize_template PROTO((catalogs_list_t *));
extern status_t read_catalogs_list PROTO((file_info_t *catalogs_file, catalogs_list_t *catalogs_list, int max_cat, char *p_err_string));

#endif
