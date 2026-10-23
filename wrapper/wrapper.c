// LEAN_EXPORT must export the C entry points for clients that link another DLL to MD4Lean.
#ifndef LEAN_EXPORTING
#define LEAN_EXPORTING
#endif
#include <lean/lean.h>
#include <md4c-html.h>

#ifndef __cplusplus
// To avoid the need for stdlib.h - lean.h does this for malloc() already
void *realloc(void *ptr, size_t new_size);
#endif

// AST constructors exported from MD4Lean.FFI. The wrapper calls these instead of
// allocating constructor objects, so the runtime layout of the AST types stays private to
// Lean-compiled code. Object arguments are owned (consumed by the callee). Nullary
// constructors take a Unit argument, passed as lean_box(0). Character-valued fields are
// passed as uint32_t; optional fields are passed as a presence flag plus a raw value.
extern lean_obj_res lean_md4c_attr_normal(lean_obj_arg s);
extern lean_obj_res lean_md4c_attr_entity(lean_obj_arg s);
extern lean_obj_res lean_md4c_attr_nullchar(lean_obj_arg unit);

extern lean_obj_res lean_md4c_text_normal(lean_obj_arg s);
extern lean_obj_res lean_md4c_text_nullchar(lean_obj_arg unit);
extern lean_obj_res lean_md4c_text_br(lean_obj_arg s);
extern lean_obj_res lean_md4c_text_softbr(lean_obj_arg s);
extern lean_obj_res lean_md4c_text_entity(lean_obj_arg s);
extern lean_obj_res lean_md4c_text_em(lean_obj_arg contents);
extern lean_obj_res lean_md4c_text_strong(lean_obj_arg contents);
extern lean_obj_res lean_md4c_text_u(lean_obj_arg contents);
extern lean_obj_res lean_md4c_text_a(lean_obj_arg href, lean_obj_arg title, uint8_t is_auto,
  lean_obj_arg contents);
extern lean_obj_res lean_md4c_text_img(lean_obj_arg src, lean_obj_arg title, lean_obj_arg alt);
extern lean_obj_res lean_md4c_text_code(lean_obj_arg contents);
extern lean_obj_res lean_md4c_text_del(lean_obj_arg contents);
extern lean_obj_res lean_md4c_text_latex_math(lean_obj_arg contents);
extern lean_obj_res lean_md4c_text_latex_math_display(lean_obj_arg contents);
extern lean_obj_res lean_md4c_text_wikilink(lean_obj_arg target, lean_obj_arg contents);

extern lean_obj_res lean_md4c_block_p(lean_obj_arg contents);
extern lean_obj_res lean_md4c_block_ul(uint8_t tight, uint32_t mark, lean_obj_arg items);
extern lean_obj_res lean_md4c_block_ol(uint8_t tight, lean_obj_arg start, uint32_t mark,
  lean_obj_arg items);
extern lean_obj_res lean_md4c_block_hr(lean_obj_arg unit);
extern lean_obj_res lean_md4c_block_header(lean_obj_arg level, lean_obj_arg contents);
extern lean_obj_res lean_md4c_block_code(lean_obj_arg info, lean_obj_arg lang, uint8_t has_fence,
  uint32_t fence_char, lean_obj_arg strings);
extern lean_obj_res lean_md4c_block_html(lean_obj_arg contents);
extern lean_obj_res lean_md4c_block_blockquote(lean_obj_arg contents);
extern lean_obj_res lean_md4c_block_table(lean_obj_arg head, lean_obj_arg body);

extern lean_obj_res lean_md4c_li(uint8_t is_task, uint32_t task_char, size_t task_offset,
  lean_obj_arg contents);
extern lean_obj_res lean_md4c_document_mk(lean_obj_arg blocks);

extern lean_obj_res lean_md4c_some_document(lean_obj_arg d);
extern lean_obj_res lean_md4c_some_string(lean_obj_arg s);

