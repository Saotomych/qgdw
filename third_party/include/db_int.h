/*-
 * Copyright (c) 1996, 2020 Oracle and/or its affiliates.  All rights reserved.
 *
 * See the file LICENSE for license information.
 *
 * $Id$
 */

#ifndef _DB_INT_H_
#define	_DB_INT_H_

/*******************************************************
 * Berkeley DB ANSI/POSIX include files.
 *******************************************************/
#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <sys/types.h>
#ifdef DIAG_MVCC
#include <sys/mman.h>
#endif
#include <sys/stat.h>

#if defined(HAVE_REPLICATION_THREADS)
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_VXWORKS
#include <selectLib.h>
#endif
#endif

#if defined(HAVE_REPMGR_SSL_SUPPORT) && defined(HAVE_REPLICATION_THREADS)
#ifdef HAVE_OPENSSL_SSL_H
#include <openssl/ssl.h>
#endif
#ifdef HAVE_OPENSSL_ERR_H
#include <openssl/err.h>
#endif
#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>
#if OPENSSL_VERSION_NUMBER < 0x10001000L
	#error "OpenSSL >= 1.0.1 required."
#endif
#if defined(HAVE_OPENSSL_SSL_H) && defined(HAVE_OPENSSL_ERR_H) && defined(OPENSSL_THREADS)
#define HAVE_REPMGR_SSL 1
#endif
#endif

#if defined(HAVE_REPLICATION_THREADS)
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#endif

#if defined(HAVE_REPLICATION_THREADS)
#ifdef HAVE_SYS_EPOLL_H
#include <sys/epoll.h>
#endif
#endif

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#ifndef HAVE_VXWORKS
#include <sys/uio.h>
#endif

#if defined(HAVE_REPLICATION_THREADS)
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include <stdarg.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#endif /* !HAVE_SYSTEM_INCLUDE_FILES */

/*
 * The Windows compiler needs to be told about structures that are available
 * outside a dll.
 */
#if defined(DB_WIN32) && defined(_MSC_VER) && \
    !defined(DB_CREATE_DLL) && !defined(_LIB)
#define	__DB_IMPORT __declspec(dllimport)
#else
#define	__DB_IMPORT
#endif

#ifdef DB_WIN32
#include "dbinc/win_db.h"

#if defined(HAVE_REPMGR_SSL_SUPPORT) && defined(HAVE_REPLICATION_THREADS)
#ifdef HAVE_OPENSSL_SSL_H
#include "openssl/ssl.h"
#endif
#ifdef HAVE_OPENSSL_ERR_H
#include "openssl/err.h"
#endif
#ifdef HAVE_OPENSSL_OPENSSLCONF_H
#define OPENSSL_THREAD_DEFINES
#include "openssl/opensslconf.h"
#endif
#if OPENSSL_VERSION_NUMBER < 0x10001000L
	#error "OpenSSL >= 1.0.1 required."
#endif
#if defined(HAVE_OPENSSL_SSL_H) && defined(HAVE_OPENSSL_ERR_H) && defined(OPENSSL_THREADS)
#define HAVE_REPMGR_SSL 1
#endif
#endif

#endif

#ifdef HAVE_DBM
#undef	DB_DBM_HSEARCH
#define	DB_DBM_HSEARCH 1
#endif

#include "db.h"
#include "clib_port.h"

#include "dbinc/queue.h"
#include "dbinc/shqueue.h"
#include "dbinc/perfmon.h"
#include "dbinc/clock.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************
 * Forward structure declarations.
 *******************************************************/
struct __db_commit_info; typedef struct __db_commit_info DB_COMMIT_INFO;
struct __db_reginfo_t;	typedef struct __db_reginfo_t REGINFO;
struct __db_txnhead;	typedef struct __db_txnhead DB_TXNHEAD;
struct __db_txnlist;	typedef struct __db_txnlist DB_TXNLIST;
struct __vrfy_childinfo;typedef struct __vrfy_childinfo VRFY_CHILDINFO;
struct __vrfy_dbinfo;   typedef struct __vrfy_dbinfo VRFY_DBINFO;
struct __vrfy_pageinfo; typedef struct __vrfy_pageinfo VRFY_PAGEINFO;

struct __db_log_verify_info;
struct __txn_verify_info;
struct __lv_filereg_info;
struct __lv_ckp_info;
struct __lv_timestamp_info;
typedef struct __db_log_verify_info DB_LOG_VRFY_INFO;
typedef struct __txn_verify_info VRFY_TXN_INFO;
typedef struct __lv_filereg_info VRFY_FILEREG_INFO;
typedef struct __lv_filelife VRFY_FILELIFE;
typedef struct __lv_ckp_info VRFY_CKP_INFO;
typedef struct __lv_timestamp_info VRFY_TIMESTAMP_INFO;

/*
 * TXNINFO_HANDLER --
 *	Callback function pointer type for __iterate_txninfo.
 */
typedef int (*TXNINFO_HANDLER) __P((DB_LOG_VRFY_INFO *, VRFY_TXN_INFO *, void *));

typedef SH_TAILQ_HEAD(__hash_head) DB_HASHTAB;

/*******************************************************
 * General purpose constants and macros.
 *******************************************************/
#undef	FALSE
#define	FALSE		0
#undef	TRUE
#define	TRUE		(!FALSE)

#define	MEGABYTE	1048576
#define	GIGABYTE	1073741824

#define	NS_PER_MS	1000000		/* Nanoseconds in a millisecond */
#define	NS_PER_US	1000		/* Nanoseconds in a microsecond */
#define	NS_PER_SEC	1000000000	/* Nanoseconds in a second */
#define	US_PER_MS	1000		/* Microseconds in a millisecond */
#define	US_PER_SEC	1000000		/* Microseconds in a second */
#define	MS_PER_SEC	1000		/* Milliseconds in a second */

#define	RECNO_OOB	0		/* Illegal record number. */

/*
 * NOP_STATEMENT has no runtime effect, yet avoids triggering empty statement
 * compiler warnings.  Use it as the text of conditionally-null macros.
 */
#define	NOP_STATEMENT	do { } while (0)

/* Test for a power-of-two (tests true for zero, which doesn't matter here). */
#define	POWER_OF_TWO(x)	(((x) & ((x) - 1)) == 0)

/* Test for valid page sizes. */
#define	DB_MIN_PGSIZE	0x000200	/* Minimum page size (512). */
#define	DB_MAX_PGSIZE	0x010000	/* Maximum page size (65536). */
#define	IS_VALID_PAGESIZE(x)						\
	(POWER_OF_TWO(x) && (x) >= DB_MIN_PGSIZE && ((x) <= DB_MAX_PGSIZE))

/* Minimum number of pages cached, by default. */
#define	DB_MINPAGECACHE	16

/*
 * Increase the cachesize by 25% and factor in the the size of the hash buckets
 * to account for our overhead if the application requests less than 500MB.
 */
#define MEMP_SMALLCACHE_ADJUST(nbytes)					\
	(((nbytes) * 5) / 4 + __db_tablesize(32) * sizeof(DB_MPOOL_HASH))

