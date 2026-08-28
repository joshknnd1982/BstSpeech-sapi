# BestSpeech SAPI5 Voices

A native Windows SAPI5 speech engine for the BestSpeech synthesizers, exposing every
language and every character voice to any SAPI5 application — Windows Narrator, NVDA,
JAWS, Balabolka, Bookworm, and anything else that speaks through SAPI.

> **Note:** early-stage software under active development. Please report problems you run into.

## What you get

**143 voices across 13 engines.** The classic 1994 English engine plus twelve 2006
language engines:

| Language | Voices | Language | Voices |
|---|---|---|---|
| English (Classic 1994) | 14 | Italian | 14 |
| English | 14 | Japanese | 1 |
| Dutch | 14 | Polish | 1 |
| French | 14 | Portuguese | 14 |
| German | 14 | Russian | 14 |
| Greek | 1 | Spanish | 14 |
| Hebrew | 14 | | |

The fourteen character voices are Fred, Sara, Hary, Wendy, Dexter, Alien, Kit, Bruno,
Ghost, Peeper, Dracula, Granny, Martha and Tim. Greek, Japanese and Polish publish a
single voice each because their text frontends ignore every voice command — publishing
fourteen identical entries would only be misleading.

**Both architectures.** A 32-bit and a 64-bit COM server are installed and registered
into their own registry views, so 32-bit hosts and 64-bit hosts (Windows 11 Narrator
among them) both see the full voice list. Every engine DLL is 32-bit, so a 64-bit host
reaches them through a dedicated `b32_helper.exe` per engine -- one engine per process,
synthesized on the thread that opened it, audio returned over a pipe. A 32-bit host runs
the engine in process and falls back to the same helper if that fails. Both paths produce
byte-identical audio.

**Full SAPI parameter support.** Rate, pitch and volume, the per-fragment `RateAdj`,
`PitchAdj` and `Volume` adjustments, rate and volume changed mid-utterance, word and
sentence boundary events, bookmarks, spell-out and silence fragments, and correct
per-engine output sample rates.

**A configuration utility.** "BestSpeech configuration" on the Start menu adjusts
every parameter the engines expose -- per-voice pitch, inflection, head size,
excitation and unvoiced volume, a global rate and volume on top of whatever the SAPI
client asks for, and the engines' own text-processing switches. Every change is saved
as it is made and heard on the next utterance of any running SAPI application.

**A Custom Voice, in every language.** "BestSpeech Custom Voice" mirrors the
configuration utility: whatever language and voice parameters the utility showed when
it was last closed, that is what it speaks. Alongside it, every language whose engine
accepts voice commands publishes a pinned variant -- "BestSpeech Custom Voice -
German" and so on -- that speaks that language with the same sculpted parameters, and
carries a truthful Language attribute so applications that filter voices by language
find it. A voice you shape once, available everywhere.

## Installing

Run `BestSpeechSAPI_Setup.exe` and accept the elevation prompt — registering voices
writes to `HKEY_LOCAL_MACHINE`. The voices appear immediately in any SAPI5 application;
restart the application if it caches its voice list.

### Choosing what gets installed

All 143 voices is a lot to scroll past in a voice list, and most people want a handful.
The wizard's **Choose languages and voices** page has two branches you tick
independently:

* **Languages** — the thirteen engines. Each one is its own DLL, so leaving a language
  out saves its disk space as well as its entries in the voice list.
* **Character voices** — Fred, Sara, Hary and the other eleven. These cost nothing on
  disk: a character voice is a set of engine parameters, not a file.

The two are **crossed**. Tick five languages and three character voices and you get
fifteen voices, not five and not three. Greek, Japanese and Polish sit outside the cross
— their frontends ignore every voice command, so each contributes exactly one voice
whatever is ticked under Character voices. The **Custom Voice** is a third, separate
tick. The Ready page tells you how many voices you are about to get.

Four ready-made choices are offered before the page, and picking one just pre-ticks the
boxes, which you are then free to change:

