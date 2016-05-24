/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/

#ifndef __STORE_ERRNO_H__
#define __STORE_ERRNO_H__

/**
 * store error number is base on HTTP status code.
 *
 * error_no:
 *      4xx: client error
 *      5xx: server error
 *      >=1000: 仅限模块内部使用，不得暴露到模块外
 *      目前，已有错误号均可满足使用，不使用自定义的错误号
 *
 * error_msg:
 *      1. 必须以错误号名称开头
 *         i.e. "Bad Request: xxx"
 *      2. 提供错误描述，错误描述尽可能使用预定义的模板
 *         i.e. "Bad Request: parameter(%s) missing", 
 *              "Not Found: key(%s) not found"
 */

/**
 * error_no
 */
enum store_error_t {
    STORE_OK = 0,

    // 4xx
    STORE_ERR_BAD_REQUEST               = 400,  // 非法请求
    STORE_ERR_UNAUTHORIZED              = 401,  // 未授权
    STORE_ERR_FORBIDDEN                 = 403,  // 禁止访问
    STORE_ERR_NOT_FOUND                 = 404,  // 未找到
    STORE_ERR_UNKNOWN_METHOD            = 405,  // 方法不存在
    STORE_ERR_REQUEST_TIMEOUT           = 408,  // 请求超时
    STORE_ERR_ENTITIY_TOO_LARGE         = 413,  // 超出大小
    STORE_ERR_UNSUPPORTED_TYPE          = 415,  // 类型不支持
    STORE_ERR_TOO_MANY_REQUEST          = 429,  // 请求过多

    // 5xx
    STORE_ERR_INTERNAL_ERROR            = 500,  // 内部错误
    STORE_ERR_NOT_IMPLEMENTED           = 501,  // 未实现
    STORE_ERR_SERVICE_UNAVAILABLE       = 503,  // 服务不可用
    STORE_ERR_SERVICE_TIMEOUT           = 504,  // 服务超时
    //STORE_ERR_VERSION_NOT_SUPPORTED     = 505,  // 版本不支持
};

/**
 * error_msg template
 */

// Bad Request: invalid pack
// Bad Request: mcpack version(%u) not supported
// Bad Request: parameter(%s) missing
// Bad Request: parameter(%s) invalid
//
// Unauthorized: invalid uname(%s) or tk(%s)
//
// Forbidden: read-only slave
// Forbidden: read-only user(%s)
//  
// Not Found: key(%s) not found
// Not Found: binlog(%s) not found
// Not Found: xxx(%s) not found
//
// Unknown Method: unknown method(%s)
//
// Request Timeout: service(%s) timeout
//
// Entity Too Large: request(%s) exceed limit(%u)
// Entity Too Large: value(%s) exceed limit(%u)
// Entity Too Large: response(%s) exceed limit(%u)
//
// Unsupported Type: method(%s) incompatible with type(%s)
//
// Too Many Request: ...

// Internal Error: inner logic error
// Internal Error: init error
// Internal Error: io error
// Internal Error: pack error
// Internal Error: mmap error
// Internal Error: binlog error
//
// Not Implemented: ...
//
// Service Unavailable: connection error
// Service Unavailable: cannot fetch connection
// Service Unavailable: cannot fetch shard
//
// Service Timeout: service(%s) timeout
//
// Version Not Supported: ...

const int STORE_ERR_MSG_LEN = 128;

#define STORE_SET_ERR(r, e, ...) do { \
    (r)->err_no = (e); \
    snprintf((r)->err_msg, sizeof((r)->err_msg), __VA_ARGS__); \
} while(0)

#define STORE_CLEAR_ERR(r) do { \
    (r)->err_no = 0; \
    (r)->err_msg[0] = 'O'; \
    (r)->err_msg[1] = 'K'; \
    (r)->err_msg[2] = '\0'; \
} while(0)

#define STORE_SET_ERR_BAD_REQUEST(r, ...)           STORE_SET_ERR(r, STORE_ERR_BAD_REQUEST, "Bad Request: "__VA_ARGS__)
#define STORE_SET_ERR_UNAUTHORIZED(r, ...)          STORE_SET_ERR(r, STORE_ERR_UNAUTHORIZED, "Unauthorized: "__VA_ARGS__)
#define STORE_SET_ERR_FORBIDDEN(r, ...)             STORE_SET_ERR(r, STORE_ERR_FORBIDDEN, "Forbidden: "__VA_ARGS__)
#define STORE_SET_ERR_NOT_FOUND(r, ...)             STORE_SET_ERR(r, STORE_ERR_NOT_FOUND, "Not Found: "__VA_ARGS__)
#define STORE_SET_ERR_UNKNOWN_METHOD(r, ...)        STORE_SET_ERR(r, STORE_ERR_UNKNOWN_METHOD, "Unknown Method: "__VA_ARGS__)
#define STORE_SET_ERR_REQUEST_TIMEOUT(r, ...)       STORE_SET_ERR(r, STORE_ERR_REQUEST_TIMEOUT, "Request Timeout: "__VA_ARGS__)
#define STORE_SET_ERR_ENTITIY_TOO_LARGE(r, ...)     STORE_SET_ERR(r, STORE_ERR_ENTITIY_TOO_LARGE, "Entity Too Large: "__VA_ARGS__)
#define STORE_SET_ERR_UNSUPPORTED_TYPE(r, ...)      STORE_SET_ERR(r, STORE_ERR_UNSUPPORTED_TYPE, "Unsupported Type: "__VA_ARGS__)
#define STORE_SET_ERR_TOO_MANY_REQUEST(r, ...)      STORE_SET_ERR(r, STORE_ERR_TOO_MANY_REQUEST, "Too Many Request: "__VA_ARGS__)

#define STORE_SET_ERR_INTERNAL_ERROR(r, ...)        STORE_SET_ERR(r, STORE_ERR_INTERNAL_ERROR, "Internal Error: "__VA_ARGS__)
#define STORE_SET_ERR_NOT_IMPLEMENTED(r, ...)       STORE_SET_ERR(r, STORE_ERR_NOT_IMPLEMENTED, "Not Implemented: "__VA_ARGS__)
#define STORE_SET_ERR_SERVICE_UNAVAILABLE(r, ...)   STORE_SET_ERR(r, STORE_ERR_SERVICE_UNAVAILABLE, "Service Unavailable: "__VA_ARGS__)
#define STORE_SET_ERR_SERVICE_TIMEOUT(r, ...)       STORE_SET_ERR(r, STORE_ERR_SERVICE_TIMEOUT, "Service Timeout: "__VA_ARGS__)
//#define STORE_SET_ERR_VERSION_NOT_SUPPORTED(r, ...) STORE_SET_ERR(r, STORE_ERR_VERSION_NOT_SUPPORTED, "Version Not Supported: "__VA_ARGS__)

#endif //__STORE_ERRNO_H__

