/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

// --- shared by EbookSettingsDialogCommon.cpp and each app's EbookSettingsDialog.cpp ---

Str FontDefaultLabel();
// the font-slot dropdowns' first entry: the document's own font, no override
Str FontSlotDefaultLabel();
void ParseMargin(Str s, Vec<float>& out);
TempStr MarginTextTemp(const Vec<float>* margin);
bool MarginEq(const Vec<float>& a, const Vec<float>* b);

// what the controls hold, ready to be written to a settings struct
struct EbookVals {
    Str fontName; // not owned, points into the controls' temp strings
    float fontSize = 0;
    Vec<float> margin; // 1, 2 or 4 values; empty means unset
    float lineSpacing = 0;
    bool ignoreDocumentCSS = false;
    Str customCSS; // not owned; empty unless useCustomCSS
    bool useCustomCSS = false;
    // the four font slots: a font file or an installed font family, empty = the
    // document's own font for that category
    Str fontSerif;
    Str fontSans;
    Str fontMono;
    Str fontCjk;
};
