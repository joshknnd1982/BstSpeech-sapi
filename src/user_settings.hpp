#pragma once

#include <windows.h>
#include <algorithm>
#include <string>

#include "engines.hpp"
#include "install_selection.hpp"

// User-adjustable speech settings, shared between the SAPI engine and the
// BestSpeech configuration utility.
//
// Everything lives under HKCU\Software\BestSpeech, the key the engine already
// reads for Logging and WorkerEngines. HKCU\Software is not WOW64-redirected,
// so the 32-bit utility, the 32-bit engine and the 64-bit engine all see the
// same values without any view games. The engine re-reads the values on every
// utterance -- a handful of registry reads, microseconds against synthesis --
// which is what makes a change in the utility land on the very next thing a
// running screen reader says, with nothing restarted.
//
// A value that is absent means "use the built-in default", so an installation
// where the utility has never been run behaves byte-for-byte like one built
// before it existed.
namespace Bestspeech {
namespace settings {

inline constexpr const wchar_t* ROOT_KEY   = L"Software\\BestSpeech";
inline constexpr const wchar_t* VOICES_KEY = L"Software\\BestSpeech\\Voices";
inline constexpr const wchar_t* CUSTOM_KEY = L"Software\\BestSpeech\\CustomVoice";

// ---------------------------------------------------------------------------
// The ~n text-parser switches (see bestspeechparams.txt), plus ~~2.
// Which engines obey which switch is measured, not assumed: parser_cmds in
// engines.hpp holds the per-engine result.
// ---------------------------------------------------------------------------

// Every switch defaults to off, matching the engines themselves: the dlls boot
// with all ten ~n modes off (measured -- the Keynote manual's "default on" for
// ~n9 and ~n10 does not hold for these builds), so all-off here means the
// engine is driven exactly as it was before these settings existed.
struct text_flags
{
    bool spell_words         = false;  // ~n1: letter names instead of words
    bool digits_individually = false;  // ~n2: each digit on its own
    bool speak_punctuation   = false;  // ~n3: commas, periods and the rest
    bool speak_whitespace    = false;  // ~n4: space, return, tab
    bool math_mode           = false;  // ~n5: mathematical texts
    bool full_numbers        = false;  // ~n6: no digit grouping
    bool caps_as_words       = false;  // ~n7: uppercase groups read as words
    bool control_chars       = false;  // ~n8: control characters
    bool times_of_day        = false;  // ~n9: 8:00 = eight o'clock
    bool abbreviations       = false;  // ~n10: abbreviation expansion
    bool phrase_prediction   = false;  // ~~2: speak without waiting for punctuation

