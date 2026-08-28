#pragma once

#include <windows.h>
#include <string>

#include "engines.hpp"

namespace Bestspeech {
namespace sapi {

// Which languages and character voices this machine was actually installed with.
//
// The installer lets the two be chosen independently -- any set of languages crossed
// with any set of character voices -- so the tokens to publish are no longer derivable
// from engines.hpp alone. The choice is written to voices.ini beside the engine and read
// back here: by DllRegisterServer when it writes the tokens, by the configuration
// utility so it only offers what is installed, and by the diagnostics tool so a
// deliberately absent voice is not reported as a fault.
//
//   [Selection]
//   Languages=classic,eng,spa
//   Voices=Fred,Sara,Kit
//   CustomVoice=1
//
// A missing file means everything is installed, which is what a developer build
// registered by hand with regsvr32 gets, and what every release before this one did.
inline constexpr const wchar_t* selection_file_name = L"voices.ini";

namespace detail {

// Ascii only, because every id and voice name in engines.hpp is ascii and the file is
// written by the installer from those same strings.
[[nodiscard]] inline char lower_ascii(char c) noexcept
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

[[nodiscard]] inline bool equals_ascii_ci(const std::string& a, const char* b) noexcept
{
    size_t i = 0;
    for (; i < a.size() && b[i]; ++i) {
        if (lower_ascii(a[i]) != lower_ascii(b[i])) {
            return false;
        }
    }
    return i == a.size() && !b[i];
}

[[nodiscard]] inline bool equals_ascii_ci(const std::string& a, const wchar_t* b) noexcept
{
    size_t i = 0;
    for (; i < a.size() && b[i]; ++i) {
        if (b[i] > 127) {
            return false;
        }
        if (lower_ascii(a[i]) != lower_ascii(static_cast<char>(b[i]))) {
            return false;
        }
    }
    return i == a.size() && !b[i];
}

[[nodiscard]] inline std::string trim(const std::string& s)
{
    size_t b = 0;
    size_t e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) {
        ++b;
    }
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) {
        --e;
    }
    return s.substr(b, e - b);
}

// The whole file, as bytes. It is a few hundred of them, so a hard cap costs nothing and
// keeps a corrupted or wrong file from being read into memory wholesale.
[[nodiscard]] inline bool read_file(const std::wstring& path, std::string& out)
{
    const HANDLE h = CreateFileW(path.c_str(), GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }
    out.clear();
    char buf[1024];
    DWORD got = 0;
    while (out.size() < 64 * 1024 && ReadFile(h, buf, sizeof(buf), &got, nullptr) && got > 0) {
        out.append(buf, got);
    }
    CloseHandle(h);
    return true;
}

// The value of Key= on its own line. Section headers are ignored: the file has exactly
// one section, and tolerating a stray one is friendlier than rejecting the file.
[[nodiscard]] inline bool value_for(const std::string& text, const char* key, std::string& out)
{
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t end = text.find('\n', pos);
        if (end == std::string::npos) {
            end = text.size();
        }
        const std::string line = trim(text.substr(pos, end - pos));
        pos = end + 1;

        if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '[') {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        if (equals_ascii_ci(trim(line.substr(0, eq)), key)) {
            out = trim(line.substr(eq + 1));
            return true;
        }
    }
    return false;
}

// Calls hit(item) for each comma separated item, skipping empties so a trailing comma or
// a doubled separator is harmless.
template <typename Fn>
void for_each_item(const std::string& list, Fn hit)
{
    size_t pos = 0;
    while (pos <= list.size()) {
        size_t end = list.find(',', pos);
        if (end == std::string::npos) {
            end = list.size();
        }
        const std::string item = trim(list.substr(pos, end - pos));
        if (!item.empty()) {
            hit(item);
        }
        pos = end + 1;
    }
}

// The directory holding this very module, whether that is the engine dll or one of the
// tools -- taken from an address inside it, so nothing has to be plumbed through.
[[nodiscard]] inline std::wstring own_directory()
{
    HMODULE mod = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&own_directory), &mod)) {
        mod = nullptr;
    }
    wchar_t buf[MAX_PATH];
    const DWORD n = GetModuleFileNameW(mod, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return std::wstring();
    }
    std::wstring path(buf, n);
    const size_t slash = path.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? std::wstring() : path.substr(0, slash);
}