static void
process_output(const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    lean_object **p_html_string = (lean_object**)userdata;
    lean_object *new_string = lean_mk_string_from_bytes(text, size);
    *p_html_string = lean_string_append(*p_html_string, new_string);
    lean_dec_ref(new_string);
}

LEAN_EXPORT lean_obj_res lean_md4c_markdown_to_html(b_lean_obj_arg s, uint32_t p_flags, uint32_t r_flags) {
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
        html_string = lean_md4c_some_string(html_string);
    }

    return html_string;
}

typedef union {
    MD_BLOCKTYPE block;
    MD_SPANTYPE span;
    MD_TEXTTYPE text;
} NODE_TYPE;


// Which Lean type are we constructing in the AST right now? This is used to deal with the fact that
// md4c places both text nodes and block nodes underneath LI nodes. In particular,
//  * blah
//    ```lang
//    code
//    ```
// is parsed as:
//   - UL
//     - LI
//       - TEXT "blah"
//       - CODEBLOCK
//         - TEXT "lang"
//         - CODE "code"
// but we want to make an AST for Lean like:
//   - UL
//     - LI
//       - P
//         - TEXT "blah"
//       - CODEBLOCK
//         - TEXT "lang"
//         - CODE "code"
// so we implicitly open and close P nodes inside of LI nodes, but this must be tracked.
//
// It works like this:
//  - When encountering text, check if the stack top is LI. If so, push TAG_IMPLICIT_P. Here, "text"
//    can be either md4c's notion of text or its notion of span.
//  - When ending an LI, if the top of the stack is not LI, it must be an implicit P. Create the P
//    node.
//  - When starting a block, if the top of the stack is TAG_IMPLICIT_P, close that paragraph before
//    starting the new block. This is for blocks that are siblings of text under an LI.
typedef enum {TAG_BLOCK, TAG_TEXT, TAG_LI, TAG_IMPLICIT_P} tag;

typedef union details {
    MD_BLOCK_UL_DETAIL ul_details;
    MD_BLOCK_OL_DETAIL ol_details;
    MD_BLOCK_LI_DETAIL li_details;
    MD_BLOCK_H_DETAIL h_details;
    MD_BLOCK_CODE_DETAIL code_details;
    MD_SPAN_A_DETAIL a_details;
    uint8_t no_details; // always 0
} details;

static details no_detail = (details)(uint8_t)0;

typedef struct parse_stack {
    size_t size;
    size_t top;
    lean_object **args;
    details *details;
    tag *tags;
} parse_stack;

parse_stack *parse_stack_new() {
    parse_stack *stk = malloc(sizeof(parse_stack));
    if (stk == NULL) lean_internal_panic_out_of_memory();
    stk->size = 64;
    stk->top = 0;
    stk->args = malloc(sizeof(lean_object *) * stk->size);
    if (stk->args == NULL) lean_internal_panic_out_of_memory();
    stk->details = malloc(sizeof(details) * stk->size);
    if (stk->details == NULL) lean_internal_panic_out_of_memory();
    stk->tags = malloc(sizeof(tag) * stk->size);
    if (stk->tags == NULL) lean_internal_panic_out_of_memory();
    stk->args[0] = lean_mk_empty_array();
    stk->tags[0] = TAG_BLOCK;
    stk->details[0] = no_detail;

    return stk;
}

void parse_stack_push(parse_stack *stk, details details, tag tag) {
    if (stk->top >= stk->size - 1) {
        size_t newsize = stk->size * 2;
        stk->args = realloc(stk->args, sizeof(lean_object *) * newsize);
        if (stk->args == NULL) lean_internal_panic_out_of_memory();
        stk->details = realloc(stk->details, sizeof(details) * newsize);
        if (stk->details == NULL) lean_internal_panic_out_of_memory();
        stk->tags = realloc(stk->tags, sizeof(tag) * newsize);
        if (stk->tags == NULL) lean_internal_panic_out_of_memory();
        stk->size = newsize;
    }
    stk->top++;
    stk->args[stk->top] = (lean_object *)lean_mk_empty_array();
    stk->details[stk->top] = details;
    stk->tags[stk->top] = tag;
}

