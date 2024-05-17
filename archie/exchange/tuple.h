#ifndef _TUPLE_H_
#define	_TUPLE_H_

#define DEFAULT_TUPLE_LIST_SIZE		1000   /* Default size of tuple array */
#define MAX_TUPLE_SIZE			257   /* Maximum size of the tuple */

typedef char	tuple_t[MAX_TUPLE_SIZE];

#define match_all_domains(x)	!strcmp((x), DOMAIN_ALL)

#endif