    [[nodiscard]] bool is_default() const noexcept
    {
        return !spell_words && !digits_individually && !speak_punctuation &&
               !speak_whitespace && !math_mode && !full_numbers &&
               !caps_as_words && !control_chars && !times_of_day &&
               !abbreviations && !phrase_prediction;
    }
};

// One registry value per flag, so absent values keep their defaults and a
// future flag costs nothing to add.
struct flag_value { const wchar_t* name; bool text_flags::*member; bool default_on; };

inline constexpr flag_value FLAG_VALUES[] = {
    { L"SpellWords",         &text_flags::spell_words,         false },
    { L"DigitsIndividually", &text_flags::digits_individually, false },
    { L"SpeakPunctuation",   &text_flags::speak_punctuation,   false },
    { L"SpeakWhitespace",    &text_flags::speak_whitespace,    false },
    { L"MathMode",           &text_flags::math_mode,           false },
    { L"FullNumbers",        &text_flags::full_numbers,        false },
    { L"CapsAsWords",        &text_flags::caps_as_words,       false },
    { L"ControlChars",       &text_flags::control_chars,       false },
    { L"TimesOfDay",         &text_flags::times_of_day,        false },
    { L"Abbreviations",      &text_flags::abbreviations,       false },
    { L"PhrasePrediction",   &text_flags::phrase_prediction,   false },
};

// ---------------------------------------------------------------------------
// Global settings: apply to every voice, on top of whatever the SAPI client
// asks for. Rate multiplies the client's rate; volume adds to its gain.
// ---------------------------------------------------------------------------

inline constexpr int RATE_PERCENT_MIN = 25;
inline constexpr int RATE_PERCENT_MAX = 400;
inline constexpr int VOLUME_DB_MIN    = -40;
inline constexpr int VOLUME_DB_MAX    = 12;

struct global_settings
{
    int rate_percent = 100;
    int volume_db    = 0;
    text_flags flags;
};

namespace detail {

// REG_DWORD read as a signed int, clamped; absent or mistyped values fall back.
[[nodiscard]] inline int get_int(HKEY key, const wchar_t* name, int def, int lo, int hi)
{
    DWORD type = 0;
    DWORD raw = 0;
    DWORD size = sizeof(raw);
    if (RegQueryValueExW(key, name, nullptr, &type,
                         reinterpret_cast<LPBYTE>(&raw), &size) == ERROR_SUCCESS &&
        type == REG_DWORD) {
        return std::clamp(static_cast<int>(raw), lo, hi);
    }
    return def;
}

[[nodiscard]] inline bool get_bool(HKEY key, const wchar_t* name, bool def)
{
    return get_int(key, name, def ? 1 : 0, 0, 1) != 0;
}

}  // namespace detail

[[nodiscard]] inline global_settings load_global()
{
    global_settings s;
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, ROOT_KEY, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return s;
    }
    s.rate_percent = detail::get_int(key, L"RatePercent", 100, RATE_PERCENT_MIN, RATE_PERCENT_MAX);
    s.volume_db = detail::get_int(key, L"VolumeDB", 0, VOLUME_DB_MIN, VOLUME_DB_MAX);
    for (const flag_value& f : FLAG_VALUES) {
        s.flags.*(f.member) = detail::get_bool(key, f.name, f.default_on);
    }
    RegCloseKey(key);
    return s;
}

// ---------------------------------------------------------------------------
// Per-voice overrides: the character voices' pitch, inflection, head size,
// excitation and unvoiced gain, keyed the same way as the voice tokens so a
// user's tweaks survive an upgrade. Absent values keep the built-in character.
// ---------------------------------------------------------------------------

// "eng_Fred", or just "gre" for the single-voice engines.
[[nodiscard]] inline std::wstring voice_key_name(const engine_info& eng, int voice_index)
{
    std::wstring name;
    for (const char* p = eng.id; *p; ++p) {
        name += static_cast<wchar_t>(*p);
    }
    if (eng.voice_count > 1 && voice_index >= 0 && voice_index < voice_count) {
        name += L'_';
        name += voices[voice_index].name;
    }
    return name;
}

// The voice as it should actually sound: the built-in character with any user
// overrides applied on top.
[[nodiscard]] inline voice_info effective_voice(const engine_info& eng, int voice_index)
{
    if (voice_index < 0 || voice_index >= voice_count) {
        voice_index = 0;
    }
    voice_info v = voices[voice_index];

    const std::wstring path = std::wstring(VOICES_KEY) + L"\\" + voice_key_name(eng, voice_index);
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return v;
    }
    v.pitch      = detail::get_int(key, L"PitchHz",    v.pitch,      PITCH_MIN_HZ, PITCH_MAX_HZ);
    v.inflection = detail::get_int(key, L"Inflection", v.inflection, -300, 100);
    v.headsize   = detail::get_int(key, L"HeadSize",   v.headsize,   0, 6);
    v.excitation = detail::get_int(key, L"Excitation", v.excitation, 1, 6);
    v.unvoiced   = detail::get_int(key, L"UnvoicedDB", v.unvoiced,   -70, 20);
    RegCloseKey(key);
    return v;
}

// ---------------------------------------------------------------------------
// The Custom Voice: one published SAPI voice ("BestSpeech Custom Voice") whose
// engine and parameters are whatever the configuration utility last saved when
// it closed. Its token carries BstEngine "custom"; the definition itself lives
// here, in HKCU, so the utility can update it without elevation and the engine
// picks the change up on the next utterance.
// ---------------------------------------------------------------------------

struct custom_voice
{
    int engine_index = 0;        // the classic English engine until configured
    voice_info voice = voices[0];  // Fred's parameters until configured
};

// The engine the Custom Voice should speak with, given what the installer put on this
// machine. A snapshot naming a language that was left out -- or that a later run of the
// installer removed -- would send the engine after a dll that is not there, so it falls
// back to the first language that is installed.
[[nodiscard]] inline int installed_engine_or_first(int wanted)
{
    const sapi::install_selection& sel = sapi::install_selection::current();
    if (sel.has_engine(wanted)) {
        return wanted;
    }
    const int first = sel.first_engine();
    return (first >= 0) ? first : 0;
}