| Setup type | What it ticks | Voices |
|---|---|---|
| Everything | all 13 languages, all 14 character voices | 143, or 154 with the Custom Voice |
| English only | both English engines, all 14 character voices | 28, or 31 with the Custom Voice |
| Compact | English with Fred, Sara, Hary and Wendy | 4, or 6 with the Custom Voice |
| Custom | whatever you tick yourself | as many as you tick |

To change your mind later, run the installer again and tick a different set. Languages
you untick have their engine DLL deleted and their voices unregistered; languages you
add are registered on the spot. Nothing else about your setup is disturbed — the
per-voice parameters you saved in the configuration utility live under
`HKEY_CURRENT_USER` and survive.

The choice is recorded in plain text as `voices.ini` in the install directory:

```ini
[Selection]
Languages=classic,eng,spa
Voices=Fred,Sara,Kit
CustomVoice=1
```

That file is what the engine reads when it registers its tokens, and what the
configuration utility and the diagnostics tool read to know what is installed. Editing
it by hand changes nothing until the engine is registered again, so run the installer
instead. If the file is missing altogether — a developer build registered by hand with
`regsvr32` — everything is published, exactly as it was before this was configurable.

## The configuration utility

"BestSpeech configuration" on the Start menu (`BestSpeechConfig.exe` in the install
directory) is a plain Win32 dialog: every control is labelled, carries an access key
and sits in the tab order, which is what a screen reader handles best.

Every change is written the moment it is made, to
`HKEY_CURRENT_USER\Software\BestSpeech`, and the engine re-reads those values on every
utterance — so a change lands on the very next thing a running screen reader says,
nothing needs restarting, and everything is already saved when the dialog closes.
"Play sample" speaks the selected voice through SAPI itself, current settings included.

**Per voice** — pick the language and voice at the top, then adjust pitch (43 to 413
Hz), inflection (-300 flat to 100 lively), unvoiced volume, head size (obeyed by the
classic English engine only) and excitation. Overrides are stored per voice, keyed the
same way as the voice tokens so they survive an upgrade; "Reset this voice" returns
the built-in character.

**Global** — the rate (25 to 400 percent) multiplies whatever rate the SAPI client
asks for, and the volume adjustment (-40 to +12 dB) adds to its volume, so the screen
reader's own controls keep working on top. Greek, Japanese and Polish, whose frontends
take no inline commands, get both through the time stretcher and gain scaling instead.

**Text processing** — the engines' own parser switches (`~n1` through `~n10`, and the
`~~2` read-immediately command behind "Phrase prediction"). Every switch was measured
against every dll — fresh engine session per utterance, since these modes persist and
contaminate sequential tests — and is only ever sent where it is swallowed cleanly.
Which switches audibly do something varies by frontend:

| Setting | Audibly works on |
|---|---|
| Number processing off (speak digits individually) | every command-taking engine |
| Spell out all words | every command-taking engine |
| Speak punctuation | every command-taking engine |
| Speak spaces and line breaks | every command-taking engine |
| Speak capitalized letter groups as words | Dutch, French, German, Hebrew, Portuguese, Spanish |
| Expand abbreviations | the English engines |
| Speak times of day | the English engines |
| Math mode, full numbers, control characters, phrase prediction | swallowed everywhere, no audible effect measured |

Three measured details shape how the switches are sent. The dlls boot with every
switch off — the Keynote manual's "default on" for times-of-day and abbreviations does
not hold for these builds. The modes persist for the life of an engine session, so the
engine states every switch explicitly on each utterance and restores them after the
text, ahead of the `~|` flush — without that, a mode like punctuation naming names the
flush itself and every utterance grows a second of junk. And the Hebrew dll crashes
outright on `~~2,1]` followed by text, so Hebrew alone never receives the
phrase-prediction command.

