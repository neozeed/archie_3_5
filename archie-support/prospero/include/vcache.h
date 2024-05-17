/* Internal commands used in user/vcache */
void setpeer(char *hostn);
void set_type(char *t);
int recvrequest(char *cmd, char *local, char *remote, char *amode);
int ruserpass(char *host, char **aname, char **apass, char **aacct);
void lostpeer(void);
int login(char *host);
/* Dont define command - it should use vargs, but doesnt */
void pswitch(int flag);

