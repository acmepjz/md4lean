#include <lean/lean.h>
#include <md4c-html.h>

static void
process_output(const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    lean_object **p_html_string = (lean_object**)userdata;
    lean_object *new_string = lean_mk_string_from_bytes(text, size);
    *p_html_string = lean_string_append(*p_html_string, new_string);
    lean_dec_ref(new_string);
}

lean_obj_res lean_md4c_markdown_to_html(b_lean_obj_res s, uint32_t p_flags, uint32_t r_flags) {
    size_t input_size = lean_string_size(s) - 1;
    lean_object *html_string = lean_mk_string("");

    int ret = md_html(lean_string_cstr(s), (MD_SIZE)(lean_string_size(s) - 1), process_output,
        (void*) &html_string, p_flags, r_flags);

    if(ret != 0) {
        /* free the broken string */
        lean_dec_ref(html_string);
        /* Option.none */
        html_string = lean_box(0);
    } else {
        /* Option.some */
        lean_object *tmp = lean_alloc_ctor(1, 1, 0);
        lean_ctor_set(tmp, 0, html_string);
        html_string = tmp;
    }

    return html_string;
}
