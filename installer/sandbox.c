#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <dlfcn.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static int (*real_connect)(int, const struct sockaddr *, socklen_t) = NULL;
static int (*real_getaddrinfo)(const char *, const char *, const struct addrinfo *, struct addrinfo **) = NULL;
static __thread char last_resolved_host[256] = {0};
static __thread int last_host_checked = 0; // 标记是否已检查过域名（1=已检查且允许）

/** 检查是否符合允许规则 */
static int is_allowed_by_env(const char *target, const char *env_val) {
    if (!target) return 0;
    if (!env_val || !*env_val) {
        fprintf(stderr, "[sandbox] ❌ No allow rules set — deny all by default\n");
        return 0;
    }

    char *patterns = strdup(env_val);
    char *token = strtok(patterns, ",");
    int allowed = 0;

    while (token) {
        while (*token == ' ' || *token == '\t') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t')) *end-- = '\0';

        if (*token) {
            if (strncmp(token, "!=", 2) == 0) {
                const char *pattern = token + 2;
                regex_t regex;
                if (regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB | REG_ICASE) != 0) {
                    fprintf(stderr, "[sandbox] ⚠️ Invalid regex ignored: %s\n", pattern);
                } else {
                    if (regexec(&regex, target, 0, NULL, 0) == 0) {
                        fprintf(stderr, "[sandbox] ❌ Deny %s (matched deny /%s/)\n", target, pattern);
                        regfree(&regex);
                        free(patterns);
                        return 0;
                    }
                    regfree(&regex);
                }
            } else {
                regex_t regex;
                if (regcomp(&regex, token, REG_EXTENDED | REG_NOSUB | REG_ICASE) != 0) {
                    fprintf(stderr, "[sandbox] ⚠️ Invalid regex ignored: %s\n", token);
                } else {
                    if (regexec(&regex, target, 0, NULL, 0) == 0)
                        allowed = 1;
                    regfree(&regex);
                }
            }
        }
        token = strtok(NULL, ",");
    }

    free(patterns);
    return allowed;
}

/** 检查逻辑封装 */
static int check_host(const char *host) {
    const char *env = getenv("SANDBOX_ALLOW_HOSTS_REGEXES");
    return is_allowed_by_env(host, env);
}

/** 拦截 getaddrinfo() — 检查域名 */
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res) {
    if (!real_getaddrinfo)
        real_getaddrinfo = dlsym(RTLD_NEXT, "getaddrinfo");

    if (node) {
        strncpy(last_resolved_host, node, sizeof(last_resolved_host) - 1);
        last_resolved_host[sizeof(last_resolved_host) - 1] = '\0';
        last_host_checked = 0;

        // 判断是否为纯 IP（跳过 IPv4/IPv6）
        struct in_addr ipv4;
        struct in6_addr ipv6;
        int is_ip = (inet_pton(AF_INET, node, &ipv4) == 1) ||
                    (inet_pton(AF_INET6, node, &ipv6) == 1);

        if (!is_ip) {
            if (!check_host(node)) {
                fprintf(stderr, "[sandbox] 🚫 Blocked DNS lookup for %s\n", node);
                return EAI_FAIL;
            }
            last_host_checked = 1; // 已检查并通过
        }
    }

    return real_getaddrinfo(node, service, hints, res);
}

/** 拦截 connect() — 检查 IP（仅当没检查过域名） */
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (!real_connect)
        real_connect = dlsym(RTLD_NEXT, "connect");

    char ip[INET6_ADDRSTRLEN] = {0};

    if (addr->sa_family == AF_INET)
        inet_ntop(AF_INET, &((struct sockaddr_in *)addr)->sin_addr, ip, sizeof(ip));
    else if (addr->sa_family == AF_INET6)
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)addr)->sin6_addr, ip, sizeof(ip));

    // 如果域名已经检查通过，则跳过 IP 检查
    if (last_host_checked) {
        return real_connect(sockfd, addr, addrlen);
    }

    // 没有检查过域名（可能是 IP 直连，如 curl）
    if (!check_host(ip)) {
        fprintf(stderr, "[sandbox] 🚫 Blocked connect to %s (no domain check)\n", ip);
        return -1;
    }

    return real_connect(sockfd, addr, addrlen);
}
