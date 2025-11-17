module
public import MD4Lean

open MD4Lean

namespace MD4Lean.Test.Parser

/--
Runs a concrete parser test.

Parameters:

 * `successes` is a counter of successes so far, to be incremented if the test passes

 * `failures` is an array of parse errors that consist of the input,
   the expected output, and the actual output (if produced)

 * `expected` is the expected contents of the document

 * `input` is the string to parse

 * `parserFlags` is a set of flags for `md4c`, which defaults to `MD_DIALECT_COMMONMARK`
-/
def test
    (successes : IO.Ref Nat)
    (failures : IO.Ref (Array (String × Document × Option Document)))
    (expected : Array Block)
    (input : String)
    (parserFlags : UInt32 := MD_DIALECT_COMMONMARK) :
    IO Unit := do
  let actual := parse (parserFlags := parserFlags) input

  if some expected == actual.map (·.blocks) then
    successes.modify (· + 1)
  else
    failures.modify (·.push (input, ⟨expected⟩, actual))

/--
Runs a battery of parser tests, counting passed tests in `successes` and accumulating errors in `failures`.

The parameter `i` is used to allow consecutive runs to vary a bit.
-/
public def runTests (successes : IO.Ref Nat) (failures : IO.Ref (Array (String × Document × Option Document))) (i : Nat) : IO Unit := do
  test successes failures #[.p #[.normal s!"{i}"]] s!"{i}"
  test successes failures #[.p #[.normal "x"]] "x"
  test successes failures #[.p #[.normal "x", .softbr "\n", .normal "y"]] "x\ny"
  test successes failures #[.p #[.normal "x", .br "\n", .normal "y"]] "x\ny" (parserFlags := MD_FLAG_HARD_SOFT_BREAKS)
  test successes failures #[.p #[.normal "x"], .p #[.normal "y"]] "x\n\ny"
  test successes failures #[.p #[.normal "x", .nullchar, .normal "y"]] "x\x00y"
  test successes failures #[.p #[.normal "x", .entity "&emdash;", .normal "y"]] "x&emdash;y"
  test successes failures #[.html #["<br/>", "\n"]] "<br/>"
  test successes failures #[.blockquote #[.p #[.normal "Hello!"]]] "> Hello!"
  test successes failures #[.ul true '*' #[{contents := #[.p #[.normal "Hello!"]]}]] "* Hello!"
  test successes failures #[.ul true '*' #[{contents := #[.p #[.normal "Hello!"]]}, {contents :=  #[.p #[.normal "Again!"]]}]] "* Hello!\n* Again!"
  test successes failures #[.ul true '*' #[{contents := #[.p #[.normal "Hello!"]]}], .ul true '-' #[{contents := #[.p #[.normal "Again!"]]}]] "* Hello!\n- Again!"
  test successes failures #[.ul false '*' #[{contents := #[.p #[.normal "A"]]}, {contents := #[.p #[.normal "B"]]}, {contents := #[.p #[.normal "C"]]}]] "\n\n* A\n\n\n* B\n\n* C"
  test successes failures #[.ol true 1 '.' #[{contents := #[.p #[.normal "x"]]}, {contents := #[.p #[.normal "y"]]}]] "1. x\n2. y"
  test successes failures #[.ol false 1 ')' #[{contents := #[.p #[.normal "x"]]}, {contents := #[.p #[.normal "y"]]}]] "1) x\n\n2) y"
  test successes failures #[.p #[.normal "x"], .hr, .p #[.normal "y"]] "x\n\n---\n\ny"
  test successes failures #[.p #[.normal "x"], .hr, .p #[.normal "y"]] "x\n\n---\n\ny"
  test successes failures #[.header 1 #[.normal "foo"]] "# foo"
  test successes failures #[.header 2 #[.normal "foo"]] "## foo"
  test successes failures #[.header 2 #[.normal "foo"]] "foo\n---"
  test successes failures #[.header 1 #[.normal "foo"]] "foo\n==="
  test successes failures #[.code #[] #[] none #["abcdef", "\n", "ghijk", "\n"], .p #[.normal "5"]] "    abcdef\n    ghijk\n\n5"
  test successes failures #[.code #[.normal "lean"] #[.normal "lean"] (some '`') #["def five := 5", "\n"], .p #[.normal "5"]] "```lean\ndef five := 5\n```\n5"
  test successes failures #[.code #[.normal "lean extrafancy"] #[.normal "lean"] (some '`')  #["def five := 5", "\n"], .p #[.normal "5"]] "```lean extrafancy\ndef five := 5\n```\n5"
  test successes failures #[.code #[.normal "lean extrafancy"] #[.normal "lean"] (some '~')  #["def five := 5", "\n"], .p #[.normal "5"]] "~~~lean extrafancy\ndef five := 5\n~~~\n5"
  test successes failures #[.p #[.normal "it's ", .em #[.normal "very"], .normal " nice"]] "it's _very_ nice"
  test successes failures #[.p #[.normal "it's ", .u #[.normal "very"], .normal " nice"]] "it's _very_ nice" (parserFlags := MD_FLAG_UNDERLINE)
  test successes failures #[.p #[.normal "it's ", .em #[.normal "very"], .normal " nice"]] "it's *very* nice"
  test successes failures #[.p #[.normal "it's ", .strong #[.normal "very"], .normal " nice"]] "it's **very** nice"
  test successes failures #[.p #[.normal "it's ", .del #[.normal "very"], .normal " nice"]] "it's ~very~ nice" (parserFlags := MD_FLAG_STRIKETHROUGH)
  test successes failures #[.p #[.normal "given by $f(x)$, it's ..."]] "given by $f(x)$, it's ..."
  test successes failures #[.p #[.normal "given by ", .latexMath #["f(x)"], .normal ", it's ..."]] "given by $f(x)$, it's ..." (parserFlags := MD_FLAG_LATEXMATHSPANS)
  test successes failures #[.p #[.normal "given by ", .latexMathDisplay #["f(x)"], .normal ", it's ..."]] "given by $$f(x)$$, it's ..." (parserFlags := MD_FLAG_LATEXMATHSPANS)
  test successes failures #[.p #[.normal "go ", .a #[.normal "https://example.com"] #[.normal "an excellent", .entity "&trade;", .normal " site"] false #[.normal "here"]]] "go [here](https://example.com \"an excellent&trade; site\")"
  test successes failures #[.p #[.a #[.normal "https://example.com"] #[] true #[.normal "https://example.com"]]] "<https://example.com>"
  test successes failures #[.p #[.a #[.normal "https://example.com"] #[] false #[.normal "txt"], .normal " [txt][nonref]"]] "[txt][ref] [txt][nonref]\n\n[ref]: https://example.com"
  test successes failures #[.p #[.img #[.normal "foo.jpg"] #[] #[.normal "blah"]]] "![blah](foo.jpg)"
  test successes failures #[.p #[.img #[.normal "foo.jpg"] #[.normal "title"] #[.normal "blah"]]] "![blah](foo.jpg \"title\")"
  test successes failures #[.p #[.wikiLink #[.normal "link"] #[.normal "link"]]] "[[link]]" (parserFlags := MD_FLAG_WIKILINKS)
  test successes failures #[.p #[.wikiLink #[.normal "tgt"] #[.normal "lbl"]]] "[[tgt|lbl]]" (parserFlags := MD_FLAG_WIKILINKS)
  test successes failures #[.p #[.normal "some ", .code #["code"]]] "some `code`"
  test successes failures
    #[.ul false '*'
      #[⟨false, none, none, #[.p #[.normal "foo"], .code #[.normal "lean"] #[.normal "lean"] (some '`') #[" ", "blah", "\n"]]⟩,
        ⟨false, none, none, #[.p #[.normal "bar"]]⟩]]
    " * foo\n\n    ```lean\n     blah\n     ```\n * bar\n"
  test successes failures #[.ul true '*' #[⟨false, none, none, #[.p #[.normal "foo  ", .em #[.normal "stuff"]]]⟩, ⟨false, none, none, #[.p #[.normal "bar"]]⟩]] " * foo  *stuff*\n * bar"
  test successes failures #[.ul true '*' #[⟨false, none, none, #[.p #[.normal "foo"], .ul true '*' #[⟨false, none, none, #[.p #[.normal "bar```"]]⟩]]⟩]] " * foo\n   * bar```\n"
  test successes failures
    #[.ul true '*'
      #[⟨false, none, none,
        #[.p #[MD4Lean.Text.normal "foo"],
          .code #[.normal "lean"] #[.normal "lean"] (some '`') #["blah", "\n"]]⟩]]
    " * foo\n   ```lean\n   blah\n   ```\n"
  test successes failures tableAst  tableString (parserFlags := MD_FLAG_TABLES)
  test successes failures #[.ul true '*' #[⟨true, some ' ', some 4, #[.p #[.normal "one"]] ⟩, ⟨true, some 'X', some 15, #[.p #[.normal "two"]] ⟩]] " * [ ] one\n * [X] two" (parserFlags := MD_FLAG_TASKLISTS)
