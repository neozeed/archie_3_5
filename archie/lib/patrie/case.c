#include <string.h>
#include "case.h"
#include "defs.h"
#include "utils.h"


/*
 *  We assume that we won't be required to convert EOF with this table.
 *  
 *  bug: extend this to all Latin-1 characters.  Also, convert accented
 *  characters to their unaccented equivalents?
 */

/*  
 *  Same table with accented character mapped to their equivelant lower case.
 */  
char _patrieChLowerCase[] = {
  '\000',  '\001',  '\002',  '\003',  '\004',  '\005',  '\006',  '\007',
  '\010',  '\011',  '\012',  '\013',  '\014',  '\015',  '\016',  '\017',
  '\020',  '\021',  '\022',  '\023',  '\024',  '\025',  '\026',  '\027',
  '\030',  '\031',  '\032',  '\033',  '\034',  '\035',  '\036',  '\037',
  '\040',  '\041',  '\042',  '\043',  '\044',  '\045',  '\046',  '\047',
  '\050',  '\051',  '\052',  '\053',  '\054',  '\055',  '\056',  '\057',
  '\060',  '\061',  '\062',  '\063',  '\064',  '\065',  '\066',  '\067',
  '\070',  '\071',  '\072',  '\073',  '\074',  '\075',  '\076',  '\077',
  '\100',  '\141',  '\142',  '\143',  '\144',  '\145',  '\146',  '\147',
  '\150',  '\151',  '\152',  '\153',  '\154',  '\155',  '\156',  '\157',
  '\160',  '\161',  '\162',  '\163',  '\164',  '\165',  '\166',  '\167',
  '\170',  '\171',  '\172',  '\133',  '\134',  '\135',  '\136',  '\137',
  '\140',  '\141',  '\142',  '\143',  '\144',  '\145',  '\146',  '\147',
  '\150',  '\151',  '\152',  '\153',  '\154',  '\155',  '\156',  '\157',
  '\160',  '\161',  '\162',  '\163',  '\164',  '\165',  '\166',  '\167',
  '\170',  '\171',  '\172',  '\173',  '\174',  '\175',  '\176',  '\177',
  '\200',  '\201',  '\202',  '\203',  '\204',  '\205',  '\206',  '\207',
  '\210',  '\211',  '\212',  '\213',  '\214',  '\215',  '\216',  '\217',
  '\220',  '\221',  '\222',  '\223',  '\224',  '\225',  '\226',  '\227',
  '\230',  '\231',  '\232',  '\233',  '\234',  '\235',  '\236',  '\237',
  '\240',  '\241',  '\242',  '\243',  '\244',  '\245',  '\246',  '\247',
  '\250',  '\251',  '\252',  '\253',  '\254',  '\255',  '\256',  '\257',
  '\260',  '\261',  '\262',  '\263',  '\264',  '\265',  '\266',  '\267',
  '\270',  '\271',  '\272',  '\273',  '\274',  '\275',  '\276',  '\277',
  '\340',  '\341',  '\342',  '\343',  '\344',  '\345',  '\346',  '\347',
  '\350',  '\351',  '\352',  '\353',  '\354',  '\355',  '\356',  '\357',
  '\360',  '\361',  '\362',  '\363',  '\364',  '\365',  '\366',  '\327',
  '\370',  '\371',  '\372',  '\373',  '\374',  '\375',  '\376',  '\337',
  '\340',  '\341',  '\342',  '\343',  '\344',  '\345',  '\346',  '\347',
  '\350',  '\351',  '\352',  '\353',  '\354',  '\355',  '\356',  '\357',
  '\360',  '\361',  '\362',  '\363',  '\364',  '\365',  '\366',  '\367',
  '\370',  '\371',  '\372',  '\373',  '\374',  '\375',  '\376',  '\377'
};


/*
 *  Map accented characters to their equivelant non-accented letters (if
 *  possible)
 *  
 *  ?? I am still not sure about the characters: Ð, Ø, Þ, ß and their
 *  lower cases!  I don't consider the character Æ and its lower case æ to
 *  be a mutation of A and a respectively anymore. 
 */
