/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <perrno.h>
#include <stdio.h>
#include <pfs_threads.h>

/* This file and perrno.h should always be updated simultaneously */

/* Don't declare perrno here
   because some Prospero applications declare it themselves, internally,
   and the two can conflict.
   There is a version of perrno provided in lib/ardp/ardp_perrno.c
   which will be linked in in case of a problem.

   int	perrno = 0;
*/

/* These need to be per-thread in case of two simultaneous errors*/
#ifdef NEVERDEFINED
int	pwarn = 0;
char	*p_err_string = NULL;
char	*p_warn_string = NULL;
#endif

EXTERN_INT_DEF(pwarn);
EXTERN_CHARP_DEF(p_err_string);
EXTERN_CHARP_DEF(p_warn_string);

/* These are initialized in p_initialize(). */

/* This depends upon the internals of lib/ardp/ardp_error.c */

char *ardp___please_do_not_optimize_me_out_of_existence; 
/* See lib/ardp/usc_lic_str.c */
extern char *usc_license_string;


/* This function definition is shadowed in lib/ardp/ardp_error.c.  Change it
   there too if you change it here. */

void
p_clear_errors(void)
{
    ardp___please_do_not_optimize_me_out_of_existence = usc_license_string;
    perrno = PSUCCESS; if (p_err_string) p_err_string[0] = '\0';
    pwarn = PNOWARN; if (p_warn_string) p_warn_string[0] = '\0';
}

