#ifndef _INCLUDE_EIXX_CONFIG_H
#define _INCLUDE_EIXX_CONFIG_H 1
 
/* include/eixx/config.h. Generated automatically at end of configure. */
/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Algorithm AES in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_AES
#define EIXX_CRYPTO_WITH_AES 1
#endif

/* Algorithm BF in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_BF
#define EIXX_CRYPTO_WITH_BF 1
#endif

/* Algorithm CAMELLIA in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_CAMELLIA
#define EIXX_CRYPTO_WITH_CAMELLIA 1
#endif

/* Algorithm CAST in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_CAST
#define EIXX_CRYPTO_WITH_CAST 1
#endif

/* Algorithm DES in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_DES
#define EIXX_CRYPTO_WITH_DES 1
#endif

/* Algorithm DH in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_DH
#define EIXX_CRYPTO_WITH_DH 1
#endif

/* Algorithm DSA in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_DSA
#define EIXX_CRYPTO_WITH_DSA 1
#endif

/* Algorithm IDEA in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_IDEA
#define EIXX_CRYPTO_WITH_IDEA 1
#endif

/* Algorithm MD2 in openssl crypto library */
/* #undef CRYPTO_WITH_MD2 */

/* Algorithm MD4 in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_MD4
#define EIXX_CRYPTO_WITH_MD4 1
#endif

/* Algorithm MD5 in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_MD5
#define EIXX_CRYPTO_WITH_MD5 1
#endif

/* Algorithm RC2 in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_RC2
#define EIXX_CRYPTO_WITH_RC2 1
#endif

/* Algorithm RC5 in openssl crypto library */
/* #undef CRYPTO_WITH_RC5 */

/* Algorithm RIPEMD in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_RIPEMD
#define EIXX_CRYPTO_WITH_RIPEMD 1
#endif

/* Algorithm RSA in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_RSA
#define EIXX_CRYPTO_WITH_RSA 1
#endif

/* Algorithm SHA in openssl crypto library */
#ifndef EIXX_CRYPTO_WITH_SHA
#define EIXX_CRYPTO_WITH_SHA 1
#endif

/* define if the Boost library is available */
#ifndef EIXX_HAVE_BOOST
#define EIXX_HAVE_BOOST /**/
#endif

/* define if the Boost::ASIO library is available */
#ifndef EIXX_HAVE_BOOST_ASIO
#define EIXX_HAVE_BOOST_ASIO /**/
#endif

/* define if the Boost::Date_Time library is available */
#ifndef EIXX_HAVE_BOOST_DATE_TIME
#define EIXX_HAVE_BOOST_DATE_TIME /**/
#endif

/* define if the Boost::System library is available */
#ifndef EIXX_HAVE_BOOST_SYSTEM
#define EIXX_HAVE_BOOST_SYSTEM /**/
#endif

/* define if the Boost::Thread library is available */
#ifndef EIXX_HAVE_BOOST_THREAD
#define EIXX_HAVE_BOOST_THREAD /**/
#endif

/* define if the Boost::Unit_Test_Framework library is available */
#ifndef EIXX_HAVE_BOOST_UNIT_TEST_FRAMEWORK
#define EIXX_HAVE_BOOST_UNIT_TEST_FRAMEWORK /**/
#endif

/* Openssl crypto library is available */
#ifndef EIXX_HAVE_CRYPTO
#define EIXX_HAVE_CRYPTO 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#ifndef EIXX_HAVE_DLFCN_H
#define EIXX_HAVE_DLFCN_H 1
#endif

/* erl_interface/src/epmd/ei_epmd.h is available */
#ifndef EIXX_HAVE_EI_EPMD
#define EIXX_HAVE_EI_EPMD 1
#endif

/* Define to 1 if you have the `gettimeofday' function. */
#ifndef EIXX_HAVE_GETTIMEOFDAY
#define EIXX_HAVE_GETTIMEOFDAY 1
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef EIXX_HAVE_INTTYPES_H
#define EIXX_HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef EIXX_HAVE_MEMORY_H
#define EIXX_HAVE_MEMORY_H 1
#endif

