#include <stdio.h>
#include <ctype.h>

static int extract_href(tag_buff,href_buff )
char *tag_buff;
char *href_buff;
{

   char *tmp;
   tmp = tag_buff;

   while ( *tmp != '\0' ) {
      if ( *tmp == 'h' || *tmp == 'H' ) {
	 if ( strncasecmp(tmp,"href",4) == 0 ){
	    tmp+=4;
	    break;
	 }
      }
      tmp++;
   }
   if ( *tmp == '\0' )
      return 1;

   if ( *tmp != '=' )
      return 1;
   tmp++;

   if ( *tmp == '\0' ) 
      return 1;
   
   while ( *tmp != '\0' ) {
     if ( isspace(*tmp) ){
       tmp++;
       continue;
     }
     
     if ( *tmp == '"' ) {
       if ( *(tmp+1) == '\0' || *(tmp+1) == '"' )
         return 1;
       else {
         tmp++;
         break;
       }
     }
     else
        break;
     tmp++;
   }
     
   if ( *tmp == '\0' ) 
      return 1;

   while ( *tmp != '\0' && *tmp != '"' && *tmp != ' ' ) {
      *href_buff = *tmp;
      tmp++;
      href_buff++;
   }

/*   if ( *tmp == '\0' ) 
      return 1;
*/
   *href_buff = '\0';

   return 0;
}


main()
{
  char t[100],h[100];

  strcpy(t,"HREF=\r \n \tabsolute");
  extract_href(t,h);

  fprintf(stderr,"t,h = --%s-- --%s--",t,h);
}
