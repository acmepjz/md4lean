namespace MD4Lean

@[extern "lean_md4c_markdown_to_html"]
opaque renderHtml (md : @& String) : Option String

end MD4Lean
