<p align="center">
  <img src=".github/repo_icon.png" width="96" height="96" alt="ViKey">
</p>

<h1 align="center">ViKey</h1>

<p align="center">
  <strong>Vietnamese Telex Keyboard for Android</strong><br>
  The only FOSS Telex IME with a fully algorithmic syllable engine.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Kotlin-7F52FF?style=flat&logo=kotlin&logoColor=white" alt="Kotlin">
  <img src="https://img.shields.io/badge/API_26+-3DDC84?style=flat&logo=android&logoColor=white" alt="API 26+">
  <img src="https://img.shields.io/badge/License-Apache_2.0-blue" alt="License">
</p>

## Why ViKey

Every other Telex keyboard uses a lookup table — a giant dictionary mapping every possible key sequence to its Vietnamese output. If a combination isn't in the table, the engine can't produce it. This is fundamentally limited: new words, rare syllables, inconsistent tone placement, and accumulated drift are baked into the architecture.

ViKey replaces this with a **syllable-based recomposition engine** written in pure Kotlin. It understands Vietnamese phonology — not just character mappings. Each keystroke triggers a full parse of the current syllable, applies Vietnamese orthographic rules, and recomposes the correct output from first principles. No JSON. No lookup tables. No accumulated drift.

```kotlin
// FlorisBoard (mutation-based): "chaos" → "chào"
//   requires table entry for "ào" or cascading replacements
// ViKey (recomposition-based): parse("chao") + applyTone('f') → "chào"
//   algorithmic — any valid Vietnamese sequence works
```

## At a Glance

| | ViKey | FlorisBoard | Other Telex IMEs |
|---|---|---|---|
| **Engine** | Syllable-based recomposition | JSON replacement rules | Lookup table or regex |
| **Vocabulary** | Infinite (algorithmic) | Table-bound | Table-bound |
| **Tone placement** | Orthographic (1984 Quốc Ngữ) | Priority-heuristic | Heuristic or last-vowel |
| **`gi`/`qu` tone target** | On the real vowel (`giá`) | On `i` (`gía`) | Usually wrong |
| **Diphthong/triphthong rules** | 30+ vowel clusters | Partial | Partial |
| **`z` undo** | Strip tones, return to base | Not supported | Not supported |
| **Shortcut undo** | `ư+w→uw`, `â+a→aa`, `ô+o→oo` | Not supported | Not supported |
| **`ww`→`w` lifecycle** | `w→ư`, `ư+w→uw`, `ww→w` | Produces `ưw` | Usually broken |
| **English fallback** | 3 heuristics (patterns, coda, density) | Not supported | Manual mode switch |
| **Case preservation** | Per-character, 3 modes | First-char only | Partial |
| **Recomposition** | Full re-parse — no state, no drift | Accumulates errors | Cumulative mutations |

---

## Architecture

### Pure-Function Composer

The `Composer` interface is a stateless, deterministic function — given the same preceding text and the same input character, it always produces the same output. There is no internal buffer, no mutable state, no hidden accumulator. This eliminates entire classes of bugs that plague traditional IMEs:

- No composing buffer desync after cursor moves
- No corruption from external text changes (paste, auto-correct)
- No drift on long typing sessions
- Trivially testable and fuzzable

### Syllable Recomposition (Not Mutation)

Every keystroke triggers a full recomposition cycle:

```
Keypress → Decompose → Parse Syllable → Apply Rules → Rebuild → Output
                ↑                                            ↓
          precedingText                              (deleteCount, replacement)
```

**Tone placement is recalculated from scratch each time**, not applied as a transformation on a previous state. This means errors never compound — every keypress produces the correct output for the current syllable state.

### Flicker-Free Composing

ViKey uses Android's native `setComposingText()` for atomic in-place updates, wrapped in a `beginBatchEdit()/endBatchEdit()` pair. The `ExpectedContentQueue` optimistically predicts editor state after each commit and verifies it asynchronously — far more robust than synchronized `InputConnection` calls.

---

## Vietnamese Linguistics

### Syllable Parser

Decomposes any Vietnamese string into standard phonological components using longest-first greedy matching:

```
  n g u y ễ n
  ↑↑   ↑↑  ↑
onset nucleus coda + tone
```

Handles all Vietnamese onset clusters (`ngh`, `ng`, `ch`, `gh`, `gi`, `kh`, `nh`, `ph`, `th`, `tr`, `qu`) with a **disambiguation guard** that prevents false onset matching on vowel-ending digraphs like `qu`.

### Orthographic Tone Placement

Tone marks are placed following the official 1984 Quốc Ngữ rules via a 4-level cascade:

1. **Explicit diphthong/triphthong rules** — 30+ vowel clusters mapped to their correct tone target: `oa→a`, `oe→e`, `uy→y`, `iê→ê`, `yê→ê`, `uô→ô`, `ươ→ơ`, `uôi→ô`, `ươi→ơ`, `oai→a`, `iêu→ê`, `yêu→ê`, ...
2. **Horn vowel priority** — `ê`, `ơ` take tone before other vowels
3. **Circumflex/breve priority** — `â`, `ă`, `ô` take tone next
4. **Last vowel** — default Vietnamese rule for simple diphthongs