/* Define to 1 if you have the <openssl/md5.h> header file. */
#ifndef EIXX_HAVE_OPENSSL_MD5_H
#define EIXX_HAVE_OPENSSL_MD5_H 1
#endif

/* Define to 1 if you have the <openssl/opensslconf.h> header file. */
#ifndef EIXX_HAVE_OPENSSL_OPENSSLCONF_H
#define EIXX_HAVE_OPENSSL_OPENSSLCONF_H 1
#endif

/* Define to 1 if you have the `socket' function. */
#ifndef EIXX_HAVE_SOCKET
#define EIXX_HAVE_SOCKET 1
#endif

/* Define to 1 if you have the `sqrt' function. */
/* #undef HAVE_SQRT */

/* Define to 1 if stdbool.h conforms to C99. */
#ifndef EIXX_HAVE_STDBOOL_H
#define EIXX_HAVE_STDBOOL_H 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef EIXX_HAVE_STDINT_H
#define EIXX_HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef EIXX_HAVE_STDLIB_H
#define EIXX_HAVE_STDLIB_H 1
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifndef EIXX_HAVE_STRINGS_H
#define EIXX_HAVE_STRINGS_H 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#ifndef EIXX_HAVE_STRING_H
#define EIXX_HAVE_STRING_H 1
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef EIXX_HAVE_SYS_STAT_H
#define EIXX_HAVE_SYS_STAT_H 1
#endif

/* Define to 1 if you have the <sys/time.h> header file. */
#ifndef EIXX_HAVE_SYS_TIME_H
#define EIXX_HAVE_SYS_TIME_H 1
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#ifndef EIXX_HAVE_SYS_TYPES_H
#define EIXX_HAVE_SYS_TYPES_H 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef EIXX_HAVE_UNISTD_H
#define EIXX_HAVE_UNISTD_H 1
#endif

/* Define to 1 if the system has the type `_Bool'. */
#ifndef EIXX_HAVE__BOOL
#define EIXX_HAVE__BOOL 1
#endif

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#ifndef EIXX_LT_OBJDIR
#define EIXX_LT_OBJDIR ".libs/"
#endif

/* Name of package */
#ifndef EIXX_PACKAGE
#define EIXX_PACKAGE "eixx"
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef EIXX_PACKAGE_BUGREPORT
#define EIXX_PACKAGE_BUGREPORT "BUG-REPORT-ADDRESS"
#endif

/* Define to the full name of this package. */
#ifndef EIXX_PACKAGE_NAME
#define EIXX_PACKAGE_NAME "eixx"
#endif

/* Define to the full name and version of this package. */
#ifndef EIXX_PACKAGE_STRING
#define EIXX_PACKAGE_STRING "eixx 1.1"
#endif

/* Define to the one symbol short name of this package. */
#ifndef EIXX_PACKAGE_TARNAME
#define EIXX_PACKAGE_TARNAME "eixx"
#endif

/* Define to the home page for this package. */
#ifndef EIXX_PACKAGE_URL
#define EIXX_PACKAGE_URL ""
#endif

/* Define to the version of this package. */
#ifndef EIXX_PACKAGE_VERSION
#define EIXX_PACKAGE_VERSION "1.1"
#endif

/* Define to 1 if you have the ANSI C header files. */
#ifndef EIXX_STDC_HEADERS
#define EIXX_STDC_HEADERS 1
#endif

/* Version number of package */
#ifndef EIXX_VERSION
#define EIXX_VERSION "1.1"
#endif

/* Define for Solaris 2.5.1 so the uint32_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT32_T */

/* Define for Solaris 2.5.1 so the uint64_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT64_T */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to the type of an unsigned integer type of width exactly 32 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint32_t */

/* Define to the type of an unsigned integer type of width exactly 64 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint64_t */
 
/* once: _INCLUDE_EIXX_CONFIG_H */
#endif
