#ifdef WIN32 
  #ifndef _WIN32
    #define _WIN32
  #endif  
#endif

#ifdef _WIN32 
  
 // #define _WIN32_WINNT 0x0501
  #include <stdlib.h>  
  #include <sys/types.h>
  #include <windows.h>
  #include <winsock.h>  
  #include <ws2tcpip.h>
  
  #define inline __inline
  
  //Misc
  #define sleep(x) Sleep((x)*1000)
  #define random() rand()
  
  //Files
  #define fseeko(stream, offset, origin) fseeko64(stream, offset, origin)
  #define ftello(stream) ftello64(stream)
  
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

  //Signals
  #define SIGNULL  0 /* Null	Check access to pid*/
  #define SIGHUP	 1 /* Hangup	Terminate; can be trapped*/
  #define SIGINT	 2 /* Interrupt	Terminate; can be trapped */
  #define SIGQUIT	 3 /* Quit	Terminate with core dump; can be trapped */
  #define SIGTRAP  5
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
  
  // Socekts  
  //#define EINTR WSAEINTR
  //#define EAGAIN WSAEWOULDBLOCK
  #define EMSGSIZE WSAEMSGSIZE
  #define EAFNOSUPPORT WSAEAFNOSUPPORT
  #define EWOULDBLOCK WSAEWOULDBLOCK
  #define ECONNRESET WSAECONNRESET
  #define EINPROGRESS WSAEINPROGRESS
  #define ENOBUFS WSAENOBUFS
  #define EPROTONOSUPPORT WSAEPROTONOSUPPORT
  #define ECONNREFUSED WSAECONNREFUSED
  #define EBADFD WSAENOTSOCK
  #define EOPNOTSUPP WSAEOPNOTSUPP

  #define setsockopt(a,b,c,d,e) rpl_setsockopt(a,b,c,d,e)
  
  int rpl_setsockopt(int socket, int level, int optname, 
                     const void *optval, socklen_t optlen);
                     
  int kill(pid_t pid, int sig);
  int fsync (int fd);
  pid_t wait3(int *stat_loc, int options, void *rusage);
  int w32CeaseAndDesist(pid_t pid);
 // int inet_aton(const char *cp_arg, struct in_addr *addr)  
  
#endif
