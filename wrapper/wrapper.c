#include <lean/lean.h>
#include <md4c-html.h>

#ifndef __cplusplus
// To avoid the need for stdlib.h - lean.h does this for malloc() already
void *realloc(void *ptr, size_t new_size);
#endif

static void
process_output(const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    lean_object **p_html_string = (lean_object**)userdata;
    lean_object *new_string = lean_mk_string_from_bytes(text, size);
    *p_html_string = lean_string_append(*p_html_string, new_string);
    lean_dec_ref(new_string);
}

lean_obj_res lean_md4c_markdown_to_html(b_lean_obj_arg s, uint32_t p_flags, uint32_t r_flags) {
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

typedef union {
    MD_BLOCKTYPE block;
    MD_SPANTYPE span;
    MD_TEXTTYPE text;
} NODE_TYPE;

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
} parse_stack;

parse_stack *parse_stack_new() {
    parse_stack *stk = malloc(sizeof(parse_stack));
    if (stk == 0) lean_internal_panic_out_of_memory();
    stk->size = 64;
    stk->top = 0;
    stk->args = malloc(sizeof(lean_object *) * stk->size);
    if (stk->args == 0) lean_internal_panic_out_of_memory();
    stk->details = malloc(sizeof(details) * stk->size);
    if (stk->details == 0) lean_internal_panic_out_of_memory();
    stk->args[0] = lean_mk_empty_array();

    return stk;
}

void parse_stack_push(parse_stack *stk, details details) {
    if (stk->top >= stk->size - 1) {
        size_t newsize = stk->size * 2;
        stk->args = realloc(stk->args, sizeof(lean_object) * newsize);
        if (stk->args == 0) lean_internal_panic_out_of_memory();
        stk->details = realloc(stk->details, sizeof(details) * newsize);
        if (stk->details == 0) lean_internal_panic_out_of_memory();
        stk->size = newsize;
    }
    stk->top++;
    stk->args[stk->top] = (lean_object *)lean_mk_empty_array();
    stk->details[stk->top] = details;
}

void parse_stack_save(parse_stack *stk, lean_obj_arg arg) {
    stk->args[stk->top] = lean_array_push(stk->args[stk->top], arg);
}

lean_obj_res parse_stack_pop(parse_stack *stk) {
    lean_object *argarray = stk->args[stk->top];
    stk->top--;
    return argarray;
}

void parse_stack_free(parse_stack *stk) {
    // If the parser left junk on the stack, clean it up
    while (stk->top > 0) {
        lean_dec_ref(parse_stack_pop(stk));
    }
    lean_dec_ref(stk->args[0]);
    free(stk->args);
    free(stk->details);
    free(stk);
}

