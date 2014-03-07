ngx_http_markdown_module
========================

#### UNDER DEVELOPING

* Introduction

ngx_http_markdown_module is a nginx extension, 
which enables you to directly give response 
by interpreting local Markdown file to html with 
[Sundown](https://github.com/vmg/sundown).

* Conf

Add in location

    markdown on  
    markdown_engine sundown #currently only sundown is available
