import { join, resolve } from "node:path";
import { detectVisualStudio, runLogged } from "./util";

// unit tests are compiled into SumatraPDFMod.exe only in Debug builds
const { msbuildPath } = detectVisualStudio();
const slnPath = join("vs2022", "SumatraPDF.sln");
await runLogged(msbuildPath, [slnPath, "/t:SumatraPDF:Rebuild", `/p:Configuration=Debug;Platform=x64`, `/m`]);

const dir = join("out", "dbg64");
await runLogged(resolve(join(dir, "SumatraPDFMod.exe")), ["-unit-tests"], dir);