void parse_stack_save(parse_stack *stk, lean_obj_arg arg) {
    stk->args[stk->top] = lean_array_push(stk->args[stk->top], arg);
}

lean_obj_res parse_stack_pop(parse_stack *stk) {
    lean_object *argarray = stk->args[stk->top];
    stk->top--;
    return argarray;
}

tag parse_stack_top_tag(parse_stack *stk) {
    return stk->tags[stk->top];
}

void parse_stack_free(parse_stack *stk) {
    // If the parser left junk on the stack, clean it up
    while (stk->top > 0) {
        lean_dec_ref(parse_stack_pop(stk));
    }
    lean_dec_ref(stk->args[0]);
    free(stk->args);
    free(stk->details);
    free(stk->tags);
    free(stk);
}

lean_obj_res get_attr(MD_ATTRIBUTE attr, lean_obj_arg dest) {
    assert(lean_is_array(dest));
    if (attr.size == 0)
        return dest;
    for (unsigned i = 0; attr.substr_offsets[i] < attr.size; i++) {
        size_t start = attr.substr_offsets[i];
        size_t end = attr.substr_offsets[i + 1];
        switch (attr.substr_types[i]) {
        case MD_TEXT_NORMAL: {
            lean_object *str =
                lean_mk_string_from_bytes(attr.text + start, end - start);
            dest = lean_array_push(dest, lean_md4c_attr_normal(str));
            break;
        }
        case MD_TEXT_ENTITY: {
            lean_object *str =
                lean_mk_string_from_bytes(attr.text + start, end - start);
            dest = lean_array_push(dest, lean_md4c_attr_entity(str));
            break;
        }
        case MD_TEXT_NULLCHAR: {
            dest = lean_array_push(dest, lean_md4c_attr_nullchar(lean_box(0)));
            break;
        }
        default:
            lean_internal_panic_unreachable();
        }
    }
    return dest;
}

static int enter_block_callback(MD_BLOCKTYPE type, void *detail, void *stack) {
    details block_details = no_detail;

    // See note on typedef tag
    if (parse_stack_top_tag(stack) == TAG_IMPLICIT_P) {
        lean_object *texts = parse_stack_pop((parse_stack *)stack);
        parse_stack_save(stack, lean_md4c_block_p(texts));
        assert(parse_stack_top_tag(stack) == TAG_LI);
    }

    switch (type) {
    case MD_BLOCK_UL: {
        block_details = (details)(*((MD_BLOCK_UL_DETAIL *)detail));
        break;
    }
    case MD_BLOCK_OL: {
        block_details = (details)(*((MD_BLOCK_OL_DETAIL *)detail));
        break;
    }
    case MD_BLOCK_H: {
        block_details = (details)(*((MD_BLOCK_H_DETAIL *)detail));
        break;
    }
    default:
        block_details = no_detail;
    }

    parse_stack_push((parse_stack *)stack, block_details, type == MD_BLOCK_LI ? TAG_LI : TAG_BLOCK);
    return 0;
}