where
  tableAst : Array Block :=
    #[.table
      #[#[.normal "a"], #[.normal "b"]]
      #[#[#[.normal "x"], #[.code #["y"]]],
        #[#[.normal "1"], #[.normal "2"]]]]
  tableString := r#"
| a | b   |
|---|-----|
| x | `y` |
| 1 | 2   |
"#


/--
Given the results from `runTests`, report them in a friendly manner
and return an exit code (`0` if all succeeded, non-zero otherwise).
-/
public def report
    (successes : IO.Ref Nat)
    (failures : IO.Ref (Array (String × Document × Option Document))) :
    IO UInt32 := do
  let successes ← successes.get
  let failures ← failures.get
  if failures.isEmpty then
    IO.println s!"Succeeded after running {successes} parses"
    pure 0
  else
    IO.println s!"Failed {failures.size} times with {successes} successful parses."
    for (str, expected, actual) in failures do
      IO.println "--------------------"
      if let some actual := actual then
        IO.println "Parsed:"
        IO.println str
        IO.println "Expected:"
        IO.println <| repr <| expected
        IO.println "Got:"
        IO.println <| repr actual
      else
        IO.println "Failed to parse:"
        IO.println str
        IO.println "Expected:"
        IO.println <| repr <| expected
    IO.println "--------------------"
    pure 1