/*
 * If we are unable to determine the underlying filesystem block size, use
 * 8K on the grounds that most OS's use less than 8K for a VM page size.
 */
#define	DB_DEF_IOSIZE	(8 * 1024)

/* Align an integer to a specific boundary. */
#undef	DB_ALIGN
#define	DB_ALIGN(v, bound)						\
	(((v) + (bound) - 1) & ~(((uintmax_t)(bound)) - 1))

/* Increment a pointer to a specific boundary. */
#undef	ALIGNP_INC
#define	ALIGNP_INC(p, bound)						\
	(void *)(((uintptr_t)(p) + (bound) - 1) & ~(((uintptr_t)(bound)) - 1))

/*
 * DB_ALIGN8 adjusts structure alignments to make sure shared structures have
 * fixed size and filed offset on both 32bit and 64bit platforms when
 * HAVE_MIXED_SIZE_ADDRESSING is defined.
 */
#ifdef HAVE_MIXED_SIZE_ADDRESSING
#define DB_ALIGN8 __attribute__ ((aligned (8)))
#else
#define DB_ALIGN8
#endif

/*
 * Berkeley DB uses the va_copy macro from C99, not all compilers include
 * it, so add a dumb implementation compatible with pre C99 implementations.
 */
#ifndef	va_copy
#define	va_copy(d, s)	((d) = (s))
#endif

/*
 * Print an address as a u_long (a u_long is the largest type we can print
 * portably).  Most 64-bit systems have made longs 64-bits, so this should
 * work.
 */
#define	P_TO_ULONG(p)	((u_long)(uintptr_t)(p))

/*
 * Convert a pointer to an integral value.
 *
 * The (u_int16_t)(uintptr_t) cast avoids warnings: the (uintptr_t) cast
 * converts the value to an integral type, and the (u_int16_t) cast converts
 * it to a small integral type so we don't get complaints when we assign the
 * final result to an integral type smaller than uintptr_t.
 */
#define	P_TO_UINT32(p)	((u_int32_t)(uintptr_t)(p))
#define	P_TO_UINT16(p)	((u_int16_t)(uintptr_t)(p))
#define	P_TO_ROFF(p)	((roff_t)(uintptr_t)(p))

/* The converse of P_TO_ROFF() above. */
#define	ROFF_TO_P(roff)	((void *)(uintptr_t)(roff))

/*
 * There are several on-page structures that are declared to have a number of
 * fields followed by a variable length array of items.  The structure size
 * without including the variable length array or the address of the first of
 * those elements can be found using SSZ.
 *
 * This macro can also be used to find the offset of a structure element in a
 * structure.  This is used in various places to copy structure elements from
 * unaligned memory references, e.g., pointers into a packed page.
 *
 * There are two versions because compilers object if you take the address of
 * an array.
 */
#undef	SSZ
#define	SSZ(name, field)  P_TO_UINT16(&(((name *)0)->field))

#undef	SSZA
#define	SSZA(name, field) P_TO_UINT16(&(((name *)0)->field[0]))

/* Structure used to print flag values. */
typedef struct __fn {
	u_int32_t mask;			/* Flag value. */
	const char *name;		/* Flag name. */
} FN;

/* Set, clear and test flags. */
#define	FLD_CLR(fld, f)		(fld) &= ~(f)
#define	FLD_ISSET(fld, f)	((fld) & (f))
#define	FLD_SET(fld, f)		(fld) |= (f)
#define	F_CLR(p, f)		(p)->flags &= ~(f)
#define	F_ISSET(p, f)		((p)->flags & (f))
#define	F_SET(p, f)		(p)->flags |= (f)
#define	F2_CLR(p, f)		((p)->flags2 &= ~(f))
#define	F2_ISSET(p, f)		((p)->flags2 & (f))
#define	F2_SET(p, f)		((p)->flags2 |= (f))
#define	LF_CLR(f)		((flags) &= ~(f))
#define	LF_ISSET(f)		((flags) & (f))
#define	LF_SET(f)		((flags) |= (f))

/*
 * Calculate a percentage.  The values can overflow 32-bit integer arithmetic
 * so we use floating point.
 *
 * When calculating a bytes-vs-page size percentage, we're getting the inverse
 * of the percentage in all cases, that is, we want 100 minus the percentage we
 * calculate.
 */
#define	DB_PCT(v, total)						\
	((int)((total) == 0 ? 0 : ((double)(v) * 100) / (total)))
#define	DB_PCT_PG(v, total, pgsize)					\
	((int)((total * pgsize) == 0 ? 0 :				\
	    100 - ((double)(v) * 100) / (((double)total) * (pgsize))))

/*
 * Statistics update shared memory and so can be expensive -- don't update the
 * values unless we're going to display the results.
 *
 * STAT_INC() increases a statistics value by one.
 * STAT_DEC() decreases it by one.
 * STAT_ADJUST() changes it by a given delta. 
 * STAT_SET() overwrites it with a brand new value.
 *
 * Performance monitoring (--enable-perfmon) allows the changed value to be
 * published (via DTrace or SystemTap)) along with another associated value or
 * two.  Each monitored event has a two-part name (category, subcategory) to
 * identify the value.  When defining a new statistic, a new subcategory will
 * probably be needed.  Users of these macros should check that 'cat' and
 * 'subcat' have corresponding entries in dist/events.in.
 *
 * The STAT_XXX_VERB() macros differ only by including an additional value in
 * the DTrace or SystemTap event; typically that provides additional context.
 */
#undef	STAT
#ifdef	HAVE_STATISTICS
#define	STAT(x)	x
#define	STAT_ADJUST(env, cat, subcat, val, amount, id)			\
	do {								\
		(val) += (amount);					\
		STAT_PERFMON2((env), cat, subcat, (val), (id));		\
	} while (0)
#define	STAT_ADJUST_VERB(env, cat, subcat, val, amount, id1, id2)	\
	do {								\
		(val) += (amount);					\
		STAT_PERFMON3((env), cat, subcat, (val), (id1), (id2));	\
	} while (0)

#define	STAT_INC(env, cat, subcat, val, id) 				\
	STAT_ADJUST(env, cat, subcat, (val), 1, (id))
#define	STAT_INC_VERB(env, cat, subcat, val, id1, id2) 			\
	STAT_ADJUST_VERB((env), cat, subcat, (val), 1, (id1), (id2))
/*
 * STAT_DEC() subtracts one rather than adding (-1) with STAT_ADJUST(); the
 * latter can generate a compilation warning for an unsigned value.
 */
#define	STAT_DEC(env, cat, subcat, val, id) 				\
	do {								\
		(val)--;						\
		STAT_PERFMON2((env), cat, subcat, (val), (id));		\
	} while (0)
/* Add STAT_DEC_VERB() here, if it is ever needed. */

#define	STAT_SET(env, cat, subcat, val, newval, id) 			\
	do {								\
		(val) = (newval);					\
		STAT_PERFMON2((env), cat, subcat, (val), (id));		\
	} while (0)
#define	STAT_SET_VERB(env, cat, subcat, val, newval, id1, id2) 		\
	do {								\
		(val) = (newval);					\
		STAT_PERFMON3((env), cat, subcat, (val), (id1), (id2));	\
	} while (0)