static int leave_block_callback(MD_BLOCKTYPE type, void *detail, void *userdata) {
    parse_stack *stack = (parse_stack *)userdata;

    switch (type) {
    case MD_BLOCK_DOC: {
        assert(stack->top == 1);
        lean_object *blocks = parse_stack_pop(stack);
        parse_stack_save(stack, lean_md4c_document_mk(blocks));
        break;
    }
    case MD_BLOCK_UL: {
        // The details provided as an argument here are incorrect; use the ones
        // passed to the enter callback
        MD_BLOCK_UL_DETAIL ul_detail = stack->details[stack->top].ul_details;
        lean_object *items = parse_stack_pop(stack);
        parse_stack_save(stack,
            lean_md4c_block_ul(ul_detail.is_tight ? 1 : 0, ul_detail.mark, items));
        break;
    }
    case MD_BLOCK_QUOTE: {
        lean_object *blocks = parse_stack_pop(stack);
        parse_stack_save(stack, lean_md4c_block_blockquote(blocks));
        break;
    }
    case MD_BLOCK_OL: {
        // The details provided as an argument here are incorrect; use the ones
        // passed to the enter callback
        MD_BLOCK_OL_DETAIL ol_detail = stack->details[stack->top].ol_details;
        lean_object *items = parse_stack_pop(stack);
        parse_stack_save(stack,
            lean_md4c_block_ol(ol_detail.is_tight ? 1 : 0,
                lean_unsigned_to_nat(ol_detail.start), ol_detail.mark_delimiter, items));
        break;
    }
    case MD_BLOCK_LI: {
        // The details provided to the enter callback are incorrect; use the
        // ones passed here
        MD_BLOCK_LI_DETAIL *li_detail = (MD_BLOCK_LI_DETAIL *)detail;

        // If the tag on the stack isn't LI, then a paragraph block was pushed for implicit nesting.
        // Close it!
        if (parse_stack_top_tag(stack) != TAG_LI) {
            assert(parse_stack_top_tag(stack) == TAG_IMPLICIT_P);
            lean_object *texts = parse_stack_pop(stack);
            parse_stack_save(stack, lean_md4c_block_p(texts));
            assert(parse_stack_top_tag(stack) == TAG_LI);
        }

        lean_object *blocks = parse_stack_pop(stack);
        // task_mark and task_mark_offset are read by the constructor only when is_task is set
        parse_stack_save(stack,
            lean_md4c_li(li_detail->is_task ? 1 : 0, li_detail->task_mark,
                li_detail->task_mark_offset, blocks));
        break;
    }
    case MD_BLOCK_HR: {
        lean_object *items = parse_stack_pop(stack);
        assert(lean_array_size(items) == 0);
        lean_dec_ref(items);
        parse_stack_save(stack, lean_md4c_block_hr(lean_box(0)));
        break;
    }
    case MD_BLOCK_H: {
        // The details provided as an argument here are incorrect; use the ones
        // passed to the enter callback
        MD_BLOCK_H_DETAIL h_detail = stack->details[stack->top].h_details;
        lean_object *texts = parse_stack_pop(stack);
        parse_stack_save(stack,
            lean_md4c_block_header(lean_unsigned_to_nat(h_detail.level), texts));
        break;
    }
    case MD_BLOCK_CODE: {
        MD_BLOCK_CODE_DETAIL *code_detail = (MD_BLOCK_CODE_DETAIL *) detail;
        lean_object *info = get_attr(code_detail->info, lean_mk_empty_array());
        lean_object *lang = get_attr(code_detail->lang, lean_mk_empty_array());
        lean_object *strings = parse_stack_pop(stack);
        parse_stack_save(stack,
            lean_md4c_block_code(info, lang, code_detail->fence_char != 0,
                code_detail->fence_char, strings));
        break;
    }
    case MD_BLOCK_HTML: {
        lean_object *texts = parse_stack_pop(stack);
        parse_stack_save(stack, lean_md4c_block_html(texts));
        break;
    }
    case MD_BLOCK_P: {
        lean_object *texts = parse_stack_pop(stack);
        parse_stack_save(stack, lean_md4c_block_p(texts));
        break;
    }
    case MD_BLOCK_TABLE: {
        // There is a table detail object with the row and column counts, but we
        // don't need it for anything, so it's ignored
        lean_object *args = parse_stack_pop(stack);
        assert(lean_is_array(args));
        assert(lean_array_size(args) == 2);
        lean_object *thead = lean_array_uget(args, 0);
        lean_object *tbody = lean_array_uget(args, 1);
        lean_dec_ref(args);
        parse_stack_save(stack, lean_md4c_block_table(thead, tbody));
        break;
    }
    case MD_BLOCK_THEAD: {
        lean_object *head_row = parse_stack_pop(stack);
        // Here we got a row. But there's only ever one row in the header
        // (according to md4c docs), so no sense saving an extra layer of array.
        assert(lean_is_array(head_row));
        assert(lean_array_size(head_row) == 1);
        lean_object *row = lean_array_uget(head_row, 0);
        lean_dec_ref(head_row);
        parse_stack_save(stack, row);
        break;
    }
    case MD_BLOCK_TBODY: {
        lean_object *tbody = parse_stack_pop(stack);
        parse_stack_save(stack, tbody);
        break;
    }
    case MD_BLOCK_TR: {
        lean_object *tr = parse_stack_pop(stack);
        parse_stack_save(stack, tr);
        break;
    }
    case MD_BLOCK_TH: {
        lean_object *th = parse_stack_pop(stack);
        parse_stack_save(stack, th);
        break;
    }
    case MD_BLOCK_TD: {
        lean_object *td = parse_stack_pop(stack);
        parse_stack_save(stack, td);
        break;
    }
    }

    return 0;
}