char _patrieChNoAccent[] = {
  '\000',  '\001',  '\002',  '\003',  '\004',  '\005',  '\006',  '\007',
  '\010',  '\011',  '\012',  '\013',  '\014',  '\015',  '\016',  '\017',
  '\020',  '\021',  '\022',  '\023',  '\024',  '\025',  '\026',  '\027',
  '\030',  '\031',  '\032',  '\033',  '\034',  '\035',  '\036',  '\037',
  '\040',  '\041',  '\042',  '\043',  '\044',  '\045',  '\046',  '\047',
  '\050',  '\051',  '\052',  '\053',  '\054',  '\055',  '\056',  '\057',
  '\060',  '\061',  '\062',  '\063',  '\064',  '\065',  '\066',  '\067',
  '\070',  '\071',  '\072',  '\073',  '\074',  '\075',  '\076',  '\077',
  '\100',  '\101',  '\102',  '\103',  '\104',  '\105',  '\106',  '\107',
  '\150',  '\111',  '\112',  '\113',  '\114',  '\115',  '\116',  '\117',
  '\160',  '\121',  '\122',  '\123',  '\124',  '\125',  '\126',  '\127',
  '\170',  '\131',  '\132',  '\133',  '\134',  '\135',  '\136',  '\137',
  '\140',  '\141',  '\142',  '\143',  '\144',  '\145',  '\146',  '\147',
  '\150',  '\151',  '\152',  '\153',  '\154',  '\155',  '\156',  '\157',
  '\160',  '\161',  '\162',  '\163',  '\164',  '\165',  '\166',  '\167',
  '\170',  '\171',  '\172',  '\173',  '\174',  '\175',  '\176',  '\177',
  '\200',  '\201',  '\202',  '\203',  '\204',  '\205',  '\206',  '\207',
  '\210',  '\211',  '\212',  '\213',  '\214',  '\215',  '\216',  '\217',
  '\220',  '\221',  '\222',  '\223',  '\224',  '\225',  '\226',  '\227',
  '\230',  '\231',  '\232',  '\233',  '\234',  '\235',  '\236',  '\237',
  '\240',  '\241',  '\242',  '\243',  '\244',  '\245',  '\246',  '\247',
  '\250',  '\251',  '\252',  '\253',  '\254',  '\255',  '\256',  '\257',
  '\260',  '\261',  '\262',  '\263',  '\264',  '\265',  '\266',  '\267',
  '\270',  '\271',  '\272',  '\273',  '\274',  '\275',  '\276',  '\277',
  '\101',  '\101',  '\101',  '\101',  '\101',  '\101',  '\306',  '\103',
  '\105',  '\105',  '\105',  '\105',  '\111',  '\111',  '\111',  '\111',
  '\320',  '\116',  '\117',  '\117',  '\117',  '\117',  '\117',  '\327',
  '\117',  '\125',  '\125',  '\125',  '\125',  '\131',  '\336',  '\337',
  '\141',  '\141',  '\141',  '\141',  '\141',  '\141',  '\346',  '\143',
  '\145',  '\145',  '\145',  '\145',  '\151',  '\151',  '\151',  '\151',
  '\360',  '\156',  '\157',  '\157',  '\157',  '\157',  '\157',  '\367',
  '\157',  '\165',  '\165',  '\165',  '\165',  '\171',  '\376',  '\171'
};


int NoAccent(const char c)
{
  return (_patrieChNoAccent[(int)(unsigned char)(c)]);
}

/*  
 *
 *
 *                       String comparison functions.
 *
 *
 */  

int _patrieUStrCaseCmp(const char *s0, const char *s1)
{
  char c0, c1;

  do {
    if (*s0 != *s1) {
      c0 = TOLOWER(*s0);
      c1 = TOLOWER(*s1);

      if (c0 != c1) {
        return (unsigned char)c0 < (unsigned char)c1 ? -1 : 1;
      }
    }
  } while (*s0++ && *s1++);

  return 0;
}

int _patrieUStrCmp(const char *s0, const char *s1)
{
#if 1
  /*
   *  We assume that the two strings do differ.
   */
  
  while (*s0++ == *s1++) {
    continue;
  }

  return (unsigned char)*(s0-1) < (unsigned char)*(s1-1) ? -1 : 1;
  
#else
  
#define CMP(i) \
  do { \
    if (s0[i] != s1[i]) return (unsigned char)s0[i] < (unsigned char)s1[i] ? -1 : 1; \
    else if (s0[i] == '\0')  return 0; \
  } while (0)
#define INC(i) do { s0 += i; s1 += i; } while (0)

  while (1) {
    CMP(0); CMP(1); CMP(2); CMP(3); CMP(4);
    INC(5);
  }

#undef CMP
#undef INC
#endif
}


/*
 *  Compare two characters with no regard to accents on accented charcters.
 *  ie: 'å' is treated as 'a'.
 */

int _patrieUStrAccCmp(const char *s0, const char *s1)
{
  char c0, c1;

  do {
    if (*s0 != *s1) {
      c0 = NoAccent(*s0);
      c1 = NoAccent(*s1);

      if (c0 != c1) {
        return (unsigned char)c0 < (unsigned char)c1 ? -1 : 1;
      }
    }
  } while (*s0++ && *s1++);

  return 0;
}

/*
 *  Takes advantage of the two functions listed above that perform case
 *  insensitive/sensitive and accent insesitive/sensitive comparisons.
 */

/*
 *  Case insensitive - Accent insensitive
 */