#else
#define	STAT(x)							NOP_STATEMENT
#define	STAT_ADJUST(env, cat, subcat, val, amt, id)		NOP_STATEMENT
#define	STAT_ADJUST_VERB(env, cat, subcat, val, amt, id1, id2)	NOP_STATEMENT
#define	STAT_INC(env, cat, subcat, val, id)			NOP_STATEMENT
#define	STAT_INC_VERB(env, cat, subcat, val, id1, id2)		NOP_STATEMENT
#define	STAT_DEC(env, cat, subcat, val, id)			NOP_STATEMENT
#define	STAT_SET(env, cat, subcat, val, newval, id)		NOP_STATEMENT
#define	STAT_SET_VERB(env, cat, subcat, val, newval, id1, id2)	NOP_STATEMENT
#endif

#if defined HAVE_SIMPLE_THREAD_TYPE
#define DB_THREADID_INIT(t)	COMPQUIET((t), 0)
#else
#define DB_THREADID_INIT(t)	memset(&(t), 0, sizeof(t))
#endif

/*
 * These macros are used when an error condition is first noticed.  They allow
 * one to be notified (via e.g. DTrace, SystemTap, ...) when an error occurs
 * deep inside DB, rather than when it is returned back through the API.
 *
 * The second actual argument to these is the second part of the error or
 * warning event name.  They work when 'errcode' is a symbolic name e.g.
 * EINVAL or DB_LOCK_DEALOCK, not a variable.  Noticing system call failures
 * would be handled by tracing on syscall exit; when e.g., it returns < 0.
 */
#define	ERR_ORIGIN(env, errcode)        				\
	(PERFMON0(env, error, errcode), errcode)

#define	ERR_ORIGIN_MSG(env, errcode, msg)				\
	(PERFMON1(env, error, errcode, msg), errcode)

#define	WARNING_ORIGIN(env, errcode)					\
	(PERFMON0(env, warning, errcode), errcode)

/*
 * Structure used for callback message aggregation.
 *
 * DB_MSGBUF_FLUSH displays values in XXX_stat_print calls.
 * DB_MSGBUF_REP_FLUSH displays replication system messages.
 */
typedef struct __db_msgbuf {
	char *buf;			/* Heap allocated buffer. */
	char *cur;			/* Current end of message. */
	size_t len;			/* Allocated length of buffer. */
	int flags;
} DB_MSGBUF;
#define DB_MSGBUF_PREALLOCATED		0x0001

#define	DB_MSGBUF_INIT(a) do {						\
	(a)->buf = (a)->cur = NULL;					\
	(a)->len = (a)->flags = 0;					\
} while (0)

#define	DB_MSGBUF_INIT_BUFFER(a, buffer, length) do {			\
	(a)->buf = (a)->cur = (buffer);					\
	(a)->len = (length); 						\
	(a)->flags = DB_MSGBUF_PREALLOCATED;				\
} while (0)

#define	DB_MSGBUF_FLUSH(env, a) do {					\
	if ((a)->buf != NULL) {						\
		if ((a)->cur != (a)->buf)				\
			__db_msg(env, "%s", (a)->buf);			\
 		if (!F_ISSET((a), DB_MSGBUF_PREALLOCATED))		\
 			__os_free(env, (a)->buf);			\
		DB_MSGBUF_INIT(a);					\
	}								\
} while (0)
#define	DB_MSGBUF_REP_FLUSH(env, a, diag_msg, regular_msg) do {		\
	if ((a)->buf != NULL) {						\
		if ((a)->cur != (a)->buf && diag_msg)			\
			__db_repmsg(env, "%s", (a)->buf);		\
		if (regular_msg)					\
			DB_MSGBUF_FLUSH(env, a);			\
		else {							\
 			if (!F_ISSET((a), DB_MSGBUF_PREALLOCATED))	\
 				__os_free(env, (a)->buf);		\
			DB_MSGBUF_INIT(a);				\
		}							\
	}								\
} while (0)
#define	STAT_FMT(msg, fmt, type, v)					\
	__db_msg(env, fmt "\t%s", (type)(v), msg);
#define	STAT_HEX(msg, v)						\
	__db_msg(env, "%#lx\t%s", (u_long)(v), msg)
#define	STAT_ISSET(msg, p)						\
	__db_msg(env, "%sSet\t%s", (p) == NULL ? "!" : " ", msg)
#define	STAT_LONG(msg, v)						\
	__db_msg(env, "%ld\t%s", (long)(v), msg)
#define	STAT_LSN(msg, lsnp)						\
	__db_msg(env, "%lu/%lu\t%s",					\
	    (u_long)(lsnp)->file, (u_long)(lsnp)->offset, msg)
#define	STAT_POINTER(msg, v)						\
	__db_msg(env, "%#lx\t%s", P_TO_ULONG(v), msg)
#define	STAT_STRING(msg, p) do {					\
	const char *__p = p;	/* p may be a function call. */		\
	__db_msg(env, "%s\t%s", __p == NULL ? "!Set" : __p, msg);	\
} while (0)
#define	STAT_ULONG(msg, v)						\
	__db_msg(env, "%lu\t%s", (u_long)(v), msg)

/*
 * The following macros are used to control how error and message strings are
 * output by Berkeley DB.  There are essentially three different controls
 * available:
 *  - Default behavior is to output error strings with its unique identifier.
 *  - If HAVE_STRIPPED_MESSAGES is enabled, a unique identifier along with any
 *    parameters to the error string will be output.
 *  - If HAVE_LOCALIZATION is defined, and the '_()' macro is implemented, a
 *    gettext or ICU style translation will be done.
 *
 * Each new string that will be output should be wrapped in a DB_STR* macro.
 * There are three versions of this macro for different scenarions:
 *  - DB_STR for strings that need an identifier, and don't have any argument.
 *  - DB_STR_A for strings that need an identifier, and have argument(s).
 *  - DB_STR_P for strings that don't need an identifier, and don't have
 *    arguments.
 *
 * Error message IDs are automatically assigned by dist/s_message_id script.
 */
#ifdef HAVE_STRIPPED_MESSAGES
#define DB_STR_C(msg, fmt)	fmt
#else
#define DB_STR_C(msg, fmt)	msg
#endif

#ifdef HAVE_LOCALIZATION
#define _(msg)	(msg)	/* Replace with localization function. */
#else
#define _(msg)	msg
#endif

#define DB_STR(id, msg)			_("BDB" id " " DB_STR_C(msg, ""))
#define DB_STR_A(id, msg, fmt)	_("BDB" id " " DB_STR_C(msg, fmt))
#define DB_STR_P(msg)			_(msg)

/*
 * There are quite a few places in Berkeley DB where we want to initialize
 * a DBT from a string or other random pointer type, using a length typed
 * to size_t in most cases.  This macro avoids a lot of casting.  The macro
 * comes in two flavors because we often want to clear the DBT first.
 */
#define	DB_SET_DBT(dbt, d, s)  do {					\
	(dbt).data = (void *)(d);					\
	(dbt).size = (u_int32_t)(s);					\
} while (0)
#define	DB_INIT_DBT(dbt, d, s)  do {					\
	memset(&(dbt), 0, sizeof(dbt));					\
	DB_SET_DBT(dbt, d, s);						\
} while (0)

