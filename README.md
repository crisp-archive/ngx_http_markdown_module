ngx_http_markdown_module
========================

#### Introduction

ngx_http_markdown_module is a nginx extension,
which enables you to directly give response
by interpreting local Markdown file to html.

#### Dependencies

* peg-markdown

#### Compiling

```
./configure --add-module=/path/to/module
make
make install
```

#### Configuration

ngx_http_markdown_module conf 

```
location /markdown {  
    markdown             on;  
    markdown_html_header /path/to/header.html;  
    markdown_html_header /path/to/footer.html;  
}
```

#### Troubleshooting

* peg-markdown may have problem in building

Editing objs/Makefile, adding libglib-2.0 and libpeg-markdown includes path to ALL_INCS:

    -I /home/users/zhangwanlong/private/peg-markdown/trunk \
    -I /home/users/zhangwanlong/.jumbo/lib/glib-2.0/include \
    -I /home/users/zhangwanlong/.jumbo/include/glib-2.0

And library path to $(link):

    -L /home/users/zhangwanlong/private/peg-markdown/trunk -lpeg-markdown \
    -L /home/users/zhangwanlong/.jumbo/lib -lglib-2.0


#### Known Issues

* when master_process is on, the use of markdown_html_header will cause crash.

