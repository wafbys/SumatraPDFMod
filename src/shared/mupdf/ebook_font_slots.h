// Per-thread ebook font slots: the reader's four font files (serif, sans, mono,
// CJK) used instead of the fonts a reflowable document names.
// Implemented in mupdf_load_system_font.c, which mupdf calls for every family a
// stylesheet refers to. EngineMupdf enables the slots around laying out and
// drawing a reflowable document, so fixed-page documents (PDF) keep their own
// fonts.
#ifndef EBOOK_FONT_SLOTS_H
#define EBOOK_FONT_SLOTS_H

// Each argument is the path of a font file (or the family name of an installed
// font), or NULL/"" for "slot not set". `straight_quotes` asks for "dumb"
// quotes in Latin text (CJK keeps its own). Applies to this thread until
// EbookFontSlotsEnd().
void EbookFontSlotsBegin(const char* serif, const char* sans, const char* mono, const char* cjk, int straight_quotes);
void EbookFontSlotsEnd(void);

// Whether the thread currently laying out wants straight quotes in Latin text;
// read by the patched html parser (ext/patches/0062-*).
int EbookTextWantsStraightQuotes(void);

// The font the slots answer with for a stylesheet family, or NULL when no slot
// covers it (or no slots are set at all). Called by the patched
// fz_load_html_font() ahead of the document's own @font-face entries, and by
// the system-font hook for families a document names.
// NB: fz_font must already be declared where this header is included (mupdf's
// fitz.h is included first in every caller).
fz_font* EbookFontSlotFont(fz_context* ctx, const char* family, int is_bold, int is_italic);

#endif
