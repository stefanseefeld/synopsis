
#ifdef __STDC__
#define	__P(p)	p
#else
#define	__P(p)	()
#endif

typedef int sigfpe_code_type;	/* Type of SIGFPE code. */

typedef void (*sigfpe_handler_type)();	/* Pointer to exception handler */

extern sigfpe_handler_type sigfpe __P((sigfpe_code_type, sigfpe_handler_type));

