
#ifndef AUTHUSER_H
#define AUTHUSER_H

extern unsigned short auth_tcpport;

extern char *auth_xline(register char *user, register int fd, register long unsigned int *in);

extern int auth_fd(register int fd, register long unsigned int *in, register short unsigned int *local, register short unsigned int *remote);

extern char *auth_tcpuser(register long unsigned int in, register short unsigned int local, register short unsigned int remote);

#endif
