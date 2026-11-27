#include "proclaunch.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  /* ... */
#else
  #include <unistd.h>
  #include <sys/wait.h>
  #include <errno.h>
  #include <spawn.h>
  extern char **environ;
  int pl_start(const char *const argv[], pl_process *proc) {
    pid_t pid;
    int rc = posix_spawnp(&pid, argv[0], NULL, NULL,
                          (char *const *)argv, environ);
    if (rc != 0) return -1;
    proc->pid    = (long)pid;
    proc->handle = 0;
    return 0;
}

int pl_wait(pl_process *proc, int *exit_code) {
    int status;
    pid_t r;
    do { r = waitpid((pid_t)proc->pid, &status, 0); }
    while (r < 0 && errno == EINTR);
    if (r < 0) return -1;
    if (WIFEXITED(status))        *exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) *exit_code = 128 + WTERMSIG(status);
    else                          *exit_code = -1;
    return 0;
}

int pl_poll(pl_process *proc, int *exit_code) {
    int status;
    pid_t r = waitpid((pid_t)proc->pid, &status, WNOHANG);
    if (r == 0) return 0;
    if (r < 0)  return -1;
    if (WIFEXITED(status))        *exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) *exit_code = 128 + WTERMSIG(status);
    else                          *exit_code = -1;
    return 1;
}

#endif
