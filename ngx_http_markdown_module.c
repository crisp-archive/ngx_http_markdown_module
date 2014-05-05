/*
 * ngx_http_markdown_module
 * Copyright(c) David Zhang, 2014
 */

#include <ngx_core.h>
#include <ngx_http.h>

#include "markdown_lib.h"

static ngx_int_t g_markdown_buffer_size = 10240;
/*
static ngx_str_t g_markdown_html_header = ngx_string("./html/header.html");
static ngx_str_t g_markdown_html_footer = ngx_string("./html/footer.html");
*/

typedef struct {
    ngx_flag_t   md_is_enabled;
    ngx_int_t    md_buffer_size;
    ngx_path_t   md_html_header;
    ngx_path_t   md_html_footer;
} ngx_http_markdown_conf_t;

typedef struct {
    ngx_int_t    is_filtered;
    u_char*      markdown_buf;
    ngx_uint_t   markdown_pos;
} ngx_http_markdown_ctx_t;

static ngx_command_t ngx_http_markdown_commands[] = {
    {
        ngx_string("markdown"),
        NGX_HTTP_LOC_CONF | NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_conf_t, md_is_enabled),
        NULL
    },
    {
        ngx_string("markdown_buffer_size"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_conf_t, md_buffer_size),
        NULL
    },
    {
        ngx_string("markdown_html_header"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_conf_t, md_html_header),
        NULL
    },
    {
        ngx_string("markdown_html_footer"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_conf_t, md_html_footer),
        NULL
    },
    ngx_null_command
};

static void *ngx_http_markdown_create_loc_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_markdown_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_markdown_body_filter(ngx_http_request_t *r, ngx_chain_t *in);

static ngx_http_output_body_filter_pt ngx_http_next_body_filter;

static ngx_http_module_t ngx_http_markdown_module_ctx = {
    NULL, /* pre conf */
    ngx_http_markdown_init, /* post conf */

    NULL, /* create main conf */
    NULL, /* init main conf */

    NULL, /* create srv conf */
    NULL, /* merge srv conf */

    ngx_http_markdown_create_loc_conf, /* create loc conf */
    NULL, /* merge loc conf */
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
    ngx_http_markdown_conf_t *mdcf;
    mdcf = (ngx_http_markdown_conf_t *)ngx_palloc(cf->pool, sizeof(ngx_http_markdown_conf_t));
    if (mdcf == NULL) {
        return NULL;
    }

    mdcf->md_is_enabled  = NGX_CONF_UNSET;
    mdcf->md_buffer_size = NGX_CONF_UNSET;
    /*
    mdcf->md_html_header = NGX_CONF_UNSET;
    mdcf->md_html_footer = NGX_CONF_UNSET;
    */
    return mdcf;
}

static ngx_int_t ngx_http_markdown_init(ngx_conf_t *cf)
{
    /* output filter */
    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_markdown_body_filter;

    return NGX_OK;
}

static ngx_int_t 
ngx_http_markdown_body_filter(ngx_http_request_t *r, ngx_chain_t *in) 
{
    ngx_http_markdown_conf_t *cf;
    ngx_http_markdown_ctx_t  *ctx;
    /* get conf */
    cf = (ngx_http_markdown_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_markdown_module);
    if (cf == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    if (cf->md_is_enabled == NGX_CONF_UNSET || cf->md_is_enabled == 0) {
        return ngx_http_next_body_filter(r, in);
    }
    /* default values */
    if (cf->md_buffer_size == NGX_CONF_UNSET) {
        cf->md_buffer_size = g_markdown_buffer_size; 
    }
    /*
    if (cf->md_html_header == NGX_CONF_UNSET) {
        cf->md_html_header = g_markdown_html_header;
    }
    if (cf->md_html_footer == NGX_CONF_UNSET) {
        cf->md_html_footer = g_markdown_html_footer;
    }
    */
    /* get ctx */
    ctx = (ngx_http_markdown_ctx_t *)ngx_http_get_module_ctx(r, ngx_http_markdown_module);
    if (ctx == NULL) {
        ctx = (ngx_http_markdown_ctx_t *)ngx_palloc(r->pool, sizeof(ngx_http_markdown_ctx_t));
        if (ctx == NULL) {
            ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "ngx_palloc ngx_http_markdown_ctx_t failed");
            return ngx_http_next_body_filter(r, in);
        }

        ctx->markdown_pos = 0;
        ctx->markdown_buf = (u_char *)ngx_palloc(r->pool, cf->md_buffer_size);
        if (ctx->markdown_buf == NULL) {
            ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "ngx_palloc ngx_http_markdown_ctx_t failed");
            return ngx_http_next_body_filter(r, in);
        }

        ngx_http_set_ctx(r, ctx, ngx_http_markdown_module);
    }
    else {
        if (ctx->markdown_buf == NULL) {
            ctx->markdown_pos = 0;
            ctx->markdown_buf = (u_char *)ngx_palloc(r->pool, cf->md_buffer_size);
        }

        if (ctx->markdown_buf == NULL) {
            ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "ngx_palloc ngx_http_markdown_ctx_t failed");
            return ngx_http_next_body_filter(r, in);
        }
    }
    /* skip if proceeded */
    if (ctx->is_filtered == 1) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "already filtered, skip.");
        return ngx_http_next_body_filter(r, in);
    }

    if (r->headers_out.status != NGX_HTTP_OK) {
        return ngx_http_next_body_filter(r, in);
    }

    ngx_chain_t *iter;
    for (iter = in; iter != NULL; iter = iter->next) {
        if (iter->buf->last == iter->buf->pos) {
            /* empty buf */
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "skip empty buf");
        }
        else {
            if (ctx->markdown_pos + iter->buf->last - iter->buf->pos >= cf->md_buffer_size) {
                ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "markdown file is overbuf!");
                return ngx_http_next_body_filter(r, in);
            }

            memcpy(ctx->markdown_buf + ctx->markdown_pos, iter->buf->pos, iter->buf->last - iter->buf->pos);
            ctx->markdown_pos += iter->buf->last - iter->buf->pos;
        }
        
        iter->buf->pos = iter->buf->last;

        if (iter->buf->last_buf) {
            u_char *html = (u_char *)markdown_to_string((char *)ctx->markdown_buf, 0, HTML_FORMAT);
            size_t  nbuf = strlen((const char *)html);
            u_char *hbuf = (u_char *)ngx_palloc(r->pool, nbuf);
            ngx_memcpy(hbuf, html, nbuf);
            free(html);

            ngx_chain_t *rc = ngx_alloc_chain_link(r->pool);
            if (rc == NULL) {
                return NGX_ERROR;
            }

            rc->next = NULL;
            rc->buf = (ngx_buf_t *)ngx_calloc_buf(r->pool);
            if (rc->buf == NULL) {
                return NGX_ERROR;
            }

            rc->buf->memory   = 1;
            rc->buf->last_buf = 0;
            rc->buf->pos      = hbuf;
            rc->buf->last     = hbuf + nbuf;
            /* set filter flag */
            ctx->is_filtered  = 1;

            return ngx_http_next_body_filter(r, rc);
        }
          
    }

    return ngx_http_next_body_filter(r, in);
}

