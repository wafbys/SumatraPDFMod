# MOD：电子书四槽字体（SumatraPDFMod）

这个分支（`mod`）在上游 SumatraPDF 的基础上加了「四槽字体」：读 EPUB / HTML / TXT 等
**重排文档**时，用读者指定的字体替换书里声明的字体，连书里内嵌的字体也一起替换。
PDF、封面图、扫描页不变。附带一个开关，把拉丁文字里的弯引号显示成直引号。

| 槽 | 管什么 |
| --- | --- |
| `ModFontSerif` | `serif`、`Georgia`、`Times New Roman`… 以及其它没归类的族名 |
| `ModFontSans` | `sans-serif`、`Arial`、`Segoe UI`、`微软雅黑`… |
| `ModFontMono` | `monospace`、`Courier`、`Consolas`… |
| `ModFontCjk` | 中日韩族名，以及前三档画不出的缺字（逐字符回退） |

每个槽可以填**字体文件路径**，也可以填**系统已安装的字体族名**（后者不需要字体副本）。

## 名字都带 Mod

| 东西 | 名字 |
| --- | --- |
| 产物（`out/rel64/`、`out/dbg64/`） | `SumatraPDFMod.exe`（静态版 `SumatraPDFMod-static.exe`） |
| 引擎 DLL / 命令行工具 | `libsumatrapdfMod.dll`、`SumatraPDFMod-tool.exe` |
| 窗口标题、关于页、exe 文件属性 | `SumatraPDFMod`（`kAppName`、`kSumatraWindowTitle`） |
| 便携设置文件 | `SumatraPDFMod-settings.txt`（exe 旁；见下） |
| 本分支自己的设置项 | 一律 `Mod` 前缀，排在各自结构（`EBookUI` / `FileEBookUI`）**最前面**、按字母序；`EBookUI` 块本身也排在设置文件最前，Mod 组之后有一条分割线注释 `# ---- mod settings above; plain SumatraPDF settings below ----` |
| 不变的名字 | `PdfFilter.dll`、`PdfPreview.dll`（Windows 搜索/预览组件，名字里本来就没有 SumatraPDF）；premake 工程名与源文件名（`SumatraPDF.vcxproj`、`src/SumatraPDF.cpp`…）也是原样，改它们只动开发环境、不动产物 |

产物的名字有两处，premake 重新生成时会保持一致：`premake5.lua` 里各工程的
`targetname`，以及已提交的 `vs2022/*.vcxproj` 里的 `<TargetName>`（`cmd/build.ts` 默认
走 MSBuild + vcxproj；`-ninja` 时走 `cmd/ninja.ts` 的资源表，同样已改）。exe 的
`/DELAYLOAD:libsumatrapdfMod.dll`、预打包命令里的文件名、`EmbeddedResources.cpp` 里
"DLL 找同目录的 exe 读嵌入资源"的那一处，也都跟着改了。

**设置文件**：便携版把设置放在 exe 旁，所以本 mod 用**自己的文件名**，免得和官方
SumatraPDF 放在一个目录时互相覆盖（官方版保存时会丢掉它不认识的 `Mod*` 字段）。
第一次启动如果只找到官方的 `SumatraPDF-settings.txt`，会把它**改名**成
`SumatraPDFMod-settings.txt`（改名失败就照旧读旧文件，不丢数据）。实现在
`src/shared/AppSettings.cpp`（`GetSettingsFileNameTemp` + `LoadSettings` 里的搬迁）。

改名前写的旧键名（`FontSerif` 等）**不保留、不兼容**：本分支里已经没有这些名字，
配置文件里也不会有（本仓库的部署目录已经改成 `Mod*`）。要在自己的文件里改：
`EBookUI` / `FileEBookUI` 块里的键名前面加上 `Mod` 即可，没改的按未知设置忽略。

## 在界面里设置

打开一本 EPUB，按 `Ctrl+Shift+K`（或 `Ctrl+K` → 输入 `>Change eBook Settings` → 回车）打开
`eBook Settings` 面板；面板**最上面**就是四个下拉框（列出系统已安装字体族，也可直接输入文件路径）
和直引号勾选框；往下一条分割线，线下面才是原生设置（字体/字号/边距/行距/忽略原书样式/自定义 CSS）。
`This file` / `For all ebooks` 决定是只改当前书还是全部。

## 在设置文件里设置

```ini
EBookUI [
	ModFontSerif = Valkyrie A
	ModFontSans  = Inter Variable Text
	ModFontMono  = NewComputerModernMono10
	ModFontCjk   = KingHwaOldSong-GB
]
```

每本书的覆盖写在 `FileStates → [ → FilePath = … → EBookUI [ … ]`，字段同名。

## 每项设置详解

**上半区（本 mod 的四槽字体 + 直引号）**

| 控件 | 含义 |
| --- | --- |
| `Serif:` / `Sans:` / `Mono:` / `CJK:` | 四槽。可填**系统已安装的字体族名**（下拉里挑或手输；变量字体的实例名如 `Inter Variable Text` 也认，GDI 兜底）或**字体文件路径**。留空 = `(the book's font)` = 不覆盖这一档。书里声明的族名按名字归类：中日韩族名/表意文字 → CJK，等宽族名 → Mono，无衬线族名 → Sans，**其余全部 → Serif**（Serif 是兜底档）。CJK 槽还负责**缺字回退**。优先级：四槽 → 书内 `@font-face` → 内置字体 → 系统字体。只对重排文档生效；粗体斜体由引擎合成。 |
| `Straight quotes in English text` | 拉丁文字里的 `“ ” ‘ ’` → 直引号 `" '`，中日韩引号不动；按字符的文字系统判定，与书的语言无关。默认开。 |