#define	DB_INIT_DBT_USERMEM(dbt, d, s)  do {				\
	memset(&(dbt), 0, sizeof(dbt));					\
	(dbt).data = (void *)(d);					\
	(dbt).ulen = (u_int32_t)(s);					\
	(dbt).flags = DB_DBT_USERMEM;					\
} while (0)

/*
 * char *__db_tohex(source, len, buf) 'prints' the bytes of source as a hex
 * string into buf.  The result buffer needs to be at least this large.
 */
#define DB_TOHEX_BUFSIZE(len)	(2 * (len) + 1)

/*******************************************************
 * API return values
 *******************************************************/
/*
 * Return values that are OK for each different call.  Most calls have a
 * standard 'return of 0 is only OK value', but some, like db->get have
 * DB_NOTFOUND as a return value, but it really isn't an error.
 */
#define	DB_RETOK_STD(ret)	((ret) == 0)
#define	DB_RETOK_DBCDEL(ret)	((ret) == 0 || (ret) == DB_KEYEMPTY || \
				    (ret) == DB_NOTFOUND)
#define	DB_RETOK_DBCGET(ret)	((ret) == 0 || (ret) == DB_KEYEMPTY || \
				    (ret) == DB_NOTFOUND)
#define	DB_RETOK_DBCPUT(ret)	((ret) == 0 || (ret) == DB_KEYEXIST || \
				    (ret) == DB_NOTFOUND)
#define	DB_RETOK_DBDEL(ret)	DB_RETOK_DBCDEL(ret)
#define	DB_RETOK_DBGET(ret)	DB_RETOK_DBCGET(ret)
#define	DB_RETOK_DBPUT(ret)	((ret) == 0 || (ret) == DB_KEYEXIST)
#define	DB_RETOK_EXISTS(ret)	DB_RETOK_DBCGET(ret)
#define	DB_RETOK_LGGET(ret)	((ret) == 0 || (ret) == DB_NOTFOUND)
#define	DB_RETOK_MPGET(ret)	((ret) == 0 || (ret) == DB_PAGE_NOTFOUND)
#define	DB_RETOK_REPPMSG(ret)	((ret) == 0 || \
				    (ret) == DB_REP_IGNORE || \
				    (ret) == DB_REP_ISPERM || \
				    (ret) == DB_REP_NEWMASTER || \
				    (ret) == DB_REP_NEWSITE || \
				    (ret) == DB_REP_NOTPERM || \
				    (ret) == DB_REP_WOULDROLLBACK)
#define	DB_RETOK_REPMGR_LOCALSITE(ret)	((ret) == 0 || (ret) == DB_NOTFOUND)
#define	DB_RETOK_REPMGR_START(ret) ((ret) == 0 || (ret) == DB_REP_IGNORE)
#define	DB_RETOK_TXNAPPLIED(ret) ((ret) == 0 || \
				    (ret) == DB_NOTFOUND ||		\
				    (ret) == DB_TIMEOUT ||		\
				    (ret) == DB_KEYEMPTY)

/* Find a reasonable operation-not-supported error. */
#ifdef	EOPNOTSUPP
#define	DB_OPNOTSUP	EOPNOTSUPP
#else
#ifdef	ENOTSUP
#define	DB_OPNOTSUP	ENOTSUP
#else
#define	DB_OPNOTSUP	EINVAL
#endif
#endif

/*******************************************************
 * Files.
 *******************************************************/
/*
 * We use 1024 as the maximum path length.  It's too hard to figure out what
 * the real path length is, as it was traditionally stored in <sys/param.h>,
 * and that file isn't always available.
 */
#define	DB_MAXPATHLEN	1024

#define	PATH_DOT	"."	/* Current working directory. */
				/* Path separator character(s). */
#define	PATH_SEPARATOR	"/"

/*******************************************************
 * Environment.
 *******************************************************/
/* Type passed to __db_appname(). */
typedef enum {
	DB_APP_NONE=0,			/* No type (region). */
	DB_APP_BLOB,			/* Blob file. */
	DB_APP_DATA,			/* Data file. */
	DB_APP_LOG,			/* Log file. */
	DB_APP_META,			/* Persistent metadata file. */
	DB_APP_RECOVER,			/* We are in recovery. */
	DB_APP_TMP,			/* Temporary file. */
	DB_APP_REGION			/* Region file. */
} APPNAME;

/*
 * A set of macros to check if various functionality has been configured.
 *
 * ALIVE_ON	The is_alive function is configured.
 * CDB_LOCKING	CDB product locking.
 * CRYPTO_ON	Security has been configured.
 * LOCKING_ON	Locking has been configured.
 * LOGGING_ON	Logging has been configured.
 * MUTEX_ON	Mutexes have been configured.
 * MPOOL_ON	Memory pool has been configured.
 * REP_ON	Replication has been configured.
 * TXN_ON	Transactions have been configured.
 *
 * REP_ON is more complex than most: if the BDB library was compiled without
 * replication support, ENV->rep_handle will be NULL; if the BDB library has
 * replication support, but it was not configured, the region reference will
 * be NULL.
 */
#define	ALIVE_ON(env)		((env)->dbenv->is_alive != NULL)
#define	CDB_LOCKING(env)	F_ISSET(env, ENV_CDB)
#define	CRYPTO_ON(env)		((env)->crypto_handle != NULL)
#define	LOCKING_ON(env)		((env)->lk_handle != NULL)
#define	LOGGING_ON(env)		((env)->lg_handle != NULL)
#define	MPOOL_ON(env)		((env)->mp_handle != NULL)
#define	MUTEX_ON(env)		((env)->mutex_handle != NULL)
#define	REP_ON(env)							\
	((env)->rep_handle != NULL && (env)->rep_handle->region != NULL)
#define	TXN_ON(env)		((env)->tx_handle != NULL)

/* Determine whether this environment is an active user of slices. */
#ifdef HAVE_SLICES
#define SLICES_ON(env)		((env)->slice_envs != NULL)
/*
 * SLICE_FOREACH precedes a statement (usually a { ... } block) that is to be
 * executed once for each slice.  It skips the statement block in non-sliced
 * environments or if slices are not enabled.  If the statement is followed
 * by an 'else', then enclose the entire SLICE_FOREACH() <statement> in { }
 * in order to shield the 'else' from being tied to the 'if' below.
 */
#define SLICE_FOREACH(dbenv, slice, pos)				\
	if (SLICES_ON((dbenv)->env)) 					\
	    for ((pos) = -1;						\
		((slice) = __slice_iterate((dbenv), &(pos))) != NULL; )
#else
#define SLICES_ON(env)		(0)
#define SLICE_FOREACH(dbenv, slice, pos)				\
	COMPQUIET((slice), NULL);					\
	COMPQUIET((pos), 0);						\
	for (; 0; )
#endif

/*
 * STD_LOCKING	Standard locking, that is, locking was configured and CDB
 *		was not.  We do not do locking in off-page duplicate trees,
 *		so we check for that in the cursor first.
 */
