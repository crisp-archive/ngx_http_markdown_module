ngx_http_markdown_module
========================

#### UNDER DEVELOPING

* Introduction

ngx_http_markdown_module is a nginx extension,
which enables you to directly give response
by interpreting local Markdown file to html.

* Compiling

Executing

    ./configure --add-module=/path/to/module

Editing objs/Makefile, adding libglib-2.0 and libpeg-markdown includes path to ALL_INCS like:

    -I /home/users/zhangwanlong/private/peg-markdown/trunk \
    -I /home/users/zhangwanlong/.jumbo/lib/glib-2.0/include \
    -I /home/users/zhangwanlong/.jumbo/include/glib-2.0

And also to $(link) like:

    -L /home/users/zhangwanlong/private/peg-markdown/trunk -lpeg-markdown \
    -L /home/users/zhangwanlong/.jumbo/lib -lglib-2.0

* Conf (NOT IMPLEMENTED YET)

Add in location

    location /markdown {
        # switch
        markdown            on;

        # engine, currently only peg-markdown is available
        markdown_engine     peg-markdown;

        # utility functions
        markdown_title      "Crisp Home";
        markdown_highlight  on;
        markdown_add_script /path/to/js/file.js;
        markdown_add_css    /path/to/css/file.css;
    }