static int enter_span_callback(MD_SPANTYPE type, void *detail, void *stack) {
    // If the span is nested right below a LI, push a block as well. See note next to typedef tag.
    if (parse_stack_top_tag((parse_stack *)stack) == TAG_LI) {
        parse_stack_push((parse_stack *)stack, (details)no_detail, TAG_IMPLICIT_P);
    }
    // The details provided to the enter callback for spans are typically
    // incorrect, so there's no sense saving them
    parse_stack_push((parse_stack *)stack, (details)no_detail, TAG_TEXT);
    return 0;
}

// Build a span that wraps a single array argument. The element type of the array varies by
// constructor, but the construction is uniform.
static lean_obj_res span_single_ctor(MD_SPANTYPE type, lean_obj_arg contents) {
    switch (type) {
    case MD_SPAN_EM:
        return lean_md4c_text_em(contents);
    case MD_SPAN_STRONG:
        return lean_md4c_text_strong(contents);
    case MD_SPAN_U:
        return lean_md4c_text_u(contents);
    case MD_SPAN_DEL:
        return lean_md4c_text_del(contents);
    case MD_SPAN_CODE:
        return lean_md4c_text_code(contents);
    case MD_SPAN_LATEXMATH:
        return lean_md4c_text_latex_math(contents);
    case MD_SPAN_LATEXMATH_DISPLAY:
        return lean_md4c_text_latex_math_display(contents);
    default:
        lean_internal_panic_unreachable();
    }
}

static int leave_span_callback(MD_SPANTYPE type, void *detail, void *userdata) {
    parse_stack *stack = (parse_stack *)userdata;
    switch (type) {
    // All these spans take an array of arguments. Even though the arguments
    // aren't the same type for each constructor, it doesn't matter here.
    //
    // These constructors take an array of further spans/text objects
    case MD_SPAN_EM:
    case MD_SPAN_STRONG:
    case MD_SPAN_U:
    case MD_SPAN_DEL:
    // These constructors take an array of special text objects (which are just
    // pushed as strings by the respective text handlers, as each has their own
    // uniquely-determined md4c text type)
    case MD_SPAN_CODE:
    case MD_SPAN_LATEXMATH:
    case MD_SPAN_LATEXMATH_DISPLAY: {
        lean_object *txt = parse_stack_pop(stack);
        parse_stack_save(stack, span_single_ctor(type, txt));
        break;
    }
    case MD_SPAN_A: {
        // Here we need the details provided to the leave callback
        MD_SPAN_A_DETAIL *a_detail = (MD_SPAN_A_DETAIL *)detail;
        lean_object *txt = parse_stack_pop(stack);
        lean_object *href = get_attr(a_detail->href, lean_mk_empty_array());
        lean_object *title = get_attr(a_detail->title, lean_mk_empty_array());
        parse_stack_save(stack,
            lean_md4c_text_a(href, title, a_detail->is_autolink ? 1 : 0, txt));
        break;
    }
    case MD_SPAN_IMG: {
        // Here we need the details provided to the leave callback
        MD_SPAN_IMG_DETAIL *img_detail = (MD_SPAN_IMG_DETAIL *)detail;
        lean_object *alt = parse_stack_pop(stack);
        lean_object *src = get_attr(img_detail->src, lean_mk_empty_array());
        lean_object *title = get_attr(img_detail->title, lean_mk_empty_array());
        parse_stack_save(stack, lean_md4c_text_img(src, title, alt));
        break;
    }
    case MD_SPAN_WIKILINK: {
        MD_SPAN_WIKILINK_DETAIL *wl_detail = (MD_SPAN_WIKILINK_DETAIL *)detail;
        lean_object *txt = parse_stack_pop(stack);
        lean_object *target = get_attr(wl_detail->target, lean_mk_empty_array());
        parse_stack_save(stack, lean_md4c_text_wikilink(target, txt));
        break;
    }
    }

    return 0;
}