[[nodiscard]] inline std::wstring parent_directory(const std::wstring& dir)
{
    const size_t slash = dir.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? std::wstring() : dir.substr(0, slash);
}

}

class install_selection
{
public:
    // Everything installed: the state a hand-registered developer build is in.
    install_selection() noexcept
    {
        for (bool& b : engines_) {
            b = true;
        }
        for (bool& b : voices_) {
            b = true;
        }
    }

    [[nodiscard]] bool has_engine(int e) const noexcept
    {
        return (e >= 0 && e < engine_count) ? engines_[e] : false;
    }

    [[nodiscard]] bool has_voice(int v) const noexcept
    {
        return (v >= 0 && v < voice_count) ? voices_[v] : false;
    }

    [[nodiscard]] bool has_custom() const noexcept { return custom_; }

    // A single-voice engine publishes its one token whichever character voices were
    // chosen, because its frontend ignores every voice command: the characters are not
    // what is being selected there.
    [[nodiscard]] bool has_token(int engine, int voice) const noexcept
    {
        if (!has_engine(engine)) {
            return false;
        }
        return (engines[engine].voice_count <= 1) || has_voice(voice);
    }

    [[nodiscard]] bool has_token(int token_index) const noexcept
    {
        int e = 0;
        int v = 0;
        return token_at(token_index, e, v) && has_token(e, v);
    }

    // The first installed engine, so a caller holding a stale index can fall back to one
    // that actually exists. -1 when nothing at all is installed.
    [[nodiscard]] int first_engine() const noexcept
    {
        for (int e = 0; e < engine_count; ++e) {
            if (engines_[e]) {
                return e;
            }
        }
        return -1;
    }

    [[nodiscard]] int first_voice() const noexcept
    {
        for (int v = 0; v < voice_count; ++v) {
            if (voices_[v]) {
                return v;
            }
        }
        return -1;
    }

    [[nodiscard]] static install_selection load(const std::wstring& directory)
    {
        install_selection sel;
        if (directory.empty()) {
            return sel;
        }

        std::string text;
        if (!detail::read_file(directory + L"\\" + selection_file_name, text)) {
            return sel;
        }

        std::string list;
        if (detail::value_for(text, "Languages", list)) {
            for (bool& b : sel.engines_) {
                b = false;
            }
            detail::for_each_item(list, [&sel](const std::string& item) {
                for (int e = 0; e < engine_count; ++e) {
                    if (detail::equals_ascii_ci(item, engines[e].id)) {
                        sel.engines_[e] = true;
                    }
                }
            });
            // A list naming nothing this build knows about is a damaged file, not a
            // request for an engine with no languages at all. Everything is safer than
            // nothing: it leaves the configuration utility usable rather than empty.
            if (sel.first_engine() < 0) {
                for (bool& b : sel.engines_) {
                    b = true;
                }
            }
        }

        if (detail::value_for(text, "Voices", list)) {
            for (bool& b : sel.voices_) {
                b = false;
            }
            detail::for_each_item(list, [&sel](const std::string& item) {
                for (int v = 0; v < voice_count; ++v) {
                    if (detail::equals_ascii_ci(item, voices[v].name)) {
                        sel.voices_[v] = true;
                    }
                }
            });
            if (sel.first_voice() < 0) {
                for (bool& b : sel.voices_) {
                    b = true;
                }
            }
        }

        std::string flag;
        if (detail::value_for(text, "CustomVoice", flag)) {
            sel.custom_ = !(flag == "0" || detail::equals_ascii_ci(flag, "no") ||
                            detail::equals_ascii_ci(flag, "false"));
        }

        return sel;
    }

    // Loaded once per process from the directory this module sits in. The 64-bit engine
    // and diagnostics tool live in an x64 subdirectory of the install directory, so the
    // parent is tried as well before falling back to assuming a full install.
    [[nodiscard]] static const install_selection& current()
    {
        static const install_selection sel = []() {
            const std::wstring dir = detail::own_directory();
            std::string probe;
            if (!dir.empty() && detail::read_file(dir + L"\\" + selection_file_name, probe)) {
                return load(dir);
            }
            return load(detail::parent_directory(dir));
        }();
        return sel;
    }

private:
    bool engines_[engine_count];
    bool voices_[voice_count];
    bool custom_ = true;
};

}
}
