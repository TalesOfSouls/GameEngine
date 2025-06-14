/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_JINGGA_HTTP_HEADER_KEY_H
#define COMS_JINGGA_HTTP_HEADER_KEY_H

#include "../../stdlib/Types.h"

enum HttpHeaderKey : byte {
    // Standard HTTP/1.1 & HTTP/2 Headers (RFC 9110, 9113, etc.)
    HTTP_HEADER_KEY_UNKNOWN,

    HTTP_HEADER_KEY_HOST,
    HTTP_HEADER_KEY_USER_AGENT,
    HTTP_HEADER_KEY_ACCEPT,
    HTTP_HEADER_KEY_ACCEPT_CHARSET,
    HTTP_HEADER_KEY_ACCEPT_ENCODING,
    HTTP_HEADER_KEY_ACCEPT_LANGUAGE,
    HTTP_HEADER_KEY_ACCEPT_DATETIME,
    HTTP_HEADER_KEY_ACCEPT_PATCH,
    HTTP_HEADER_KEY_ACCEPT_RANGES,
    HTTP_HEADER_KEY_AGE,
    HTTP_HEADER_KEY_ALLOW,
    HTTP_HEADER_KEY_AUTHORIZATION,
    HTTP_HEADER_KEY_CACHE_CONTROL,
    HTTP_HEADER_KEY_CONNECTION,
    HTTP_HEADER_KEY_CONTENT_DISPOSITION,
    HTTP_HEADER_KEY_CONTENT_ENCODING,
    HTTP_HEADER_KEY_CONTENT_LANGUAGE,
    HTTP_HEADER_KEY_CONTENT_LENGTH,
    HTTP_HEADER_KEY_CONTENT_LOCATION,
    HTTP_HEADER_KEY_CONTENT_MD5,
    HTTP_HEADER_KEY_CONTENT_RANGE,
    HTTP_HEADER_KEY_CONTENT_TYPE,
    HTTP_HEADER_KEY_COOKIE,
    HTTP_HEADER_KEY_DATE,
    HTTP_HEADER_KEY_ETAG,
    HTTP_HEADER_KEY_EXPECT,
    HTTP_HEADER_KEY_EXPIRES,
    HTTP_HEADER_KEY_FROM,
    HTTP_HEADER_KEY_IF_MATCH,
    HTTP_HEADER_KEY_IF_MODIFIED_SINCE,
    HTTP_HEADER_KEY_IF_NONE_MATCH,
    HTTP_HEADER_KEY_IF_RANGE,
    HTTP_HEADER_KEY_IF_UNMODIFIED_SINCE,
    HTTP_HEADER_KEY_LAST_MODIFIED,
    HTTP_HEADER_KEY_LINK,
    HTTP_HEADER_KEY_LOCATION,
    HTTP_HEADER_KEY_MAX_FORWARDS,
    HTTP_HEADER_KEY_ORIGIN,
    HTTP_HEADER_KEY_PRAGMA,
    HTTP_HEADER_KEY_PROXY_AUTHENTICATE,
    HTTP_HEADER_KEY_PROXY_AUTHORIZATION,
    HTTP_HEADER_KEY_RANGE,
    HTTP_HEADER_KEY_REFERER,
    HTTP_HEADER_KEY_RETRY_AFTER,
    HTTP_HEADER_KEY_SERVER,
    HTTP_HEADER_KEY_SET_COOKIE,
    HTTP_HEADER_KEY_STRICT_TRANSPORT_SECURITY,
    HTTP_HEADER_KEY_TE,
    HTTP_HEADER_KEY_TRAILER,
    HTTP_HEADER_KEY_TRANSFER_ENCODING,
    HTTP_HEADER_KEY_UPGRADE,
    HTTP_HEADER_KEY_VARY,
    HTTP_HEADER_KEY_VIA,
    HTTP_HEADER_KEY_WARNING,
    HTTP_HEADER_KEY_WWW_AUTHENTICATE,

    // Common Non-Standard (X-*) and Extension Headers
    HTTP_HEADER_KEY_X_FORWARDED_FOR,
    HTTP_HEADER_KEY_X_FORWARDED_HOST,
    HTTP_HEADER_KEY_X_FORWARDED_PROTO,
    HTTP_HEADER_KEY_X_REQUESTED_WITH,
    HTTP_HEADER_KEY_X_CSRF_TOKEN,
    HTTP_HEADER_KEY_X_XSS_PROTECTION,
    HTTP_HEADER_KEY_X_CONTENT_TYPE_OPTIONS,
    HTTP_HEADER_KEY_X_FRAME_OPTIONS,
    HTTP_HEADER_KEY_X_POWERED_BY,
    HTTP_HEADER_KEY_X_UPLOAD_ID,
    HTTP_HEADER_KEY_X_RATE_LIMIT_LIMIT,
    HTTP_HEADER_KEY_X_RATE_LIMIT_REMAINING,
    HTTP_HEADER_KEY_X_RATE_LIMIT_RESET,
    HTTP_HEADER_KEY_X_UA_COMPATIBLE,
    HTTP_HEADER_KEY_X_DNS_PREFETCH_CONTROL,
    HTTP_HEADER_KEY_X_DOWNLOAD_OPTIONS,
    HTTP_HEADER_KEY_X_PERMITTED_CROSS_DOMAIN_POLICIES,

