import MD4Lean

/-- info: some "<p>Hello <em>world</em></p>\n" -/
#guard_msgs in
#eval MD4Lean.renderHtml "Hello *world*"
