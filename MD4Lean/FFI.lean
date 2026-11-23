module

import MD4Lean.Basic

/-! # FFI constructors

The C wrapper builds the Markdown AST by calling the functions exported here rather than
allocating constructor objects directly, keeping the runtime layout of the AST types private
to Lean-compiled code.

Each function carries an `@[export]` C symbol. Object arguments are consumed. Character-valued
fields are passed as `UInt32` and `Option`-valued fields are passed as a presence flag plus a
raw value, so the C side never constructs a `Char` or an `Option`.
-/

namespace MD4Lean

set_option linter.missingDocs false

@[export lean_md4c_attr_normal] def attrNormal := AttrText.normal
@[export lean_md4c_attr_entity] def attrEntity := AttrText.entity
@[export lean_md4c_attr_nullchar] def attrNullchar (_ : Unit) : AttrText := .nullchar

@[export lean_md4c_text_normal] def textNormal := Text.normal
@[export lean_md4c_text_nullchar] def textNullchar (_ : Unit) : Text := .nullchar
@[export lean_md4c_text_br] def textBr := Text.br
@[export lean_md4c_text_softbr] def textSoftbr := Text.softbr
@[export lean_md4c_text_entity] def textEntity := Text.entity
@[export lean_md4c_text_em] def textEm := Text.em
@[export lean_md4c_text_strong] def textStrong := Text.strong
@[export lean_md4c_text_u] def textU := Text.u
@[export lean_md4c_text_a] def textA := Text.a
@[export lean_md4c_text_img] def textImg := Text.img
@[export lean_md4c_text_code] def textCode := Text.code
@[export lean_md4c_text_del] def textDel := Text.del
@[export lean_md4c_text_latex_math] def textLatexMath := Text.latexMath
@[export lean_md4c_text_latex_math_display] def textLatexMathDisplay := Text.latexMathDisplay
@[export lean_md4c_text_wikilink] def textWikiLink := Text.wikiLink
@[export lean_md4c_text_html] def textHtml := Text.html

@[export lean_md4c_block_p] def blockP := Block.p
@[export lean_md4c_block_ul] def blockUl := Block.ul
@[export lean_md4c_block_ol] def blockOl := Block.ol
@[export lean_md4c_block_hr] def blockHr (_ : Unit) : Block := .hr
@[export lean_md4c_block_header] def blockHeader := Block.header
@[export lean_md4c_block_html] def blockHtml := Block.html
@[export lean_md4c_block_blockquote] def blockBlockquote := Block.blockquote
@[export lean_md4c_block_table] def blockTable := Block.table

/-- Build a fenced or indented code block. `fenceChar` is used only when `hasFence` is true. -/
@[export lean_md4c_block_code]
def blockCode (info lang : Array AttrText) (hasFence : Bool) (fenceChar : UInt32)
    (strings : Array String) : Block :=
  .code info lang (if hasFence then some (Char.ofNat fenceChar.toNat) else none) strings

/-- Build a list item. `taskChar` and `taskOffset` are used only when `isTask` is true. -/
@[export lean_md4c_li]
def mkLi (isTask : Bool) (taskChar : UInt32) (taskOffset : USize)
    (contents : Array Block) : Li Block :=
  .li isTask
    (if isTask then some (Char.ofNat taskChar.toNat) else none)
    (if isTask then some taskOffset else none)
    contents

@[export lean_md4c_document_mk] def documentMk := Document.mk

-- We need two functions here because a polymorphic version exports a two-argument function that
-- requires a garbage type argument at the start.
@[export lean_md4c_some_document] def someDocument (d : Document) : Option Document := some d
@[export lean_md4c_some_string] def someString (s : String) : Option String := some s
