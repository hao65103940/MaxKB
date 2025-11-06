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

static const char *ENV_NAME = "SANDBOX_BANNED_HOSTS";

/**
 * 精确匹配黑名单
 * target: 待检测字符串
 * env_val: 逗号分隔的黑名单列表
 * 返回 1 = 匹配，0 = 不匹配
 */
static int match_env_patterns(const char *target, const char *env_val) {
    if (!target || !env_val || !*env_val) return 0;

    char *patterns = strdup(env_val);
    char *token = strtok(patterns, ",");
    int matched = 0;

    while (token) {
        // 去掉前后空格
        while (*token == ' ' || *token == '\t') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t')) *end-- = '\0';

        if (*token) {
            regex_t regex;
            // 精确匹配，加 ^ 和 $，忽略大小写
            char fullpattern[512];
            snprintf(fullpattern, sizeof(fullpattern), "^%s$", token);

            if (regcomp(&regex, fullpattern, REG_EXTENDED | REG_NOSUB | REG_ICASE) == 0) {
                if (regexec(&regex, target, 0, NULL, 0) == 0) {
                    matched = 1;
                    regfree(&regex);
                    break;
                }
                regfree(&regex);
            } else {
                fprintf(stderr, "[sandbox] ⚠️ Invalid regex '%s' — allowing host by default\n", token);
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

    const char *banned_env = getenv(ENV_NAME);

    char ip[INET6_ADDRSTRLEN] = {0};
    if (addr->sa_family == AF_INET)
        inet_ntop(AF_INET, &((struct sockaddr_in *)addr)->sin_addr, ip, sizeof(ip));
    else if (addr->sa_family == AF_INET6)
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)addr)->sin6_addr, ip, sizeof(ip));

    if (banned_env && match_env_patterns(ip, banned_env)) {
        fprintf(stderr, "[sandbox] 🚫 Access to host %s is banned\n", ip);
        errno = EACCES;
        return -1;
    }

    return real_connect(sockfd, addr, addrlen);
}

/** 拦截 getaddrinfo() —— 精确匹配域名 */
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res) {
    static int (*real_getaddrinfo)(const char *, const char *,
                                   const struct addrinfo *, struct addrinfo **) = NULL;
    if (!real_getaddrinfo)
        real_getaddrinfo = dlsym(RTLD_NEXT, "getaddrinfo");

    const char *banned_env = getenv(ENV_NAME);

    if (banned_env && node && match_env_patterns(node, banned_env)) {
        fprintf(stderr, "[sandbox] 🚫 Access to host %s is banned\n", node);
        return EAI_FAIL; // 模拟 DNS 失败
    }

    return real_getaddrinfo(node, service, hints, res);
}
