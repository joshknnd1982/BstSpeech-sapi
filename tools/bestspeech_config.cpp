// BestSpeech configuration utility.
//
// A plain Win32 dialog, chosen deliberately: standard controls with labels in
// tab order are what screen readers handle best, with no framework in the way.
// Every change is written to HKCU\Software\BestSpeech the moment it is made,
// and the SAPI engine re-reads those values on every utterance, so a change
// here lands on the next thing a running screen reader says -- nothing has to
// be restarted, and closing the dialog loses nothing.
//
// The utility edits one voice at a time (the Language and Voice boxes pick
// which), previews it through SAPI itself with "Play sample", and holds the
// global settings -- rate, volume and the text-processing switches -- that
// apply to every voice.

#include <windows.h>
#include <commctrl.h>
#include <sapi.h>
// initguid ahead of oleacc instantiates the accessibility GUIDs (the
// annotation service and the Name property) right here, with no extra lib.
#include <initguid.h>
#include <oleacc.h>
#include <cstring>
#include <string>

#include "engines.hpp"
#include "user_settings.hpp"
#include "voice_attributes.hpp"
#include "voice_registry.hpp"
#include "bestspeech_config_res.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace Bestspeech;

namespace {

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

int g_engine = 0;   // index into engines[]
int g_voice = 0;    // index into voices[]

ISpVoice* g_preview = nullptr;

// A short sample in each engine's own script, so the preview sounds like real
// speech rather than transliterated English.
const wchar_t* sample_text(const char* engine_id)
{
    struct sample { const char* id; const wchar_t* text; };
    static const sample samples[] = {
        { "classic", L"Hello, this is a test of the speech engine." },
        { "eng", L"Hello, this is a test of the speech engine." },
        { "dut", L"Hallo, dit is een test van de spraakmachine." },
        { "fre", L"Bonjour, ceci est un test de la synthese vocale." },
        { "ger", L"Hallo, dies ist ein Test der Sprachausgabe." },
        // Non-Latin samples are \u escapes so the source stays plain ascii,
        // the same convention engines.hpp uses.
        { "gre", L"\u0393\u03b5\u03b9\u03b1 \u03c3\u03b1\u03c2, \u03b1\u03c5\u03c4\u03cc "
                 L"\u03b5\u03af\u03bd\u03b1\u03b9 \u03bc\u03b9\u03b1 \u03b4\u03bf\u03ba"
                 L"\u03b9\u03bc\u03ae \u03c4\u03b7\u03c2 \u03c6\u03c9\u03bd\u03ae\u03c2." },
        { "heb", L"\u05e9\u05dc\u05d5\u05dd, \u05d6\u05d5 \u05d1\u05d3\u05d9\u05e7\u05d4 "
                 L"\u05e9\u05dc \u05de\u05e0\u05d5\u05e2 \u05d4\u05d3\u05d9\u05d1\u05d5\u05e8." },
        { "ita", L"Ciao, questo e un test del sintetizzatore vocale." },
        { "jpn", L"\u3053\u3093\u306b\u3061\u306f\u3002\u3053\u308c\u306f\u304a\u3093"
                 L"\u305b\u3044\u3054\u3046\u305b\u3044\u306e\u3066\u3059\u3068\u3067"
                 L"\u3059\u3002" },
        { "pol", L"Dzie\u0144 dobry, to jest test syntezatora mowy." },
        { "por", L"Ol\u00e1, este \u00e9 um teste do sintetizador de voz." },
        { "rus", L"\u041f\u0440\u0438\u0432\u0435\u0442, \u044d\u0442\u043e \u0442\u0435"
                 L"\u0441\u0442 \u0441\u0438\u043d\u0442\u0435\u0437\u0430\u0442\u043e"
                 L"\u0440\u0430 \u0440\u0435\u0447\u0438." },
        { "spa", L"Hola, esto es una prueba del sintetizador de voz." },
    };
    for (const sample& s : samples) {
        if (strcmp(s.id, engine_id) == 0) {
            return s.text;
        }
    }
    return L"Hello, this is a test of the speech engine.";
}

// ---------------------------------------------------------------------------
// The checkbox <-> registry flag mapping. "Number processing" is stored
// inverted: the checkbox reads naturally ("read numbers as words", on by
// default) while the engine switch underneath is ~n2 "digits individually".
// ---------------------------------------------------------------------------

struct check_map { int id; const wchar_t* value_name; bool inverted; };

const check_map CHECKS[] = {
    { IDC_NUMPROC,    L"DigitsIndividually", true  },
    { IDC_ABBREV,     L"Abbreviations",      false },
    { IDC_PHRASEPRED, L"PhrasePrediction",   false },
    { IDC_SPELL,      L"SpellWords",         false },
    { IDC_PUNCT,      L"SpeakPunctuation",   false },
    { IDC_WHITESPACE, L"SpeakWhitespace",    false },
    { IDC_TIMES,      L"TimesOfDay",         false },
    { IDC_FULLNUM,    L"FullNumbers",        false },
    { IDC_CAPS,       L"CapsAsWords",        false },
    { IDC_MATH,       L"MathMode",           false },
    { IDC_CTRLCHARS,  L"ControlChars",       false },
};

bool flag_by_name(const settings::text_flags& flags, const wchar_t* name)
{
    for (const settings::flag_value& f : settings::FLAG_VALUES) {
        if (wcscmp(f.name, name) == 0) {
            return flags.*(f.member);
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Trackbars
// ---------------------------------------------------------------------------

struct slider_info { int id; int val_id; int min; int max; int line; int page; int tick; };

const slider_info SLIDERS[] = {
    { IDC_PITCH,      IDC_PITCH_VAL,      PITCH_MIN_HZ, PITCH_MAX_HZ, 5, 25, 50 },
    { IDC_INFLECTION, IDC_INFLECTION_VAL, -300, 100, 5, 25, 50 },
    { IDC_UNVOICED,   IDC_UNVOICED_VAL,   -70, 20, 1, 5, 10 },
    { IDC_RATE,       IDC_RATE_VAL,       settings::RATE_PERCENT_MIN, settings::RATE_PERCENT_MAX, 5, 25, 50 },
    { IDC_VOLUME,     IDC_VOLUME_VAL,     settings::VOLUME_DB_MIN, settings::VOLUME_DB_MAX, 1, 5, 5 },
};

void init_slider(HWND dlg, const slider_info& s)
{
    HWND tb = GetDlgItem(dlg, s.id);
    SendMessageW(tb, TBM_SETRANGEMIN, FALSE, s.min);
    SendMessageW(tb, TBM_SETRANGEMAX, FALSE, s.max);
    SendMessageW(tb, TBM_SETLINESIZE, 0, s.line);
    SendMessageW(tb, TBM_SETPAGESIZE, 0, s.page);
    SendMessageW(tb, TBM_SETTICFREQ, s.tick, 0);
}

void set_slider(HWND dlg, int id, int value)
{
    for (const slider_info& s : SLIDERS) {
        if (s.id == id) {
            SendMessageW(GetDlgItem(dlg, id), TBM_SETPOS, TRUE, value);
            wchar_t buf[16];
            _snwprintf_s(buf, _TRUNCATE, L"%d", value);
            SetDlgItemTextW(dlg, s.val_id, buf);
            return;
        }
    }
}

int slider_pos(HWND dlg, int id)
{
    return static_cast<int>(SendMessageW(GetDlgItem(dlg, id), TBM_GETPOS, 0, 0));
}

// ---------------------------------------------------------------------------
// Accessibility: pin an explicit accessible name onto every slider, combo box
// and value display. Screen readers otherwise guess a control's name from the
// nearest preceding static, and that guess is fragile -- it is exactly what
// left the sliders unlabelled when the value displays sat between them and
// their labels. With the annotation set, the name no longer depends on layout.
// ---------------------------------------------------------------------------

void set_accessible_names(HWND dlg)
{
    IAccPropServices* props = nullptr;
    if (FAILED(CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC_SERVER,
                                IID_IAccPropServices,
                                reinterpret_cast<void**>(&props))) || !props) {
        return;
    }
    const struct { int id; const wchar_t* name; } names[] = {
        { IDC_LANGUAGE,       L"Language" },
        { IDC_VOICE,          L"Voice" },
        { IDC_PITCH,          L"Pitch, 43 to 413 hertz" },
        { IDC_INFLECTION,     L"Inflection, -300 flat to 100 lively" },
        { IDC_UNVOICED,       L"Unvoiced volume, -70 to 20 dB" },
        { IDC_HEADSIZE,       L"Head size, 0 to 6, classic English only" },
        { IDC_EXCITATION,     L"Excitation" },
        { IDC_RATE,           L"Rate, 25 to 400 percent of normal" },
        { IDC_VOLUME,         L"Volume adjustment, -40 to 12 dB" },
        { IDC_PITCH_VAL,      L"Pitch value" },
        { IDC_INFLECTION_VAL, L"Inflection value" },
        { IDC_UNVOICED_VAL,   L"Unvoiced volume value" },
        { IDC_RATE_VAL,       L"Rate value" },
        { IDC_VOLUME_VAL,     L"Volume adjustment value" },
    };
    for (const auto& n : names) {
        if (HWND ctl = GetDlgItem(dlg, n.id)) {
            props->SetHwndPropStr(ctl, OBJID_CLIENT, CHILDID_SELF,
                                  PROPID_ACC_NAME, n.name);
        }
    }
    props->Release();
}

// ---------------------------------------------------------------------------
// Loading control state from the registry
// ---------------------------------------------------------------------------

void load_voice_controls(HWND dlg)
{
    const engine_info& eng = engines[g_engine];
    const voice_info v = settings::effective_voice(eng, g_voice);

    set_slider(dlg, IDC_PITCH, v.pitch);
    set_slider(dlg, IDC_INFLECTION, v.inflection);
    set_slider(dlg, IDC_UNVOICED, v.unvoiced);
    SendDlgItemMessageW(dlg, IDC_HEADSIZE, CB_SETCURSEL, v.headsize, 0);
    SendDlgItemMessageW(dlg, IDC_EXCITATION, CB_SETCURSEL, v.excitation - 1, 0);

    // The engines whose frontend ignores every inline command have nothing for
    // these controls to change, so they are disabled rather than lying.
    const BOOL enabled = (eng.commands != cmd_mode::none);
    for (const int id : { IDC_PITCH, IDC_INFLECTION, IDC_UNVOICED,
                          IDC_HEADSIZE, IDC_EXCITATION, IDC_RESET_VOICE }) {
        EnableWindow(GetDlgItem(dlg, id), enabled);
    }
}

void load_global_controls(HWND dlg)
{
    const settings::global_settings s = settings::load_global();
    set_slider(dlg, IDC_RATE, s.rate_percent);
    set_slider(dlg, IDC_VOLUME, s.volume_db);
    for (const check_map& c : CHECKS) {
        bool on = flag_by_name(s.flags, c.value_name);
        if (c.inverted) {
            on = !on;
        }
        CheckDlgButton(dlg, c.id, on ? BST_CHECKED : BST_UNCHECKED);
    }
}

void rebuild_voice_combo(HWND dlg)
{
    HWND combo = GetDlgItem(dlg, IDC_VOICE);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);

    const engine_info& eng = engines[g_engine];
    if (eng.voice_count <= 1) {
        SendMessageW(combo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(L"Default voice"));
        SendMessageW(combo, CB_SETCURSEL, 0, 0);
        EnableWindow(combo, FALSE);
        g_voice = 0;
        return;
    }

    EnableWindow(combo, TRUE);
    for (int i = 0; i < voice_count; ++i) {
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(voices[i].name));
    }
    if (g_voice < 0 || g_voice >= voice_count) {
        g_voice = 0;
    }
    SendMessageW(combo, CB_SETCURSEL, g_voice, 0);
}

// ---------------------------------------------------------------------------
// Preview through SAPI: the real installed engine, the real settings path.
// ---------------------------------------------------------------------------

void play_sample(HWND dlg)
{
    HRESULT hr = S_OK;
    if (!g_preview) {
        hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL,
                              __uuidof(ISpVoice), reinterpret_cast<void**>(&g_preview));
        if (FAILED(hr) || !g_preview) {
            MessageBoxW(dlg, L"Could not create a SAPI voice.",
                        L"BestSpeech Configuration", MB_OK | MB_ICONERROR);
            return;
        }
    }

