#define _GNU_SOURCE
#include <dlfcn.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <pwd.h>
#include <stdarg.h>
#include <spawn.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <execinfo.h>
#include <dlfcn.h>

#define CONFIG_FILE ".sandbox.conf"

static char *banned_hosts = NULL;
static int allow_subprocess = 0; // 默认禁止

static void load_sandbox_config() {
     Dl_info info;
     if (dladdr((void *)load_sandbox_config, &info) == 0 || !info.dli_fname) {
         banned_hosts = strdup("");
         allow_subprocess = 0;
         return;
     }
     char so_path[PATH_MAX];
     strncpy(so_path, info.dli_fname, sizeof(so_path));
     so_path[sizeof(so_path) - 1] = '\0';
     char *dir = dirname(so_path);
     char config_path[PATH_MAX];
     snprintf(config_path, sizeof(config_path), "%s/%s", dir, CONFIG_FILE);
     FILE *fp = fopen(config_path, "r");
     if (!fp) {
         banned_hosts = strdup("");
         allow_subprocess = 0;
         return;
     }
    char line[512];
    banned_hosts = strdup("");
    allow_subprocess = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");
        if (!key || !value) continue;
        while (*key == ' ' || *key == '\t') key++;
        char *kend = key + strlen(key) - 1;
        while (kend > key && (*kend == ' ' || *kend == '\t')) *kend-- = '\0';
        while (*value == ' ' || *value == '\t') value++;
        char *vend = value + strlen(value) - 1;
        while (vend > value && (*vend == ' ' || *vend == '\t')) *vend-- = '\0';
        if (strcmp(key, "SANDBOX_PYTHON_BANNED_HOSTS") == 0) {
            free(banned_hosts);
            banned_hosts = strdup(value);
        } else if (strcmp(key, "SANDBOX_PYTHON_ALLOW_SUBPROCESS") == 0) {
            allow_subprocess = atoi(value);
        }
    }
    fclose(fp);
}
static void ensure_config_loaded() {
    if (!banned_hosts) load_sandbox_config();
}
static int is_sandbox_user() {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (!pw || !pw->pw_name) {
        return 1;  // 无法识别用户 → 认为是sandbox
    }
    if (strcmp(pw->pw_name, "sandbox") == 0) {
        return 1;
    }
    return 0;
}
/**
 * 精确匹配黑名单
 */
static int match_env_patterns(const char *target, const char *env_val) {
    if (!target || !env_val || !*env_val) return 0;
    char *patterns = strdup(env_val);
    char *token = strtok(patterns, ",");
    int matched = 0;
    while (token) {
        while (*token == ' ' || *token == '\t') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t')) *end-- = '\0';
        if (*token) {
            regex_t regex;
            char fullpattern[512];
            snprintf(fullpattern, sizeof(fullpattern), "^%s$", token);
            if (regcomp(&regex, fullpattern, REG_EXTENDED | REG_NOSUB | REG_ICASE) == 0) {
                if (regexec(&regex, target, 0, NULL, 0) == 0) {
                    matched = 1;
                    regfree(&regex);
                    break;
                }
                regfree(&regex);
            }
        }
        token = strtok(NULL, ",");
    }
    free(patterns);
    return matched;
}

/** 拦截 connect() —— 精确匹配 IP */
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    static int (*real_connect)(int, const struct sockaddr *, socklen_t) = NULL;
    if (!real_connect)
        real_connect = dlsym(RTLD_NEXT, "connect");
    ensure_config_loaded();
    char ip[INET6_ADDRSTRLEN] = {0};
    if (addr->sa_family == AF_INET)
        inet_ntop(AF_INET, &((struct sockaddr_in *)addr)->sin_addr, ip, sizeof(ip));
    else if (addr->sa_family == AF_INET6)
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)addr)->sin6_addr, ip, sizeof(ip));
    if (is_sandbox_user() && banned_hosts && *banned_hosts && match_env_patterns(ip, banned_hosts)) {
        fprintf(stderr, "[sandbox] 🚫 Access to host %s is banned\n", ip);
        errno = EACCES; // EACCES 的值是 13, 意思是 Permission denied
        return -1;
    }
    return real_connect(sockfd, addr, addrlen);
}

/** 拦截 getaddrinfo() —— 只拦截域名，不拦截纯 IP */
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res) {
    static int (*real_getaddrinfo)(const char *, const char *,
                                   const struct addrinfo *, struct addrinfo **) = NULL;
    if (!real_getaddrinfo)
        real_getaddrinfo = dlsym(RTLD_NEXT, "getaddrinfo");
    ensure_config_loaded();
    if (banned_hosts && *banned_hosts && node) {
        // 检测 node 是否是 IP
        struct in_addr ipv4;
        struct in6_addr ipv6;
        int is_ip = (inet_pton(AF_INET, node, &ipv4) == 1) ||
                    (inet_pton(AF_INET6, node, &ipv6) == 1);
        // 只对“非IP的域名”进行屏蔽
        if (is_sandbox_user() && !is_ip && match_env_patterns(node, banned_hosts )) {
            fprintf(stderr, "[sandbox] 🚫 Access to host %s is banned (DNS blocked)\n", node);
            return EAI_FAIL; // 模拟 DNS 层禁止
        }
    }
    return real_getaddrinfo(node, service, hints, res);
}
/* ------------------ 禁止创建子进程------------------ */
static int allow_create_subprocess() {
    ensure_config_loaded();
    return allow_subprocess || !is_sandbox_user();
}
static int return_deny() {
    fprintf(stderr, "[sandbox] Permission denied to create subprocess in sandbox.\n");
    _exit(1);
    return -1;
}
int execve(const char *filename, char *const argv[], char *const envp[]) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }
    static int (*real_execve)(const char *, char *const[], char *const[]) = NULL;
    if (!real_execve) real_execve = dlsym(RTLD_NEXT, "execve");
    return real_execve(filename, argv, envp);
}

