import MD4Lean

/--
  Read the input file and output the rendered HTML.
-/
def main (args : List String) : IO Unit := do
  match args.get? 0 with
  | some file =>
    let input ← IO.FS.readFile file
    let .some html := MD4Lean.renderHtml input | throw <| .userError "Parsing failed"
    IO.println <| html
  | _         => IO.println "Usage: md4lean <file>"