    // CORS Headers
    HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_ORIGIN,
    HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_CREDENTIALS,
    HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_HEADERS,
    HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_METHODS,
    HTTP_HEADER_KEY_ACCESS_CONTROL_EXPOSE_HEADERS,
    HTTP_HEADER_KEY_ACCESS_CONTROL_MAX_AGE,
    HTTP_HEADER_KEY_ACCESS_CONTROL_REQUEST_HEADERS,
    HTTP_HEADER_KEY_ACCESS_CONTROL_REQUEST_METHOD,

    // Security Headers
    HTTP_HEADER_KEY_CONTENT_SECURITY_POLICY,
    HTTP_HEADER_KEY_PERMISSIONS_POLICY,
    HTTP_HEADER_KEY_REFERRER_POLICY,
    HTTP_HEADER_KEY_EXPECT_CT,
    HTTP_HEADER_KEY_FEATURE_POLICY,
    HTTP_HEADER_KEY_CROSS_ORIGIN_EMBEDDER_POLICY,
    HTTP_HEADER_KEY_CROSS_ORIGIN_OPENER_POLICY,
    HTTP_HEADER_KEY_CROSS_ORIGIN_RESOURCE_POLICY,

    // WebSocket Headers
    HTTP_HEADER_KEY_SEC_WEBSOCKET_KEY,
    HTTP_HEADER_KEY_SEC_WEBSOCKET_ACCEPT,
    HTTP_HEADER_KEY_SEC_WEBSOCKET_VERSION,
    HTTP_HEADER_KEY_SEC_WEBSOCKET_PROTOCOL,
    HTTP_HEADER_KEY_SEC_WEBSOCKET_EXTENSIONS,

    // HTTP/3 and QUIC Headers
    HTTP_HEADER_KEY_ALT_SVC,
    HTTP_HEADER_KEY_EARLY_DATA,

    // Cloud & CDN Headers
    HTTP_HEADER_KEY_CF_CONNECTING_IP,
    HTTP_HEADER_KEY_CF_IPCOUNTRY,
    HTTP_HEADER_KEY_CF_RAY,
    HTTP_HEADER_KEY_TRUE_CLIENT_IP,
    HTTP_HEADER_KEY_X_AMZ_CF_ID,
    HTTP_HEADER_KEY_X_AMZN_TRACE_ID,

    // Custom/Experimental Headers
    HTTP_HEADER_KEY_DNT, // Do Not Track
    HTTP_HEADER_KEY_SAVE_DATA,
    HTTP_HEADER_KEY_DOWNLINK,
    HTTP_HEADER_KEY_ECT, // Effective Connection Type
    HTTP_HEADER_KEY_RTT,
    HTTP_HEADER_KEY_PURPOSE,
    HTTP_HEADER_KEY_SEC_FETCH_SITE,
    HTTP_HEADER_KEY_SEC_FETCH_MODE,
    HTTP_HEADER_KEY_SEC_FETCH_USER,
    HTTP_HEADER_KEY_SEC_FETCH_DEST,
    HTTP_HEADER_KEY_SERVICE_WORKER_NAVIGATION_PRELOAD,
    HTTP_HEADER_KEY_LAST_EVENT_ID,
    HTTP_HEADER_KEY_REPORT_TO,
    HTTP_HEADER_KEY_PRIORITY,
    HTTP_HEADER_KEY_SIGNATURE,
    HTTP_HEADER_KEY_SIGNATURE_KEY,
    HTTP_HEADER_KEY_FORWARDED,
    HTTP_HEADER_KEY_ORIGINAL_METHOD,
    HTTP_HEADER_KEY_ORIGINAL_URL,
    HTTP_HEADER_KEY_ORIGINAL_HOST,
};