[[nodiscard]] inline custom_voice load_custom_voice()
{
    custom_voice c;
    c.voice.name = L"Custom";
    c.engine_index = installed_engine_or_first(c.engine_index);

    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, CUSTOM_KEY, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return c;
    }
    wchar_t engine_id[32] = {};
    DWORD type = 0;
    DWORD size = sizeof(engine_id) - sizeof(wchar_t);
    if (RegQueryValueExW(key, L"Engine", nullptr, &type,
                         reinterpret_cast<LPBYTE>(engine_id), &size) == ERROR_SUCCESS &&
        type == REG_SZ) {
        char narrow[32] = {};
        for (int i = 0; engine_id[i] && i < 31; ++i) {
            narrow[i] = static_cast<char>(engine_id[i]);
        }
        const int e = engine_by_id(narrow);
        if (e >= 0) {
            c.engine_index = installed_engine_or_first(e);
        }
    }
    c.voice.pitch      = detail::get_int(key, L"PitchHz",    c.voice.pitch,      PITCH_MIN_HZ, PITCH_MAX_HZ);
    c.voice.inflection = detail::get_int(key, L"Inflection", c.voice.inflection, -300, 100);
    c.voice.headsize   = detail::get_int(key, L"HeadSize",   c.voice.headsize,   0, 6);
    c.voice.excitation = detail::get_int(key, L"Excitation", c.voice.excitation, 1, 6);
    c.voice.unvoiced   = detail::get_int(key, L"UnvoicedDB", c.voice.unvoiced,   -70, 20);
    RegCloseKey(key);
    return c;
}

inline bool write_custom_voice(const engine_info& eng, const voice_info& v)
{
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, CUSTOM_KEY, 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    std::wstring id;
    for (const char* p = eng.id; *p; ++p) {
        id += static_cast<wchar_t>(*p);
    }
    bool ok = RegSetValueExW(key, L"Engine", 0, REG_SZ,
                             reinterpret_cast<const BYTE*>(id.c_str()),
                             static_cast<DWORD>((id.size() + 1) * sizeof(wchar_t)))
              == ERROR_SUCCESS;
    const struct { const wchar_t* name; int value; } ints[] = {
        { L"PitchHz", v.pitch }, { L"Inflection", v.inflection },
        { L"HeadSize", v.headsize }, { L"Excitation", v.excitation },
        { L"UnvoicedDB", v.unvoiced },
    };
    for (const auto& item : ints) {
        const DWORD raw = static_cast<DWORD>(item.value);
        ok = RegSetValueExW(key, item.name, 0, REG_DWORD,
                            reinterpret_cast<const BYTE*>(&raw), sizeof(raw))
                 == ERROR_SUCCESS && ok;
    }
    RegCloseKey(key);
    return ok;
}

// ---------------------------------------------------------------------------
// Write side, used by the configuration utility. HKCU needs no elevation.
// ---------------------------------------------------------------------------

inline bool write_int(const std::wstring& subkey, const wchar_t* name, int value)
{
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subkey.c_str(), 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    const DWORD raw = static_cast<DWORD>(value);
    const bool ok = RegSetValueExW(key, name, 0, REG_DWORD,
                                   reinterpret_cast<const BYTE*>(&raw),
                                   sizeof(raw)) == ERROR_SUCCESS;
    RegCloseKey(key);
    return ok;
}

inline bool write_global_int(const wchar_t* name, int value)
{
    return write_int(ROOT_KEY, name, value);
}

inline bool write_voice_int(const engine_info& eng, int voice_index,
                            const wchar_t* name, int value)
{
    return write_int(std::wstring(VOICES_KEY) + L"\\" + voice_key_name(eng, voice_index),
                     name, value);
}

inline void delete_voice_overrides(const engine_info& eng, int voice_index)
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, VOICES_KEY, 0,
                      KEY_SET_VALUE | DELETE, &key) == ERROR_SUCCESS) {
        RegDeleteKeyW(key, voice_key_name(eng, voice_index).c_str());
        RegCloseKey(key);
    }
}

// Removes every value the utility manages, and only those: Logging and
// WorkerEngines are diagnostic settings, not speech settings, and stay put.
inline void delete_all_settings()
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, ROOT_KEY, 0,
                      KEY_SET_VALUE | DELETE, &key) == ERROR_SUCCESS) {
        RegDeleteValueW(key, L"RatePercent");
        RegDeleteValueW(key, L"VolumeDB");
        for (const flag_value& f : FLAG_VALUES) {
            RegDeleteValueW(key, f.name);
        }
        RegCloseKey(key);
    }
    for (int e = 0; e < engine_count; ++e) {
        for (int v = 0; v < engines[e].voice_count; ++v) {
            delete_voice_overrides(engines[e], v);
        }
    }
    HKEY root = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, ROOT_KEY, 0,
                      KEY_SET_VALUE | DELETE, &root) == ERROR_SUCCESS) {
        RegDeleteKeyW(root, L"Voices");  // fails harmlessly if subkeys remain
        RegDeleteKeyW(root, L"CustomVoice");
        RegCloseKey(root);
    }
}

}
}
