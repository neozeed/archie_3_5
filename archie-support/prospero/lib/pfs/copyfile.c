#include <errno.h>
#include <perrno.h>
#include <pfs.h>
#include <stdio.h>		/* FILE, fopen etc */

int
copyFile(char *source,char *destn) 
{
    /* This code is adapted from user/vget:copy_file, but errors are now
       not put on standard out, and it doesnt use "goto"  */
    char buf[1024];
    int numread,numwritten;
    FILE *in, *out;
    if ((in = fopen(source, "r")) == NULL) {
	p_err_string = qsprintf_stcopyr(p_err_string, 
		"Couldn't open the file %s for reading.  Copy failed.\n", 
		source);
        RETURNPFAILURE;
    } 
    if ((out = fopen(destn, "w")) == NULL) {
	p_err_string = qsprintf_stcopyr(p_err_string, 
        	"Couldn't open the file %s for writing.  Copy failed.\n", 
		destn);
        fclose(in);
        RETURNPFAILURE;
    }
    while ((numread = fread(buf, 1, sizeof buf, in)) >0 ) {
      numwritten = fwrite(buf, 1, numread, out);
      if (numwritten != numread) {
	p_err_string = qsprintf_stcopyr(p_err_string, 
            "tried to write %d items to %s; only wrote %d. Copying aborted.\n",
	     numread, destn, numwritten);
        fclose(in); 
        fclose(out);
        RETURNPFAILURE;
      }
    }
    if (ferror(in)) {
	    p_err_string = qsprintf_stcopyr(p_err_string, 
            	"Error while reading from %s; copying to %s aborted\n", 
		source, destn);
            fclose(in); 
            fclose(out);
            RETURNPFAILURE;
    } else {
            /* done! */
            fclose(in);
            fclose(out);
            return PSUCCESS;
    }
}

#include <errno.h>

int
renameOrCopyAndDelete(char *source, char *destn)
{
    int retval;
    if (!(rename(source,destn)))
            return 0;

    switch (errno) {
        case EXDEV:
            if (retval = copyFile(source,destn))
                    return retval;	/* Couldnt copy , p_err_string set*/

            if(retval = unlink(source)) {
                p_err_string = qsprintf_stcopyr(p_err_string,
                    "Couldnt unlink %s: %s",source, unixerrstr());
            }
            return(retval);	/* success or failure */
        default: {
                p_err_string = qsprintf_stcopyr(p_err_string,
                    "Couldnt rename %s to %s: %s",
                    source,destn, unixerrstr());
            return -1;	/* Failed to rename for another reason*/
        }
    }
}
