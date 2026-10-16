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
| 窗口标题、关于页、exe 文件属性 | `SumatraPDFMod`（`kAppName`、`kSumatraWindowTitle`） |
| 便携设置文件 | `SumatraPDFMod-settings.txt`（exe 旁；见下） |
| 本分支自己的设置项 | 一律 `Mod` 前缀，且排在各自结构（`EBookUI` / `FileEBookUI`）**最前面**、按字母序 |
| 随附组件 | `libsumatrapdf.dll`、`PdfFilter.dll`、`PdfPreview.dll`、`sumatrapdf-tool.exe` 仍是上游名（内部组件，改名要动延迟加载） |

产物的名字有两处，premake 重新生成时会保持一致：`premake5.lua` 里两个工程的
`targetname`，以及已提交的 `vs2022/SumatraPDF.vcxproj`、`vs2022/SumatraPDF-static.vcxproj`
里的 `<TargetName>`（`cmd/build.ts` 默认走 MSBuild + vcxproj；`-ninja` 时走
`cmd/ninja.ts` 的资源表，同样已改）。

**设置文件**：便携版把设置放在 exe 旁，所以本 mod 用**自己的文件名**，免得和官方
SumatraPDF 放在一个目录时互相覆盖（官方版保存时会丢掉它不认识的 `Mod*` 字段）。
第一次启动如果只找到官方的 `SumatraPDF-settings.txt`，会把它**改名**成
`SumatraPDFMod-settings.txt`（改名失败就照旧读旧文件，不丢数据）。实现在
`src/shared/AppSettings.cpp`（`GetSettingsFileNameTemp` + `LoadSettings` 里的搬迁）。

改名前写的字段名（`FontSerif` 等）不会被解析，但文件里会原样保留 —— 把
`EBookUI` / `FileEBookUI` 块里的键名前面加上 `Mod` 即可（本仓库的部署目录已经改好）。

## 在界面里设置

打开一本 EPUB，按 `Ctrl+K`（`eBook Settings`）→ 面板中部四个下拉框（列出系统已安装字体族，
也可直接输入文件路径），底部 `This file` / `For all ebooks` 决定是只改当前书还是全部。

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

## 优先级

读者的四槽 → 书内 `@font-face` 字体 → 内置字体 → 系统字体。
（书内字体优先于四槽是上游行为；本分支的 `ext/patches/0065` 把它反过来。）

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
实现是 `ext/patches/0066`：排版前按文本节点的 script 就地改写，只缩短字符串。

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
以及 `ext/mupdf` 的两个补丁（0065 四槽优先、0066 直引号）。
上游升级 vendored mupdf 时按 `ext/patches/README.md` 的流程重放并重新生成补丁。

## 验证入口（本机脚本，未入库）

仓库根的 `.work/`（gitignore）里有做过的自检脚本：`slot-sig.ts`（按槽渲染样张并比像素；
用 `-window-pos` 让窗口开窗即最终尺寸，否则两次运行可能落在不同的布局状态）、
`mod-name-check.ts`（窗口标题、设置文件名、旧文件名搬迁）、
`slot-verify2.ts`（族名 / 每书覆盖）、`dialog-check.ts`、`dialog-save-check.ts`（面板链路）、
`bands.ts` / `page-rect.ts`（量行带与页面矩形，先确认两次运行在同一状态再比像素）。
