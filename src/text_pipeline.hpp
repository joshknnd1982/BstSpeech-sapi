#pragma once

#include <string>
#include "engines.hpp"
#include "user_settings.hpp"

namespace Bestspeech {
namespace text {

// Rewrites one fragment of SAPI text into something the given engine can actually
// pronounce: unicode punctuation folded down, characters that silence or crash the
// frontend neutralized, digits spelled out where the frontend cannot read them, and
// Latin script transliterated for the engines that drop it.
[[nodiscard]] std::wstring prepare(const std::wstring& text, const engine_info& eng);

// Same, but every character is read out individually (SPVA_SpellOut fragments).
[[nodiscard]] std::wstring prepare_spelled(const std::wstring& text, const engine_info& eng);

// Encodes prepared text in the byte encoding the engine's frontend expects.
[[nodiscard]] std::string encode(const std::wstring& text, const engine_info& eng);

// The inline tilde command prefix for one utterance. Empty for cmd_mode::none, where
// commands would be read aloud or vocalized as junk; those engines get their rate,
// pitch and volume applied to the audio instead.
//
// When flags is non-null, an explicit value for every parser switch the engine
// obeys is appended too. Explicit, not just the non-default ones: the switches
// persist for the life of an engine session, so each utterance has to state the
// state it wants rather than inherit whatever the previous one left behind.
[[nodiscard]] std::wstring command_prefix(const engine_info& eng,
                                          const voice_info& voice,
                                          int native_rate,
                                          int pitch_hz,
                                          int gain_db,
                                          const settings::text_flags* flags = nullptr);

// Restores every parser switch the prefix moved off its default, emitted after
// the utterance text and ahead of the ~| flush. Without it a mode like spelling
// or punctuation naming would apply to the flush command itself -- " ~|" would
// come back as "tilde vertical line" -- and a cancelled utterance could leave
// the mode stuck on for whatever the engine session speaks next.
[[nodiscard]] std::wstring command_suffix(const engine_info& eng,
                                          const settings::text_flags* flags);

// Latin -> target script, exposed for the verification tooling.
[[nodiscard]] std::wstring to_greek(const std::wstring& text);
[[nodiscard]] std::wstring to_cyrillic(const std::wstring& text);

// Digit run -> words, for the frontends that cannot read digits at all.
[[nodiscard]] std::wstring number_to_words(unsigned long long n, number_mode mode);

}
}