int execveat(int dirfd, const char *pathname,
             char *const argv[], char *const envp[], int flags) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }
    static int (*real_execveat)(int, const char *, char *const[], char *const[], int) = NULL;
    if (!real_execveat) real_execveat = dlsym(RTLD_NEXT, "execveat");
    return real_execveat(dirfd, pathname, argv, envp, flags);
}

pid_t fork(void) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }
    static pid_t (*real_fork)(void) = NULL;
    if (!real_fork) real_fork = dlsym(RTLD_NEXT, "fork");
    return real_fork();
}

pid_t vfork(void) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }
    static pid_t (*real_vfork)(void) = NULL;
    if (!real_vfork) real_vfork = dlsym(RTLD_NEXT, "vfork");
    return real_vfork();
}

int clone(int (*fn)(void *), void *child_stack, int flags, void *arg, ...) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }
    static int (*real_clone)(int (*)(void *), void *, int, void *, ...) = NULL;
    if (!real_clone) real_clone = dlsym(RTLD_NEXT, "clone");
    va_list ap;
    va_start(ap, arg);
    int ret = real_clone(fn, child_stack, flags, arg, ap);
    va_end(ap);
    return ret;
}

int posix_spawn(pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attrp,
                char *const argv[], char *const envp[]) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }
    static int (*real_posix_spawn)(pid_t *, const char *, const posix_spawn_file_actions_t *,
                                   const posix_spawnattr_t *, char *const[], char *const[]) = NULL;
    if (!real_posix_spawn) real_posix_spawn = dlsym(RTLD_NEXT, "posix_spawn");
    return real_posix_spawn(pid, path, file_actions, attrp, argv, envp);
}

int posix_spawnp(pid_t *pid, const char *file,
                 const posix_spawn_file_actions_t *file_actions,
                 const posix_spawnattr_t *attrp,
                 char *const argv[], char *const envp[]) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }
    static int (*real_posix_spawnp)(pid_t *, const char *, const posix_spawn_file_actions_t *,
                                    const posix_spawnattr_t *, char *const[], char *const[]) = NULL;
    if (!real_posix_spawnp) real_posix_spawnp = dlsym(RTLD_NEXT, "posix_spawnp");
    return real_posix_spawnp(pid, file, file_actions, attrp, argv, envp);
}
int __posix_spawn(pid_t *pid, const char *path,
                  const posix_spawn_file_actions_t *file_actions,
                  const posix_spawnattr_t *attrp,
                  char *const argv[], char *const envp[]) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }

    static int (*real___posix_spawn)(pid_t *, const char *,
                                     const posix_spawn_file_actions_t *,
                                     const posix_spawnattr_t *,
                                     char *const[], char *const[]) = NULL;

    if (!real___posix_spawn)
        real___posix_spawn = dlsym(RTLD_NEXT, "__posix_spawn");

    return real___posix_spawn(pid, path, file_actions, attrp, argv, envp);
}

int __posix_spawnp(pid_t *pid, const char *file,
                   const posix_spawn_file_actions_t *file_actions,
                   const posix_spawnattr_t *attrp,
                   char *const argv[], char *const envp[]) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }

    static int (*real___posix_spawnp)(pid_t *, const char *,
                                      const posix_spawn_file_actions_t *,
                                      const posix_spawnattr_t *,
                                      char *const[], char *const[]) = NULL;

    if (!real___posix_spawnp)
        real___posix_spawnp = dlsym(RTLD_NEXT, "__posix_spawnp");

    return real___posix_spawnp(pid, file, file_actions, attrp, argv, envp);
}

int system(const char *command) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }
    static int (*real_system)(const char *) = NULL;
    if (!real_system) real_system = dlsym(RTLD_NEXT, "system");
    return real_system(command);
}
int __libc_system(const char *command) {
    if (!allow_create_subprocess()) {
        return return_deny();
    }
    static int (*real___libc_system)(const char *) = NULL;
    if (!real___libc_system)
        real___libc_system = dlsym(RTLD_NEXT, "__libc_system");

    return real___libc_system(command);
}
long (*real_syscall)(long, ...) = NULL;

long syscall(long number, ...) {
    if (!real_syscall) real_syscall = dlsym(RTLD_NEXT, "syscall");

    va_list ap;
    va_start(ap, number);
    long a1 = va_arg(ap, long);
    long a2 = va_arg(ap, long);
    long a3 = va_arg(ap, long);
    long a4 = va_arg(ap, long);
    long a5 = va_arg(ap, long);
    long a6 = va_arg(ap, long);
    va_end(ap);

    if (number == SYS_execve || number == SYS_execveat ||
        number == SYS_fork || number == SYS_vfork || number == SYS_clone) {
        if (!allow_create_subprocess()) {
            return return_deny();
        }
    }

    return real_syscall(number, a1, a2, a3, a4, a5, a6);
}