/*
 * ngx_http_markdown_module
 * Copyright(c) David Zhang, 2014.
 */

#ifndef NGX_HTTP_MARKDOWN_MODULE_H
#define NGX_HTTP_MARKDOWN_MODULE_H

static ngx_int_t g_markdown_buffer_size = 10240;
static ngx_str_t g_markdown_path        = "./html/markdown";
static ngx_str_t g_markdown_html_header = "./html/header.html";
static ngx_str_t g_markdown_html_footer = "./html/footer.html";

typedef struct {
    ngx_flag_t   md_is_enabled;
    ngx_int_t    md_buffer_size;
    ngx_path_t   md_html_header;
    ngx_path_t   md_html_footer;
} ngx_http_markdown_conf_t;

static void *ngx_http_markdown_create_loc_conf(ngx_conf_t *cf);
static void *ngx_http_markdown_init(ngx_http_request_t *r);
static void *ngx_http_markdown_header_filter(ngx_http_request_t *r);
static void *ngx_http_markdown_body_filter(ngx_http_request_t *r, ngx_chain_t *in);

#endif

