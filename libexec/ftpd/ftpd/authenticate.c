/* Copyright (c) 1993, 1994  Washington University in Saint Louis
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * Washington University in Saint Louis and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASHINGTON UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASHINGTON
 * UNIVERSITY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "stdio.h"
#include "string.h"
#include "authuser.h"
#include "authenticate.h"

#define AUTHNAMESIZE 100

char authuser[AUTHNAMESIZE];
int authenticated;

authenticate()
{
#if USE_A_RFC931
    unsigned long in;
    unsigned short local, remote;
#endif /* USE_A_RFC931 */
    char *user;

    /* Ideally more authentication schemes would be called from here, with
     * the strongest called first.  One possible double-check would be to
     * verify that the results of all authentication calls (returning
     * identical data!) are checked against each other. */

    authenticated = 0;          /* this is a bitmask, one bit per method */

    user = "*";

#if USE_A_RFC931
    if (auth_fd(0, &in, &local, &remote) == -1)
        user = "?";             /* getpeername/getsockname failure */
    else {
        if (!(user = auth_tcpuser(in, local, remote))) {
            user = "*";         /* remote host doesn't support RFC 931 */
        } else {
            authenticated |= A_RFC931;
        }
    }
#endif /* USE_A_RFC931 */

    strncpy(authuser, user, sizeof(authuser));
    authuser[AUTHNAMESIZE - 1] = '\0';
}