static int text_callback(MD_TEXTTYPE type, const MD_CHAR *text, MD_SIZE size, void *userdata) {
    parse_stack *stack = (parse_stack *)userdata;

    // If the span is nested right below a LI, push a block as well. See note next to typedef tag.
    if (parse_stack_top_tag(stack) == TAG_LI) {
        parse_stack_push(stack, (details)no_detail, TAG_IMPLICIT_P);
    }

    switch (type) {
    case MD_TEXT_NORMAL: {
        parse_stack_save(stack, lean_md4c_text_normal(lean_mk_string_from_bytes(text, size)));
        break;
    }
    case MD_TEXT_NULLCHAR: {
        parse_stack_save(stack, lean_md4c_text_nullchar(lean_box(0)));
        break;
    }
    case MD_TEXT_BR: {
        parse_stack_save(stack, lean_md4c_text_br(lean_mk_string_from_bytes(text, size)));
        break;
    }
    case MD_TEXT_SOFTBR: {
        parse_stack_save(stack, lean_md4c_text_softbr(lean_mk_string_from_bytes(text, size)));
        break;
    }
    case MD_TEXT_ENTITY: {
        parse_stack_save(stack, lean_md4c_text_entity(lean_mk_string_from_bytes(text, size)));
        break;
    }
    // The following cases occur only as immediate children of particular
    // block/inline nodes, and are uniquely determined by the surrounding node.
    // Thus, there's no need to allocate a constructor here, because the Lean
    // AST expects strings.
    case MD_TEXT_HTML: {
        // Invariant: occurs only in HTML elements, which expect arrays of
        // strings as args
        parse_stack_save(stack, lean_mk_string_from_bytes(text, size));
        break;
    }
    case MD_TEXT_CODE: {
        // Invariant: occurs only and always inside of a code block or a code
        // inline. A given code block may have many of these in a row, however
        parse_stack_save(stack, lean_mk_string_from_bytes(text, size));
        break;
    }
    case MD_TEXT_LATEXMATH: {
        // Invariant: occurs only in math elements, which expect arrays of
        // strings as args
        parse_stack_save(stack, lean_mk_string_from_bytes(text, size));
        break;
    }

    default:
        lean_internal_panic_unreachable();
    }

    return 0;
}

LEAN_EXPORT lean_obj_res lean_md4c_markdown_parse(b_lean_obj_arg str, uint32_t p_flags) {
    size_t input_size = lean_string_size(str) - 1;

    parse_stack *stack = parse_stack_new();

    MD_PARSER parser = {
        0,
        p_flags,
        enter_block_callback,
        leave_block_callback,
        enter_span_callback,
        leave_span_callback,
        text_callback,
        NULL, /* debug log */
        NULL  /* Reserved field, always NULL*/
    };

    int ret = md_parse(lean_string_cstr(str), input_size, &parser, stack);

    if (ret != 0) {
        parse_stack_free(stack);
        /* Option.none */
        return lean_box(0);
    } else {
        assert(stack->top == 0);
        assert(lean_is_array(stack->args[0]));
        assert(lean_array_size(stack->args[0]) == 1);
        lean_object *doc = lean_array_uget(stack->args[0], 0);
        parse_stack_free(stack);
        assert(lean_is_exclusive(doc));

        return lean_md4c_some_document(doc);
    }
}
