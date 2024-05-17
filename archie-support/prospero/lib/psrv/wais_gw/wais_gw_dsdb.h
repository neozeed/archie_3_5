extern int wais_gw_dsdb(RREQ    req,     /* Request pointer (unused) */
            char    *hsoname,   /* Name of the directory                 */
            long version,       /* Version #; currently ignored */
            long magic_no,      /* Magic #; currently ignored */
            int flags,          /* Currently only recognize DRO_VERIFY */
            struct dsrobject_list_options *listopts, /* options (use *remcompp
                                                        and *thiscompp) */
            P_OBJECT    ob);     /* Object to be filled in */