const char* http_header_key_text(HttpHeaderKey header_key) {
    switch (header_key) {
        case HTTP_HEADER_KEY_HOST:
            return "Host";
        case HTTP_HEADER_KEY_USER_AGENT:
            return "User-Agent";
        case HTTP_HEADER_KEY_ACCEPT:
            return "Accept";
        case HTTP_HEADER_KEY_ACCEPT_CHARSET:
            return "Accept-Charset";
        case HTTP_HEADER_KEY_ACCEPT_ENCODING:
            return "Accept-Encoding";
        case HTTP_HEADER_KEY_ACCEPT_LANGUAGE:
            return "Accept-Language";
        case HTTP_HEADER_KEY_ACCEPT_DATETIME:
            return "Accept-Datetime";
        case HTTP_HEADER_KEY_ACCEPT_PATCH:
            return "Accept-Patch";
        case HTTP_HEADER_KEY_ACCEPT_RANGES:
            return "Accept-Ranges";
        case HTTP_HEADER_KEY_AGE:
            return "Age";
        case HTTP_HEADER_KEY_ALLOW:
            return "Allow";
        case HTTP_HEADER_KEY_AUTHORIZATION:
            return "Authorization";
        case HTTP_HEADER_KEY_CACHE_CONTROL:
            return "Cache-Control";
        case HTTP_HEADER_KEY_CONNECTION:
            return "Connection";
        case HTTP_HEADER_KEY_CONTENT_DISPOSITION:
            return "Content-Disposition";
        case HTTP_HEADER_KEY_CONTENT_ENCODING:
            return "Content-Encoding";
        case HTTP_HEADER_KEY_CONTENT_LANGUAGE:
            return "Content-Language";
        case HTTP_HEADER_KEY_CONTENT_LENGTH:
            return "Content-Length";
        case HTTP_HEADER_KEY_CONTENT_LOCATION:
            return "Content-Location";
        case HTTP_HEADER_KEY_CONTENT_MD5:
            return "Content-Md5";
        case HTTP_HEADER_KEY_CONTENT_RANGE:
            return "Content-Range";
        case HTTP_HEADER_KEY_CONTENT_TYPE:
            return "Content-Type";
        case HTTP_HEADER_KEY_COOKIE:
            return "Cookie";
        case HTTP_HEADER_KEY_DATE:
            return "Date";
        case HTTP_HEADER_KEY_ETAG:
            return "Etag";
        case HTTP_HEADER_KEY_EXPECT:
            return "Expect";
        case HTTP_HEADER_KEY_EXPIRES:
            return "Expires";
        case HTTP_HEADER_KEY_FROM:
            return "From";
        case HTTP_HEADER_KEY_IF_MATCH:
            return "If-Match";
        case HTTP_HEADER_KEY_IF_MODIFIED_SINCE:
            return "If-Modified-Since";
        case HTTP_HEADER_KEY_IF_NONE_MATCH:
            return "If-None-Match";
        case HTTP_HEADER_KEY_IF_RANGE:
            return "If-Range";
        case HTTP_HEADER_KEY_IF_UNMODIFIED_SINCE:
            return "If-Unmodified-Since";
        case HTTP_HEADER_KEY_LAST_MODIFIED:
            return "Last-Modified";
        case HTTP_HEADER_KEY_LINK:
            return "Link";
        case HTTP_HEADER_KEY_LOCATION:
            return "Location";
        case HTTP_HEADER_KEY_MAX_FORWARDS:
            return "Max-Forwards";
        case HTTP_HEADER_KEY_ORIGIN:
            return "Origin";
        case HTTP_HEADER_KEY_PRAGMA:
            return "Pragma";
        case HTTP_HEADER_KEY_PROXY_AUTHENTICATE:
            return "Proxy-Authenticate";
        case HTTP_HEADER_KEY_PROXY_AUTHORIZATION:
            return "Proxy-Authorization";
        case HTTP_HEADER_KEY_RANGE:
            return "Range";
        case HTTP_HEADER_KEY_REFERER:
            return "Referer";
        case HTTP_HEADER_KEY_RETRY_AFTER:
            return "Retry-After";
        case HTTP_HEADER_KEY_SERVER:
            return "Server";
        case HTTP_HEADER_KEY_SET_COOKIE:
            return "Set-Cookie";
        case HTTP_HEADER_KEY_STRICT_TRANSPORT_SECURITY:
            return "Strict-Transport-Security";
        case HTTP_HEADER_KEY_TE:
            return "Te";
        case HTTP_HEADER_KEY_TRAILER:
            return "Trailer";
        case HTTP_HEADER_KEY_TRANSFER_ENCODING:
            return "Transfer-Encoding";
        case HTTP_HEADER_KEY_UPGRADE:
            return "Upgrade";
        case HTTP_HEADER_KEY_VARY:
            return "Vary";
        case HTTP_HEADER_KEY_VIA:
            return "Via";
        case HTTP_HEADER_KEY_WARNING:
            return "Warning";
        case HTTP_HEADER_KEY_WWW_AUTHENTICATE:
            return "Www-Authenticate";
        case HTTP_HEADER_KEY_X_FORWARDED_FOR:
            return "X-Forwarded-For";
        case HTTP_HEADER_KEY_X_FORWARDED_HOST:
            return "X-Forwarded-Host";
        case HTTP_HEADER_KEY_X_FORWARDED_PROTO:
            return "X-Forwarded-Proto";
        case HTTP_HEADER_KEY_X_REQUESTED_WITH:
            return "X-Requested-With";
        case HTTP_HEADER_KEY_X_CSRF_TOKEN:
            return "X-Csrf-Token";
        case HTTP_HEADER_KEY_X_XSS_PROTECTION:
            return "X-Xss-Protection";
        case HTTP_HEADER_KEY_X_CONTENT_TYPE_OPTIONS:
            return "X-Content-Type-Options";
        case HTTP_HEADER_KEY_X_FRAME_OPTIONS:
            return "X-Frame-Options";
        case HTTP_HEADER_KEY_X_POWERED_BY:
            return "X-Powered-By";
        case HTTP_HEADER_KEY_X_UPLOAD_ID:
            return "X-Upload-Id";
        case HTTP_HEADER_KEY_X_RATE_LIMIT_LIMIT:
            return "X-Rate-Limit-Limit";
        case HTTP_HEADER_KEY_X_RATE_LIMIT_REMAINING:
            return "X-Rate-Limit-Remaining";
        case HTTP_HEADER_KEY_X_RATE_LIMIT_RESET:
            return "X-Rate-Limit-Reset";
        case HTTP_HEADER_KEY_X_UA_COMPATIBLE:
            return "X-Ua-Compatible";
        case HTTP_HEADER_KEY_X_DNS_PREFETCH_CONTROL:
            return "X-Dns-Prefetch-Control";
        case HTTP_HEADER_KEY_X_DOWNLOAD_OPTIONS:
            return "X-Download-Options";
        case HTTP_HEADER_KEY_X_PERMITTED_CROSS_DOMAIN_POLICIES:
            return "X-Permitted-Cross-Domain-Policies";
        case HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_ORIGIN:
            return "Access-Control-Allow-Origin";
        case HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_CREDENTIALS:
            return "Access-Control-Allow-Credentials";
        case HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_HEADERS:
            return "Access-Control-Allow-Headers";
        case HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_METHODS:
            return "Access-Control-Allow-Methods";
        case HTTP_HEADER_KEY_ACCESS_CONTROL_EXPOSE_HEADERS:
            return "Access-Control-Expose-Headers";
        case HTTP_HEADER_KEY_ACCESS_CONTROL_MAX_AGE:
            return "Access-Control-Max-Age";
        case HTTP_HEADER_KEY_ACCESS_CONTROL_REQUEST_HEADERS:
            return "Access-Control-Request-Headers";
        case HTTP_HEADER_KEY_ACCESS_CONTROL_REQUEST_METHOD:
            return "Access-Control-Request-Method";
        case HTTP_HEADER_KEY_CONTENT_SECURITY_POLICY:
            return "Content-Security-Policy";
        case HTTP_HEADER_KEY_PERMISSIONS_POLICY:
            return "Permissions-Policy";
        case HTTP_HEADER_KEY_REFERRER_POLICY:
            return "Referrer-Policy";
        case HTTP_HEADER_KEY_EXPECT_CT:
            return "Expect-Ct";
        case HTTP_HEADER_KEY_FEATURE_POLICY:
            return "Feature-Policy";
        case HTTP_HEADER_KEY_CROSS_ORIGIN_EMBEDDER_POLICY:
            return "Cross-Origin-Embedder-Policy";
        case HTTP_HEADER_KEY_CROSS_ORIGIN_OPENER_POLICY:
            return "Cross-Origin-Opener-Policy";
        case HTTP_HEADER_KEY_CROSS_ORIGIN_RESOURCE_POLICY:
            return "Cross-Origin-Resource-Policy";
        case HTTP_HEADER_KEY_SEC_WEBSOCKET_KEY:
            return "Sec-Websocket-Key";
        case HTTP_HEADER_KEY_SEC_WEBSOCKET_ACCEPT:
            return "Sec-Websocket-Accept";
        case HTTP_HEADER_KEY_SEC_WEBSOCKET_VERSION:
            return "Sec-Websocket-Version";
        case HTTP_HEADER_KEY_SEC_WEBSOCKET_PROTOCOL:
            return "Sec-Websocket-Protocol";
        case HTTP_HEADER_KEY_SEC_WEBSOCKET_EXTENSIONS:
            return "Sec-Websocket-Extensions";
        case HTTP_HEADER_KEY_ALT_SVC:
            return "Alt-Svc";
        case HTTP_HEADER_KEY_EARLY_DATA:
            return "Early-Data";
        case HTTP_HEADER_KEY_CF_CONNECTING_IP:
            return "Cf-Connecting-Ip";
        case HTTP_HEADER_KEY_CF_IPCOUNTRY:
            return "Cf-Ipcountry";
        case HTTP_HEADER_KEY_CF_RAY:
            return "Cf-Ray";
        case HTTP_HEADER_KEY_TRUE_CLIENT_IP:
            return "True-Client-Ip";
        case HTTP_HEADER_KEY_X_AMZ_CF_ID:
            return "X-Amz-Cf-Id";
        case HTTP_HEADER_KEY_X_AMZN_TRACE_ID:
            return "X-Amzn-Trace-Id";
        case HTTP_HEADER_KEY_DNT:
            return "Dnt";
        case HTTP_HEADER_KEY_SAVE_DATA:
            return "Save-Data";
        case HTTP_HEADER_KEY_DOWNLINK:
            return "Downlink";
        case HTTP_HEADER_KEY_ECT:
            return "Ect";
        case HTTP_HEADER_KEY_RTT:
            return "Rtt";
        case HTTP_HEADER_KEY_PURPOSE:
            return "Purpose";
        case HTTP_HEADER_KEY_SEC_FETCH_SITE:
            return "Sec-Fetch-Site";
        case HTTP_HEADER_KEY_SEC_FETCH_MODE:
            return "Sec-Fetch-Mode";
        case HTTP_HEADER_KEY_SEC_FETCH_USER:
            return "Sec-Fetch-User";
        case HTTP_HEADER_KEY_SEC_FETCH_DEST:
            return "Sec-Fetch-Dest";
        case HTTP_HEADER_KEY_SERVICE_WORKER_NAVIGATION_PRELOAD:
            return "Service-Worker-Navigation-Preload";
        case HTTP_HEADER_KEY_LAST_EVENT_ID:
            return "Last-Event-Id";
        case HTTP_HEADER_KEY_REPORT_TO:
            return "Report-To";
        case HTTP_HEADER_KEY_PRIORITY:
            return "Priority";
        case HTTP_HEADER_KEY_SIGNATURE:
            return "Signature";
        case HTTP_HEADER_KEY_SIGNATURE_KEY:
            return "Signature-Key";
        case HTTP_HEADER_KEY_FORWARDED:
            return "Forwarded";
        case HTTP_HEADER_KEY_ORIGINAL_METHOD:
            return "Original-Method";
        case HTTP_HEADER_KEY_ORIGINAL_URL:
            return "Original-Url";
        case HTTP_HEADER_KEY_ORIGINAL_HOST:
            return "Original-Host";
        default:
            UNREACHABLE();
    }
}

