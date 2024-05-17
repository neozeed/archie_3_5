#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef struct
{
  short year;
  short month;
  short day;
  short hour;
  short min;
}
datestruct;

typedef unsigned long db_date;
typedef unsigned int size_type;
typedef unsigned int parenti_type;
typedef unsigned int fchildi_type;
typedef unsigned short perms_type;

struct site_entry
{
  size_type size;		/* Size of file                 */
  parenti_type parent_ind;	/* Record number of parent      */
  fchildi_type first_child_ind;	/* Record number of first child */
  db_date mod_time;		/* Modification time            */
  union
  {
    long strings_ind;		/* Index of name                */
    struct in_addr ipaddress;	/* or site IP addr (root rec)   */
  } in_or_addr;
  perms_type perms;		/* File permissions             */
  char dir_or_f;		/* Directory or file flag       */

};

struct file_entry
{
  struct in_addr site_addr;	/* stored as 32bit IP address     */
  int site_ind;			/* record index within the file   */
  int next_ind;			/* pointer to next entry in chain */
};

typedef struct site_entry site_rec;
typedef struct file_entry file_rec;

struct globals_db_t
{
  int site_recno;
  int next_file_recnum;
  int file_begin;
  int file_recno;
#ifdef DO_NOT_COMPILE
  site_rec *site_begin;
  int site_size;
#endif
};