char	*p_err_text[256] = {
    /*   0 */ "Success (prospero)",
    /*   1 */ "Port unknown (ardp)",
    /*   2 */ "Can't open local UDP port (ardp)",
    /*   3 */ "Can't resolve hostname (ardp)",
    /*   4 */ "Attempt to send message failed (ardp)",
    /*   5 */ "Select failed (ardp)",
    /*   6 */ "Recvfrom failed (ardp)",
    /*   7 */ "Unknown protocol version number (ardp)",
    /*   8 */ "Inconsistent or unexpected form for request structure (ardp)",
    /*   9 */ "Timed out (ardp)",
    /*  10 */ "Connection refused by server (ardp)",
    /*  11 */ "Generic failure sending or receiving message (ardp)",
    /*  12 */ "Undefined error  12 (prospero)",
    /*  13 */ "Undefined error  13 (prospero)",
    /*  14 */ "Undefined error  14 (prospero)",
    /*  15 */ "Undefined error  15 (prospero)",
    /*  16 */ "Undefined error  16 (prospero)",
    /*  17 */ "Undefined error  17 (prospero)",
    /*  18 */ "Undefined error  18 (prospero)",
    /*  19 */ "Undefined error  19 (prospero)",
    /*  20 */ "Undefined error  20 (prospero)",
    /*  21 */ "Link already exists (vl_insert)",
    /*  22 */ "Link with same name already exists (vl_insert)",
    /*  23 */ "Undefined error  23 (prospero)",
    /*  24 */ "Undefined error  24 (prospero)",
    /*  25 */ "Link already exists (ul_insert)",
    /*  26 */ "Replacing existing link (ul_insert)",
    /*  27 */ "Previous entry not found in dir->ulinks (ul_insert)",
    /*  28 */ "Undefined error  28 (prospero)",
    /*  29 */ "Undefined error  29 (prospero)",
    /*  30 */ "Undefined error  30 (prospero)",
    /*  31 */ "Undefined error  31 (prospero)",
    /*  32 */ "Undefined error  32 (prospero)",
    /*  33 */ "Undefined error  33 (prospero)",
    /*  34 */ "Undefined error  34 (prospero)",
    /*  35 */ "Undefined error  35 (prospero)",
    /*  36 */ "Undefined error  36 (prospero)",
    /*  37 */ "Undefined error  37 (prospero)",
    /*  38 */ "Undefined error  38 (prospero)",
    /*  39 */ "Undefined error  39 (prospero)",
    /*  40 */ "Undefined error  40 (prospero)",
    /*  41 */ "Temporary not found (rd_vdir)",
    /*  42 */ "Namespace not closed with object (rd_vdir)",
    /*  43 */ "Alias for namespace not defined (rd_vdir)",
    /*  44 */ "Specified namespace not found (rd_vdir)",
    /*  45 */ "Undefined error  45 (prospero)",
    /*  46 */ "Undefined error  46 (prospero)",
    /*  47 */ "Undefined error  47 (prospero)",
    /*  48 */ "Undefined error  48 (prospero)",
    /*  49 */ "Undefined error  49 (prospero)",
    /*  50 */ "Undefined error  50 (prospero)",
    /*  51 */ "File access method not supported (pfs_access)",
    /*  52 */ "Undefined error  52 (prospero)",
    /*  53 */ "Undefined error  53 (prospero)",
    /*  54 */ "Undefined error  54 (prospero)",
    /*  55 */ "Pointer to cached copy - delete on close (p__map_cache)",
    /*  56 */ "Unable to retrieve file (p__map_cache)",
    /*  57 */ "Undefined error  57 (prospero)",
    /*  58 */ "Undefined error  58 (prospero)",
    /*  59 */ "Undefined error  59 (prospero)",
    /*  60 */ "Undefined error  60 (prospero)",
    /*  61 */ "Directory already exists (mk_vdir)",
    /*  62 */ "Link with same name already exists (mk_vdir)",
    /*  63 */ "Undefined error  63 (prospero)",
    /*  64 */ "Undefined error  64 (prospero)",
    /*  65 */ "Not a virtual system (vfsetenv)",
    /*  66 */ "Can't find directory (vfsetenv)",
    /*  67 */ "Undefined error  67 (prospero)",
    /*  68 */ "Undefined error  68 (prospero)",
    /*  69 */ "Undefined error  69 (prospero)",
    /*  70 */ "Undefined error  70 (prospero)",
    /*  71 */ "Link already exists (add_vlink)",
    /*  72 */ "Link with same name already exists (add_vlink)",
    /*  73 */ "Undefined error  73 (prospero)",
    /*  74 */ "Undefined error  74 (prospero)",
    /*  75 */ "Undefined error  75 (prospero)",
    /*  76 */ "Undefined error  76 (prospero)",
    /*  77 */ "Undefined error  77 (prospero)",
    /*  78 */ "Undefined error  78 (prospero)",
    /*  79 */ "Undefined error  79 (prospero)",
    /*  80 */ "Undefined error  80 (prospero)",
    /*  81 */ "This link does not refer to an object; can't set an OBJECT \
attribute on it (pset_at)",
    /*  82 */ "Undefined error  82 (prospero)",
    /*  83 */ "Undefined error  83 (prospero)",
    /*  84 */ "Undefined error  84 (prospero)",
    /*  85 */ "Undefined error  85 (prospero)",
    /*  86 */ "Undefined error  86 (prospero)",
    /*  87 */ "Undefined error  87 (prospero)",
    /*  88 */ "Undefined error  88 (prospero)",
    /*  89 */ "Undefined error  89 (prospero)",
    /*  90 */ "Undefined error  90 (prospero)",
    /*  91 */ "Undefined error  91 (prospero)",
    /*  92 */ "Undefined error  92 (prospero)",
    /*  93 */ "Undefined error  93 (prospero)",
    /*  94 */ "Undefined error  94 (prospero)",
    /*  95 */ "Undefined error  95 (prospero)",
    /*  96 */ "Undefined error  96 (prospero)",
    /*  97 */ "Undefined error  97 (prospero)",
    /*  98 */ "Undefined error  98 (prospero)",
    /*  99 */ "Undefined error  99 (prospero)",
    /* 100 */ "Undefined error 100 (prospero)",
    /* 101 */ "Error parsing Prospero Protocol Input",
    /* 102 */ "Undefined error 102 (prospero)",
    /* 103 */ "Undefined error 103 (prospero)",
    /* 104 */ "Undefined error 104 (prospero)",
    /* 105 */ "Undefined error 105 (prospero)",
    /* 106 */ "Undefined error 106 (prospero)",
    /* 107 */ "Undefined error 107 (prospero)",
    /* 108 */ "Undefined error 108 (prospero)",
    /* 109 */ "Undefined error 109 (prospero)",
    /* 110 */ "Undefined error 110 (prospero)",
    /* 111 */ "Undefined error 111 (prospero)",
    /* 112 */ "Undefined error 112 (prospero)",
    /* 113 */ "Undefined error 113 (prospero)",
    /* 114 */ "Undefined error 114 (prospero)",
    /* 115 */ "Undefined error 115 (prospero)",
    /* 116 */ "Undefined error 116 (prospero)",
    /* 117 */ "Undefined error 117 (prospero)",
    /* 118 */ "Undefined error 118 (prospero)",
    /* 119 */ "Undefined error 119 (prospero)",
    /* 120 */ "Undefined error 120 (prospero)",
    /* 121 */ "Undefined error 121 (prospero)",
    /* 122 */ "Undefined error 122 (prospero)",
    /* 123 */ "Undefined error 123 (prospero)",
    /* 124 */ "Undefined error 124 (prospero)",
    /* 125 */ "Undefined error 125 (prospero)",
    /* 126 */ "Undefined error 126 (prospero)",
    /* 127 */ "Undefined error 127 (prospero)",
    /* 128 */ "Undefined error 128 (prospero)",
    /* 129 */ "Undefined error 129 (prospero)",
    /* 130 */ "Undefined error 130 (prospero)",
    /* 131 */ "Undefined error 131 (prospero)",
    /* 132 */ "Undefined error 132 (prospero)",
    /* 133 */ "Undefined error 133 (prospero)",
    /* 134 */ "Undefined error 134 (prospero)",
    /* 135 */ "Undefined error 135 (prospero)",
    /* 136 */ "Undefined error 136 (prospero)",
    /* 137 */ "Undefined error 137 (prospero)",
    /* 138 */ "Undefined error 138 (prospero)",
    /* 139 */ "Undefined error 139 (prospero)",
    /* 140 */ "Undefined error 140 (prospero)",
    /* 141 */ "Undefined error 141 (prospero)",
    /* 142 */ "Undefined error 142 (prospero)",
    /* 143 */ "Undefined error 143 (prospero)",
    /* 144 */ "Undefined error 144 (prospero)",
    /* 145 */ "Undefined error 145 (prospero)",
    /* 146 */ "Undefined error 146 (prospero)",
    /* 147 */ "Undefined error 147 (prospero)",
    /* 148 */ "Undefined error 148 (prospero)",
    /* 149 */ "Undefined error 149 (prospero)",
    /* 150 */ "Undefined error 150 (prospero)",
    /* 151 */ "Undefined error 151 (prospero)",
    /* 152 */ "Undefined error 152 (prospero)",
    /* 153 */ "Undefined error 153 (prospero)",
    /* 154 */ "Undefined error 154 (prospero)",
    /* 155 */ "Undefined error 155 (prospero)",
    /* 156 */ "Undefined error 156 (prospero)",
    /* 157 */ "Undefined error 157 (prospero)",
    /* 158 */ "Undefined error 158 (prospero)",
    /* 159 */ "Undefined error 159 (prospero)",
    /* 160 */ "Undefined error 160 (prospero)",
    /* 161 */ "Undefined error 161 (prospero)",
    /* 162 */ "Undefined error 162 (prospero)",
    /* 163 */ "Undefined error 163 (prospero)",
    /* 164 */ "Undefined error 164 (prospero)",
    /* 165 */ "Undefined error 165 (prospero)",
    /* 166 */ "Undefined error 166 (prospero)",
    /* 167 */ "Undefined error 167 (prospero)",
    /* 168 */ "Undefined error 168 (prospero)",
    /* 169 */ "Undefined error 169 (prospero)",
    /* 170 */ "Undefined error 170 (prospero)",
    /* 171 */ "Undefined error 171 (prospero)",
    /* 172 */ "Undefined error 172 (prospero)",
    /* 173 */ "Undefined error 173 (prospero)",
    /* 174 */ "Undefined error 174 (prospero)",
    /* 175 */ "Undefined error 175 (prospero)",
    /* 176 */ "Undefined error 176 (prospero)",
    /* 177 */ "Undefined error 177 (prospero)",
    /* 178 */ "Undefined error 178 (prospero)",
    /* 179 */ "Undefined error 179 (prospero)",
    /* 180 */ "Undefined error 180 (prospero)",
    /* 181 */ "Undefined error 181 (prospero)",
    /* 182 */ "Undefined error 182 (prospero)",
    /* 183 */ "Undefined error 183 (prospero)",
    /* 184 */ "Undefined error 184 (prospero)",
    /* 185 */ "Undefined error 185 (prospero)",
    /* 186 */ "Undefined error 186 (prospero)",
    /* 187 */ "Undefined error 187 (prospero)",
    /* 188 */ "Undefined error 188 (prospero)",
    /* 189 */ "Undefined error 189 (prospero)",
    /* 190 */ "Undefined error 190 (prospero)",
    /* 191 */ "Undefined error 191 (prospero)",
    /* 192 */ "Undefined error 192 (prospero)",
    /* 193 */ "Undefined error 193 (prospero)",
    /* 194 */ "Undefined error 194 (prospero)",
    /* 195 */ "Undefined error 195 (prospero)",
    /* 196 */ "Undefined error 196 (prospero)",
    /* 197 */ "Undefined error 197 (prospero)",
    /* 198 */ "Undefined error 198 (prospero)",
    /* 199 */ "Undefined error 199 (prospero)",
    /* 200 */ "Undefined error 200 (prospero)",
    /* 201 */ "Undefined error 201 (prospero)",
    /* 202 */ "Undefined error 202 (prospero)",
    /* 203 */ "Undefined error 203 (prospero)",
    /* 204 */ "Undefined error 204 (prospero)",
    /* 205 */ "Undefined error 205 (prospero)",
    /* 206 */ "Undefined error 206 (prospero)",
    /* 207 */ "Undefined error 207 (prospero)",
    /* 208 */ "Undefined error 208 (prospero)",
    /* 209 */ "Undefined error 209 (prospero)",
    /* 210 */ "Undefined error 210 (prospero)",
    /* 211 */ "Undefined error 211 (prospero)",
    /* 212 */ "Undefined error 212 (prospero)",
    /* 213 */ "Undefined error 213 (prospero)",
    /* 214 */ "Undefined error 214 (prospero)",
    /* 215 */ "Undefined error 215 (prospero)",
    /* 216 */ "Undefined error 216 (prospero)",
    /* 217 */ "Undefined error 217 (prospero)",
    /* 218 */ "Undefined error 218 (prospero)",
    /* 219 */ "Undefined error 219 (prospero)",
    /* 220 */ "Undefined error 220 (prospero)",
    /* 221 */ "Undefined error 221 (prospero)",
    /* 222 */ "Undefined error 222 (prospero)",
    /* 223 */ "Undefined error 223 (prospero)",
    /* 224 */ "Undefined error 224 (prospero)",
    /* 225 */ "Undefined error 225 (prospero)",
    /* 226 */ "Undefined error 226 (prospero)",
    /* 227 */ "Undefined error 227 (prospero)",
    /* 228 */ "Undefined error 228 (prospero)",
    /* 229 */ "Undefined error 229 (prospero)",
    /* 230 */ "File not found (prospero)",
    /* 231 */ "Directory not found (prospero)",
    /* 232 */ "Symbolic links nested too deep (prospero)",
    /* 233 */ "Environment not initialized - source vfsetup.source then run vfsetup",
    /* 234 */ "Can't traverse an external file (prospero)",
    /* 235 */ "Forwarding chain is too long (prospero)",
    /* 236 */ "Undefined error 236 (prospero)",
    /* 237 */ "Undefined error 237 (prospero)",
    /* 238 */ "Undefined error 238 (prospero)",
    /* 239 */ "Undefined error 239 (prospero)",
    /* 240 */ "WAIS gateway",
    /* 241 */ "Gopher gateway",
    /* 242 */ "Authentication required (prospero server)",
    /* 243 */ "Not authorized (prospero server)",
    /* 244 */ "Not found (prospero server)",
    /* 245 */ "Bad version number (prospero server)",
    /* 246 */ "Not a directory (prospero server)",
    /* 247 */ "Already exists (prospero server)",
    /* 248 */ "Link with same name already exists (prospero server)",
    /* 249 */ "Too many items to return (prospero server)",
    /* 250 */ "Undefined error 250 (prospero)",
    /* 251 */ "Command not implemented on server (dirsrv)",
    /* 252 */ "Bad format for response (dirsrv)",
    /* 253 */ "Protocol error (prospero server)",
    /* 254 */ "Unspecified server failure (prospero server)",
    /* 255 */ "Generic Failure (prospero)"};

