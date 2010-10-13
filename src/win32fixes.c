#ifdef _WIN32

#include <windows.h>
#include <process.h>
#include <errno.h>
#include <winsock2.h>
#include <signal.h>
#include "redis.h"
#include "win32fixes.h"

/* Winsock requires library initialization on startup  */
int w32initWinSock(void) {

    WSADATA t_wsa; // WSADATA structure
    WORD wVers; // version number
    int iError; // error number

    wVers = MAKEWORD(2, 2); // Set the version number to 2.2
    iError = WSAStartup(wVers, &t_wsa); // Start the WSADATA

    if(iError != NO_ERROR || LOBYTE(t_wsa.wVersion) != 2 || HIBYTE(t_wsa.wVersion) != 2 ) {
        return 0; /* not done; check WSAGetLastError() for error number */
    };

    return 1; /* Initialized */
}

/* Placeholder for terminating forked process. */
/* fork() is nonexistatn on windows, background cmds are todo */
int w32CeaseAndDesist(pid_t pid) {

    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);

    /* invalid process; no access rights; etc   */
    if (h == NULL)
        return errno = EINVAL;

    if (!TerminateProcess(h, 127))
        return errno = EINVAL;

    errno = WaitForSingleObject(h, INFINITE);
    CloseHandle(h);

    return 0;
}

/* Behaves as posix, works withot ifdefs, makes compiler happy */
int sigaction(int sig, struct sigaction *in, struct sigaction *out) {
    REDIS_NOTUSED(out);

    /* When the SA_SIGINFO flag is set in sa_flags then sa_sigaction
     * is used. Otherwise, sa_handler is used */
    if (in->sa_flags & SA_SIGINFO)
     	signal(sig, in->sa_sigaction);
    else
        signal(sig, in->sa_handler);

    return 0;
}

/* Terminates process, implemented only for SIGKILL */
int kill(pid_t pid, int sig) {

    if (sig == SIGKILL) {

        HANDLE h = OpenProcess(PROCESS_TERMINATE, 0, pid);

        if (!TerminateProcess(h, 127)) {
            errno = EINVAL; /* GetLastError() */
            CloseHandle(h);
            return -1;
        };

        CloseHandle(h);
        return 0;
    } else {
        errno = EINVAL;
        return -1;
    };
}

/* Forced write to disk */
int fsync (int fd) {
    HANDLE h = (HANDLE) _get_osfhandle (fd);
    DWORD err;

    if (h == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return -1;
    }

    if (!FlushFileBuffers (h)) {
        /* Windows error -> Unix */
        err = GetLastError ();
        switch (err) {
            case ERROR_INVALID_HANDLE:
            errno = EINVAL;
            break;

            default:
            errno = EIO;
        }
        return -1;
    }

    return 0;
}

/* Missing wait3() implementation */
pid_t wait3(int *stat_loc, int options, void *rusage) {
    REDIS_NOTUSED(stat_loc);
    REDIS_NOTUSED(options);
    REDIS_NOTUSED(rusage);
    return waitpid((pid_t) -1, 0, WAIT_FLAGS);
}

/* Placeholder for more complex windows sockets, does nothing */
int replace_setsockopt(int socket, int level, int optname, const void *optval, socklen_t optlen) {
    return (setsockopt)(socket, level, optname, optval, optlen);
}

/* Rename which works on Windows when file exists */
int replace_rename(const char *src, const char *dst) {

    if (MoveFileEx(src, dst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
        return 0;
    else
        /* On error we will return generic eroor code without GetLastError() */
        return EIO;
}

/* Proxy structure to pass fnuc and arg to thread */
typedef struct thread_params
{
    void *(*func)(void *);
    void * arg;
} thread_params;

/* Proxy function by windows thread requirements */
static unsigned __stdcall win32_proxy_threadproc(void *arg) {

    thread_params *p = arg;
    p->func(p->arg);

    /* Dealocate params */
    zfree(p);

    _endthreadex(0);
	return 0;
}

int pthread_create(pthread_t *thread, const void *unused,
		   void *(*start_routine)(void*), void *arg) {

    REDIS_NOTUSED(unused);
    HANDLE h;
    thread_params *params = zmalloc(sizeof(thread_params));

    params->func = start_routine;
    params->arg  = arg;

    /*  Arguments not supported in this port */
    if (arg) exit(1);

    REDIS_NOTUSED(arg);
	h =(HANDLE) _beginthreadex(NULL,  /* Security not used */
                               REDIS_THREAD_STACK_SIZE, /* Set custom stack size */
                               win32_proxy_threadproc,  /* calls win32 stdcall proxy */
                               params, /* real threadproc is passed as paremeter */
                               STACK_SIZE_PARAM_IS_A_RESERVATION,  /* reserve stack */
                               thread /* returned thread id */
                );

	if (!h)
		return errno;

    CloseHandle(h);
	return 0;
}

/* Noop in windows */
int pthread_detach (pthread_t thread) {
    REDIS_NOTUSED(thread);
    return 0; /* noop */
  }

pthread_t pthread_self(void) {
	return GetCurrentThreadId();
}

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset) {
    REDIS_NOTUSED(set);
    REDIS_NOTUSED(oset);
    switch (how) {
      case SIG_BLOCK:
      case SIG_UNBLOCK:
      case SIG_SETMASK:
           break;
      default:
            errno = EINVAL;
            return -1;
    }

  errno = ENOSYS;
  return -1;
}

/* Redis forks to perform backgroud writnig */
/* fork() on unix will split process in two */
/* marking memory pages as Copy-On-Write so */
/* child process will have data snapshot.   */
/* Windows has no support for fork().       */
int fork(void) {
#ifdef _WIN32_FORK
  /* TODO: Implement fork() for redis background writing */
  return -1;
#else
  return -1;
#endif
 }

/* Redis CPU GetProcessTimes -> rusage  */
int getrusage(int who, struct rusage * r) {

   FILETIME starttime, exittime, kerneltime, usertime;
   ULARGE_INTEGER li;

   if (r == NULL) {
       errno = EFAULT;
       return -1;
   }

   memset(r, 0, sizeof(struct rusage));

   if (who == RUSAGE_SELF) {
     if (!GetProcessTimes(GetCurrentProcess(),
                        &starttime,
                        &exittime,
                        &kerneltime,
                        &usertime))
     {
         errno = EFAULT;
         return -1;
     }
   }

   if (who == RUSAGE_CHILDREN) {
        /* Childless on windows */
        starttime.dwLowDateTime = 0;
        starttime.dwHighDateTime = 0;
        exittime.dwLowDateTime = 0;
        exittime.dwHighDateTime = 0;
        kerneltime.dwLowDateTime  = 0;
        kerneltime.dwHighDateTime  = 0;
        usertime.dwLowDateTime = 0;
        usertime.dwHighDateTime = 0;
   }
    //
    memcpy(&li, &kerneltime, sizeof(FILETIME));
    li.QuadPart /= 10L;         //
    r->ru_stime.tv_sec = li.QuadPart / 1000000L;
    r->ru_stime.tv_usec = li.QuadPart % 1000000L;

    memcpy(&li, &usertime, sizeof(FILETIME));
    li.QuadPart /= 10L;         //
    r->ru_utime.tv_sec = li.QuadPart / 1000000L;
    r->ru_utime.tv_usec = li.QuadPart % 1000000L;

    return 0;
}


#endif
