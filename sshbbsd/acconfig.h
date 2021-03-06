#ifndef _CONFIG_H
#define _CONFIG_H

/* Generated automatically from acconfig.h by autoheader. */
/* Please make your changes there */

@TOP@

/* Define if you want to disable PAM support */
#undef DISABLE_PAM

/* Define if you want to disable AIX4's authenticate function */
#undef WITH_AIXAUTHENTICATE

/* Define if you want to disable lastlog support */
#undef DISABLE_LASTLOG

/* Location of lastlog file */
#undef LASTLOG_LOCATION

/* If lastlog is a directory */
#undef LASTLOG_IS_DIR

/* Location of random number pool  */
#undef RANDOM_POOL

/* Are we using the Entropy gathering daemon */
#undef HAVE_EGD

/* Define if using the Dante SOCKS library. */
#undef HAVE_DANTE

/* Define if using the Socks4 SOCKS library. */
#undef HAVE_SOCKS4

/* Define if using the Socks5 SOCKS library. */
#undef HAVE_SOCKS5

/* Define if you want to install preformatted manpages.*/
#undef MANTYPE

/* Define if your ssl headers are included with #include <ssl/header.h>  */
#undef HAVE_SSL

/* Define if your ssl headers are included with #include <openssl/header.h>  */
#undef HAVE_OPENSSL

/* Define if you are linking against RSAref.  Used only to print the right
 * message at run-time. */
#undef RSAREF

/* struct utmp and struct utmpx fields */
#undef HAVE_HOST_IN_UTMP
#undef HAVE_HOST_IN_UTMPX
#undef HAVE_ADDR_IN_UTMP
#undef HAVE_ADDR_IN_UTMPX
#undef HAVE_ADDR_V6_IN_UTMP
#undef HAVE_ADDR_V6_IN_UTMPX
#undef HAVE_SYSLEN_IN_UTMPX
#undef HAVE_PID_IN_UTMP
#undef HAVE_TYPE_IN_UTMP
#undef HAVE_TV_IN_UTMP
#undef HAVE_ID_IN_UTMP

/* Define if you want to use utmpx */
#undef USE_UTMPX

/* Define is libutil has login() function */
#undef HAVE_LIBUTIL_LOGIN

/* Define if you want external askpass support */
#undef USE_EXTERNAL_ASKPASS

/* Define if libc defines __progname */
#undef HAVE___PROGNAME

/* Define if you want Kerberos 4 support */
#undef KRB4

/* Define if you want AFS support */
#undef AFS

/* Define if you want S/Key support */
#undef SKEY

/* Define if you want TCP Wrappers support */
#undef LIBWRAP

/* Define if your libraries define login() */
#undef HAVE_LOGIN

/* Define if your libraries define daemon() */
#undef HAVE_DAEMON

/* Define if your libraries define getpagesize() */
#undef HAVE_GETPAGESIZE

/* Define if xauth is found in your path */
#undef XAUTH_PATH

/* Define if rsh is found in your path */
#undef RSH_PATH

/* Define if you want to allow MD5 passwords */
#undef HAVE_MD5_PASSWORDS

/* Define if you want to disable shadow passwords */
#undef DISABLE_SHADOW

/* Define if you want have trusted HPUX */
#undef HAVE_HPUX_TRUSTED_SYSTEM_PW

/* Define if you have an old version of PAM which takes only one argument */
/* to pam_strerror */
#undef HAVE_OLD_PAM

/* Set this to your mail directory if you don't have maillock.h */
#undef MAIL_DIRECTORY

/* Data types */
#undef HAVE_INTXX_T
#undef HAVE_U_INTXX_T
#undef HAVE_UINTXX_T
#undef HAVE_SOCKLEN_T
#undef HAVE_SIZE_T
#undef HAVE_STRUCT_SOCKADDR_STORAGE
#undef HAVE_STRUCT_ADDRINFO
#undef HAVE_STRUCT_IN6_ADDR
#undef HAVE_STRUCT_SOCKADDR_IN6

/* Fields in struct sockaddr_storage */
#undef HAVE_SS_FAMILY_IN_SS
#undef HAVE___SS_FAMILY_IN_SS

/* Define if you have /dev/ptmx */
#undef HAVE_DEV_PTMX

/* Define if you have /dev/ptc */
#undef HAVE_DEV_PTS_AND_PTC

/* Define if you need to use IP address instead of hostname in $DISPLAY */
#undef IPADDR_IN_DISPLAY

/* Specify default $PATH */
#undef USER_PATH

/* Specify location of ssh.pid */
#undef PIDDIR

/* Use IPv4 for connection by default, IPv6 can still if explicity asked */
#undef IPV4_DEFAULT

/* getaddrinfo is broken (if present) */
#undef BROKEN_GETADDRINFO

/* Workaround more Linux IPv6 quirks */
#undef DONT_TRY_OTHER_AF

/* Detect IPv4 in IPv6 mapped addresses and treat as IPv4 */
#undef IPV4_IN_IPV6

@BOTTOM@

/* ******************* Shouldn't need to edit below this line ************** */

#include "defines.h"

#endif /* _CONFIG_H */
