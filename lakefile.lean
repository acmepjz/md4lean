import Lake
open System Lake DSL

package MD4Lean where
  testDriver := "test"

def md4cDir : FilePath := "md4c"
def wrapperDir := "wrapper"
def srcNames := #["entity", "md4c", "md4c-html"]
def wrapperName := "wrapper"
def buildDir := defaultBuildDir

def md4cOTarget (pkg : Package) (srcName : String) : FetchM (Job FilePath) := do
  let oFile := pkg.dir / buildDir / md4cDir / ⟨ srcName ++ ".o" ⟩
  let srcTarget ← inputTextFile <| pkg.dir / md4cDir / ⟨ srcName ++ ".c" ⟩
  buildFileAfterDep oFile srcTarget fun srcFile => do
    if Platform.isWindows then
      let flags := #["-I", ((← getLeanIncludeDir) / "clang").toString,
        "-I", (pkg.dir / md4cDir).toString,
        "-I", (pkg.dir / md4cDir / "adhoc_include").toString, "-fPIC"]
      compileO oFile srcFile flags (← getLeanCc)
    else
      let flags := #["-I", (pkg.dir / md4cDir).toString, "-fPIC"]
      compileO oFile srcFile flags

def wrapperOTarget (pkg : Package) : FetchM (Job FilePath) := do
  let oFile := pkg.dir / buildDir / wrapperDir / ⟨ wrapperName ++ ".o" ⟩
  let srcTarget ← inputTextFile <| pkg.dir / wrapperDir / ⟨ wrapperName ++ ".c" ⟩
  buildFileAfterDep oFile srcTarget fun srcFile => do
    if Platform.isWindows then
      let flags := #["-I", (← getLeanIncludeDir).toString,
        "-I", ((← getLeanIncludeDir) / "clang").toString,
        "-I", (pkg.dir / md4cDir).toString,
        "-I", (pkg.dir / md4cDir / "adhoc_include").toString, "-fPIC"]
      compileO oFile srcFile flags (← getLeanCc)
    else
      let flags := #["-I", (← getLeanIncludeDir).toString,
        "-I", (pkg.dir / md4cDir).toString, "-fPIC"]
      compileO oFile srcFile flags

@[default_target]
lean_lib MD4Lean where
  precompileModules := true

extern_lib md4c (pkg) := do
  let name := nameToStaticLib "leanmd4c"
  let oTargets := (←srcNames.mapM (md4cOTarget pkg)) ++ #[←wrapperOTarget pkg]
  buildStaticLib (pkg.nativeLibDir / name) oTargets

lean_exe «example» where
  root := `Main


lean_lib MD4LeanTest

lean_exe test where
  root := `MD4LeanTestDriver
