/*
 * mattconte TLSF 3.1 接入框架所用的本地配置。
 * 每段传入内存区域的最大字节数，不是所有区域容量的总和。
 * 设置为 0 时，保留上游按架构确定的索引范围。
 */
#ifndef RC_TLSF_CONFIG_H
#define RC_TLSF_CONFIG_H

#ifndef TLSF_MAX_POOL_SIZE
#define TLSF_MAX_POOL_SIZE 65536UL
#endif

#if TLSF_MAX_POOL_SIZE != 0 && \
    (TLSF_MAX_POOL_SIZE < 1024UL || TLSF_MAX_POOL_SIZE > 1073741824UL)
#error "TLSF_MAX_POOL_SIZE must be 0 or between 1024 and 1073741824 bytes."
#endif

#endif