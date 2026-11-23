module
public section

/-! # md4lean AST

The data model for parsed Markdown documents.
-/
set_option linter.missingDocs true

namespace MD4Lean

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
  /--
  Inline raw HTML (see `MD_FLAG_NOHTMLSPANS` or `MD_FLAG_NOHTML`).

  Each piece of raw HTML that md4c reports is a separate element. Tags, comments, processing
  instructions, declarations or `CDATA` sections that span several lines are split at line breaks.
  A line break inside raw HTML is treated as part of the HTML, rather than a `softbr`.
  -/
  | html : String → Text
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
  /-- Raw HTML block (see `MD_FLAG_NOHTMLBLOCKS` or `MD_FLAG_NOHTML`) -/
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

end MD4Lean
