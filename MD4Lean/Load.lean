module

import Lean.LoadDynlib

public section

namespace MD4Lean

private def sharedLibExt : String :=
  if System.Platform.isWindows then
    "dll"
  else if System.Platform.isOSX then
    "dylib"
  else
    "so"

private def sharedLibName : String :=
  let name := "MD4Lean_MD4Lean"
  let libPrefix := if System.Platform.isWindows then "" else "lib"
  libPrefix ++ name ++ "." ++ sharedLibExt

initialize
  try
    Lean.loadDynlib sharedLibName
  catch _ =>
    pure ()

end MD4Lean