#define	STD_LOCKING(dbc)						\
	(!F_ISSET(dbc, DBC_OPD) &&					\
	    !CDB_LOCKING((dbc)->env) && LOCKING_ON((dbc)->env))

/*
 * IS_RECOVERING: The system is running recovery.
 */
#define	IS_RECOVERING(env)						\
	(LOGGING_ON(env) && F_ISSET((env)->lg_handle, DBLOG_RECOVER))

/* Initialization methods are often illegal before/after open is called. */
#define	ENV_ILLEGAL_AFTER_OPEN(env, name)				\
	if (F_ISSET((env), ENV_OPEN_CALLED))				\
		return (__db_mi_open(env, name, 1));
#define	ENV_ILLEGAL_BEFORE_OPEN(env, name)				\
	if (!F_ISSET((env), ENV_OPEN_CALLED))				\
		return (__db_mi_open(env, name, 0));

/* We're not actually user hostile, honest. */
#define	ENV_REQUIRES_CONFIG(env, handle, i, flags)			\
	if (handle == NULL)						\
		return (__env_not_config(env, i, flags));
#define	ENV_REQUIRES_CONFIG_XX(env, handle, i, flags)			\
	if ((env)->handle->region == NULL)				\
		return (__env_not_config(env, i, flags));
#define	ENV_NOT_CONFIGURED(env, handle, i, flags)			\
	if (F_ISSET((env), ENV_OPEN_CALLED))				\
		ENV_REQUIRES_CONFIG(env, handle, i, flags)

/*
 * The ENV_ENTER and ENV_LEAVE macros announce to other threads that
 * the current thread is entering or leaving the BDB api.
 */
#define	ENV_ENTER_RET(env, ip, ret) do {				\
	ret = 0;							\
	DISCARD_HISTORY(env);						\
	PANIC_CHECK_RET(env, ret);					\
 	if (ret == 0) {							\
		if ((env)->thr_hashtab == NULL)				\
			ip = NULL;					\
		else 							\
			ret = __env_set_state(env, &(ip), THREAD_ACTIVE);\
	}								\
} while (0)

#define	ENV_ENTER(env, ip) do {						\
	int __ret;							\
	ip = NULL;							\
	ENV_ENTER_RET(env, ip, __ret);					\
	if (__ret != 0)							\
		return (__ret);						\
} while (0)

/*
 * Publicize the current thread's intention to run failchk.  This invokes
 * DB_ENV->is_alive() in the mutex code, to avoid hanging on dead processes.
 */
#define	FAILCHK_THREAD(env, ip) do {					\
	if ((ip) != NULL)						\
		(ip)->dbth_state = THREAD_FAILCHK;			\
} while (0)

#define	ENV_GET_THREAD_INFO(env, ip) 	do {				\
	if ((env)->thr_hashtab == NULL)					\
		ip = NULL;						\
	else 								\
		(void)__env_set_state(env, &(ip), THREAD_VERIFY);	\
} while (0)

#define	ENV_LEAVE(env, ip) do {						\
	if ((ip) != NULL) {	\
		DB_ASSERT((env), (ip)->dbth_state == THREAD_ACTIVE  ||	\
		    (ip)->dbth_state == THREAD_FAILCHK);		\
		(ip)->dbth_state = THREAD_OUT;				\
	}								\
} while (0)


#ifdef DIAGNOSTIC
#define	CHECK_THREAD(env) do {						\
	if ((env)->thr_hashtab != NULL)					\
		(void)__env_set_state(env, NULL, THREAD_VERIFY);	\
} while (0)
#ifdef HAVE_STATISTICS
#define	CHECK_MTX_THREAD(env, mtx) do {					\
	if (mtx->alloc_id != MTX_MUTEX_REGION &&			\
	    mtx->alloc_id != MTX_ENV_REGION &&				\
	    mtx->alloc_id != MTX_APPLICATION)				\
		CHECK_THREAD(env);					\
} while (0)
#else
#define	CHECK_MTX_THREAD(env, mtx)	NOP_STATEMENT
#endif
#else
#define	CHECK_THREAD(env)		NOP_STATEMENT
#define	CHECK_MTX_THREAD(env, mtx)	NOP_STATEMENT
#endif

typedef enum {
	THREAD_SLOT_NOT_IN_USE=0,
	THREAD_OUT,
	THREAD_OUT_DEAD,
	THREAD_ACTIVE,
	THREAD_BLOCKED,
	THREAD_BLOCKED_DEAD,
	THREAD_CTR_VERIFY,
	THREAD_FAILCHK,
	THREAD_VERIFY
} DB_THREAD_STATE;

typedef struct __pin_list {
	roff_t b_ref;		/* offset to buffer. */
	int region;		/* region containing buffer. */
} PIN_LIST;
#define	PINMAX 4

typedef enum {
	MUTEX_ACTION_UNLOCKED=0,
	MUTEX_ACTION_INTEND_SHARE,	/* Thread is attempting a read-lock. */
	MUTEX_ACTION_SHARED		/* Thread has gotten a read lock. */
} MUTEX_ACTION;

typedef struct __mutex_state {	/* SHARED */
	db_mutex_t	mutex;
	MUTEX_ACTION	action;
#ifdef DIAGNOSTIC
	db_timespec	when;
#endif
} MUTEX_STATE;

#define MUTEX_STATE_MAX 10	/* It only needs enough for shared latches. */


struct __db_thread_info { /* SHARED */
	DB_THREAD_STATE	dbth_state;
	pid_t		dbth_pid;
	db_threadid_t	dbth_tid;
	/* This contains the overflow chain in env->thr_hashtab[indx]. */
	SH_TAILQ_ENTRY	dbth_links;
	/*
	 * The next field contains the (process local) reference to the XA
	 * transaction currently associated with this thread of control.
	 */
	SH_TAILQ_HEAD(__dbth_xatxn) dbth_xatxn;
	u_int32_t	dbth_xa_status;
	/*
	 * The following fields track which buffers this thread of
	 * control has pinned in the mpool buffer cache.
	 */
	u_int16_t	dbth_pincount;	/* Number of pins for this thread. */
	u_int16_t	dbth_pinmax;	/* Number of slots allocated. */
	roff_t		dbth_pinlist;	/* List of pins. */
	PIN_LIST	dbth_pinarray[PINMAX];	/* Initial array of slots. */

	/*
	 * While thread tracking is active this caches one of the lockers
	 * created by each thread.  This locker remains allocated, with an
	 * invalid id, even after the locker id is freed.
	 */
	roff_t		dbth_local_locker;
	/*
	 * Each latch shared by this thread has an entry here.  Exclusive
	 * ownership, for both latches and mutexes, are in the DB_MUTEX.
	 */
	MUTEX_STATE	dbth_latches[MUTEX_STATE_MAX];
#ifdef DIAGNOSTIC
	roff_t		dbth_locker;	/* Current locker for this thread. */
	u_int32_t	dbth_check_off;	/* Count of number of LOCK_OFF calls. */
#endif
	db_timespec	dbth_failtime;	/* Time when its crash was detected. */
	u_int32_t 	mtx_ctr;	/* Counts mutexes held by thread. */
};