### `gi`/`qu` Exceptions

In Vietnamese, `gi` and `qu` are complex initials — the `i` in `gi` and the `u` in `qu` are part of the consonant, not the vowel nucleus. `findVowelPositions()` explicitly skips them:

```
Type "gias"  →  ViKey: "giá"    Everywhere else: "gía"
Type "quas"  →  ViKey: "quá"    Everywhere else: "qúá"
```

Only skipped when a real vowel exists elsewhere in the syllable.

### Semivowel Coda Detection

Distinguishes true consonant codas from semivowel offglides. Vietnamese diphthongs like `oai`, `iêu`, `ươu` have vowel nuclei followed by a semivowel (`i`, `u`, `y`, `o`). The parser correctly separates these from the nucleus so that tone placement targets the correct vowel.

---

## UX Features

### `z` Undo

Type `z` at the end of any word to strip all tones:

| Type | See |
|------|-----|
| `cháof` | `chào` |
| `chàoz` | `chao` |

If no tones exist, `z` is literal text.

### Shortcut Undo

Press the second shortcut key again to undo it:

| Type | See |
|------|-----|
| `aa` | `â` |
| `âa` | `aa` |
| `uw` | `ư` |
| `ưw` | `uw` |
| `uow` | `ươ` |
| `ươw` | `uow` |

Works for all 7 shortcuts: `aw(ă)` `aa(â)` `ee(ê)` `oo(ô)` `ow(ơ)` `uw(ư)` `dd(đ)`.

### `w` Lifecycle

`w` plays multiple Telex roles — shortcut partner for `ă`, `ơ`, `ư`, standalone `ư` producer, and composition toggler:

| Type | See | Path |
|------|-----|------|
| `w` | `ư` | First character → `ư` |
| `ww` | `w` | Undo: `ư` → `w` |
| `kw` | `kư` | Consonant + `w` → `kư` |
| `kưw` | `kuw` | Undo: `ư` → `u` + `w` |
| `aw` | `ă` | Shortcut: `aw` → `ă` |
| `uw` | `ư` | Shortcut: `uw` → `ư` |

### 3-Letter Shortcut

`uow` produces `ươ` in a single shortcut — a unique efficiency feature not found in standard Telex.

### English Fallback Detection

Three heuristics prevent false Telex transforms when typing English:

1. **English patterns** — `tion`, `ness`, `ship`, `str`, `ight`, `ould`, `ough`, `sch`, `scr`, `dge`, ...
2. **Coda validation** — checks if the word's final consonant cluster is possible in Vietnamese: only `c`, `m`, `n`, `p`, `t` (single) or `ch`, `ng`, `nh` (double) or `ngh` (triple) are valid Vietnamese codas
3. **Vowel density** — consonant runs exceeding 3 without intervening vowels flag the word as English (Vietnamese max is 3)

No manual mode switch needed — `process` types as English, `phở` types as Vietnamese.

### Case Preservation

Three-case mode system threaded through every transform path:

| Mode | Input | Output |
|------|-------|--------|
| UPPER | `AA` | `Â` |
| UPPER | `UOWS` | `ỚỚ` |
| Capitalized | `Aa` | `Ââ` |
| Capitalized | `Uow` | `Ươ` |
| lower | `aa` | `â` |
| lower | `uows` | `ướ` |

---

## Theme

| Dark | Light |
|:---:|:---:|
| <img src=".github/theme-dark.jpg" width="300" alt="Dark theme"> | <img src=".github/theme-light.jpg" width="300" alt="Light theme"> |

...and 12+ more custom themes built-in.

---

## Technical Highlights

- **Zero dictionary dependency** — no word list, no ML, no network. Pure algorithmic Vietnamese phonology.
- **No JSON lookup tables** — 100% Kotlin. The entire engine is 591 lines.
- **30+ orthographic tone rules** — covers all Vietnamese diphthongs and triphthongs.
- **3-layer English detection** — patterns, coda validation, vowel density heuristics.
- **Stateless composer** — pure function, no mutation, no drift. Trivially testable.
- **Fork of FlorisBoard** — inherits all features (themes, layouts, glide typing, clipboard, emoji, spell check, extension system) while replacing only the Telex engine with a ground-up rewrite.
- **Compatible with all FlorisBoard features** — every FlorisBoard layout, theme, and plugin works with ViKey.

---

## Privacy

ViKey follows FlorisBoard's privacy-first design: **no network access, no tracking, no analytics**. Every keystroke stays on your device. The Telex engine is entirely local — no internet connection required.

---

## License

Apache 2.0. See [LICENSE](LICENSE).

Original copyright © 2020-2026 The FlorisBoard Contributors.  
Algorithmic Telex engine © 2026 NgocThanhGL.
