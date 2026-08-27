#pragma once

#include <windows.h>
#include <sapi.h>
#include <string>

#include "registry.hpp"
#include "voice_attributes.hpp"

namespace Bestspeech {
namespace sapi {

// Voice token registration, factored out of DllRegisterServer so the verification tools
// can drive the very same code against HKEY_CURRENT_USER. Registration used to be
// reachable only through regsvr32 into HKLM, which needs elevation, so nothing tested it
// -- and a bug that only showed up once SAPI read a token back from the registry could
// pass every suite.
inline constexpr const wchar_t* voices_path =
    L"Software\\Microsoft\\Speech\\Voices\\Tokens";

// The tokens that are not a (language, character) pair: the Custom Voice,
// whose parameters are whatever the configuration utility last saved into
// HKCU\Software\BestSpeech\CustomVoice. The plain token follows the snapshot's
// engine too (BstEngine "custom"); it is joined by one token per language that
// pins its engine and takes only the parameters (BstCustom "1"), so the
// sculpted voice can be chosen for any language under a truthful Language
// attribute.
inline constexpr const wchar_t* custom_token_id = L"BestSpeech_custom";
inline constexpr const wchar_t* custom_token_name = L"BestSpeech Custom Voice";

[[nodiscard]] inline std::wstring custom_token_id_for(const engine_info& eng)
{
    std::wstring id = custom_token_id;
    id += L'_';
    for (const char* p = eng.id; *p; ++p) {
        id += static_cast<wchar_t>(*p);
    }
    return id;
}

// Every voice is written as its own static token rather than being produced by a
// dynamic token enumerator. Static tokens are what every SAPI5 client reads, including
// Windows Narrator, so this is the form with the widest compatibility.
inline void write_voice_tokens(HKEY root, const std::wstring& clsid_str)
{
    using namespace Bestspeech::registry;

    key tokens(root, voices_path, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, true);

    for (int i = 0; i < total_token_count(); ++i) {
        const voice_attributes v(i);
        const std::wstring name = v.get_name();

        key token(tokens, v.get_token_id(), KEY_CREATE_SUB_KEY | KEY_SET_VALUE, true);
        token.set(name);
        token.set(L"CLSID", clsid_str);
        // SAPI looks a display name up under a value named for the LCID it is asking
        // about, falling back to the key's default value, which is set just above.
        token.set(L"409", name);

        key attrs(token, L"Attributes", KEY_SET_VALUE, true);
        attrs.set(L"Name", name);
        attrs.set(L"Gender", v.get_gender());
        attrs.set(L"Age", v.get_age());
        attrs.set(L"Language", v.get_language());
        attrs.set(L"Vendor", v.get_vendor());
        // Read back by SetObjectToken, so the exact engine and voice are recovered
        // without parsing a display name apart.
        attrs.set(L"BstEngine", v.get_engine_id());
        attrs.set(L"BstVoice", v.get_voice_id());
    }

    // The Custom Voice. Registered under English since a token needs a language,
    // but its actual engine -- and so its actual language -- is read from HKCU
    // each time it speaks. No Gender attribute: the user's parameters decide.
    key token(tokens, custom_token_id, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, true);
    token.set(custom_token_name);
    token.set(L"CLSID", clsid_str);
    token.set(L"409", custom_token_name);

    key attrs(token, L"Attributes", KEY_SET_VALUE, true);
    attrs.set(L"Name", custom_token_name);
    attrs.set(L"Age", L"Adult");
    attrs.set(L"Language", L"409");
    attrs.set(L"Vendor", L"BestSpeech");
    attrs.set(L"BstEngine", L"custom");
    attrs.set(L"BstVoice", L"0");

    // And one custom token per language, each pinned to its engine with a real
    // Language attribute, all sharing the snapshot's parameters. The three
    // engines whose frontend ignores every voice command (Greek, Japanese,
    // Polish) are left out: a custom token there could only duplicate the
    // single voice they already publish.
    for (int e = 0; e < engine_count; ++e) {
        if (engines[e].commands == cmd_mode::none) {
            continue;
        }
        const std::wstring name =
            std::wstring(custom_token_name) + L" - " + engines[e].display;

        key ctoken(tokens, custom_token_id_for(engines[e]),
                   KEY_CREATE_SUB_KEY | KEY_SET_VALUE, true);
        ctoken.set(name);
        ctoken.set(L"CLSID", clsid_str);
        ctoken.set(L"409", name);

        key cattrs(ctoken, L"Attributes", KEY_SET_VALUE, true);
        cattrs.set(L"Name", name);
        cattrs.set(L"Age", L"Adult");
        cattrs.set(L"Language", engines[e].lcid);
        cattrs.set(L"Vendor", L"BestSpeech");
        std::wstring engid;
        for (const char* p = engines[e].id; *p; ++p) {
            engid += static_cast<wchar_t>(*p);
        }
        cattrs.set(L"BstEngine", engid);
        cattrs.set(L"BstVoice", L"0");
        cattrs.set(L"BstCustom", L"1");
    }
}

inline void remove_voice_tokens(HKEY root) noexcept
{
    using namespace Bestspeech::registry;

    try {
        key tokens(root, voices_path, KEY_ALL_ACCESS);
        const auto remove_one = [&tokens](const std::wstring& id) noexcept {
            try {
                key token(tokens, id, KEY_ALL_ACCESS);
                token.delete_subkey(L"Attributes");
            }
            catch (...) {
            }
            try {
                tokens.delete_subkey(id);
            }
            catch (...) {
            }
        };
        for (int i = 0; i < total_token_count(); ++i) {
            remove_one(voice_attributes(i).get_token_id());
        }
        remove_one(custom_token_id);
        for (int e = 0; e < engine_count; ++e) {
            remove_one(custom_token_id_for(engines[e]));
        }
    }
    catch (...) {
    }
}
}
}
