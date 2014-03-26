/*
 * ngx_http_markdown_module
 * Copyright(c) David Zhang, 2014
 */

#include "markdown_lib.h"

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_int_t ngx_http_markdown_handler(ngx_http_request_t *r);
static char *ngx_http_markdown_conf(ngx_conf_t *cf, ngx_command_t *command, void *conf);
static ngx_int_t ngx_markdown_handler(const char *path_info);

static ngx_command_t ngx_http_markdown_commands[] = {
    {
        ngx_string("markdown"),
        NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        ngx_http_markdown_conf,
        NGX_HTTP_LOC_CONF_OFFSET,
        NULL
    },
    /*
    {
        ngx_string("markdown_engine"), 
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offset(ngx_http_markdown_conf_t, engine),
        NULL
    },
    */
    ngx_null_command
};

static ngx_http_module_t ngx_http_markdown_module_ctx = {
    NULL, /* pre conf */
    NULL, /* post conf */

    NULL, /* create main conf */
    NULL, /* init main conf */

    NULL, /* create srv conf */
    NULL, /* merge srv conf */

    NULL, /* create loc conf */
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

static char *ngx_http_markdown_conf(ngx_conf_t *cf, ngx_command_t *command, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_markdown_handler;

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_markdown_handler(ngx_http_request_t *r)
{
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK)
        return rc;

    ngx_str_t content_type = ngx_string("text/html");
    char *md = "*** hello markdown";
    char *html = markdown_to_string(md, 0, HTML_FORMAT);
    ngx_str_t response = ngx_string(html);
    free(html);
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = response.len;
    r->headers_out.content_type = content_type;
    
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
        return rc;

    ngx_buf_t *b;
    b = ngx_create_temp_buf(r->pool, response.len);
    if (b == NULL)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    ngx_memcpy(b->pos, response.data, response.len);
    b->last = b->pos + response.len;
    b->last_buf = 1;

    ngx_chain_t out;
    out.buf  = b;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);

}
