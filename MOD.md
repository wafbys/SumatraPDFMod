# MOD：电子书四槽字体

这个分支（`mod`）在上游的基础上加了「四槽字体」：读 EPUB / HTML / TXT 等**重排文档**时，
用读者指定的字体替换书里声明的字体，连书里内嵌的字体也一起替换。PDF、封面图、扫描页不变。

| 槽 | 管什么 |
| --- | --- |
| `FontSerif` | `serif`、`Georgia`、`Times New Roman`… 以及其它没归类的族名 |
| `FontSans` | `sans-serif`、`Arial`、`Segoe UI`、`微软雅黑`… |
| `FontMono` | `monospace`、`Courier`、`Consolas`… |
| `FontCjk` | 中日韩族名，以及前三档画不出的缺字（逐字符回退） |

每个槽可以填**字体文件路径**，也可以填**系统已安装的字体族名**（后者不需要字体副本）。

## 在界面里设置

打开一本 EPUB，按 `Ctrl+K`（`eBook Settings`）→ 面板中部四个下拉框（列出系统已安装字体族，
也可直接输入文件路径），底部 `This file` / `For all ebooks` 决定是只改当前书还是全部。

## 在设置文件里设置

```ini
EBookUI [
	FontSerif = Valkyrie A
	FontSans  = Inter Variable Text
	FontMono  = NewComputerModernMono10
	FontCjk   = KingHwaOldSong-GB
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

`EBookUI.StraightQuotes`（默认 `true`）：拉丁文字里的 `“ ”` `‘ ’` 一律显示成直引号，
中文/日文/韩文里的引号保持原样（那里的弯引号才是正确用法）。面板上有勾选框
「Straight quotes in English text」，也支持按书覆盖（`FileStates → EBookUI → StraightQuotes`）。
实现是 `ext/patches/0062`：排版前按文本节点的 script 就地改写，只缩短字符串。

## 跟随上游

```sh
git fetch origin                      # origin = sumatrapdfreader/sumatrapdf
git log --oneline mod..origin/master  # 上游都有什么新东西
git rebase origin/master              # 把这一个提交重放到上游最新代码上
bun cmd/build.ts -rel                 # 需要 VS 2022 + bun，见上游 agents.md
```

`ext/mupdf` 的改动记在 `ext/patches/0065-html-font-slots-first.patch`；
上游升级 vendored mupdf 时按 `ext/patches/README.md` 的流程重放并重新生成补丁。

## 验证入口（本机脚本，未入库）

仓库根的 `.work/`（gitignore）里有做过的自检脚本：`slot-sig.ts`（按槽渲染样张并比像素；
用 `-window-pos` 让窗口开窗即最终尺寸，否则两次运行可能落在不同的布局状态）、
`slot-verify2.ts`（族名 / 每书覆盖）、`dialog-check.ts`、`dialog-save-check.ts`（面板链路）、
`bands.ts` / `page-rect.ts`（量行带与页面矩形，先确认两次运行在同一状态再比像素）。