**The Custom Voice** — closing the utility (any way: the Close button, Escape, or
Alt+F4) snapshots the selected language and that voice's current parameters into
`HKEY_CURRENT_USER\Software\BestSpeech\CustomVoice`, and that snapshot is what the
"BestSpeech Custom Voice" token speaks. Tune Granny's Russian until it sounds right,
close the dialog, and any SAPI application that selects the Custom Voice gets exactly
that — while the ordinary tokens stay what they always were. Until the utility has
been closed once, it speaks classic English with Fred's parameters.

The same snapshot's parameters also drive the per-language custom tokens —
"BestSpeech Custom Voice - Russian" always speaks Russian, with whatever pitch,
inflection, head size, excitation and unvoiced volume the utility last saved. Ten
languages publish one: Greek, Japanese and Polish are left out because their
frontends ignore every voice command, so a custom token there could only duplicate
the single voice they already have. The language-less token is registered under
English (a token needs a language attribute) but speaks whatever language the
snapshot names; the per-language ones carry their own language.

Settings live under `HKEY_CURRENT_USER\Software\BestSpeech`: global values on the key
itself, one subkey per voice under `Voices\`, and the Custom Voice snapshot under
`CustomVoice\`. A value that is absent means "use the built-in default", so deleting
the key — or pressing "Reset everything" — returns the engine to exactly its
out-of-the-box sound.

## Engine quirks this works around

These engines are old and have sharp edges. Each of the following was established by
measurement — synthesizing through the real DLLs and comparing the audio — not by
guesswork, and each is re-checked by `tools/verify_engines.py`.

- **Greek and Russian drop the Latin alphabet entirely.** Every ASCII letter is
  discarded silently, so "Open Firefox settings" reaching either engine is pure silence.
  Latin text is transliterated into Greek or Cyrillic before it is sent. A lone Latin
  letter becomes the English letter name, which is what a screen reader means when it
  echoes a character.
- **Greek, Japanese and Polish cannot read digits.** Greek and Japanese ignore ASCII
  digits outright; Polish only spells them one at a time. Numbers are converted to words
  in the target language first.
- **Braces silence every 2006 engine,** and can crash the Russian one outright once it
  has synthesized a few utterances. Russian additionally truncates on `)`, `]`, `*`, `^`
  and goes silent on a backtick. These are neutralized before they reach the engine.
- **Command order is load-bearing.** `~v` (headsize) resets the fundamental frequency,
  and `~f` (frequency) in turn resets the inflection, so a prefix that sets them in the
  wrong order silently discards the settings made before them. The order in
  `src/text_pipeline.cpp` was verified one command at a time.
- **A dot between digits is sentence punctuation** to these frontends, so "7.1" reads as
  "seven. one". Decimals, version strings and IP addresses are spelled out first.
- **The 2006 engines are far quieter than the classic one** — Russian peaks below a
  fifth of full scale — so each carries a fixed gain trim, set just short of where the
  loudest voices hit the engine's internal limiter.
- **`~v` headsize is ignored by every 2006 engine,** and Dutch additionally ignores `~e`
  excitation, which is why its Bruno and Ghost voices are indistinguishable.
- **German has no inflection command at all.** Rather than obeying `~h` it reads the
  command out as text, so a stray fragment was spoken ahead of every German utterance —
  and because it landed before the gain command had been applied, that fragment was
  quiet and the real speech then jumped in louder. German no longer receives `~h`, and
  the gain command now leads the prefix so nothing can be spoken at the wrong volume.
- **The in-process audio capture is unreliable, which is why Portuguese can go silent.**
  The shim captures what an engine plays by hooking `waveOut`, and that capture only
  happens when two conditions hold together: the hook state is set for the calling thread
  (it lives in a `thread_local`) and the engine echoes the shim's own state pointer back
  through `waveOutOpen`'s callback argument. Miss either and the hook passes the call
  through to the real winmm -- no audio reaches the caller and nothing reports an error.
  The engine loads fine, `Speak` returns success, and the voice is simply silent. When it
  happens the shim also never sees the wave format, so it reports a default 10000 Hz
  instead of the engine's real rate, which makes the failure easy to spot in the log.

  Observed on Portuguese inside NVDA and the Speech control panel, intermittently: the
  same binaries in the same directory succeed on one run and fail on the next. Every
  voice is therefore checked with a throwaway utterance when its engine loads, and an
  engine that produces nothing is moved out of process for the rest of the session --
  before the user's first real word rather than after it.

  Out of process it is reliable, because a dedicated `b32_helper.exe` runs one engine on
  the thread that opened it. That is the same mechanism the BestSpeech NVDA add-on uses,
  which is the configuration known to work on affected machines. A 64-bit host has no
  in-process option at all -- the engines are 32-bit -- so it always takes that route. An
  engine can also be pinned there up front on 32-bit, see below.

- **Arabic is deliberately excluded.** `dll_ara.dll`'s synthesis core is a stub that
  emits the same buffer of digital silence for any input, in either script.

## Talking to b32_helper.exe

The helper's wire format is small and unforgiving, and one detail is worth stating
plainly because getting it wrong is catastrophic rather than merely wrong:

| Command | Bytes |
|---|---|
| Speak | `uint32` text length, `float32` rate multiplier, then the text |
| Cancel | `uint32` zero — **and nothing else** |
| Quit | `uint32` 0xFFFFFFFF |

A cancel is *not* a zero-length speak. Sending the rate multiplier after the zero leaves
four bytes in the pipe that the helper reads as the next command's length: `0x3F800000`,
about a gigabyte, which it then tries to read as utterance text. The helper balloons to a
gigabyte of memory and stops answering, and because the caller is blocked reading the
pipe, the application doing the talking freezes with it. That is what made 64-bit hosts
die a few seconds in, once the first utterance got cancelled.

Every read in `src/helper_client.cpp` is therefore bounded, with the clock restarting on
each byte received, so a wedged helper can slow a voice down but can never hang the host.
`cancel_probe` exercises this directly.

## Diagnostic log

A log is written to `%LOCALAPPDATA%\BestSpeech\bestspeech.log` while the engine is
still settling. It records the engine and voice chosen, the resolved rate, pitch and
volume, and — most usefully — the exact byte string handed to the engine, which is where
a "what is it saying?" problem becomes obvious. Every line is tagged with the process,
its bitness and its pid, since a 64-bit host's utterance crosses two processes.

The file is capped at 4 MB with one previous copy kept, so it cannot fill a disk.

To turn it off, set this registry value and restart the speaking application:

```
HKEY_CURRENT_USER\Software\BestSpeech   DWORD   Logging = 0
```

Set it to `1`, or delete it, to turn logging back on.

The log is opened for shared append. An earlier build opened it exclusively, which meant
the first process to log locked out every other one -- and since a screen reader and the
worker are both long-lived, that silently hid exactly the processes worth looking at.

## Forcing an engine out of process

If a language is silent in one application but fine elsewhere, it can be pinned to a
dedicated `b32_helper.exe`, which runs that engine in isolation:

```
HKEY_CURRENT_USER\Software\BestSpeech   REG_SZ   WorkerEngines = por
```

Comma-separate several ids, or use `*` for all of them. The ids are the short names in
`src/engines.hpp`: `classic eng dut fre ger gre heb ita jpn pol por rus spa`. This is
only a shortcut -- an engine that fails its load-time check is moved out of process
automatically. Both paths are covered by the verification suite.

## Checking an installation

`BestSpeechDiagnostics.exe`, installed alongside the engine and on the Start menu as
"Check BestSpeech voices", walks every voice through SAPI as an application would and
writes a report to `%LOCALAPPDATA%\BestSpeech\diagnostics.txt`. It changes nothing --
no registration, no settings, no default voice. A 64-bit copy sits in the `x64` folder.

It reads `voices.ini` too, so a voice you chose not to install is reported as "not
installed" rather than counted as a fault.

## Building from source

```batch
build_all.bat
```

Builds both architectures, stages `output\`, and compiles the installer.

**Requirements:** Windows 10 or later, Visual Studio 2022 Build Tools with the C++
workload, CMake 3.15+, and Inno Setup 6 for the installer step (the build skips it and
leaves a usable `output\` if Inno Setup is absent).

## Verifying

Two suites, both of which drive the real engines and fail loudly:

```batch
python tools\verify_engines.py --probe build_x86\bin\Release\pipeline_probe.exe
```

Drives every engine DLL through `b32_helper.exe` and checks that each speaks its own
language, that every voice is audible and distinct, that Latin text survives on the
engines that drop it, and that no ASCII symbol silences, truncates or crashes synthesis.

```batch
python tools\verify_sapi.py --probe sapi_probe32.exe --dll output\BestspeechSAPI.dll
```

Creates the real COM object through `DllGetClassObject`, hands it each of the 143 voice
tokens in turn, and captures what it writes — the same path a screen reader takes, with
nothing registered and no elevation needed. Point it at `output\x64\BestspeechSAPI.dll`
with `sapi_probe64.exe` to exercise the 64-bit bridge.

```batch
cancel_probe.exe outputd\BestspeechSAPI.dll por 60
```

Speaks sixty utterances, abandoning most of them part way through the way a screen reader
does when the user keeps moving. It fails if any utterance after a cancellation comes back
empty, or if the run stops making progress.

`tools/sapi_probe.cpp` also works standalone for listening to a single voice:

```batch
sapi_probe32.exe output\BestspeechSAPI.dll rus 11 out.wav "Hello world" --rate 5
```

## Layout

| Path | Purpose |
|---|---|
| `src/engines.hpp` | The engine and voice tables: languages, LCIDs, sample rates, capabilities |
| `src/text_pipeline.cpp` | Sanitizing, transliteration, number words, inline command prefixes |
| `src/user_settings.hpp` | The user settings, shared by the engine and the configuration utility |
| `src/install_selection.hpp` | Reads `voices.ini`: which languages and voices the installer put here |
| `src/ISpTTSEngineImpl.cpp` | The SAPI engine itself |
| `tools/bestspeech_config.cpp` | The configuration utility dialog |
| `src/b32_wrapper.cpp` | Loader for `b32_wrapper.dll`, the shim that drives both engine families |
| `src/bestspeech_server.cpp` | 32-bit worker for 64-bit hosts |
| `src/sapi_main.cpp` | COM registration and voice token registration |
| `installer/BestspeechSAPI.iss` | The installer, including the language and voice component pages |
| `tools/translit_ref.py` | Transliteration and number tables, and the reference implementation |
| `tools/gen_translit.py` | Generates `src/translit_tables.inc` from those tables |

The transliteration tables live in one place and are generated into C++, so the engine
and the Python reference cannot drift apart.

## Contributing

Contributions are welcome. Please open an issue with a clear description, steps to
reproduce, your Windows version and architecture, and relevant logs. For pull requests,
branch from `master`, test on both x86 and x64, and keep to the existing style.

## Support the project

If you find this useful, consider supporting development:

[![Donate with PayPal](https://img.shields.io/badge/Donate-PayPal-blue.svg)](https://paypal.me/gozaltech)

## License

GNU General Public License v3 — see [`LICENSE`](LICENSE). This applies to the whole
repository, including the speech engine binaries in `bin/`.

## Credits

- **Rommix**, who preserved the BestSpeech / Keynote Gold engines and shared them with the
  blind community, and who created the fourteen character voices
- [**samtupy**](https://github.com/samtupy/b32tts_wrapper) for the engine shim
  (`b32_wrapper.dll`, `b32_helper.exe`), released into the public domain — its source is
  what finally explained why some voices fell silent inside a host application
- [**gozaltech**](https://github.com/gozaltech/bstspeech-sapi) for the original SAPI5
  wrapper this project is forked from
