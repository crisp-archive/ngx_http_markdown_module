/*
 * ngx_http_markdown_module
 * Copyright(c) David Zhang, 2014
 */

#include <ngx_core.h>
#include <ngx_http.h>

#include "markdown_lib.h"

static ngx_uint_t ngx_markdown_default_buffer_size = 10240;
static const ngx_str_t ngx_markdown_default_html_header = ngx_string("./html/header.html");
static const ngx_str_t ngx_markdown_default_html_footer = ngx_string("./html/footer.html");

#define ngx_markdown_to_string markdown_to_string
static void *ngx_http_markdown_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_markdown_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_markdown_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_markdown_handler(ngx_http_request_t *r);

typedef struct {
    ngx_flag_t   markdown_enabled;
    ngx_int_t    markdown_buffer_size;
    ngx_path_t   markdown_html_header;
    ngx_path_t   markdown_html_footer;
} ngx_http_markdown_loc_conf_t;

static ngx_command_t ngx_http_markdown_commands[] = {
    {
        ngx_string("markdown"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LIF_CONF | NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_loc_conf_t, markdown_enabled),
        NULL
    },
    {
        ngx_string("markdown_buffer_size"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LIF_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_loc_conf_t, markdown_buffer_size),
        NULL
    },
    {
        ngx_string("markdown_html_header"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LIF_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_path_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_loc_conf_t, markdown_html_header),
        NULL
    },
    {
        ngx_string("markdown_html_footer"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LIF_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_path_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_loc_conf_t, markdown_html_footer),
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t ngx_http_markdown_module_ctx = {
    NULL,                               /* pre conf */
    ngx_http_markdown_init,             /* post conf */
    NULL,                               /* create main conf */
    NULL,                               /* init main conf */
    NULL,                               /* create srv conf */
    NULL,                               /* merge srv conf */
    ngx_http_markdown_create_loc_conf,  /* create loc conf */
    ngx_http_markdown_merge_loc_conf,   /* merge loc conf */
};

ngx_module_t ngx_http_markdown_module = {
    NGX_MODULE_V1,
    &ngx_http_markdown_module_ctx,
    ngx_http_markdown_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static void *ngx_http_markdown_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_markdown_loc_conf_t *conf;
    conf = (ngx_http_markdown_loc_conf_t *)ngx_palloc(cf->pool, sizeof(ngx_http_markdown_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->markdown_enabled          = NGX_CONF_UNSET;
    conf->markdown_buffer_size      = NGX_CONF_UNSET;
    conf->markdown_html_header.data = NULL;
    conf->markdown_html_header.len  = 0;
    conf->markdown_html_footer.data = NULL;
    conf->markdown_html_header.len  = 0;

    return conf;
}

static char *ngx_http_markdown_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_markdown_loc_conf_t *prev = (ngx_http_markdown_loc_conf_t *)parent;
    ngx_http_markdown_loc_conf_t *conf = (ngx_http_markdown_loc_conf_t *)child;

    ngx_conf_merge_value(conf->markdown_enabled, prev->markdown_enabled, 0);
    ngx_conf_merge_value(conf->markdown_buffer_size, prev->markdown_buffer_size, 0);

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_markdown_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt       *h;
    ngx_http_core_main_conf_t *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    h    = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }
    *h = ngx_http_markdown_handler;
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cf->log, 0, "ngx_http_markdown_module init completed");
    return NGX_OK;
}

static ngx_int_t ngx_http_markdown_handler(ngx_http_request_t *r) 
{
    ngx_http_markdown_loc_conf_t *cf;

    /* get conf */
    cf = (ngx_http_markdown_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_markdown_module);
    if (cf == NULL) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "ngx_http_get_module_loc_conf failed");
        return NGX_DECLINED;
    }

    if (cf->markdown_enabled == NGX_CONF_UNSET || cf->markdown_enabled == 0) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "markdown is not enabled");
        return NGX_DECLINED;
    }

    /* default values */
    if (cf->markdown_buffer_size == NGX_CONF_UNSET) {
        cf->markdown_buffer_size = ngx_markdown_default_buffer_size;
    }
    if (cf->markdown_html_header == NGX_CONF_UNSET) {
        cf->markdown_html_header = ngx_markdown_default_html_header;
    }
    if (cf->markdown_html_footer == NGX_CONF_UNSET) {
        cf->markdown_html_footer = ngx_markdown_default_html_footer;
    }
    
    /* construct response */
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "begin markdown conversion");

    ngx_int_t rc;
    ngx_int_t fr;
    ngx_str_t *fn = ngx_string("./html/markdown/index.md");

    u_char *markdown = ngx_palloc(r-pool, cf->markdown_buffer_size);
    fr = ngx_http_markdown_get_markdown(fn, markdown);

    u_char *html     = ngx_markdown_to_string(markdown, 0, HTML_FORMAT);
    size_t  nhtml    = ngx_strlen(html);
    u_char *hbuf     = ngx_palloc(r->pool, nhtml);
    ngx_memcpy(hbuf, html, nhtml);
    ngx_free(html);

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = nbuf;
    r->headers_out.content_type = ngx_string("text/html");
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    ngx_buf_t *b;
    b = ngx_create_temp_buf(r->pool, nbuf);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->pos      = hbuf;
    b->last     = b->pos + nbuf;
    b->last_buf = 1;

    ngx_chain_t out;
    out.buf  = b;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}

static ngx_int_t ngx_http_markdown_get_markdown(const ngx_str_t *filename, u_char *markdown)
{
    ngx_fd_t fd;
    ssize_t  n;
    fd = ngx_open_file(filename->data, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);

    n = ngx_read_file()

    return NGX_OK;
}

