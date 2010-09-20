#ifdef _WIN32

#include <windows.h>
#include <process.h>
#include <errno.h>
#include <winsock2.h>
#include "win32fixes.h"

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset) 
{
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

int w32CeaseAndDesist(pid_t pid)
{
  HANDLE h;
  
  h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);

  /* invalid process; no access rights; etc   */  
  if (h == NULL)
    return errno = EINVAL; 
  
  if (!TerminateProcess(h, 127)) 
    return errno = EINVAL;     
  
  errno = WaitForSingleObject(h, INFINITE);
 
  CloseHandle(h);
  
  return 0;  
}


int kill(pid_t pid, int sig)
{

  if (sig == SIGKILL) {
    HANDLE h = OpenProcess(PROCESS_TERMINATE, 0, pid);  
    if (!TerminateProcess(h, 127))
    {
      errno = EINVAL; /* GetLastError() */
      CloseHandle(h);            
      return -1;
    };
    
    CloseHandle(h);
    return 0;
  }
  else {
    errno = EINVAL;
    return -1;
  };
}

int fsync (int fd)
{
  HANDLE h = (HANDLE) _get_osfhandle (fd);
  DWORD err;

  if (h == INVALID_HANDLE_VALUE)
    {
      errno = EBADF;
      return -1;
    }

  if (!FlushFileBuffers (h))
    {
     /* Translate some Windows errors into rough approximations of Unix
       * errors.  MSDN is useless as usual - in this case it doesn't
       * document the full range of errors.
       */
      err = GetLastError ();
      switch (err)
       {
         /* eg. Trying to fsync a tty. */
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

pid_t wait3(int *stat_loc, int options, void *rusage) 
{
  return waitpid((pid_t) -1, 0, WAIT_FLAGS);
}

int rpl_setsockopt(int socket, int level, int optname, const void *optval,
              socklen_t optlen)
{
  return (setsockopt)(socket, level, optname, optval, optlen);
}               

/* mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0); */
void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset) 
{
	HANDLE h;
	void *data;

	if ((flags != MAP_SHARED) || (prot != PROT_READ))
    {
	  /*  Not supported  in this port */
      return MAP_FAILED;
    };    
    
	h = CreateFileMapping(
        (HANDLE)_get_osfhandle(fd), 
        NULL, 
        PAGE_READONLY, 
        0, 
        0, 
        NULL);

	if (!h)
		return MAP_FAILED;

	data = MapViewOfFileEx(
        h, 
        FILE_MAP_READ, 
        0, 
        0, 
        length,
        start);

	CloseHandle(h);
    
    if (!data) 
      return MAP_FAILED;

	return data;
}


int munmap(void *start, size_t length)
{
	return !UnmapViewOfFile(start);  
}

/*
int inet_aton(const char *cp_arg, struct in_addr *addr)
{
	register unsigned long val;
	register int base, n;
	register unsigned char c;
	register unsigned const char *cp = (unsigned const char *) cp_arg;
	unsigned int parts[4];
	register unsigned int *pp = parts;

	for (;;) {
		
		// Collect number up to ``.''.
		 // Values are specified as for C:
		 // 0x=hex, 0=octal, other=decimal.
		 
		val = 0; base = 10;
		if (*cp == '0') {
			if (*++cp == 'x' || *cp == 'X')
				base = 16, cp++;
			else
				base = 8;
		}
		while ((c = *cp) != '\0') {
			if (isascii(c) && isdigit(c)) {
				val = (val * base) + (c - '0');
				cp++;
				continue;
			}
			if (base == 16 && isascii(c) && isxdigit(c)) {
				val = (val << 4) + 
					(c + 10 - (islower(c) ? 'a' : 'A'));
				cp++;
				continue;
			}
			break;
		}
		if (*cp == '.') {
			//
			// Internet format:
			//	a.b.c.d
			//	a.b.c	(with c treated as 16-bits)
			//	a.b	(with b treated as 24 bits)
			//
			if (pp >= parts + 3 || val > 0xff)
				return (0);
			*pp++ = val, cp++;
		} else
			break;
	}

	// Check for trailing characters.
	
	if (*cp && (!isascii(*cp) || !isspace(*cp)))
		return (0);
	
	 // Concoct the address according to
	 // the number of parts specified.

	n = pp - parts + 1;
	switch (n) {

	case 1:				// a -- 32 bits 
		break;

	case 2:				// a.b -- 8.24 bits 
		if (val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:				//a.b.c -- 8.8.16 bits 
		if (val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:				// a.b.c.d -- 8.8.8.8 bits 
		if (val > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (addr)
		addr->s_addr = htonl(val);
	return (1);
}
*/

int fork(void) 
{
  return ENOSYS;
/*   PROCESS_INFORMATION procinfo;
  STARTUPINFO sinfo;
  OFBitmanipTemplate<char>::zeroMem((char *)&sinfo, sizeof(sinfo));
  sinfo.cb = sizeof(sinfo);

  // execute command (Attention: Do not pass DETACHED_PROCESS as sixth argument to the below
  // called function because in such a case the execution of batch-files is not going to work.)
  if( !CreateProcess(
      NULL, 
      cmd.c_str(), 
      NULL, 
      NULL, 
      0,
      0, 
      NULL, 
      NULL, 
      &sinfo, 
      &procinfo) )
  {
    fprintf( stderr, "storescp: Error while executing command '%s'.\n" , cmd.c_str() );
  };

  if (opt_execSync)
  {
      // Wait until child process exits (makes execution synchronous).
      WaitForSingleObject(procinfo.hProcess, INFINITE);
  }

  // Close process and thread handles to avoid resource leak
  CloseHandle(procinfo.hProcess);
  CloseHandle(procinfo.hThread);
 */
 }


#endif
 