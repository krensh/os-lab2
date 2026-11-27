#include "proclaunch.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    static char *build_cmdline(const char *const argv[]) {
        size_t cap = 256, len = 0;
        char *buf = malloc(cap);
        if (!buf) return NULL;
        buf[0] = '\0';

        for (int i = 0; argv[i]; i++) {
            const char *a = argv[i];
            int need_quote = (*a == '\0' || strpbrk(a, " \t") != NULL);

            // Добавляем пробел перед аргументом (кроме первого)
            if (i > 0) {
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = ' ';
            }

            // Открывающая кавычка
            if (need_quote) {
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = '"';
            }

            // Копируем сам аргумент (с экранированием)
            while (*a) {
                // Если нужно экранировать кавычку или слеш перед ней
                if (need_quote && (*a == '"' || *a == '\\')) {
                    if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    buf[len++] = '\\';
                }
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = *a++;
            }

            // Закрывающая кавычка
            if (need_quote) {
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = '"';
            }
        }
        buf[len] = '\0';
        return buf;
    }
    int pl_start(const char *const argv[], pl_process *proc) {
        char *cmd = build_cmdline(argv);
        if (!cmd) return -1;

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof si);
        si.cb = sizeof si;

        BOOL ok = CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        free(cmd);

        if (!ok) return -1;

        CloseHandle(pi.hThread);
        proc->handle = (intptr_t)pi.hProcess;
        proc->pid = (long)pi.dwProcessId;
        return 0;
    }
    int pl_wait(pl_process *proc, int *exit_code) {
        HANDLE h = (HANDLE)proc->handle;
        if (WaitForSingleObject(h, INFINITE) != WAIT_OBJECT_0) return -1;

        DWORD code;
        if (!GetExitCodeProcess(h, &code)) return -1;

        CloseHandle(h);
        *exit_code = (int)code;
        return 0;
    }

    int pl_poll(pl_process *proc, int *exit_code) {
        HANDLE h = (HANDLE)proc->handle;
        DWORD w = WaitForSingleObject(h, 0);
        if (w == WAIT_TIMEOUT) return 0;
        if (w != WAIT_OBJECT_0) return -1;

        DWORD code;
        if (!GetExitCodeProcess(h, &code)) return -1;

        CloseHandle(h);
        *exit_code = (int)code;
        return 1;
    }

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
