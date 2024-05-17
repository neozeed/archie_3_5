#include <stdio.h>
#include <time.h>


static char *wkday[]= {
  "Sun","Mon","Tue","Wed","Thu","Fri","Sat",NULL
};

static char *weekday[] = {
  "Sunday", "Monday","Tueday","Wednesday","Thursday","Friday","Saturday",NULL
};

static char *month[] = {
  "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec", NULL
};




time_t *httptime(str)
char *str;
{

  char day[20],m[20];
  static struct tm stm;
  char **tmp;


  if ( sscanf(str,"%[a-zA-Z], %2d %s %4d %2d:%2d:%2d GMT",
              day,&stm.tm_mday,m,&stm.tm_year,&stm.tm_hour,&stm.tm_min,
              &stm.tm_sec) == 7 ) {
    strptime(str,"%A, %d %h %Y %T",&stm);
  }
  else {
    if ( sscanf(str,"%[a-zA-Z], %2d-%[^-]-%2d %2d:%2d:%2d GMT",
                day,&stm.tm_mday,m,&stm.tm_year,&stm.tm_hour,&stm.tm_min,
                &stm.tm_sec) == 7 ) {
          strptime(str,"%A, %d-%h-%y %T",&stm);
    }
    else { 
      if ( sscanf(str,"%[a-zA-Z] %s %2d %2d:%2d:%2d %4d",
                  day,m,&stm.tm_mday,&stm.tm_hour,&stm.tm_min,&stm.tm_sec,
                  &stm.tm_year) == 7 ) {
        strptime(str,"%A %h %d %T %Y",&stm);
        
      }
      else {
        return 0;
      }
       
    }
  }

  return timegm(&stm);
  
}



main()
{
time_t tt;
  printf("%d\n",httptime("Mon, 02 Jun 1982 00:01:01 GMT"));
  printf("---\n");
  printf("%d\n",httptime("Monday, 02-Jun-82 00:01:01 GMT"));
    printf("---\n");
  printf("%d\n",httptime("Mon Jun 02 00:01:01 1982"));
      printf("---\n");
      printf("---\n");
    

 printf("%d\n",httptime("Wednesday, 17-Jan-96 15:47:52 GMT"));
tt = httptime("Wednesday, 17-Jan-96 15:47:52 GMT");
  printf("%s\n",ctime(&tt));
        printf("---\n");
   printf("%d\n",httptime("Tuesday, 19-Dec-95 23:45:10 GMT"));
tt = httptime("Tuesday, 19-Dec-95 23:45:10 GMT");
     printf("%s\n",ctime(&tt));
        printf("---\n");
 
}
