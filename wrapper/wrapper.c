#include <lean/lean.h>
#include <md4c-html.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Some of the following code is copied from `md2html.c` of `md4c`. */

struct membuffer {
    char* data;
    size_t asize;
    size_t size;
};

static void
membuf_init(struct membuffer* buf, MD_SIZE new_asize)
{
    buf->size = 0;
    buf->asize = new_asize;
    buf->data = malloc(buf->asize);
    if(buf->data == NULL) {
        fprintf(stderr, "membuf_init: malloc() failed.\n");
        abort();
    }
}

static void
membuf_fini(struct membuffer* buf)
{
    if(buf->data)
        free(buf->data);
}

static void
membuf_grow(struct membuffer* buf, size_t new_asize)
{
    buf->data = realloc(buf->data, new_asize);
    if(buf->data == NULL) {
        fprintf(stderr, "membuf_grow: realloc() failed.\n");
        abort();
    }
    buf->asize = new_asize;
}

static void
membuf_append(struct membuffer* buf, const char* data, MD_SIZE size)
{
    if(buf->asize < buf->size + size)
        membuf_grow(buf, buf->size + buf->size / 2 + size);
    memcpy(buf->data + buf->size, data, size);
    buf->size += size;
}

static void
process_output(const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    membuf_append((struct membuffer*) userdata, text, size);
}

/* TODO: return value should be `Option String` */
lean_obj_res lean_md4c_markdown_to_html(b_lean_obj_res s) {
    struct membuffer buf_out = {0};
    /* TODO: customizable */
    unsigned p_flags = MD_DIALECT_GITHUB | MD_FLAG_LATEXMATHSPANS | MD_FLAG_NOHTML;
    /* TODO: customizable */
    unsigned r_flags = MD_HTML_FLAG_XHTML | MD_HTML_FLAG_MATHJAX;
    size_t input_size = lean_string_size(s) - 1;
    lean_object *html_string = NULL;

    /* Input size is good estimation of output size. Add some more reserve to
     * deal with the HTML header/footer and tags. */
    membuf_init(&buf_out, (MD_SIZE)(input_size + input_size/8 + 64));

    int ret = md_html(lean_string_cstr(s), (MD_SIZE)input_size, process_output, (void*) &buf_out, p_flags, r_flags);

    if(ret != 0) {
        /* TODO: should return `Option.none` */
        html_string = lean_mk_string("");
    } else {
        /* append a nullchar */
        membuf_append(&buf_out, "", 1);
        html_string = lean_mk_string(buf_out.data);
    }

    membuf_fini(&buf_out);

    return html_string;
}
