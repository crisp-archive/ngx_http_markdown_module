/*
 * ngx_http_markdown_module
 * Copyright(c) David Zhang, 2014
 */

#include <ngx_core.h>
#include <ngx_http.h>

#include "markdown_lib.h"

#define ngx_markdown_to_string markdown_to_string
static void *ngx_http_markdown_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_markdown_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_markdown_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_markdown_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_markdown_open_file(const ngx_str_t fn, ngx_file_t *f);
static ngx_int_t ngx_http_markdown_get_file(ngx_file_t f, u_char *content);

typedef struct {
    ngx_flag_t markdown_enabled;
    ngx_str_t  markdown_html_header;
    ngx_str_t  markdown_html_footer;
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
        ngx_string("markdown_html_header"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LIF_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_loc_conf_t, markdown_html_header),
        NULL
    },
    {
        ngx_string("markdown_html_footer"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LIF_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
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
    
    /* construct response */
    ngx_int_t   rc;
    ngx_int_t   fr;
    ngx_int_t   or;
    ngx_file_t  f;
    u_char     *markdown;
    u_char     *header = NULL;
    u_char     *footer = NULL;
    size_t      nheader = 0;
    size_t      nfooter = 0;
    ngx_str_t   fn = ngx_string("./html/markdown/doc/about.md");

    /* get header */
    /*
    if (cf->markdown_html_header.data) {
        or       = ngx_http_markdown_open_file(cf->markdown_html_header, &f);
        nheader  = f.info.st_size + 1; 
        header   = ngx_palloc(r->pool, nheader);
        if (header == NULL) {
            return NGX_DECLINED;
        }
        fr       = ngx_http_markdown_get_file(f, header);
    }
    */
    /* get foofer */
    /*
    if (cf->markdown_html_footer.data) {
        or       = ngx_http_markdown_open_file(cf->markdown_html_footer, &f);
        nfooter  = f.info.st_size + 1;
        footer   = ngx_palloc(r->pool, nfooter); 
        fr       = ngx_http_markdown_get_file(f, footer);
    }
    */
    /* get markdown */
    or = ngx_http_markdown_open_file(fn, &f);
    if (or == NGX_ERROR) {
        return NGX_HTTP_NOT_FOUND;
    }
    markdown = ngx_palloc(r->pool, f.info.st_size + 1);
    if (markdown == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    fr = ngx_http_markdown_get_file(f, markdown);
    if (fr != f.info.st_size) {
        ;
    }

    u_char *html   = (u_char *)ngx_markdown_to_string((char *)markdown, 0, HTML_FORMAT);
    size_t  nhtml  = ngx_strlen(html);
    size_t  ntotal = nhtml + nheader + nfooter;
    u_char *hbuf   = ngx_palloc(r->pool, ntotal);
    if (nheader) {
        ngx_memcpy(hbuf, header, nheader);
    }
    ngx_memcpy(hbuf, html, nhtml);
    if (nfooter) {
        ngx_memcpy(hbuf, footer, nfooter);
    }
    ngx_free(html);

    ngx_str_t content_type = ngx_string("text/html");
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = ntotal; 
    r->headers_out.content_type = content_type; 
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    ngx_buf_t *b;
    b = ngx_create_temp_buf(r->pool, ntotal);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->pos      = hbuf;
    b->last     = b->pos + ntotal;
    b->last_buf = 1;

    ngx_chain_t out;
    out.buf  = b;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}

static ngx_int_t ngx_http_markdown_open_file(const ngx_str_t fn, ngx_file_t *f)
{
    ngx_fd_t        fd;
    ngx_file_info_t fi;
    fd = ngx_open_file(fn.data, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);
    if (ngx_fd_info(fd, &fi) == NGX_FILE_ERROR) {
        return NGX_ERROR;
    }
    f->fd   = fd;
    f->info = fi;

    return NGX_OK;
}

static ngx_int_t ngx_http_markdown_get_file(ngx_file_t f, u_char *content)
{
    ssize_t n;

    n = ngx_read_file(&f, content, f.info.st_size, 0);
    content[f.info.st_size] = '\0';

    ngx_close_file(f.fd);

    return n;
}