**下半区（原生 SumatraPDF 的设置，长度单位是排版点）**

| 控件 | 含义 |
| --- | --- |
| `Font:` | 整本书的默认字体族（空 = 引擎默认，通常衬线）；以 `!important` 注入 CSS 盖过书里的 `font-family`；加载不到会弹通知。填的名字同样过四槽（如填 `Georgia` → 用 `Serif:` 槽的字体）。 |
| `Size:` | 正文字号（点）；空/0 = 8.0。 |
| `Margin:` | 四周留白：1 数 = 四边相同，2 数 = 上下/左右，4 数 = 上/右/下/左；空 = 默认（3 em / 2 em，跟随字号）；`0` = 无边距；单值上限 200。 |
| `Line spacing:` | 行高倍数（如 `1.5`）；`0` = 默认；接受 0.5–5。 |
| `Ignore the document's own styling` | 丢弃书自己的 CSS，只用自定义 CSS。 |
| `Custom CSS` | 勾上 → 下面的框可编辑，且 `Font:` / `Margin:` / `Line spacing:` 变灰失效（字号仍有效）；不勾 → 框只读，显示将要注入的 CSS，什么都没设时显示 `The document's own styling is used.`。 |
| `◎ This file` / `○ For all ebooks` | 值写到哪：这本书的 `FileStates → EBookUI`（等于全局值的项不写）还是全局 `EBookUI`；OK 后立刻重排。 |
| `Reset to defaults` | 按当前范围重置：`This file` = 恢复成全局值；`For all ebooks` = 全清成默认（直引号回默认为开）。都要再按 OK 才写盘。 |
| `Cancel` / `OK` | Cancel 不保存；OK 保存并重排。 |

打开方式：`Ctrl+Shift+K`，或 `Ctrl+K` → `>Change eBook Settings` → 回车。

## 优先级

读者的四槽 → 书内 `@font-face` 字体 → 内置字体 → 系统字体。
（书内字体优先于四槽是上游行为；本分支的 `ext/patches/0061` 把它反过来。）

## 字体名怎么解析

填族名时先查 SumatraPDF 自己的字体表（按字体文件的 name 表建的）；查不到再问一次 GDI：
`CreateFontW` + `GetFontData` 取字体字节。变量字体的**实例名**（如 `Inter Variable Text`）
只出现在系统字体列表里、不在文件的 name 表里，靠这一层兜住，效果与直接填 `Inter Variable`
逐像素相同。GDI 对不认识的名字会悄悄换成默认字体，所以取到字节后用 `GetTextFace` 核对实际
选中的 face，不一致就当没找到（照旧告警 + 回退）。名字超过 31 字符（LOGFONT 上限）、或字体把
轮廓放在 CFF 表里（`GetFontData` 不给 SFNT 头）时这一层救不了，行为与以前一致。
实现在 `src/shared/mupdf/mupdf_load_system_font.c`（本分支自有文件，不动 vendored mupdf）。

## 英文直引号

`EBookUI.ModStraightQuotes`（默认 `true`）：拉丁文字里的 `“ ”` `‘ ’` 一律显示成直引号，
中文/日文/韩文里的引号保持原样（那里的弯引号才是正确用法）。判定按文本片段的 script、
与书的语言无关；纯标点的片段跟随最近的单词。面板上有勾选框「Straight quotes in English
text」，也支持按书覆盖（`FileStates → EBookUI → ModStraightQuotes`）。
实现是 `ext/patches/0062`：排版前按文本节点的 script 就地改写，只缩短字符串。

## 跟随上游

```sh
git fetch origin                      # origin = sumatrapdfreader/sumatrapdf
git log --oneline mod..origin/master  # 上游都有什么新东西
git rebase origin/master              # 把这些提交重放到上游最新代码上
bun cmd/build.ts -rel                 # 需要 VS 2022 + bun，见上游 agents.md
```

本分支对上游的改动集中在：`src/shared/EngineMupdf.*`、`src/EbookSettingsDialog.cpp`、
`src/shared/EbookSettingsDialogCommon.h`、`src/shared/mupdf/`（自有文件）、
`cmd/gen-settings.ts`（`Mod*` 字段在最前面）、`premake5.lua` 的 `targetname`、
以及 `ext/mupdf` 的两个补丁（0061 四槽优先、0062 直引号）。
上游升级 vendored mupdf 时按 `ext/patches/README.md` 的流程重放并重新生成补丁。

## 验证入口（本机脚本，未入库）

仓库根的 `.work/`（gitignore）里有做过的自检脚本：`slot-sig.ts`（按槽渲染样张并比像素；
用 `-window-pos` 让窗口开窗即最终尺寸，否则两次运行可能落在不同的布局状态）、
`mod-name-check.ts`（窗口标题、设置文件名、旧文件名搬迁）、
`slot-verify2.ts`（族名 / 每书覆盖）、`dialog-check.ts`、`dialog-save-check.ts`（面板链路）、
`bands.ts` / `page-rect.ts`（量行带与页面矩形，先确认两次运行在同一状态再比像素）。
