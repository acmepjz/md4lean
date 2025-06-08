import MD4Lean
import MD4LeanTest.Parser

open MD4Lean.Test.Parser

/-!

# Rendering tests

-/

/-- info: some "<p>Hello <em>world</em></p>\n" -/
#guard_msgs in
#eval MD4Lean.renderHtml "Hello *world*"

/-- info: some "<ul>\n<li class=\"task-list-item\"><input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled=\"true\" />Is this valid XHTML?</li>\n<li class=\"task-list-item\"><input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled=\"true\" checked=\"true\" />Is this valid XHTML?</li>\n</ul>\n" -/
#guard_msgs in
#eval MD4Lean.renderHtml "- [ ] Is this valid XHTML?\n- [x] Is this valid XHTML?"

/-!

# Parsing tests

Here, there is a `main` that is intended to be executed, as well as
run from the Lean frontend. This is to check that the library works in
both contexts.

-/


/--
Runs the parser tests.

With no arguments, the tests are run once to ensure they pass. With a
number as an argument, the tests are run that many times; this can be
helpful to ensure the absence of reference counting errors in the FFI
used by the parser.
-/
def main : List String → IO UInt32
  | [] => do
    let successes ← IO.mkRef 0
    let failures ← IO.mkRef #[]
    runTests successes failures 0
    report successes failures
  | [n] => do
    match n.toNat? with
    | none =>
      IO.eprintln s!"Didn't understand '{n}' as a Nat"
      return 1
    | some k =>
      let successes ← IO.mkRef 0
      let failures ← IO.mkRef #[]
      for i in [0:k] do
        runTests successes failures i
      report successes failures
  | _ => do
    IO.eprintln "Too many arguments"
    return 2

/-!

Also ensure that the parser can be used from Lean itself

-/

/--
info: Succeeded after running 47 parses
---
info: 0
-/
#guard_msgs in
#eval main []