    // Load the selected voice's token straight from its registry path, the way
    // token_probe does; on this 32-bit process the WOW64 view is automatic.
    const sapi::voice_attributes va(g_engine, g_voice);
    const std::wstring id = std::wstring(L"HKEY_LOCAL_MACHINE\\") +
                            sapi::voices_path + L"\\" + va.get_token_id();

    ISpObjectToken* token = nullptr;
    hr = CoCreateInstance(CLSID_SpObjectToken, nullptr, CLSCTX_ALL,
                          __uuidof(ISpObjectToken), reinterpret_cast<void**>(&token));
    if (SUCCEEDED(hr)) {
        hr = token->SetId(nullptr, id.c_str(), FALSE);
    }
    if (SUCCEEDED(hr)) {
        hr = g_preview->SetVoice(token);
    }
    if (token) {
        token->Release();
    }
    if (FAILED(hr)) {
        MessageBoxW(dlg,
                    L"This voice is not installed. Run the BestSpeech installer, "
                    L"then try again.",
                    L"BestSpeech Configuration", MB_OK | MB_ICONERROR);
        return;
    }

    g_preview->Speak(sample_text(engines[g_engine].id),
                     SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_IS_NOT_XML, nullptr);
}

// ---------------------------------------------------------------------------
// Dialog proc
// ---------------------------------------------------------------------------