#ifdef DIAGNOSTIC
#define LOCK_CHECK_OFF(ip) if ((ip) != NULL)				\
	(ip)->dbth_check_off++

#define LOCK_CHECK_ON(ip) if ((ip) != NULL)				\
	(ip)->dbth_check_off--

#define LOCK_CHECK(dbc, pgno, mode, type)				\
	DB_ASSERT((dbc)->dbp->env, (dbc)->locker == NULL ||		\
	     __db_haslock((dbc)->dbp->env,				\
	    (dbc)->locker, (dbc)->dbp->mpf, pgno, mode, type) == 0)
#else
#define LOCK_CHECK_OFF(ip)	NOP_STATEMENT
#define LOCK_CHECK_ON(ip)	NOP_STATEMENT
#define LOCK_CHECK(dbc, pgno, mode)	NOP_STATEMENT
#endif

typedef struct __env_thread_info { /* SHARED */
	u_int32_t	thr_count;
	u_int32_t	thr_init;
	u_int32_t	thr_max;
	u_int32_t	thr_nbucket;
	roff_t		thr_hashoff;
} THREAD_INFO;

#define	DB_EVENT(env, e, einfo) do {					\
	DB_ENV *__dbenv = (env)->dbenv;					\
	if (__dbenv->db_event_func != NULL)				\
		__dbenv->db_event_func(__dbenv, e, einfo);		\
} while (0)

typedef struct __flag_map {
	u_int32_t inflag, outflag;
} FLAG_MAP;

typedef struct __db_backup_handle {
	int	(*open) __P((DB_ENV *, const char *, const char *, void **));
	int	(*write) __P((DB_ENV *,
		    u_int32_t, u_int32_t, u_int32_t, u_int8_t *, void *));
	int	(*close) __P((DB_ENV *, const char *, void *));
	u_int32_t	size;
	u_int32_t	read_count;
	u_int32_t	read_sleep;
#define	BACKUP_WRITE_DIRECT	0x0001
	int	flags;
} DB_BACKUP;

/*
 * Internal database environment structure.
 *
 * This is the private database environment handle.  The public environment
 * handle is the DB_ENV structure.   The library owns this structure, the user
 * owns the DB_ENV structure.  The reason there are two structures is because
 * the user's configuration outlives any particular DB_ENV->open call, and
 * separate structures allows us to easily discard internal information without
 * discarding the user's configuration.
 */
struct __env {
	DB_ENV *dbenv;			/* Linked DB_ENV structure */

	/*
	 * The ENV structure can be used concurrently, so field access is
	 * protected.
	 */
	db_mutex_t mtx_env;		/* ENV structure mutex */

	/*
	 * Some fields are included in the ENV structure rather than in the
	 * DB_ENV structure because they are only set as arguments to the
	 * DB_ENV->open method.  In other words, because of the historic API,
	 * not for any rational reason.
	 *
	 * Arguments to DB_ENV->open.
	 */
	char	 *db_home;		/* Environment's home directory. */
	u_int32_t open_flags;		/* Flags */
	int	  db_mode;		/* Default open permissions */
	u_int32_t envid;		/* Cached env id, panic if != REGENV. */

	pid_t	pid_cache;		/* Cached process ID */

	DB_FH	*lockfhp;		/* fcntl(2) locking file handle */

	DB_LOCKER *env_lref;		/* Locker in non-threaded handles */

	DB_DISTAB   recover_dtab;	/* Dispatch table for recover funcs */

#ifdef HAVE_SLICES
	/*
	 * A containing environment has slice_envs[] set, the others are zero.
	 * A slice (subordinate) environment has both slice_container and
	 * slice_index set.  A non-slice-aware environment sets all to zero.
	 */
	DB_ENV	   **slice_envs;	/* Array of slice_cnt dbenvs, +1 NULL */
	ENV	    *slice_container;	/* The containing env of this slice. */
	db_slice_t   slice_index;	/* Position in container's slice_envs */
#endif

	int dir_mode;			/* Intermediate directory perms. */

#define ENV_DEF_DATA_LEN		100
	u_int32_t data_len;		/* Data length in __db_prbytes. */

	/* Registered processes */ 
	size_t	num_active_pids;	/* number of entries in active_pids */ 
	size_t	size_active_pids;	/* allocated size of active_pids */ 
	pid_t	*active_pids;		/* array active pids */ 

	/*
	 * Thread tracking: a kind of configurable thread local storage that is
	 * located in the environment region.  Allocating a new entry requires
	 * locking mtx_regenv.  Entries are neither deleted nor moved between
	 * buckets, which permits safe lookups without requiring any mutexes.
	 */
	u_int32_t	 thr_nbucket;	/* Number of hash buckets */
	DB_HASHTAB	*thr_hashtab;	/* Hash table of DB_THREAD_INFO */

	/*
	 * List of open DB handles for this ENV, used for cursor
	 * adjustment.  Must be protected for multi-threaded support.
	 */
	db_mutex_t mtx_dblist;
	int	   db_ref;		/* DB handle reference count */
	TAILQ_HEAD(__dblist, __db) dblist;

	/*
	 * List of open file handles for this ENV.  Must be protected
	 * for multi-threaded support.
	 */
	TAILQ_HEAD(__fdlist, __fh_t) fdlist;

	db_mutex_t	 mtx_mt;	/* Mersenne Twister mutex */
	int		 mti;		/* Mersenne Twister index */
	u_long		*mt;		/* Mersenne Twister state vector */

	DB_CIPHER	*crypto_handle;	/* Crypto handle */
	DB_LOCKTAB	*lk_handle;	/* Lock handle */
	DB_LOG		*lg_handle;	/* Log handle */
	DB_MPOOL	*mp_handle;	/* Mpool handle */
	DB_MUTEXMGR	*mutex_handle;	/* Mutex handle */
	DB_REP		*rep_handle;	/* Replication handle */
	DB_TXNMGR	*tx_handle;	/* Txn handle */

	DB_BACKUP	*backup_handle;	/* database copy configuration. */

	/*
	 * XA support.
	 */
	int		 xa_rmid;	/* XA Resource Manager ID */
	int		 xa_ref;	/* XA Reference count */
	TAILQ_ENTRY(__env) links;	/* XA environments */

	/* Application callback to copy data to/from a custom data source */
#define	DB_USERCOPY_GETDATA	0x0001
#define	DB_USERCOPY_SETDATA	0x0002
	int (*dbt_usercopy)
	    __P((DBT *, u_int32_t, void *, u_int32_t, u_int32_t));

	int (*log_verify_wrap) __P((ENV *, const char *, u_int32_t,
	    const char *, const char *, time_t, time_t, u_int32_t,  u_int32_t,
	    u_int32_t, u_int32_t, int, int));