HttpHeaderKey http_header_key_text(const char* header) {
    if (str_compare_caseless(header, "Host:") == 0) {
        return HTTP_HEADER_KEY_HOST;
    } else if (str_compare_caseless(header, "User-Agent:") == 0) {
        return HTTP_HEADER_KEY_USER_AGENT;
    } else if (str_compare_caseless(header, "Accept:") == 0) {
        return HTTP_HEADER_KEY_ACCEPT;
    } else if (str_compare_caseless(header, "Accept-Charset:") == 0) {
        return HTTP_HEADER_KEY_ACCEPT_CHARSET;
    } else if (str_compare_caseless(header, "Accept-Encoding:") == 0) {
        return HTTP_HEADER_KEY_ACCEPT_ENCODING;
    } else if (str_compare_caseless(header, "Accept-Language:") == 0) {
        return HTTP_HEADER_KEY_ACCEPT_LANGUAGE;
    } else if (str_compare_caseless(header, "Accept-Datetime:") == 0) {
        return HTTP_HEADER_KEY_ACCEPT_DATETIME;
    } else if (str_compare_caseless(header, "Accept-Patch:") == 0) {
        return HTTP_HEADER_KEY_ACCEPT_PATCH;
    } else if (str_compare_caseless(header, "Accept-Ranges:") == 0) {
        return HTTP_HEADER_KEY_ACCEPT_RANGES;
    } else if (str_compare_caseless(header, "Age:") == 0) {
        return HTTP_HEADER_KEY_AGE;
    } else if (str_compare_caseless(header, "Allow:") == 0) {
        return HTTP_HEADER_KEY_ALLOW;
    } else if (str_compare_caseless(header, "Authorization:") == 0) {
        return HTTP_HEADER_KEY_AUTHORIZATION;
    } else if (str_compare_caseless(header, "Cache-Control:") == 0) {
        return HTTP_HEADER_KEY_CACHE_CONTROL;
    } else if (str_compare_caseless(header, "Connection:") == 0) {
        return HTTP_HEADER_KEY_CONNECTION;
    } else if (str_compare_caseless(header, "Content-Disposition:") == 0) {
        return HTTP_HEADER_KEY_CONTENT_DISPOSITION;
    } else if (str_compare_caseless(header, "Content-Encoding:") == 0) {
        return HTTP_HEADER_KEY_CONTENT_ENCODING;
    } else if (str_compare_caseless(header, "Content-Language:") == 0) {
        return HTTP_HEADER_KEY_CONTENT_LANGUAGE;
    } else if (str_compare_caseless(header, "Content-Length:") == 0) {
        return HTTP_HEADER_KEY_CONTENT_LENGTH;
    } else if (str_compare_caseless(header, "Content-Location:") == 0) {
        return HTTP_HEADER_KEY_CONTENT_LOCATION;
    } else if (str_compare_caseless(header, "Content-Md5:") == 0) {
        return HTTP_HEADER_KEY_CONTENT_MD5;
    } else if (str_compare_caseless(header, "Content-Range:") == 0) {
        return HTTP_HEADER_KEY_CONTENT_RANGE;
    } else if (str_compare_caseless(header, "Content-Type:") == 0) {
        return HTTP_HEADER_KEY_CONTENT_TYPE;
    } else if (str_compare_caseless(header, "Cookie:") == 0) {
        return HTTP_HEADER_KEY_COOKIE;
    } else if (str_compare_caseless(header, "Date:") == 0) {
        return HTTP_HEADER_KEY_DATE;
    } else if (str_compare_caseless(header, "Etag:") == 0) {
        return HTTP_HEADER_KEY_ETAG;
    } else if (str_compare_caseless(header, "Expect:") == 0) {
        return HTTP_HEADER_KEY_EXPECT;
    } else if (str_compare_caseless(header, "Expires:") == 0) {
        return HTTP_HEADER_KEY_EXPIRES;
    } else if (str_compare_caseless(header, "From:") == 0) {
        return HTTP_HEADER_KEY_FROM;
    } else if (str_compare_caseless(header, "If-Match:") == 0) {
        return HTTP_HEADER_KEY_IF_MATCH;
    } else if (str_compare_caseless(header, "If-Modified-Since:") == 0) {
        return HTTP_HEADER_KEY_IF_MODIFIED_SINCE;
    } else if (str_compare_caseless(header, "If-None-Match:") == 0) {
        return HTTP_HEADER_KEY_IF_NONE_MATCH;
    } else if (str_compare_caseless(header, "If-Range:") == 0) {
        return HTTP_HEADER_KEY_IF_RANGE;
    } else if (str_compare_caseless(header, "If-Unmodified-Since:") == 0) {
        return HTTP_HEADER_KEY_IF_UNMODIFIED_SINCE;
    } else if (str_compare_caseless(header, "Last-Modified:") == 0) {
        return HTTP_HEADER_KEY_LAST_MODIFIED;
    } else if (str_compare_caseless(header, "Link:") == 0) {
        return HTTP_HEADER_KEY_LINK;
    } else if (str_compare_caseless(header, "Location:") == 0) {
        return HTTP_HEADER_KEY_LOCATION;
    } else if (str_compare_caseless(header, "Max-Forwards:") == 0) {
        return HTTP_HEADER_KEY_MAX_FORWARDS;
    } else if (str_compare_caseless(header, "Origin:") == 0) {
        return HTTP_HEADER_KEY_ORIGIN;
    } else if (str_compare_caseless(header, "Pragma:") == 0) {
        return HTTP_HEADER_KEY_PRAGMA;
    } else if (str_compare_caseless(header, "Proxy-Authenticate:") == 0) {
        return HTTP_HEADER_KEY_PROXY_AUTHENTICATE;
    } else if (str_compare_caseless(header, "Proxy-Authorization:") == 0) {
        return HTTP_HEADER_KEY_PROXY_AUTHORIZATION;
    } else if (str_compare_caseless(header, "Range:") == 0) {
        return HTTP_HEADER_KEY_RANGE;
    } else if (str_compare_caseless(header, "Referer:") == 0) {
        return HTTP_HEADER_KEY_REFERER;
    } else if (str_compare_caseless(header, "Retry-After:") == 0) {
        return HTTP_HEADER_KEY_RETRY_AFTER;
    } else if (str_compare_caseless(header, "Server:") == 0) {
        return HTTP_HEADER_KEY_SERVER;
    } else if (str_compare_caseless(header, "Set-Cookie:") == 0) {
        return HTTP_HEADER_KEY_SET_COOKIE;
    } else if (str_compare_caseless(header, "Strict-Transport-Security:") == 0) {
        return HTTP_HEADER_KEY_STRICT_TRANSPORT_SECURITY;
    } else if (str_compare_caseless(header, "Te:") == 0) {
        return HTTP_HEADER_KEY_TE;
    } else if (str_compare_caseless(header, "Trailer:") == 0) {
        return HTTP_HEADER_KEY_TRAILER;
    } else if (str_compare_caseless(header, "Transfer-Encoding:") == 0) {
        return HTTP_HEADER_KEY_TRANSFER_ENCODING;
    } else if (str_compare_caseless(header, "Upgrade:") == 0) {
        return HTTP_HEADER_KEY_UPGRADE;
    } else if (str_compare_caseless(header, "Vary:") == 0) {
        return HTTP_HEADER_KEY_VARY;
    } else if (str_compare_caseless(header, "Via:") == 0) {
        return HTTP_HEADER_KEY_VIA;
    } else if (str_compare_caseless(header, "Warning:") == 0) {
        return HTTP_HEADER_KEY_WARNING;
    } else if (str_compare_caseless(header, "Www-Authenticate:") == 0) {
        return HTTP_HEADER_KEY_WWW_AUTHENTICATE;
    } else if (str_compare_caseless(header, "X-Forwarded-For:") == 0) {
        return HTTP_HEADER_KEY_X_FORWARDED_FOR;
    } else if (str_compare_caseless(header, "X-Forwarded-Host:") == 0) {
        return HTTP_HEADER_KEY_X_FORWARDED_HOST;
    } else if (str_compare_caseless(header, "X-Forwarded-Proto:") == 0) {
        return HTTP_HEADER_KEY_X_FORWARDED_PROTO;
    } else if (str_compare_caseless(header, "X-Requested-With:") == 0) {
        return HTTP_HEADER_KEY_X_REQUESTED_WITH;
    } else if (str_compare_caseless(header, "X-Csrf-Token:") == 0) {
        return HTTP_HEADER_KEY_X_CSRF_TOKEN;
    } else if (str_compare_caseless(header, "X-Xss-Protection:") == 0) {
        return HTTP_HEADER_KEY_X_XSS_PROTECTION;
    } else if (str_compare_caseless(header, "X-Content-Type-Options:") == 0) {
        return HTTP_HEADER_KEY_X_CONTENT_TYPE_OPTIONS;
    } else if (str_compare_caseless(header, "X-Frame-Options:") == 0) {
        return HTTP_HEADER_KEY_X_FRAME_OPTIONS;
    } else if (str_compare_caseless(header, "X-Powered-By:") == 0) {
        return HTTP_HEADER_KEY_X_POWERED_BY;
    } else if (str_compare_caseless(header, "X-Upload-Id:") == 0) {
        return HTTP_HEADER_KEY_X_UPLOAD_ID;
    } else if (str_compare_caseless(header, "X-Rate-Limit-Limit:") == 0) {
        return HTTP_HEADER_KEY_X_RATE_LIMIT_LIMIT;
    } else if (str_compare_caseless(header, "X-Rate-Limit-Remaining:") == 0) {
        return HTTP_HEADER_KEY_X_RATE_LIMIT_REMAINING;
    } else if (str_compare_caseless(header, "X-Rate-Limit-Reset:") == 0) {
        return HTTP_HEADER_KEY_X_RATE_LIMIT_RESET;
    } else if (str_compare_caseless(header, "X-Ua-Compatible:") == 0) {
        return HTTP_HEADER_KEY_X_UA_COMPATIBLE;
    } else if (str_compare_caseless(header, "X-Dns-Prefetch-Control:") == 0) {
        return HTTP_HEADER_KEY_X_DNS_PREFETCH_CONTROL;
    } else if (str_compare_caseless(header, "X-Download-Options:") == 0) {
        return HTTP_HEADER_KEY_X_DOWNLOAD_OPTIONS;
    } else if (str_compare_caseless(header, "X-Permitted-Cross-Domain-Policies:") == 0) {
        return HTTP_HEADER_KEY_X_PERMITTED_CROSS_DOMAIN_POLICIES;
    } else if (str_compare_caseless(header, "Access-Control-Allow-Origin:") == 0) {
        return HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_ORIGIN;
    } else if (str_compare_caseless(header, "Access-Control-Allow-Credentials:") == 0) {
        return HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_CREDENTIALS;
    } else if (str_compare_caseless(header, "Access-Control-Allow-Headers:") == 0) {
        return HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_HEADERS;
    } else if (str_compare_caseless(header, "Access-Control-Allow-Methods:") == 0) {
        return HTTP_HEADER_KEY_ACCESS_CONTROL_ALLOW_METHODS;
    } else if (str_compare_caseless(header, "Access-Control-Expose-Headers:") == 0) {
        return HTTP_HEADER_KEY_ACCESS_CONTROL_EXPOSE_HEADERS;
    } else if (str_compare_caseless(header, "Access-Control-Max-Age:") == 0) {
        return HTTP_HEADER_KEY_ACCESS_CONTROL_MAX_AGE;
    } else if (str_compare_caseless(header, "Access-Control-Request-Headers:") == 0) {
        return HTTP_HEADER_KEY_ACCESS_CONTROL_REQUEST_HEADERS;
    } else if (str_compare_caseless(header, "Access-Control-Request-Method:") == 0) {
        return HTTP_HEADER_KEY_ACCESS_CONTROL_REQUEST_METHOD;
    } else if (str_compare_caseless(header, "Content-Security-Policy:") == 0) {
        return HTTP_HEADER_KEY_CONTENT_SECURITY_POLICY;
    } else if (str_compare_caseless(header, "Permissions-Policy:") == 0) {
        return HTTP_HEADER_KEY_PERMISSIONS_POLICY;
    } else if (str_compare_caseless(header, "Referrer-Policy:") == 0) {
        return HTTP_HEADER_KEY_REFERRER_POLICY;
    } else if (str_compare_caseless(header, "Expect-Ct:") == 0) {
        return HTTP_HEADER_KEY_EXPECT_CT;
    } else if (str_compare_caseless(header, "Feature-Policy:") == 0) {
        return HTTP_HEADER_KEY_FEATURE_POLICY;
    } else if (str_compare_caseless(header, "Cross-Origin-Embedder-Policy:") == 0) {
        return HTTP_HEADER_KEY_CROSS_ORIGIN_EMBEDDER_POLICY;
    } else if (str_compare_caseless(header, "Cross-Origin-Opener-Policy:") == 0) {
        return HTTP_HEADER_KEY_CROSS_ORIGIN_OPENER_POLICY;
    } else if (str_compare_caseless(header, "Cross-Origin-Resource-Policy:") == 0) {
        return HTTP_HEADER_KEY_CROSS_ORIGIN_RESOURCE_POLICY;
    } else if (str_compare_caseless(header, "Sec-Websocket-Key:") == 0) {
        return HTTP_HEADER_KEY_SEC_WEBSOCKET_KEY;
    } else if (str_compare_caseless(header, "Sec-Websocket-Accept:") == 0) {
        return HTTP_HEADER_KEY_SEC_WEBSOCKET_ACCEPT;
    } else if (str_compare_caseless(header, "Sec-Websocket-Version:") == 0) {
        return HTTP_HEADER_KEY_SEC_WEBSOCKET_VERSION;
    } else if (str_compare_caseless(header, "Sec-Websocket-Protocol:") == 0) {
        return HTTP_HEADER_KEY_SEC_WEBSOCKET_PROTOCOL;
    } else if (str_compare_caseless(header, "Sec-Websocket-Extensions:") == 0) {
        return HTTP_HEADER_KEY_SEC_WEBSOCKET_EXTENSIONS;
    } else if (str_compare_caseless(header, "Alt-Svc:") == 0) {
        return HTTP_HEADER_KEY_ALT_SVC;
    } else if (str_compare_caseless(header, "Early-Data:") == 0) {
        return HTTP_HEADER_KEY_EARLY_DATA;
    } else if (str_compare_caseless(header, "Cf-Connecting-Ip:") == 0) {
        return HTTP_HEADER_KEY_CF_CONNECTING_IP;
    } else if (str_compare_caseless(header, "Cf-Ipcountry:") == 0) {
        return HTTP_HEADER_KEY_CF_IPCOUNTRY;
    } else if (str_compare_caseless(header, "Cf-Ray:") == 0) {
        return HTTP_HEADER_KEY_CF_RAY;
    } else if (str_compare_caseless(header, "True-Client-Ip:") == 0) {
        return HTTP_HEADER_KEY_TRUE_CLIENT_IP;
    } else if (str_compare_caseless(header, "X-Amz-Cf-Id:") == 0) {
        return HTTP_HEADER_KEY_X_AMZ_CF_ID;
    } else if (str_compare_caseless(header, "X-Amzn-Trace-Id:") == 0) {
        return HTTP_HEADER_KEY_X_AMZN_TRACE_ID;
    } else if (str_compare_caseless(header, "Dnt:") == 0) {
        return HTTP_HEADER_KEY_DNT;
    } else if (str_compare_caseless(header, "Save-Data:") == 0) {
        return HTTP_HEADER_KEY_SAVE_DATA;
    } else if (str_compare_caseless(header, "Downlink:") == 0) {
        return HTTP_HEADER_KEY_DOWNLINK;
    } else if (str_compare_caseless(header, "Ect:") == 0) {
        return HTTP_HEADER_KEY_ECT;
    } else if (str_compare_caseless(header, "Rtt:") == 0) {
        return HTTP_HEADER_KEY_RTT;
    } else if (str_compare_caseless(header, "Purpose:") == 0) {
        return HTTP_HEADER_KEY_PURPOSE;
    } else if (str_compare_caseless(header, "Sec-Fetch-Site:") == 0) {
        return HTTP_HEADER_KEY_SEC_FETCH_SITE;
    } else if (str_compare_caseless(header, "Sec-Fetch-Mode:") == 0) {
        return HTTP_HEADER_KEY_SEC_FETCH_MODE;
    } else if (str_compare_caseless(header, "Sec-Fetch-User:") == 0) {
        return HTTP_HEADER_KEY_SEC_FETCH_USER;
    } else if (str_compare_caseless(header, "Sec-Fetch-Dest:") == 0) {
        return HTTP_HEADER_KEY_SEC_FETCH_DEST;
    } else if (str_compare_caseless(header, "Service-Worker-Navigation-Preload:") == 0) {
        return HTTP_HEADER_KEY_SERVICE_WORKER_NAVIGATION_PRELOAD;
    } else if (str_compare_caseless(header, "Last-Event-Id:") == 0) {
        return HTTP_HEADER_KEY_LAST_EVENT_ID;
    } else if (str_compare_caseless(header, "Report-To:") == 0) {
        return HTTP_HEADER_KEY_REPORT_TO;
    } else if (str_compare_caseless(header, "Priority:") == 0) {
        return HTTP_HEADER_KEY_PRIORITY;
    } else if (str_compare_caseless(header, "Signature:") == 0) {
        return HTTP_HEADER_KEY_SIGNATURE;
    } else if (str_compare_caseless(header, "Signature-Key:") == 0) {
        return HTTP_HEADER_KEY_SIGNATURE_KEY;
    } else if (str_compare_caseless(header, "Forwarded:") == 0) {
        return HTTP_HEADER_KEY_FORWARDED;
    } else if (str_compare_caseless(header, "Original-Method:") == 0) {
        return HTTP_HEADER_KEY_ORIGINAL_METHOD;
    } else if (str_compare_caseless(header, "Original-Url:") == 0) {
        return HTTP_HEADER_KEY_ORIGINAL_URL;
    } else if (str_compare_caseless(header, "Original-Host:") == 0) {
        return HTTP_HEADER_KEY_ORIGINAL_HOST;
    }

    return HTTP_HEADER_KEY_UNKNOWN;
}

#endif