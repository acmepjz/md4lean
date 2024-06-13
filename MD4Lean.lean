/-! # md4lean

a Lean wrapper for the [MD4C](https://github.com/mity/md4c) Markdown parser

-/

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
def MD_FLAG_NOHTMLBLOCKS : UInt32 := x0020
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

end MD4Lean
