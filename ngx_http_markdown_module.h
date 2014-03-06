/*
 *
 */

#ifndef NGX_HTTP_MARKDOWN_MODULE_H
#define NGX_HTTP_MARKDOWN_MODULE_H

#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_config.h>

typedef struct {
    ngx_flag_t is_enable;
    ngx_uint_t engine;
} ngx_http_markdown_conf_t;

static char *ngx_http_markdown_conf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_markdown_handler(ngx_http_request_t *r);

#endif
