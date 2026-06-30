import Lake
open System Lake DSL

package MD4Lean where
  testDriver := "test"
  leanOptions := #[⟨`experimental.module, true⟩]

def md4cDir : FilePath := "md4c"
def wrapperDir := "wrapper"
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

target md4cEntityObj (pkg) : FilePath := md4cOTarget pkg "entity"
target md4cCoreObj (pkg) : FilePath := md4cOTarget pkg "md4c"
target md4cHtmlObj (pkg) : FilePath := md4cOTarget pkg "md4c-html"
target wrapperObj (pkg) : FilePath := wrapperOTarget pkg

@[default_target]
lean_lib MD4Lean where
  precompileModules := true
  defaultFacets := #[LeanLib.leanArtsFacet, LeanLib.sharedFacet]
  -- Link the md4c parser and the wrapper directly into the library. The precompiled shared
  -- library the interpreter loads is then self-contained: there is no separate md4c shared
  -- library to locate and bind at load time, and the wrapper's calls to the `@[export]`
  -- constructors stay within a single binary.
  moreLinkObjs := #[md4cEntityObj, md4cCoreObj, md4cHtmlObj, wrapperObj]

lean_exe «example» where
  root := `Main

lean_lib MD4LeanTest where
  -- Not actually needed, but we want the test to verify it compiles
  needs := #[«example»]

lean_exe test where
  root := `MD4LeanTestDriver