lean_obj_res get_attr(MD_ATTRIBUTE attr, lean_obj_arg dest) {
    assert(lean_is_array(dest));
    if (attr.size == 0)
        return dest;
    for (unsigned i = 0; attr.substr_offsets[i] < attr.size; i++) {
        size_t start = attr.substr_offsets[i];
        size_t end = attr.substr_offsets[i + 1];
        // The constructor indices below are for type AttrText, not Text
        switch (attr.substr_types[i]) {
        case MD_TEXT_NORMAL: {
            lean_object *str =
                lean_mk_string_from_bytes(attr.text + start, end - start);
            lean_object *ctor = lean_alloc_ctor(0, 1, 0);
            lean_ctor_set(ctor, 0, str);
            dest = lean_array_push(dest, ctor);
            break;
        }
        case MD_TEXT_ENTITY: {
            lean_object *str =
                lean_mk_string_from_bytes(attr.text + start, end - start);
            lean_object *ctor = lean_alloc_ctor(1, 1, 0);
            lean_ctor_set(ctor, 0, str);
            dest = lean_array_push(dest, ctor);
            break;
        }
        case MD_TEXT_NULLCHAR: {
            dest = lean_array_push(dest, lean_box(2));
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

    parse_stack_push((parse_stack *)stack, block_details);
    return 0;
}

static unsigned int block_ctor(MD_BLOCKTYPE type) {
    switch (type) {
    case MD_BLOCK_QUOTE:
        return 9;
    case MD_BLOCK_HR:
        return 5;
    case MD_BLOCK_H:
        return 6;
    case MD_BLOCK_CODE:
        return 7;
    case MD_BLOCK_HTML:
        return 8;
    case MD_BLOCK_P:
        return 0;
    case MD_BLOCK_TABLE:
        return 10;
    default:
        lean_internal_panic_unreachable();
    }
}

static int leave_block_callback(MD_BLOCKTYPE type, void *detail, void *userdata) {
    parse_stack *stack = (parse_stack *)userdata;

    switch (type) {
    case MD_BLOCK_DOC: {
        assert(stack->top == 1);
        // Here we don't allocate a document constructor because
        // of the newtype optimization
        lean_object *blocks = parse_stack_pop(stack);
        parse_stack_save(stack, blocks);
        break;
    }
    case MD_BLOCK_UL: {
        // The details provided as an argument here are incorrect; use the ones
        // passed to the enter callback
        MD_BLOCK_UL_DETAIL ul_detail = stack->details[stack->top].ul_details;
        uint8_t is_tight = ul_detail.is_tight ? 1 : 0;
        lean_object *items = parse_stack_pop(stack);
        lean_object *ul = lean_alloc_ctor(is_tight ? 1 : 2, 2, 0);
        lean_ctor_set(ul, 0, lean_box_uint32(ul_detail.mark));
        lean_ctor_set(ul, 1, items);
        parse_stack_save(stack, ul);
        break;
    }
    case MD_BLOCK_QUOTE: {
        lean_object *blocks = parse_stack_pop(stack);
        lean_object *quote = lean_alloc_ctor(block_ctor(type), 1, 0);
        lean_ctor_set(quote, 0, blocks);
        parse_stack_save(stack, quote);
        break;
    }
    case MD_BLOCK_OL: {
        // The details provided as an argument here are incorrect; use the ones
        // passed to the enter callback
        MD_BLOCK_OL_DETAIL ol_detail = stack->details[stack->top].ol_details;
        uint8_t is_tight = ol_detail.is_tight ? 1 : 0;
        lean_object *items = parse_stack_pop(stack);
        lean_object *ol = lean_alloc_ctor(is_tight ? 3 : 4, 3, 0);
        lean_ctor_set(ol, 0, lean_unsigned_to_nat(ol_detail.start));
        lean_ctor_set(ol, 1, lean_box_uint32(ol_detail.mark_delimiter));
        lean_ctor_set(ol, 2, items);
        parse_stack_save(stack, ol);
        break;
    }
    case MD_BLOCK_LI: {
        // The details provided to the enter callback are incorrect; use the
        // ones passed here
        MD_BLOCK_LI_DETAIL *li_detail = (MD_BLOCK_LI_DETAIL *)detail;
        lean_object *blocks = parse_stack_pop(stack);
        lean_object *li = lean_alloc_ctor(0, 3, 1);
        lean_ctor_set_uint8(li, 3 * sizeof(void *), li_detail->is_task ? 1 : 0);
        if (li_detail->is_task) {
            lean_object *mark = lean_alloc_ctor(1, 1, 0);
            lean_ctor_set(mark, 0, lean_box_uint32(li_detail->task_mark));
            lean_ctor_set(li, 0, mark);
            lean_object *offset = lean_alloc_ctor(1, 1, 0);
            lean_ctor_set(offset, 0,
                          lean_box_usize(li_detail->task_mark_offset));
            lean_ctor_set(li, 1, offset);
        } else {
            lean_ctor_set(li, 0, lean_box(0));
            lean_ctor_set(li, 1, lean_box(0));
        }
        lean_ctor_set(li, 2, blocks);

        parse_stack_save(stack, li);
        break;
    }
    case MD_BLOCK_HR: {
        lean_object *hr = lean_box(block_ctor(type));
        lean_object *items = parse_stack_pop(stack);
        assert(lean_array_size(items) == 0);
        lean_dec_ref(items);
        parse_stack_save(stack, hr);
        break;
    }
    case MD_BLOCK_H: {
        // The details provided as an argument here are incorrect; use the ones
        // passed to the enter callback
        MD_BLOCK_H_DETAIL h_detail = stack->details[stack->top].h_details;
        lean_object *texts = parse_stack_pop(stack);
        unsigned level = h_detail.level;
        lean_object *p = lean_alloc_ctor(block_ctor(type), 2, 0);
        lean_ctor_set(p, 0, lean_unsigned_to_nat(level));
        lean_ctor_set(p, 1, texts);
        parse_stack_save(stack, p);
        break;
    }
    case MD_BLOCK_CODE: {
        MD_BLOCK_CODE_DETAIL *code_detail = (MD_BLOCK_CODE_DETAIL *) detail;
        lean_object *info = get_attr(code_detail->info, lean_mk_empty_array());
        lean_object *lang = get_attr(code_detail->lang, lean_mk_empty_array());
        lean_object *strings = parse_stack_pop(stack);
        lean_object *code = lean_alloc_ctor(block_ctor(type), 4, 0);
        lean_ctor_set(code, 0, info);
        lean_ctor_set(code, 1, lang);
        if (code_detail->fence_char == 0) {
            lean_ctor_set(code, 2, lean_box(0));
        } else {
            lean_object *some = lean_alloc_ctor(1, 1, 0);
            lean_ctor_set(some, 0, lean_box_uint32(code_detail->fence_char));
            lean_ctor_set(code, 2, some);
        }
        lean_ctor_set(code, 3, strings);
        parse_stack_save(stack, code);
        break;
    }
    case MD_BLOCK_HTML: {
        lean_object *texts = parse_stack_pop(stack);
        lean_object *html = lean_alloc_ctor(block_ctor(type), 1, 0);
        lean_ctor_set(html, 0, texts);
        parse_stack_save(stack, html);
        break;
    }
    case MD_BLOCK_P: {
        lean_object *texts = parse_stack_pop(stack);
        lean_object *p = lean_alloc_ctor(block_ctor(type), 1, 0);
        lean_ctor_set(p, 0, texts);
        parse_stack_save(stack, p);
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
        lean_object *table = lean_alloc_ctor(block_ctor(type), 2, 0);
        lean_ctor_set(table, 0, thead);
        lean_ctor_set(table, 1, tbody);
        parse_stack_save(stack, table);
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
    // The details provided to the enter callback for spans are typically
    // incorrect, so there's no sense saving them
    parse_stack_push((parse_stack *)stack, (details)no_detail);
    return 0;
}

static unsigned span_ctor(MD_SPANTYPE type) {
    switch (type) {
    case MD_SPAN_EM:
        return 5;
    case MD_SPAN_STRONG:
        return 6;
    case MD_SPAN_U:
        return 7;
    case MD_SPAN_A:
        return 8;
    case MD_SPAN_IMG:
        return 9;
    case MD_SPAN_CODE:
        return 10;
    case MD_SPAN_DEL:
        return 11;
    case MD_SPAN_LATEXMATH:
        return 12;
    case MD_SPAN_LATEXMATH_DISPLAY:
        return 13;
    case MD_SPAN_WIKILINK:
        return 14;
    }
    lean_internal_panic_unreachable();
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
        lean_object *span = lean_alloc_ctor(span_ctor(type), 1, 0);
        lean_ctor_set(span, 0, txt);
        parse_stack_save(stack, span);
        break;
    }
    case MD_SPAN_A: {
        // Here we need the details provided to the leave callback
        MD_SPAN_A_DETAIL *a_detail = (MD_SPAN_A_DETAIL *)detail;
        lean_object *txt = parse_stack_pop(stack);
        lean_object *a = lean_alloc_ctor(span_ctor(type), 3, 1);
        lean_ctor_set_uint8(a, 3 * sizeof(void *),
                            a_detail->is_autolink ? 1 : 0);
        lean_object *href = lean_mk_empty_array();
        href = get_attr(a_detail->href, href);
        lean_ctor_set(a, 0, href);
        lean_object *title = lean_mk_empty_array();
        title = get_attr(a_detail->title, title);
        lean_ctor_set(a, 1, title);
        lean_ctor_set(a, 2, txt);
        parse_stack_save(stack, a);
        break;
    }
    case MD_SPAN_IMG: {
        // Here we need the details provided to the leave callback
        MD_SPAN_IMG_DETAIL *img_detail = (MD_SPAN_IMG_DETAIL *)detail;
        lean_object *alt = parse_stack_pop(stack);
        lean_object *img = lean_alloc_ctor(span_ctor(type), 3, 0);
        lean_object *src = lean_mk_empty_array();
        src = get_attr(img_detail->src, src);
        lean_ctor_set(img, 0, src);
        lean_object *title = lean_mk_empty_array();
        title = get_attr(img_detail->title, title);
        lean_ctor_set(img, 1, title);
        lean_ctor_set(img, 2, alt);
        parse_stack_save(stack, img);
        break;
    }
    case MD_SPAN_WIKILINK: {
        MD_SPAN_WIKILINK_DETAIL *wl_detail = (MD_SPAN_WIKILINK_DETAIL *)detail;
        lean_object *txt = parse_stack_pop(stack);
        lean_object *wl = lean_alloc_ctor(span_ctor(type), 2, 1);
        lean_object *target = lean_mk_empty_array();
        target = get_attr(wl_detail->target, target);
        lean_ctor_set(wl, 0, target);
        lean_ctor_set(wl, 1, txt);
        parse_stack_save(stack, wl);
        break;
    }
    }

    return 0;
}

static int text_callback(MD_TEXTTYPE type, const MD_CHAR *text, MD_SIZE size, void *userdata) {
    parse_stack *stack = (parse_stack *)userdata;
    switch (type) {
    case MD_TEXT_NORMAL: {
        lean_object *txt = lean_alloc_ctor(0, 1, 0);
        lean_ctor_set(txt, 0, lean_mk_string_from_bytes(text, size));
        parse_stack_save(stack, txt);
        break;
    }
    case MD_TEXT_NULLCHAR: {
        parse_stack_save(stack, lean_box(1));
        break;
    }
    case MD_TEXT_BR: {
        lean_object *txt = lean_alloc_ctor(2, 1, 0);
        lean_ctor_set(txt, 0, lean_mk_string_from_bytes(text, size));
        parse_stack_save(stack, txt);
        break;
    }
    case MD_TEXT_SOFTBR: {
        lean_object *txt = lean_alloc_ctor(3, 1, 0);
        lean_ctor_set(txt, 0, lean_mk_string_from_bytes(text, size));
        parse_stack_save(stack, txt);
        break;
    }
    case MD_TEXT_ENTITY: {
        lean_object *txt = lean_alloc_ctor(4, 1, 0);
        lean_ctor_set(txt, 0, lean_mk_string_from_bytes(text, size));
        parse_stack_save(stack, txt);
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
        // Return none
        parse_stack_free(stack);
        return lean_box(0);
    } else {
        assert(stack->top == 0);
        assert(lean_is_array(stack->args[0]));
        assert(lean_array_size(stack->args[0]) == 1);
        lean_object *doc = lean_array_uget(stack->args[0], 0);
        parse_stack_free(stack);
        assert(lean_is_exclusive(doc));

        lean_object *some = lean_alloc_ctor(1, 1, 0);
        lean_ctor_set(some, 0, doc);
        return some;
    }
}
