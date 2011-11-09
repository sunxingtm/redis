#ifndef WIN32FIXES_H
#define WIN32FIXES_H

#ifdef WIN32
  #ifndef _WIN32
    #define _WIN32
  #endif
#endif

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define NOGDI
  #define __USE_W32_SOCKETS

  #include "fmacros.h"
  #include <stdlib.h>
  #include <stdio.h>
  #include <io.h>
  #include <signal.h>
  #include <sys/types.h>
  #ifndef FD_SETSIZE
    #define FD_SETSIZE 16000
  #endif
  #include <winsock2.h>  /* setsocketopt */
  #include <ws2tcpip.h>
  #include <windows.h>
  #include <sys/time.h>
  #include <fcntl.h>    /* _O_BINARY */
  #include <limits.h>  /* INT_MAX */
  #include <sys/types.h>
  #ifdef _WIN64
    #include <ntsecapi.h>
  #endif

  //Misc
  #ifdef __STRICT_ANSI__
    #define _exit exit
    #define fileno(__F) ((__F)->_file)

    #define strcasecmp lstrcmpiA
    //#define strncasecmp StrCmpNIC

    #define fseeko(stream, offset, origin) fseek(stream, offset, origin)
    #define ftello(stream) ftell(stream)
  #else
    #define fseeko fseeko64
    #define ftello ftello64
  #endif

  #define inline __inline

  #undef ftruncate
  #define ftruncate replace_ftruncate
  int replace_ftruncate(int fd, off64_t length);

  #ifdef __MINGW32__
    #define strtod    __strtod
    #define vsnprintf __mingw_vsnprintf
    #define snprintf  __mingw_snprintf
    #define printf    __mingw_printf
  #endif

  #define sleep(x) Sleep((x)*1000)

  #if defined(_WIN64) || defined(_MSC_VER)
    #define random() (long)replace_random()
    #define rand() replace_random()
    int replace_random();
  #else
    #ifndef __RTL_GENRANDOM
    #define __RTL_GENRANDOM 1
        typedef BOOLEAN (_stdcall* RtlGenRandomFunc)(void * RandomBuffer, ULONG RandomBufferLength);
    #endif
    RtlGenRandomFunc RtlGenRandom;

    #define random() (long)replace_random()
    #define rand() replace_random()
    int replace_random();
  #endif

  // Redis calls usleep(1) to give thread some time
  // Sleep(0) should do the same on windows
  // In other cases, usleep is called with milisec resolution,
  // which can be directly translated to winapi Sleep()
  #undef usleep
  #define usleep(x) (x == 1) ? Sleep(0) : Sleep((int)((x)/1000))

  #define pipe(fds) _pipe(fds, 8192, _O_BINARY|_O_NOINHERIT)

  //Prcesses
  #define waitpid(pid,statusp,options) _cwait (statusp, pid, WAIT_CHILD)

  #define WAIT_T int
  #define WTERMSIG(x) ((x) & 0xff) /* or: SIGABRT ?? */
  #define WCOREDUMP(x) 0
  #define WEXITSTATUS(x) (((x) >> 8) & 0xff) /* or: (x) ?? */
  #define WIFSIGNALED(x) (WTERMSIG (x) != 0) /* or: ((x) == 3) ?? */
  #define WIFEXITED(x) (WTERMSIG (x) == 0) /* or: ((x) != 3) ?? */
  #define WIFSTOPPED(x) 0

  #define WNOHANG 1

  /* file mapping */
  #define PROT_READ 1
  #define PROT_WRITE 2

  #define MAP_FAILED   (void *) -1

  #define MAP_SHARED 1
  #define MAP_PRIVATE 2

  // rusage
  #define RUSAGE_SELF     0
  #define RUSAGE_CHILDREN (-1)

  #ifndef _RUSAGE_T_
  #define _RUSAGE_T_
  struct rusage {
     struct timeval ru_utime;    // user time used
     struct timeval ru_stime;    // system time used
  };
  #endif

  int getrusage(int who, struct rusage * rusage);

  //Signals
  #define SIGNULL  0 /* Null	Check access to pid*/
  #define SIGHUP	 1 /* Hangup	Terminate; can be trapped*/
  #define SIGINT	 2 /* Interrupt	Terminate; can be trapped */
  #define SIGQUIT	 3 /* Quit	Terminate with core dump; can be trapped */
  #define SIGTRAP  5
  #define SIGBUS   7
  #define SIGKILL	 9 /* Kill	Forced termination; cannot be trapped */
  #define SIGPIPE 13
  #define SIGALRM 14
  #define SIGTERM	15 /* Terminate	Terminate; can be trapped  */
  #define SIGSTOP 17
  #define SIGTSTP 18
  #define SIGCONT 19
  #define SIGCHLD 20
  #define SIGTTIN 21
  #define SIGTTOU 22
  #define SIGABRT 22
  // #define SIGSTOP	24 /*Pause the process; cannot be trapped*/
  //#define SIGTSTP	25 /*Terminal stop	Pause the process; can be trapped*/
  //#define SIGCONT	26
  #define SIGWINCH 28
  #define SIGUSR1  30
  #define SIGUSR2  31

  #define ucontext_t void*

  #define SA_NOCLDSTOP    0x00000001u
  #define SA_NOCLDWAIT    0x00000002u
  #define SA_SIGINFO      0x00000004u
  #define SA_ONSTACK      0x08000000u
  #define SA_RESTART      0x10000000u
  #define SA_NODEFER      0x40000000u
  #define SA_RESETHAND    0x80000000u
  #define SA_NOMASK       SA_NODEFER
  #define SA_ONESHOT      SA_RESETHAND
  #define SA_RESTORER     0x04000000

  #ifdef _WIN64
    #define sigset_t _sigset_t
  #else
    #ifndef _SIGSET_T_
    #define _SIGSET_T_
        typedef unsigned long sigset_t;
    #endif /* _SIGSET_T_ */
 #endif

  #define sigemptyset(pset)    (*(pset) = 0)
  #define sigfillset(pset)     (*(pset) = (unsigned int)-1)
  #define sigaddset(pset, num) (*(pset) |= (1L<<(num)))
  #define sigdelset(pset, num) (*(pset) &= ~(1L<<(num)))
  #define sigismember(pset, num) (*(pset) & (1L<<(num)))

  #ifndef SIG_SETMASK
    #define SIG_SETMASK (0)
    #define SIG_BLOCK   (1)
    #define SIG_UNBLOCK (2)
  #endif /*SIG_SETMASK*/


  struct sigaction {
    int          sa_flags;
    sigset_t     sa_mask;
    __p_sig_fn_t sa_handler;
    __p_sig_fn_t sa_sigaction;
  };

  int sigaction(int sig, struct sigaction *in, struct sigaction *out);

  // Socekts
  #ifndef _MSC_VER
  //#define EMSGSIZE WSAEMSGSIZE
  //#define EAFNOSUPPORT WSAEAFNOSUPPORT
  //#define EWOULDBLOCK WSAEWOULDBLOCK
  //#define ENOBUFS WSAENOBUFS
  //#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
  //#define ECONNREFUSED WSAECONNREFUSED
  //#define EBADFD WSAENOTSOCK
  //#define EOPNOTSUPP WSAEOPNOTSUPP

    #ifndef ECONNRESET
    #define ECONNRESET WSAECONNRESET // redis-cli.c
    #endif

    #ifndef EINPROGRESS
    #define EINPROGRESS WSAEINPROGRESS
    #endif

    #ifndef ETIMEDOUT
    #define ETIMEDOUT WSAETIMEDOUT
    #endif
  #endif

  #define setsockopt(a,b,c,d,e) replace_setsockopt(a,b,c,d,e)

  int replace_setsockopt(int socket, int level, int optname,
                     const void *optval, socklen_t optlen);

  #define rename(a,b) replace_rename(a,b)
  int replace_rename(const char *src, const char *dest);

  //threads avoiding pthread.h

  #define pthread_mutex_t CRITICAL_SECTION
  #define pthread_attr_t ssize_t
  #define PTHREAD_MUTEX_INITIALIZER (CRITICAL_SECTION) { 0 }

  #if !defined(STACK_SIZE_PARAM_IS_A_RESERVATION)
    #define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
  #endif

  //#define pthread_mutex_init(a,b) (InitializeCriticalSection((a)), 0)
  #define pthread_mutex_init(a,b) (InitializeCriticalSectionAndSpinCount((a), 0x80000400),0)
  #define pthread_mutex_destroy(a) DeleteCriticalSection((a))
  #define pthread_mutex_lock EnterCriticalSection
  #define pthread_mutex_unlock LeaveCriticalSection

  typedef int pthread_mutexattr_t;
  #define pthread_mutexattr_init(a) (*(a) = 0)
  #define pthread_mutexattr_destroy(a) do {} while (0)
  #define pthread_mutexattr_settype(a, t) 0
  #define PTHREAD_MUTEX_RECURSIVE 0

  #define pthread_equal(t1, t2) ((t1) == (t2))

  #define pthread_attr_init(x) (*(x) = 0)
  #define pthread_attr_getstacksize(x, y) (*(y) = *(x))
  #define pthread_attr_setstacksize(x, y) (*(x) = y)

  #define pthread_t u_int

  typedef struct {
	  void *(*start_routine)(void*);
	  void *arg;
    DWORD tid;
  } pthread_proxy_t;

  int pthread_create(pthread_t *thread, const void *unused,
	         		  void *(*start_routine)(void*), void *arg);

  pthread_t pthread_self(void);

  typedef struct {
        CRITICAL_SECTION waiters_lock;
        LONG waiters;
        int was_broadcast;
        HANDLE sema;
        HANDLE continue_broadcast;
  } pthread_cond_t;

  int pthread_cond_init(pthread_cond_t *cond, const void *unused);
  int pthread_cond_destroy(pthread_cond_t *cond);
  int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
  int pthread_cond_signal(pthread_cond_t *cond);
  int pthread_cond_broadcast(pthread_cond_t *cond);

  #define pthread_join(a, b) win32_pthread_join(&(a), (b))
  int win32_pthread_join(pthread_t *thread, void **value_ptr);

  //static inline void pthread_exit(void *ret) {
  //   ExitThread((DWORD)ret);
  //}

  // typedef DWORD pthread_key_t;
  // static inline int pthread_key_create(pthread_key_t *keyp, void (*destructor)(void *value))
  // {
  //       return (*keyp = TlsAlloc()) == TLS_OUT_OF_INDEXES ? EAGAIN : 0;
  // }

  // static inline int pthread_setspecific(pthread_key_t key, const void *value)
  // {
  //       return TlsSetValue(key, (void *)value) ? 0 : EINVAL;
  // }

  // static inline void *pthread_getspecific(pthread_key_t key)
  // {
  //       return TlsGetValue(key);
  // }

  int pthread_detach (pthread_t thread);
  int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);

  /* Misc Unix -> Win32 */
  int kill(pid_t pid, int sig);
  int fsync (int fd);
  pid_t wait3(int *stat_loc, int options, void *rusage);

  int w32CeaseAndDesist(pid_t pid);
  int w32initWinSock(void);
 // int inet_aton(const char *cp_arg, struct in_addr *addr)

  /* redis-check-dump  */
  void *mmap(void *start, size_t length, int prot, int flags, int fd, off offset);
  int munmap(void *start, size_t length);

  int fork(void);

#endif /* WIN32 */
#endif /* WIN32FIXES_H */
