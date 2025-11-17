module
public section

/-! # md4lean

a Lean wrapper for the [MD4C](https://github.com/mity/md4c) Markdown parser

-/
set_option linter.missingDocs true

namespace MD4Lean

/-! ## Parser flags

The default behavior is to recognize only Markdown syntax defined by the
[CommonMark specification](http://spec.commonmark.org/).

However, with appropriate flags, the behavior can be tuned to enable some
extensions.
-/

/-- With the flag `MD_FLAG_COLLAPSEWHITESPACE`, a non-trivial whitespace is
  collapsed into a single space. -/
def MD_FLAG_COLLAPSEWHITESPACE : UInt32 := 0x0001
/-- Do not require space in ATX headers ( `###header` ) -/
def MD_FLAG_PERMISSIVEATXHEADERS : UInt32 := 0x0002
/-- With the flag `MD_FLAG_PERMISSIVEURLAUTOLINKS` permissive URL autolinks
  (not enclosed in `<` and `>`) are supported. -/
def MD_FLAG_PERMISSIVEURLAUTOLINKS : UInt32 := 0x0004
/-- With the flag `MD_FLAG_PERMISSIVEEMAILAUTOLINKS`, permissive e-mail
  autolinks (not enclosed in `<` and `>`) are supported. -/
def MD_FLAG_PERMISSIVEEMAILAUTOLINKS : UInt32 := 0x0008
/-- Disable indented code blocks. (Only fenced code works.) -/
def MD_FLAG_NOINDENTEDCODEBLOCKS : UInt32 := 0x0010
/-- Disable raw HTML blocks. -/
def MD_FLAG_NOHTMLBLOCKS : UInt32 := 0x0020
/-- Disable raw HTML (inline). -/
def MD_FLAG_NOHTMLSPANS : UInt32 := 0x0040
/-- With the flag `MD_FLAG_TABLES`, GitHub-style tables are supported. -/
def MD_FLAG_TABLES : UInt32 := 0x0100
/-- With the flag `MD_FLAG_STRIKETHROUGH`, strike-through spans are enabled
  (text enclosed in tilde marks, e.g. `~foo bar~`). -/
def MD_FLAG_STRIKETHROUGH : UInt32 := 0x0200
/-- With the flag `MD_FLAG_PERMISSIVEWWWAUTOLINKS` permissive WWW autolinks
  without any scheme specified (e.g. `www.example.com`) are supported. MD4C
  then assumes `http:` scheme. -/
def MD_FLAG_PERMISSIVEWWWAUTOLINKS : UInt32 := 0x0400
/-- With the flag `MD_FLAG_TASKLISTS`, GitHub-style task lists are supported. -/
def MD_FLAG_TASKLISTS : UInt32 := 0x0800
/-- With the flag `MD_FLAG_LATEXMATHSPANS` LaTeX math spans (`$...$`) and
  LaTeX display math spans (`$$...$$`) are supported. (Note though that the
  HTML renderer outputs them verbatim in a custom tag `<x-equation>`.) -/
def MD_FLAG_LATEXMATHSPANS : UInt32 := 0x1000
/-- With the flag `MD_FLAG_WIKILINKS`, wiki-style links (`[[link label]]` and
  `[[target article|link label]]`) are supported. (Note that the HTML renderer
  outputs them in a custom tag `<x-wikilink>`.) -/
def MD_FLAG_WIKILINKS : UInt32 := 0x2000
/-- With the flag `MD_FLAG_UNDERLINE`, underscore (`_`) denotes an underline
  instead of an ordinary emphasis or strong emphasis. -/
def MD_FLAG_UNDERLINE : UInt32 := 0x4000
/-- Force all soft breaks to act as hard breaks. -/
def MD_FLAG_HARD_SOFT_BREAKS : UInt32 := 0x8000

/-- Enable all auto-linking. -/
def MD_FLAG_PERMISSIVEAUTOLINKS : UInt32 := MD_FLAG_PERMISSIVEEMAILAUTOLINKS |||
    MD_FLAG_PERMISSIVEURLAUTOLINKS ||| MD_FLAG_PERMISSIVEWWWAUTOLINKS
/-- Disable raw HTML. -/
def MD_FLAG_NOHTML : UInt32 := MD_FLAG_NOHTMLBLOCKS ||| MD_FLAG_NOHTMLSPANS

/-! Convenient sets of flags corresponding to well-known Markdown dialects.

Note we may only support subset of features of the referred dialect.
The constant just enables those extensions which bring us as close as
possible given what features we implement.

ABI compatibility note: Meaning of these can change in time as new
extensions, bringing the dialect closer to the original, are implemented.
-/

/-- The CommonMark dialect. -/
def MD_DIALECT_COMMONMARK : UInt32 := 0
/-- The GitHub dialect. -/
def MD_DIALECT_GITHUB : UInt32 := MD_FLAG_PERMISSIVEAUTOLINKS ||| MD_FLAG_TABLES |||
    MD_FLAG_STRIKETHROUGH ||| MD_FLAG_TASKLISTS

/-! ## HTML renderer flags
-/

/-- If set, debug output is sent to stderr. -/
def MD_HTML_FLAG_DEBUG : UInt32 := 0x0001
/-- If set, output the entity verbatim, otherwise translate entity to its UTF-8 equivalent. -/
def MD_HTML_FLAG_VERBATIM_ENTITIES : UInt32 := 0x0002
/-- Skip UTF-8 byte order mark (BOM) in the input string. -/
def MD_HTML_FLAG_SKIP_UTF8_BOM : UInt32 := 0x0004
/-- Output XHTML (e.g. `<br />`) instead of HTML (e.g. `<br>`). -/
def MD_HTML_FLAG_XHTML : UInt32 := 0x0008
/-- Output `\(` and `\)` (which are accepted by MathJax) instead of `<x-equation>` for
LaTeX math spans. -/
def MD_HTML_FLAG_MATHJAX : UInt32 := 0x1000
/-- Output `$` (which are accepted by MathJax) instead of `<x-equation>` for
LaTeX math spans. Must be used with `MD_HTML_FLAG_MATHJAX`, otherwise it is ignored. -/
def MD_HTML_FLAG_MATHJAX_USE_DOLLAR : UInt32 := 0x2000

/-! ## AST
-/

/--
The text that can occur in attributes such as link destinations, image sources, and titles
-/
inductive AttrText where
  /-- Normal text -/
  | normal : String → AttrText
  /--
  An HTML entity as a complete string, e.g. `"&nbsp;"`.

  No validation is performed, anything that's syntactically an entity uses this constructor.
  -/
  | entity : String → AttrText
  /-- A null character -/
  | nullchar : AttrText
deriving Inhabited, Repr, BEq

/-- Inline elements (sometimes called spans or inlines) -/
inductive Text where
  /-- Normal text -/
  | normal : String → Text
  /-- A null character -/
  | nullchar
  /--
  A hard line break, meaningful for the semantics of the text (see `MD_FLAG_HARD_SOFT_BREAKS`)
  -/
  | br : String → Text
  /-- A soft line break (renderers will often choose to ignore this) -/
  | softbr : String → Text
  /--
  An HTML entity as a complete string, e.g. `"&nbsp;"`.

  No validation is performed, anything that's syntactically an entity uses this constructor.
  -/
  | entity : String → Text
  /-- Emphasized text, typically italic -/
  | em : Array Text → Text
  /-- Strong emphasis, typically boldface -/
  | strong : Array Text → Text
  /-- Underlined text (see `MD_FLAG_UNDERLINE`) -/
  | u : Array Text → Text
  /--
  A link.

   - `href` is the destination
   - `title` is the provided link title (in quotes after the URL in Markdown syntax)
   - `isAuto` is true when the link was created from `<`...`>` syntax, false otherwise
  -/
  | a (href title : Array AttrText) (isAuto : Bool) : Array Text → Text
  /--
  An image.

   - `src` is the source URL
   - `title` is the provided image title (in quotes after the URL in Markdown syntax)
   - `alt` is the alt text for the image, to be shown if the image isn't available
  -/
  | img (src title : Array AttrText) (alt : Array Text) : Text
  /-- Code -/
  | code : Array String → Text
  /-- Deleted text, typically shown as a strikethrough -/
  | del : Array Text → Text
  /-- An inline LaTeX math element, built with `$`...`$` (see `MD_FLAG_LATEXMATHSPANS`) -/
  | latexMath : Array String → Text
  /-- An display LaTeX math element, built with `$$`...`$$` (see `MD_FLAG_LATEXMATHSPANS`) -/
  | latexMathDisplay : Array String → Text
  /-- A wiki-style link (see `MD_FLAG_WIKILINKS`) -/
  | wikiLink (target : Array AttrText) : Array Text → Text
deriving Inhabited, Repr, BEq

/-- A list item -/
structure Li (α) where
  li ::
  /-- Is this a task?  -/
  isTask : Bool := false
  /--
  What is in the checkbox? Always `'X'`, `'x'`, or `' '` if not `none`. (see `MD_FLAG_TASKLISTS`)
  -/
  taskChar : Option Char := none
  /-- Where is the task mark in the document? (see `MD_FLAG_TASKLISTS`) -/
  taskMarkOffset : Option USize := none
  /-- The contents of the list item -/
  contents : Array α
deriving Inhabited, Repr, BEq

/-- A block-level element -/
inductive Block where
  /-- A paragraph -/
  | p : Array Text → Block
  /-- An unordered list -/
  | ul (tight : Bool) (mark : Char) : Array (Li Block) → Block
  /-- An ordered list -/
  | ol (tight : Bool) (start : Nat) (mark : Char) : Array (Li Block) → Block
  /-- A thematic break, indicated with `-------` -/
  | hr
  /-- A header -/
  | header : Nat → Array Text → Block
  /--
  A code bock (see `MD_FLAG_NOINDENTEDCODEBLOCKS`).

  The `info` field contains the rest of the text after the initial fence, while `lang` contains the
  prefix of `info` that specifies the language. `fenceChar` is the character used to delimit the
  block (backtick or tilde).

  For indented code blocks, `info` and `lang` are `#[]` and `fenceChar` is `none`.
  -/
  | code (info lang : Array AttrText) (fenceChar : Option Char) : Array String → Block
  /-- Inline HTML block (see `MD_FLAG_NOHTMLBLOCKS` or `MD_FLAG_NOHTML`) -/
  | html : Array String → Block
  /-- A block quote (introduced with `>`) -/
  | blockquote : Array Block → Block
  /-- A table

    - `head` is array in which each element is a cell in the header
    - `body` is array in which each element is a row in the body. Each row is an array of cells.
  -/
  | table (head : Array (Array Text)) (body : Array (Array (Array Text))) : Block
deriving Inhabited, Repr, BEq

/-- A document -/
structure Document where
  /-- The block-level elements of the document -/
  blocks : Array Block
deriving Inhabited, Repr, BEq

/-! ## Functions
-/

/-- Render Markdown into HTML.

Note only contents of `<body>` tag is generated. Caller must generate
HTML header/footer manually before/after calling this function.

- `input` is the input markdown string.
- `parserFlags` is bitmask of `MD_FLAG_xxxx`.
- `rendererFlags` is bitmask of `MD_HTML_FLAG_xxxx`.

Return `Option.some s` if render is successful, otherwise return `Option.none`.
-/
@[extern "lean_md4c_markdown_to_html"]
opaque renderHtml (input : @& String)
    (parserFlags : UInt32 :=
      MD_DIALECT_GITHUB ||| MD_FLAG_LATEXMATHSPANS ||| MD_FLAG_NOHTML)
    (rendererFlags : UInt32 :=
      MD_HTML_FLAG_XHTML ||| MD_HTML_FLAG_MATHJAX ||| MD_HTML_FLAG_MATHJAX_USE_DOLLAR) :
    Option String

/--
Parses Markdown into an AST.

- `input` is the input markdown string.
- `parserFlags` is bitmask of `MD_FLAG_xxxx`.

Returns `some` if the underlying md4c parser succeeds, or `none` if it fails.
-/
@[extern "lean_md4c_markdown_parse"]
opaque parse (input : @& String) (parserFlags : UInt32 := MD_DIALECT_COMMONMARK) : Option Document

end MD4Lean