	REGINFO	*reginfo;		/* REGINFO structure reference */

#define	DB_TEST_ELECTINIT	 1	/* after __rep_elect_init */
#define	DB_TEST_ELECTVOTE1	 2	/* after sending VOTE1 */
#define	DB_TEST_NO_CHUNKS	 3	/* before sending BLOB data */
#define	DB_TEST_NO_PAGES	 4	/* before sending PAGE */
#define	DB_TEST_POSTDESTROY	 5	/* after destroy op */
#define	DB_TEST_POSTLOG		 6	/* after logging all pages */
#define	DB_TEST_POSTLOGMETA	 7	/* after logging meta in btree */
#define	DB_TEST_POSTOPEN	 8	/* after __os_open */
#define	DB_TEST_POSTSYNC	 9	/* after syncing the log */
#define	DB_TEST_PREDESTROY	 10	/* before destroy op */
#define	DB_TEST_PREOPEN		 11	/* before __os_open */
#define	DB_TEST_REPMGR_PERM	 12	/* repmgr perm/archiving tests */
#define	DB_TEST_SUBDB_LOCKS	 13	/* subdb locking tests */
#define	DB_TEST_REPMGR_HEARTBEAT 14	/* repmgr stop sending heartbeats */
#define	DB_TEST_NO_MUTEX         15     /* thread is holding no mutexes */
#define	DB_TEST_LATCH            16     /* thread is sharing a latch */
#define	DB_TEST_EXC_LATCH        17     /* thread has an exclusive latch */
#define	DB_TEST_EXC_MUTEX        18     /* thread is holding a mutex */
#define	DB_TEST_FAILCHK          19     /* thread is in failchk */
	int	test_abort;		/* Abort value for testing */
	int	test_check;		/* Checkpoint value for testing */
	int	test_copy;		/* Copy value for testing */

#define	ENV_CDB			0x00000001 /* DB_INIT_CDB */
#define	ENV_DBLOCAL		0x00000002 /* Environment for a private DB */
#define	ENV_LITTLEENDIAN	0x00000004 /* Little endian system. */
#define	ENV_LOCKDOWN		0x00000008 /* DB_LOCKDOWN set */
#define	ENV_NO_OUTPUT_SET	0x00000010 /* No output channel set */
#define	ENV_OPEN_CALLED		0x00000020 /* DB_ENV->open called */
#define	ENV_PRIVATE		0x00000040 /* DB_PRIVATE set */
#define	ENV_RECOVER_FATAL	0x00000080 /* Doing fatal recovery in env */
#define	ENV_REF_COUNTED		0x00000100 /* Region references this handle */
#define	ENV_SYSTEM_MEM		0x00000200 /* DB_SYSTEM_MEM set */
#define	ENV_THREAD		0x00000400 /* DB_THREAD set */
#define	ENV_FORCE_TXN_BULK	0x00000800 /* Txns use bulk mode-for testing */
#define	ENV_REMEMBER_PANIC	0x00001000 /* Panic was on during cleanup. */
#define	ENV_FORCESYNCENV	0x00002000 /* Force msync on closing. */
	u_int32_t flags;
};

/*******************************************************
 * Database Access Methods.
 *******************************************************/
/*
 * DB_IS_THREADED --
 *	The database handle is free-threaded (was opened with DB_THREAD).
 */
#define	DB_IS_THREADED(dbp)						\
	((dbp)->mutex != MUTEX_INVALID)

/* Initialization methods are often illegal before/after open is called. */
#define	DB_ILLEGAL_AFTER_OPEN(dbp, name)				\
	if (F_ISSET((dbp), DB_AM_OPEN_CALLED))				\
		return (__db_mi_open((dbp)->env, name, 1));
#define	DB_ILLEGAL_BEFORE_OPEN(dbp, name)				\
	if (!F_ISSET((dbp), DB_AM_OPEN_CALLED))				\
		return (__db_mi_open((dbp)->env, name, 0));
/* Some initialization methods are illegal if environment isn't local. */
#define	DB_ILLEGAL_IN_ENV(dbp, name)					\
	if (!F_ISSET((dbp)->env, ENV_DBLOCAL))				\
		return (__db_mi_env((dbp)->env, name));
#define	DB_ILLEGAL_METHOD(dbp, flags) {					\
	int __ret;							\
	if ((__ret = __dbh_am_chk(dbp, flags)) != 0)			\
		return (__ret);						\
}

/*
 * Common DBC->internal fields.  Each access method adds additional fields
 * to this list, but the initial fields are common.
 */
#define	__DBC_INTERNAL							\
	DBC	 *opd;			/* Off-page duplicate cursor. */\
	DBC	 *pdbc;			/* Pointer to parent cursor. */ \
									\
	void	 *page;			/* Referenced page. */		\
	db_pgno_t root;			/* Tree root. */		\
	db_pgno_t pgno;			/* Referenced page number. */	\
	db_indx_t indx;			/* Referenced key item index. */\
									\
	/* Streaming -- cache last position. */				\
	db_pgno_t stream_start_pgno;	/* Last start pgno. */		\
	u_int32_t stream_off;		/* Current offset. */		\
	db_pgno_t stream_curr_pgno;	/* Current overflow page. */	\
									\
	DB_LOCK		lock;		/* Cursor lock. */		\
	db_lockmode_t	lock_mode;	/* Lock mode. */

struct __dbc_internal {
	__DBC_INTERNAL
};

/* Actions that __db_master_update can take. */
typedef enum { MU_REMOVE, MU_RENAME, MU_OPEN, MU_MOVE } mu_action;

/*
 * Access-method-common macro for determining whether a cursor
 * has been initialized.
 */
#ifdef HAVE_PARTITION
#define	IS_INITIALIZED(dbc)	(DB_IS_PARTITIONED((dbc)->dbp) ?	\
		((PART_CURSOR *)(dbc)->internal)->sub_cursor != NULL && \
		((PART_CURSOR *)(dbc)->internal)->sub_cursor->		\
		    internal->pgno != PGNO_INVALID :			\
		(dbc)->internal->pgno != PGNO_INVALID)
#else
#define	IS_INITIALIZED(dbc)	((dbc)->internal->pgno != PGNO_INVALID)
#endif

/* Free the callback-allocated buffer, if necessary, hanging off of a DBT. */
#define	FREE_IF_NEEDED(env, dbt)					\
	if (F_ISSET((dbt), DB_DBT_APPMALLOC)) {				\
		__os_ufree((env), (dbt)->data);				\
		F_CLR((dbt), DB_DBT_APPMALLOC);				\
	}

/*
 * Use memory belonging to object "owner" to return the results of
 * any no-DBT-flag get ops on cursor "dbc".
 */
#define	SET_RET_MEM(dbc, owner)				\
	do {						\
		(dbc)->rskey = &(owner)->my_rskey;	\
		(dbc)->rkey = &(owner)->my_rkey;	\
		(dbc)->rdata = &(owner)->my_rdata;	\
	} while (0)

/* Use the return-data memory src is currently set to use in dest as well. */
#define	COPY_RET_MEM(src, dest)				\
	do {						\
		(dest)->rskey = (src)->rskey;		\
		(dest)->rkey = (src)->rkey;		\
		(dest)->rdata = (src)->rdata;		\
	} while (0)

/* Reset the returned-memory pointers to their defaults. */
#define	RESET_RET_MEM(dbc)				\
	do {						\
		(dbc)->rskey = &(dbc)->my_rskey;	\
		(dbc)->rkey = &(dbc)->my_rkey;		\
		(dbc)->rdata = &(dbc)->my_rdata;	\
	} while (0)

