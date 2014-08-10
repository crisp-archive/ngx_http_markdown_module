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
    markdown_html_footer /path/to/footer.html;  
}
```

#### Troubleshooting

* peg-markdown may have problem in building

Editing __config__, editing CORE_INCS and CORE_LIBS