// The "BestSpeech Custom Voice" SAPI token mirrors whatever this dialog showed
// when it closed: the selected language, and that voice's parameters as the
// user left them. Written on every close path, read back by the engine on the
// Custom Voice's next utterance.
void save_custom_voice()
{
    settings::write_custom_voice(
        engines[g_engine], settings::effective_voice(engines[g_engine], g_voice));
}

void on_init(HWND dlg)
{
    HWND langs = GetDlgItem(dlg, IDC_LANGUAGE);
    for (int i = 0; i < engine_count; ++i) {
        SendMessageW(langs, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(engines[i].display));
    }

    HWND head = GetDlgItem(dlg, IDC_HEADSIZE);
    for (int i = 0; i <= 6; ++i) {
        wchar_t buf[4];
        _snwprintf_s(buf, _TRUNCATE, L"%d", i);
        SendMessageW(head, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(buf));
    }

    HWND exc = GetDlgItem(dlg, IDC_EXCITATION);
    for (const wchar_t* label : { L"1 - breathy", L"2 - whispery", L"3 - normal",
                                  L"4", L"5", L"6" }) {
        SendMessageW(exc, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
    }

    for (const slider_info& s : SLIDERS) {
        init_slider(dlg, s);
    }

    // Reopen on the voice that was being edited last time.
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, settings::ROOT_KEY, 0, KEY_READ, &key)
        == ERROR_SUCCESS) {
        g_engine = settings::detail::get_int(key, L"ConfigLanguage", 0, 0, engine_count - 1);
        g_voice = settings::detail::get_int(key, L"ConfigVoice", 0, 0, voice_count - 1);
        RegCloseKey(key);
    }

    SendMessageW(langs, CB_SETCURSEL, g_engine, 0);
    rebuild_voice_combo(dlg);
    load_voice_controls(dlg);
    load_global_controls(dlg);
    set_accessible_names(dlg);
}