int _patrieUStrCiAiCmp(const char *s0, const char *s1)
{
  char c0, c1;

  do {
    if (*s0 != *s1) {
      c0 = NoAccent(*s0);
      c0 = TOLOWER(c0);
      c1 = NoAccent(*s1);
      c1 = TOLOWER(c1);

      if (c0 != c1) {
        return (unsigned char)c0 < (unsigned char)c1 ? -1 : 1;
      }
    }
  } while (*s0++ && *s1++);

  return 0;
}

/*
 *  Case insensitive - Accent sensitive
 */

int _patrieUStrCiAsCmp(const char *s0, const char *s1)
{
  return _patrieUStrCaseCmp( s0, s1 );
}

/*
 *  Case sensitive - Accent insensitive
 */

int _patrieUStrCsAiCmp(const char *s0, const char *s1)
{
  return _patrieUStrAccCmp( s0, s1 );
}

/*
 *  Case sensitive - Accent sensitive
 */

int _patrieUStrCsAsCmp(const char *s0, const char *s1)
{
  return _patrieUStrCmp( s0, s1 );
}

/*  
 *
 *
 *        Functions to find the first differing bit in two strings.
 *
 *
 */  

/*  
 *  Element `i' holds the position of the first set bit in the number `i'.
 *  Bit positions start at 0, which is the most significant bit.
 */
static unsigned int diffTab[256];


void _patrieInitDiffTab(void)
{
  int i;
  
  for (i = 0; i < 256; i++) {
    if      ((i >> 1) == 0) diffTab[i] = 7;
    else if ((i >> 2) == 0) diffTab[i] = 6;
    else if ((i >> 3) == 0) diffTab[i] = 5;
    else if ((i >> 4) == 0) diffTab[i] = 4;
    else if ((i >> 5) == 0) diffTab[i] = 3;
    else if ((i >> 6) == 0) diffTab[i] = 2;
    else if ((i >> 7) == 0) diffTab[i] = 1;
    else if ((i >> 8) == 0) diffTab[i] = 0;
  }
}


/*  
 *  Find the first differing bit in the strings `str0' and `str1'.  We assume
 *  that we will find a differing bit before running off the end of either
 *  array.
 */
long _patrieDiffBit(const char *str0, const char *str1)
{
  size_t i;

  for (i = 0; str0[i] == str1[i]; i++) {
    continue;
  }

  return 1 + i * CHAR_BIT + diffTab[(unsigned char)(str0[i] ^ str1[i])];
}


/*  
 *  Find the first differing bit in the strings `str0' and `str1'.  Letters
 *  differing only in case are treated as equal.
 */
long _patrieDiffBitCase(const char *str0, const char *str1)
{
  char c0, c1;
  size_t i;

  for (i = 0;
       ((c0 = str0[i]) == (c1 = str1[i])) ||
       ((c0 = TOLOWER(c0)) == ((c1 = TOLOWER(c1))));
       i++) {
    continue;
  }

  return 1 + i * CHAR_BIT + diffTab[(unsigned char)(c0 ^ c1)];
}  


/*  
 *  Find the first differing bit in the strings `str0' and `str1'.  Letters
 *  differing only in accents are treated as equal.
 */
long _patrieDiffBitAccent(const char *str0, const char *str1)
{
  char c0, c1;
  size_t i;

  for (i = 0;
       ((c0 = str0[i]) == (c1 = str1[i])) ||
       ((c0 = NoAccent(c0)) == ((c1 = NoAccent(c1))));
       i++) {
    continue;
  }

  return 1 + i * CHAR_BIT + diffTab[(unsigned char)(c0 ^ c1)];
}  


/*  
 *  Find the first differing bit in the strings `str0' and `str1'.  Letters
 *  differing in accents or cases are treated as equal.
 */
long _patrieDiffBitCiAi(const char *str0, const char *str1)
{
  char c0, c1;
  size_t i;

  for (i = 0;
       ((c0 = str0[i]) == (c1 = str1[i])) ||
       ((c0 = NoAccent(c0)) == ((c1 = NoAccent(c1)))) ||
       ((c0 = TOLOWER(c0)) == ((c1 = TOLOWER(c1))));
       i++) {
    continue;
  }

  return 1 + i * CHAR_BIT + diffTab[(unsigned char)(c0 ^ c1)];
}  

/*
 *  For consistency reasons
 */
long _patrieDiffBitCiAs(const char *str0, const char *str1)
{
  return  _patrieDiffBitCase( str0, str1);
}

long _patrieDiffBitCsAi(const char *str0, const char *str1)
{
  return  _patrieDiffBitAccent( str0, str1);
}

long _patrieDiffBitCsAs(const char *str0, const char *str1)
{
  return  _patrieDiffBit( str0, str1);
}