char	*p_warn_text[256] = {
    /*   0 */ "No warning",
    /*   1 */ "You are using an old version of this program",
    /*   2 */ "From server",
    /*   3 */ "Unrecognized line in response from server",
  /* 4-254 */ "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    /* 255 */ ""};

void
perrmesg(char *prefix, int no, char *text)
{
	fprintf(stderr,"%s%s%s%s\n", (prefix ? prefix : ""),
		(no ? p_err_text[no] : p_err_text[perrno]),
		((text ? (*text ? " - " : "") : 
		  (!no && *p_err_string ? " - " : ""))),
		(text ? text : (no ? "" : p_err_string)));
}

void
sperrmesg(char *buf, const char *prefix, int no, const char *text)
{
    sprintf(buf,"%s%s%s%s\n", (prefix ? prefix : ""),
            (no ? p_err_text[no] : p_err_text[perrno]),
            ((text ? (*text ? " - " : "") : 
              (!no && *p_err_string ? " - " : ""))),
            (text ? text : (no ? "" : p_err_string)));
}

void
pwarnmesg(char *prefix, int no, char *text)
{
    fprintf(stderr,"%s%s%s%s\n", (prefix ? prefix : ""),
            (no ? p_warn_text[no] : p_warn_text[pwarn]),
            ((text ? (*text ? " - " : "") : 
              (!no && *p_warn_string ? " - " : ""))),
            (text ? text : (no ? "" : p_warn_string)));
}

void
spwarnmesg(char *buf, char *prefix, int no, char *text)
{
    sprintf(buf,"%s%s%s%s\n", (prefix ? prefix : ""),
            (no ? p_warn_text[no] : p_warn_text[pwarn]),
            ((text ? (*text ? " - " : "") : 
              (!no && *p_warn_string ? " - " : ""))),
            (text ? text : (no ? "" : p_warn_string)));
}

#ifdef DEBUG_PFAILURE
void
it_failed()
{
  static int itfailed = 0;
  itfailed++;   /* So not optimized out of existence */
}
#endif