void on_slider_changed(HWND dlg, HWND slider)
{
    const int id = GetDlgCtrlID(slider);
    const int value = slider_pos(dlg, id);

    for (const slider_info& s : SLIDERS) {
        if (s.id == id) {
            wchar_t buf[16];
            _snwprintf_s(buf, _TRUNCATE, L"%d", value);
            SetDlgItemTextW(dlg, s.val_id, buf);
        }
    }

    const engine_info& eng = engines[g_engine];
    switch (id) {
    case IDC_PITCH:
        settings::write_voice_int(eng, g_voice, L"PitchHz", value);
        break;
    case IDC_INFLECTION:
        settings::write_voice_int(eng, g_voice, L"Inflection", value);
        break;
    case IDC_UNVOICED:
        settings::write_voice_int(eng, g_voice, L"UnvoicedDB", value);
        break;
    case IDC_RATE:
        settings::write_global_int(L"RatePercent", value);
        break;
    case IDC_VOLUME:
        settings::write_global_int(L"VolumeDB", value);
        break;
    }
}

INT_PTR CALLBACK dialog_proc(HWND dlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_INITDIALOG:
        on_init(dlg);
        return TRUE;

    case WM_HSCROLL:
        if (lparam) {
            on_slider_changed(dlg, reinterpret_cast<HWND>(lparam));
        }
        return TRUE;

    case WM_COMMAND: {
        const int id = LOWORD(wparam);
        const int code = HIWORD(wparam);

        if (id == IDC_LANGUAGE && code == CBN_SELCHANGE) {
            g_engine = static_cast<int>(
                SendDlgItemMessageW(dlg, IDC_LANGUAGE, CB_GETCURSEL, 0, 0));
            if (g_engine < 0 || g_engine >= engine_count) {
                g_engine = 0;
            }
            settings::write_global_int(L"ConfigLanguage", g_engine);
            rebuild_voice_combo(dlg);
            load_voice_controls(dlg);
            return TRUE;
        }
        if (id == IDC_VOICE && code == CBN_SELCHANGE) {
            g_voice = static_cast<int>(
                SendDlgItemMessageW(dlg, IDC_VOICE, CB_GETCURSEL, 0, 0));
            if (g_voice < 0 || g_voice >= voice_count) {
                g_voice = 0;
            }
            settings::write_global_int(L"ConfigVoice", g_voice);
            load_voice_controls(dlg);
            return TRUE;
        }
        if (id == IDC_HEADSIZE && code == CBN_SELCHANGE) {
            const int v = static_cast<int>(
                SendDlgItemMessageW(dlg, IDC_HEADSIZE, CB_GETCURSEL, 0, 0));
            if (v >= 0) {
                settings::write_voice_int(engines[g_engine], g_voice, L"HeadSize", v);
            }
            return TRUE;
        }
        if (id == IDC_EXCITATION && code == CBN_SELCHANGE) {
            const int v = static_cast<int>(
                SendDlgItemMessageW(dlg, IDC_EXCITATION, CB_GETCURSEL, 0, 0));
            if (v >= 0) {
                settings::write_voice_int(engines[g_engine], g_voice, L"Excitation", v + 1);
            }
            return TRUE;
        }

        for (const check_map& c : CHECKS) {
            if (id == c.id && code == BN_CLICKED) {
                bool on = IsDlgButtonChecked(dlg, c.id) == BST_CHECKED;
                if (c.inverted) {
                    on = !on;
                }
                settings::write_global_int(c.value_name, on ? 1 : 0);
                return TRUE;
            }
        }

        if (id == IDC_PREVIEW && code == BN_CLICKED) {
            play_sample(dlg);
            return TRUE;
        }
        if (id == IDC_RESET_VOICE && code == BN_CLICKED) {
            settings::delete_voice_overrides(engines[g_engine], g_voice);
            load_voice_controls(dlg);
            return TRUE;
        }
        if (id == IDC_RESET_ALL && code == BN_CLICKED) {
            if (MessageBoxW(dlg,
                            L"Reset every voice and every speech setting to the "
                            L"BestSpeech defaults?",
                            L"BestSpeech Configuration",
                            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
                settings::delete_all_settings();
                load_voice_controls(dlg);
                load_global_controls(dlg);
            }
            return TRUE;
        }
        if ((id == IDOK || id == IDCANCEL) &&
            (code == BN_CLICKED || code == 0)) {
            // Everything else was written as it was changed; closing snapshots
            // the Custom Voice and closes.
            save_custom_voice();
            EndDialog(dlg, 0);
            return TRUE;
        }
        break;
    }

    case WM_CLOSE:
        save_custom_voice();
        EndDialog(dlg, 0);
        return TRUE;
    }
    return FALSE;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_MAIN), nullptr, dialog_proc, 0);

    if (g_preview) {
        g_preview->Release();
        g_preview = nullptr;
    }
    CoUninitialize();
    return 0;
}
