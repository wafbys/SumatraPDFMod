/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "base/Base.h"
#if OS_WIN
#include "base/Win.h"
#elif OS_LINUX
#include <fontconfig/fontconfig.h>
#elif defined(__APPLE__)
#include "base/MacTypesHide.h"
#include <CoreText/CoreText.h>
#include "base/MacTypesShow.h"
#endif

#include "SystemFonts.h"

#if OS_WIN
static int CALLBACK EnumFontFamilyCb(const LOGFONTW* lf, const TEXTMETRICW*, DWORD, LPARAM lp) {
    auto* names = (StrVec*)lp;
    const WCHAR* face = lf->lfFaceName;
    // "@Name" is the vertical-writing variant of a CJK font, not a font to pick
    if (!face[0] || face[0] == L'@') {
        return 1;
    }
    TempStr name = ToUtf8Temp(face);
    if (names->FindI(name) < 0) {
        names->Append(name);
    }
    return 1;
}
#endif

// GDI returns a family's variant faces ("Foo OsF", "Foo SC") as the synthesized
// family "Foo " next to the real "Foo"; a picker shows the raw name, so the
// padded one reads as a duplicate. Merge such look-alikes, keeping the name
// that spells out the whole family.
static void MergeLookAlikeNames(StrVec& names) {
    StrVec res;
    for (int pass = 0; pass < 2; pass++) {
        for (Str name : names) {
            Str trimmed = name;
            str::TrimWsBoth(trimmed);
            bool padded = len(trimmed) < len(name);
            if (len(trimmed) == 0 || padded != (pass == 1)) {
                continue;
            }
            if (res.FindI(trimmed) < 0) {
                res.Append(trimmed);
            }
        }
    }
    names = res;
}

void GetInstalledFontNames(StrVec& names) {
#if OS_WIN
    HDC hdc = GetDC(nullptr);
    LOGFONTW lf{};
    lf.lfCharSet = DEFAULT_CHARSET;
    EnumFontFamiliesExW(hdc, &lf, EnumFontFamilyCb, (LPARAM)&names, 0);
    ReleaseDC(nullptr, hdc);
#elif OS_LINUX
    if (!FcInit()) {
        return;
    }
    FcPattern* pattern = FcPatternCreate();
    FcObjectSet* objects = FcObjectSetBuild(FC_FAMILY, NULL);
    FcFontSet* fonts = pattern && objects ? FcFontList(NULL, pattern, objects) : NULL;
    if (fonts) {
        for (int i = 0; i < fonts->nfont; i++) {
            FcChar8* family = NULL;
            if (FcPatternGetString(fonts->fonts[i], FC_FAMILY, 0, &family) == FcResultMatch) {
                Str name((char*)family);
                if (len(name) > 0 && names.FindI(name) < 0) {
                    names.Append(name);
                }
            }
        }
    }
    if (fonts) {
        FcFontSetDestroy(fonts);
    }
    if (objects) {
        FcObjectSetDestroy(objects);
    }
    if (pattern) {
        FcPatternDestroy(pattern);
    }
#elif defined(__APPLE__)
    CFArrayRef families = CTFontManagerCopyAvailableFontFamilyNames();
    if (!families) {
        return;
    }
    CFIndex count = CFArrayGetCount(families);
    for (CFIndex i = 0; i < count; i++) {
        auto name = (CFStringRef)CFArrayGetValueAtIndex(families, i);
        CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(name), kCFStringEncodingUTF8) + 1;
        if (size <= 1 || size > INT_MAX) {
            continue;
        }
        char* buf = AllocArray<char>((int)size);
        if (!buf) {
            continue;
        }
        if (CFStringGetCString(name, buf, size, kCFStringEncodingUTF8)) {
            Str family(buf);
            if (len(family) > 0 && names.FindI(family) < 0) {
                names.Append(family);
            }
        }
        free(buf);
    }
    CFRelease(families);
#else
    (void)names;
#endif
    MergeLookAlikeNames(names);
    SortNoCase(&names);
}
