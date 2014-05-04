/*
 * ngx_http_markdown_module
 * Copyright(c) David Zhang, 2014
 */

#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_http_markdown_module.h"
#include "markdown_lib.h"


static ngx_command_t ngx_http_markdown_commands[] = {
    {
        ngx_string("markdown"),
        NGX_HTTP_LOC_CONF | NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_conf_t, md_enable),
        NULL
    },
    {
        ngx_string("markdown_buffer_size"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_conf_t, md_root),
        NULL
    },
    {
        ngx_string("markdown_html_header"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_conf_t, md_root),
        NULL
    },
    {
        ngx_string("markdown_html_footer"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_markdown_conf_t, md_root),
        NULL
    },
    ngx_null_command
};

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

    mdcf->md_enable            = NGX_CONF_UNSET;
    mdcf->md_engine            = NULL;
    mdcf->md_root              = NULL;
    mdcf->md_html_header       = NULL;
    mdcf->md_html_footer       = NULL;

    return mdcf;
}

static void *ngx_http_markdown_init(ngx_http_request_t *r)
{
    ngx_http_handler_pt *h = NULL;
    ngx_http_core_main_conf_t *cmcf = NULL;
    /* get conf */
    ngx_http_markdown_conf_t mdcf;
    mdcf = (ngx_http_markdown_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_markdown_conf_t);
    if (mdcf == NULL) {
        return NGX_DECLINED;
    }

    if (mdcf->md_is_enabled == 0) {
        return NGX_DECLINED;
    }
    /* default values */
    if (mdcf->md_buffer_size == NGX_CONF_UNSET) {
        mdcf->md_buffer_size = g_markdown_buffer_size;
    }
    if (mdcf->md_html_header == NGX_CONF_UNSET) {
        mdcf->md_html_header = g_markdown_html_header;
    }
    if (mdcf->md_html_footer == NGX_CONF_UNSET) {
        mdcf->md_html_footer = g_markdown_html_footer;
    }

    /* output filter */
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_markdown_header_filter;
    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_markdown_body_filter;

    cmcf = ngx_http_conf_get_mobule_main_conf(cf, ngx_http_core_module);
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }
    *h = ngx_http_markdown_handler;

    return NGX_OK;
}

/*
static ngx_int_t ngx_http_markdown_handler(ngx_http_request_t *r)
{
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK)
        return rc;

    ngx_str_t content_type = ngx_string("text/html");
    char *fn = "/home/users/zhangwanlong/bin/nginx-md/html/test.md";
    FILE *op = fopen(fn, "r");
    char md[1000];
    char ch;
    int  fs = 0;
    while((ch = fgetc(op)) != EOF) {
        md[fs++] = ch;
    }
    md[fs] = '\0';
    char *html = markdown_to_string(md, 0, HTML_FORMAT);
    unsigned int nbuf = strlen(html);
    char *hbuf = ngx_palloc(r->pool, nbuf);
    ngx_memcpy(hbuf, html,nbuf);
    fclose(op);
    free(html);
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = nbuf;
    r->headers_out.content_type = content_type;
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
        return rc;

    ngx_buf_t *b;
    b = ngx_create_temp_buf(r->pool, nbuf);
    if (b == NULL)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    b->pos = hbuf;
    b->last = b->pos + nbuf;
    b->last_buf = 1;

    ngx_chain_t out;
    out.buf  = b;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);

}
*/