#define	COMPACT_TRUNCATE(c_data) do {			\
	if (c_data->compact_truncate > 1)		\
		c_data->compact_truncate--;		\
} while (0)

/*******************************************************
 * Mpool.
 *******************************************************/
/*
 * File types for DB access methods.  Negative numbers are reserved to DB.
 */
#define	DB_FTYPE_SET		-1		/* Call pgin/pgout functions. */
#define	DB_FTYPE_NOTSET		 0		/* Don't call... */
#define	DB_LSN_OFF_NOTSET	-1		/* Not yet set. */
#define	DB_CLEARLEN_NOTSET	UINT32_MAX	/* Not yet set. */

/* Structure used as the DB pgin/pgout pgcookie. */
typedef struct __dbpginfo {
	u_int32_t db_pagesize;		/* Underlying page size. */
	u_int32_t flags;		/* Some DB_AM flags needed. */
	DBTYPE  type;			/* DB type */
} DB_PGINFO;

/*******************************************************
 * Log.
 *******************************************************/
/* Initialize an LSN to 'zero'. */
#define	ZERO_LSN(LSN) do {						\
	(LSN).file = 0;							\
	(LSN).offset = 0;						\
} while (0)
#define	IS_ZERO_LSN(LSN)	((LSN).file == 0 && (LSN).offset == 0)

#define	IS_INIT_LSN(LSN)	((LSN).file == 1 && (LSN).offset == 0)
#define	INIT_LSN(LSN)		do {					\
	(LSN).file = 1;							\
	(LSN).offset = 0;						\
} while (0)

#define	MAX_LSN(LSN) do {						\
	(LSN).file = UINT32_MAX;					\
	(LSN).offset = UINT32_MAX;					\
} while (0)
#define	IS_MAX_LSN(LSN) \
	((LSN).file == UINT32_MAX && (LSN).offset == UINT32_MAX)

/* If logging is turned off, smash the lsn. */
#define	LSN_NOT_LOGGED(LSN) do {					\
	(LSN).file = 0;							\
	(LSN).offset = 1;						\
} while (0)
#define	IS_NOT_LOGGED_LSN(LSN) \
	((LSN).file == 0 && (LSN).offset == 1)

/*
 * LOG_COMPARE -- compare two LSNs.
 */

#define	LOG_COMPARE(lsn0, lsn1)						\
	((lsn0)->file != (lsn1)->file ?					\
	((lsn0)->file < (lsn1)->file ? -1 : 1) :			\
	((lsn0)->offset != (lsn1)->offset ?				\
	((lsn0)->offset < (lsn1)->offset ? -1 : 1) : 0))

/*******************************************************
 * Txn.
 *******************************************************/
#define	DB_NONBLOCK(C)	((C)->txn != NULL && F_ISSET((C)->txn, TXN_NOWAIT))
#define	NOWAIT_FLAG(txn) \
	((txn) != NULL && F_ISSET((txn), TXN_NOWAIT) ? DB_LOCK_NOWAIT : 0)
#define	IS_REAL_TXN(txn)						\
	((txn) != NULL && !F_ISSET(txn, TXN_FAMILY))
#define	IS_SUBTRANSACTION(txn)						\
	((txn) != NULL && (txn)->parent != NULL)

/* Checks for existence of an XA transaction in access method interfaces. */
#define	XA_CHECK_TXN(ip, txn) 						\
	if ((ip) != NULL && (txn) == NULL) {				\
		(txn) = SH_TAILQ_FIRST(&(ip)->dbth_xatxn, __db_txn);	\
		DB_ASSERT(env, txn == NULL ||				\
		    txn->xa_thr_status == TXN_XA_THREAD_ASSOCIATED);	\
	}

/* Ensure that there is no XA transaction active. */
#define	XA_NO_TXN(ip, retval) {						\
	DB_TXN *__txn;							\
	retval = 0;							\
	if ((ip) != NULL) {						\
		__txn = SH_TAILQ_FIRST(&(ip)->dbth_xatxn, __db_txn);	\
		if (__txn != NULL &&					\
		    __txn->xa_thr_status == TXN_XA_THREAD_ASSOCIATED)	\
		    	retval = USR_ERR(__txn->mgrp->env, EINVAL);	\
	}								\
}

/*******************************************************
 * Crypto.
 *******************************************************/
#define	DB_IV_BYTES     16		/* Bytes per IV */
#define	DB_MAC_KEY	20		/* Bytes per MAC checksum */

/*******************************************************
 * Compression
 *******************************************************/
#define	CMP_INT_SPARE_VAL	0xFC	/* Smallest byte value that the integer
					   compression algorithm doesn't use */

#if defined(__cplusplus)
}
#endif

/*******************************************************
 * Remaining general DB includes.
 *******************************************************/


#include "dbinc/globals.h"
#include "dbinc/debug.h"
#include "dbinc/region.h"
#include "dbinc_auto/env_ext.h"
#include "dbinc/mutex.h"
#ifdef HAVE_REPLICATION_THREADS
#include "dbinc/repmgr.h"
#endif
#include "dbinc/rep.h"
#include "dbinc/os.h"
#include "dbinc_auto/clib_ext.h"
#include "dbinc_auto/common_ext.h"
#include "dbinc_auto/blob_ext.h"

/*******************************************************
 * Remaining Log.
 * These need to be defined after the general includes
 * because they need rep.h from above.
 *******************************************************/
/*
 * Test if the environment is currently logging changes.  If we're in recovery
 * or we're a replication client, we don't need to log changes because they're
 * already in the log, even though we have a fully functional log system.
 */
#define	DBENV_LOGGING(env)						\
	(LOGGING_ON(env) && !IS_REP_CLIENT(env) && (!IS_RECOVERING(env)))

/*
 * Test if we need to log a change.  By default, we don't log operations without
 * associated transactions, unless DIAGNOSTIC, DEBUG_ROP or DEBUG_WOP are on.
 * This is because we want to get log records for read/write operations, and, if
 * we are trying to debug something, more information is always better.
 *
 * The DBC_RECOVER flag is set when we're in abort, as well as during recovery;
 * thus DBC_LOGGING may be false for a particular dbc even when DBENV_LOGGING
 * is true.
 *
 * We explicitly use LOGGING_ON/IS_REP_CLIENT here because we don't want to pull
 * in the log headers, which IS_RECOVERING (and thus DBENV_LOGGING) rely on, and
 * because DBC_RECOVER should be set anytime IS_RECOVERING would be true.
 *
 * If we're not in recovery (master - doing an abort or a client applying
 * a txn), then a client's only path through here is on an internal
 * operation, and a master's only path through here is a transactional
 * operation.  Detect if either is not the case.
 */
#if defined(DIAGNOSTIC) || defined(DEBUG_ROP)  || defined(DEBUG_WOP)
#define	DBC_LOGGING(dbc)	__dbc_logging(dbc)
#else
#define	DBC_LOGGING(dbc)						\
	((dbc)->txn != NULL && LOGGING_ON((dbc)->env) &&		\
	    !F_ISSET((dbc), DBC_RECOVER) && !IS_REP_CLIENT((dbc)->env))
#endif

#endif /* !_DB_INT_H_ */
