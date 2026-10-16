/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "base/Base.h"
#if OS_WIN
#include "base/Win.h"
#include <dwrite.h>
#pragma comment(lib, "dwrite.lib")
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

// GDI reports a family's variant faces ("Foo OsF", "Foo SC") as one synthesized
// family named after their common prefix plus a space ("Foo "); DirectWrite
// knows the real family names, so ask it for them (en-us spelling)
static void CollectDWriteFamilyNames(StrVec& out) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool uninit = SUCCEEDED(hr);
    IDWriteFactory* factory = nullptr;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&factory);
    if (FAILED(hr)) {
        if (uninit) {
            CoUninitialize();
        }
        return;
    }
    IDWriteFontCollection* fonts = nullptr;
    hr = factory->GetSystemFontCollection(&fonts, FALSE);
    if (SUCCEEDED(hr)) {
        UINT32 n = fonts->GetFontFamilyCount();
        for (UINT32 i = 0; i < n; i++) {
            IDWriteFontFamily* family = nullptr;
            if (FAILED(fonts->GetFontFamily(i, &family))) {
                continue;
            }
            IDWriteLocalizedStrings* names = nullptr;
            if (SUCCEEDED(family->GetFamilyNames(&names))) {
                UINT32 idx = 0;
                BOOL found = FALSE;
                names->FindLocaleName(L"en-us", &idx, &found);
                if (!found) {
                    idx = 0;
                }
                UINT32 cch = 0;
                if (SUCCEEDED(names->GetStringLength(idx, &cch)) && cch > 0) {
                    auto* buf = AllocArray<WCHAR>((int)cch + 1);
                    if (buf && SUCCEEDED(names->GetString(idx, buf, cch + 1))) {
                        Str name = ToUtf8Temp(WStr(buf, (int)cch));
                        if (len(name) > 0 && out.FindI(name) < 0) {
                            out.Append(name);
                        }
                    }
                    free(buf);
                }
                names->Release();
            }
            family->Release();
        }
        fonts->Release();
    }
    factory->Release();
    if (uninit) {
        CoUninitialize();
    }
}

static bool HasSynthesizedFontName(const StrVec& names) {
    for (Str name : names) {
        Str trimmed = name;
        str::TrimWsBoth(trimmed);
        if (len(trimmed) != len(name)) {
            return true;
        }
    }
    return false;
}
#endif

// Replace each synthesized name with the families it stands for (the ones it is
// a prefix of), so the variant faces stay pickable under their real names;
// when nothing matches, keep the name as GDI spelled it
void ExpandSynthesizedFontNames(StrVec& names, const StrVec& families) {
    StrVec res;
    for (Str name : names) {
        Str trimmed = name;
        str::TrimWsBoth(trimmed);
        if (len(trimmed) == len(name)) {
            if (res.FindI(trimmed) < 0) {
                res.Append(trimmed);
            }
            continue;
        }
        bool found = false;
        for (Str family : families) {
            if (!str::StartsWithI(family, name)) {
                continue;
            }
            found = true;
            if (res.FindI(family) < 0) {
                res.Append(family);
            }
        }
        if (!found && res.FindI(name) < 0) {
            res.Append(name);
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
    if (HasSynthesizedFontName(names)) {
        StrVec families;
        CollectDWriteFamilyNames(families);
        ExpandSynthesizedFontNames(names, families);
    }
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
    SortNoCase(&names);
}
