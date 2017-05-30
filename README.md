# ngx_http_markdown_module

## Introduction

ngx_http_markdown_module is a nginx extension,
which enables you to directly give response
by interpreting local Markdown file to html.

## Build

### Prerequisites

* pkg-config
* awk
* glib-2.0
* peg-markdown

### Build Prerequisites

#### Install `glib-2.0`

For macOS:

```
$ brew install glib
```

For Ubuntu Linux:

```
$ sudo apt-get update
$ sudo apt-get install glib2.0
```

#### Build `peg-markdown`

```
$ git submodule update --init --recursive
$ cd peg-markdown
$ make library
```

### Build Nginx

```
$ ./configure --add-module=/path/to/ngx_http_markdown_module
$ make
$ [sudo] make install
```

## Configuration

ngx_http_markdown_module conf 

```
location /markdown {  
    markdown             on;  
    markdown_html_header /path/to/header.html;  
    markdown_html_footer /path/to/footer.html;  
}
```

## License

* BSD-2-Clause
