# Vi 5.0 for Windows — Complete User Guide

> **Vi 5.0.0-2026**  
> *A high-performance, zero-dependency text editor natively compiled for Windows 10/11 and Windows Server 2022/2025.*

---

## Table of Contents

1. [Introduction & Mental Model](#1-introduction--mental-model)
   - [What Makes Vi for Windows Special?](#what-makes-vi-for-windows-special)
   - [The Core Philosophy: Modal Editing](#the-core-philosophy-modal-editing)
   - [The Golden Rule for Beginners: How to Exit & Get Unstuck](#the-golden-rule-for-beginners-how-to-exit--get-unstuck)
   - [The Vi Grammar](#the-vi-grammar)
2. [Beginner's First 15 Minutes](#2-beginners-first-15-minutes)
   - [Opening and Creating Files](#opening-and-creating-files)
   - [Moving Around (Normal Mode)](#moving-around-normal-mode)
   - [Entering Insert Mode & Typing Text](#entering-insert-mode--typing-text)
   - [Basic Editing: Delete, Yank (Copy), Paste, Undo](#basic-editing-delete-yank-copy-paste-undo)
   - [Saving and Quitting](#saving-and-quitting)
3. [Editing Mastery & Precision Tools](#3-editing-mastery--precision-tools)
   - [Search and Replace](#search-and-replace)
   - [Extended Regular Expression Engine](#extended-regular-expression-engine)
   - [Mouse & Scroll Wheel Support](#mouse--scroll-wheel-support)
   - [Text Objects: Surgical In-Place Edits](#text-objects-surgical-in-place-edits)
   - [Visual Modes & Visual Block Mode (Column Editing)](#visual-modes--visual-block-mode-column-editing)
   - [Marks & Jumps](#marks--jumps)
   - [Macro Recording & Replay](#macro-recording--replay)
4. [Vi as a Software Developer's Editor](#4-vi-as-a-software-developers-editor)
   - [Supported Syntax Engines & Language Detection](#supported-syntax-engines--language-detection)
   - [Color Schemes](#color-schemes)
   - [Split Windows (Horizontal & Vertical Viewports)](#split-windows-horizontal--vertical-viewports)
   - [Tab Pages & Top Tabline (`:tabnew`, `gt`, `gT`)](#tab-pages--top-tabline-tabnew-gt-gt)
   - [Side-by-Side Diff Mode & Visual Comparison (`vi -d`, `:diffsplit`)](#side-by-side-diff-mode--visual-comparison-vi--d-diffsplit)
   - [Indent-Based Code Folding (`zc`, `zo`, `za`, `zR`, `zM`)](#indent-based-code-folding-zc-zo-za-zr-zm)
   - [Project Grep & Quickfix List Navigation (`:grep`, `:copen`)](#project-grep--quickfix-list-navigation-grep-copen)
   - [In-Memory Fuzzy File Search (`:find <query>`)](#in-memory-fuzzy-file-search-find-query)
   - [Workspace Sessions (`:mksession`, `:source`, `vi -S`)](#workspace-sessions-mksession-source-vi--s)
   - [In-Memory Multi-Buffer Management](#in-memory-multi-buffer-management)
   - [Persistent Undo Histories Across Restarts (`:set undofile`)](#persistent-undo-histories-across-restarts-set-undofile)
   - [Buffer-Word Autocompletion (`Ctrl-N` / `Ctrl-P`)](#buffer-word-autocompletion-ctrl-n--ctrl-p)
   - [Code Navigation with CTags (`Ctrl-]` / `Ctrl-T`)](#code-navigation-with-ctags-ctrl--ctrl-t)
   - [Interactive Directory Explorer (`:Explore`, `vi <dir>`)](#interactive-directory-explorer-explore-vi-dir)
   - [Compiling, Testing & External Shell Commands (`:!<cmd>`)](#compiling-testing--external-shell-commands-cmd)
   - [Interactive Subshell & Embedded Terminal (`:terminal`, `:sh`)](#interactive-subshell--embedded-terminal-terminal-sh)
   - [Code Formatting with Filter Pipes (`:[range]!<cmd>`)](#code-formatting-with-filter-pipes-rangecmd)
   - [Windows System Clipboard Integration (`"+` and `"*`)](#windows-system-clipboard-integration--and-)
5. [Vi as a Windows System Administrator's Tool](#5-vi-as-a-windows-system-administrators-tool)
   - [Why Vi on Windows Server & Core?](#why-vi-on-windows-server--core)
   - [Remote Administration over Windows OpenSSH / PowerShell](#remote-administration-over-windows-openssh--powershell)
   - [Comparing Config Files Side-by-Side with Diff Mode](#comparing-config-files-side-by-side-with-diff-mode)
   - [External File Change Detection (`:set autoread`, `:checktime`)](#external-file-change-detection-set-autoread-checktime)
   - [Editing System Configuration Files (`hosts`, JSON, XML, INI, TOML, YAML)](#editing-system-configuration-files-hosts-json-xml-ini-toml-yaml)
   - [Authoring & Debugging PowerShell Scripts (`.ps1`, `.psm1`)](#authoring--debugging-powershell-scripts-ps1-psm1)
   - [Authoring Windows Batch Files (`.bat`, `.cmd`)](#authoring-windows-batch-files-bat-cmd)
   - [Rapid Log File Inspection & Analysis with Grep and Quickfix](#rapid-log-file-inspection--analysis-with-grep-and-quickfix)
   - [Data Cleaning & Column Extraction with Visual Block Mode](#data-cleaning--column-extraction-with-visual-block-mode)
   - [Crash Protection & Swap Recovery on Unstable Connections](#crash-protection--swap-recovery-on-unstable-connections)
   - [Windows File Explorer Context Menu ("Open with Vi")](#windows-file-explorer-context-menu-open-with-vi)
6. [Configuration & Customization (`~/.virc`)](#6-configuration--customization-virc)
   - [Creating Your Configuration File](#creating-your-configuration-file)
   - [Recommended Configuration for Developers](#recommended-configuration-for-developers)
   - [Recommended Configuration for Sysadmins](#recommended-configuration-for-sysadmins)
   - [Complete Options Reference Table](#complete-options-reference-table)
7. [Emergency Survival Reference Card](#7-emergency-survival-reference-card)

---

## 1. Introduction & Mental Model

### What Makes Vi for Windows Special?

Vi for Windows is a modern, clean-room implementation of the standard POSIX / BSD `vi` text editor, engineered specifically for Windows 10, Windows 11, and Windows Server (including Server Core):

- **Single Portable Binary (~300 KB)**: Zero external runtimes, DLLs, or frameworks required. Copy `vi.exe` anywhere and it works immediately.
- **Instantaneous Startup (< 10 ms)**: Opens instantly even on resource-constrained virtual machines or remote connections.
- **True Windows Virtual Terminal Support**: Built natively on the Windows Console Virtual Terminal sequences with DEC 2026 synchronized output, ensuring flicker-free, tearing-free rendering.
- **Zero Configuration Necessary**: Works out of the box with syntax highlighting, mouse-free navigation, and Windows clipboard support.

---

### The Core Philosophy: Modal Editing

Most modern text editors (Notepad, VS Code) are **modeless**: every key you press inserts a character unless you hold down `Ctrl` or `Alt`.

Vi is **modal**: the keyboard changes function depending on which **mode** you are currently in.

```
       [ i, I, a, A, o, O ]
   +--------------------------> INSERT MODE (Typing text)
   |                                 |
   |                                 |  <ESC>
   |                                 v
NORMAL MODE <-------------------+-----------------------+
(Navigation & Commands)         |                       |
   |                            |                       |
   |  [ v, V, Ctrl-V ]          |  [ : ]                |  [ / or ? ]
   v                            v                       v
VISUAL MODE                  COMMAND (EX) MODE       SEARCH MODE
(Interactive Selection)      (File saving, config)   (Find patterns)
```

1. **Normal Mode (Default)**: Your hands stay on the home row. Keystrokes are commands that navigate, delete, copy, or transform text. Think of this as your "steering wheel".
2. **Insert Mode**: The keyboard behaves like a standard typewriter. Keystrokes insert text into the file.
3. **Visual Mode**: Highlight ranges of text characterwise (`v`), linewise (`V`), or rectangular blockwise (`Ctrl-V`).
4. **Command (Ex) Mode**: Triggered by typing `:` in Normal mode. Opens a command prompt at the bottom of the screen to save files, quit, set preferences, or execute shell commands.
5. **Search Mode**: Triggered by `/` (search forward) or `?` (search backward).

---

### The Golden Rule for Beginners: How to Exit & Get Unstuck

> [!IMPORTANT]
> If you ever feel lost or accidentally type something unexpected, press **`<Esc>`** one or two times. You are now safely back in **Normal Mode**.

Here is how to exit Vi:

| Keystroke | What It Does | When to Use It |
|---|---|---|
| `:q!` `<Enter>` | **Force Quit**: Discards all unsaved changes and exits. | When you made a mistake and want to cancel everything safely. |
| `:wq` `<Enter>` | **Write and Quit**: Saves changes to disk and exits. | When you finished editing and want to save. |
| `:x` `<Enter>` | **Exit if saved**: Saves only if modified, then exits. | Fast standard exit. |
| `ZZ` (in Normal mode) | Shortcut for `:x` (no `<Enter>` needed). | The fastest way to save and exit. |
| `ZQ` (in Normal mode) | Shortcut for `:q!`. | The fastest way to quit without saving. |

---

### The Vi Grammar

Unlike editors where you must memorize hundreds of distinct shortcut combinations, Vi commands follow an intuitive **noun-verb grammar**:

$$\text{Command} = [\text{count}] + \text{operator} + [\text{count}] + \text{motion}$$

- **Operator (Verb)**: What to do
  - `d` = Delete (cut)
  - `c` = Change (delete and enter Insert mode)
  - `y` = Yank (copy)
  - `>` = Shift right (indent)
  - `<` = Shift left (unindent)
- **Motion (Noun)**: Where to do it
  - `w` = Next word
  - `e` = End of word
  - `b` = Previous word
  - `$` = End of line
  - `0` = Beginning of line
  - `G` = End of file

**Examples of Vi Grammar in Action:**
- `dw` $\rightarrow$ **D**elete **W**ord (deletes from cursor to start of next word).
- `d3w` $\rightarrow$ **D**elete **3** **W**ords.
- `d$` $\rightarrow$ **D**elete to end of line (same as uppercase `D`).
- `cw` $\rightarrow$ **C**hange **W**ord (deletes word and puts you in Insert mode so you can re-type it).
- `y$` $\rightarrow$ **Y**ank (copy) to end of line.
- `5dd` $\rightarrow$ Delete 5 whole lines.

---

## 2. Beginner's First 15 Minutes

### Opening and Creating Files

From PowerShell, Command Prompt, or Windows Terminal:

```powershell
# Open an existing file or create a new one
vi server_config.json

# Open multiple files into memory
vi script.ps1 config.ini README.md

# Open an entire folder in the Directory Explorer
vi .
vi C:\inetpub\wwwroot
```

---

### Moving Around (Normal Mode)

When Vi opens, you are in **Normal Mode**. Avoid reaching for the mouse or the arrow keys—keep your fingers on the home row:

```
          [ k ] (Up)
[ h ] (Left)      [ l ] (Right)
         [ j ] (Down)
```

- **`h`**: Move left one character.
- **`j`**: Move down one line.
- **`k`**: Move up one line.
- **`l`**: Move right one character.
- *(Standard keyboard arrow keys `Up`, `Down`, `Left`, `Right`, `Home`, `End`, `PageUp`, `PageDown` are also fully supported).*

#### Word & Line Jumps
- **`w`**: Jump forward to the start of the next word.
- **`b`**: Jump backward to the start of the previous word.
- **`e`**: Jump forward to the end of the current word.
- **`0`**: Jump to the very beginning of the line (column 0).
- **`^`**: Jump to the first non-whitespace character on the line.
- **`$`**: Jump to the end of the line.

#### File Navigation
- **`gg`**: Jump to the very first line of the file.
- **`G`**: Jump to the very last line of the file.
- **`45G`**: Jump directly to line 45.
- **`Ctrl-F`**: Page down (forward).
- **`Ctrl-B`**: Page up (backward).
- **`Ctrl-D`**: Half-page down.
- **`Ctrl-U`**: Half-page up.
- **`Ctrl-G`**: Display file status, cursor line/col, and % progress at the bottom.

---

### Entering Insert Mode & Typing Text

To type text, switch from Normal mode to Insert mode using one of these commands:

| Key | Action | Mental Tip |
|---|---|---|
| **`i`** | Insert text **before** the cursor. | "**i**nsert here" |
| **`a`** | Append text **after** the cursor. | "**a**fter cursor" |
| **`I`** | Insert text at the **beginning of the line** (first non-space). | Big jump to start |
| **`A`** | Append text at the **end of the line**. | Big jump to end |
| **`o`** | Open a new blank line **below** current line and insert. | "**o**pen below" |
| **`O`** | Open a new blank line **above** current line and insert. | "**O**pen above" |
| **`s`** | Substitute: delete character under cursor and insert. | "**s**wap char" |
| **`S`** | Substitute line: clear entire line and insert. | "**S**wap line" |
| **`R`** | Enter **Replace Mode** (overstrike existing text). | Overwrite |

> [!TIP]
> Once you finish typing your text, immediately press **`<Esc>`** to return to Normal Mode.

---

### Basic Editing: Delete, Yank (Copy), Paste, Undo

All of these are executed from **Normal Mode**:

#### Deleting
- **`x`**: Delete character under the cursor (like the `Delete` key).
- **`X`**: Delete character before cursor (like `Backspace`).
- **`dw`**: Delete word from cursor forward.
- **`dd`**: Delete the entire line.
- **`3dd`**: Delete 3 lines.
- **`D`**: Delete from cursor to the end of the line.

#### Copying & Pasting
- **`yy`** or **`Y`**: Yank (copy) the entire line into memory.
- **`3yy`**: Yank 3 lines.
- **`yw`**: Yank word.
- **`p`**: Put (paste) copied text **after** the cursor (or below current line).
- **`P`**: Put (paste) copied text **before** the cursor (or above current line).

#### Undoing & Redoing
- **`u`**: **Undo** the last change (multi-level undo stack).
- **`Ctrl-R`**: **Redo** the previously undone change.
- **`U`**: Revert current line back to its original state before you moved to it.
- **`.` (Period)**: **Repeat** the last modifying action (extremely powerful for repetitive edits!).

---

### Saving and Quitting

Type `:` in Normal mode to open the command prompt:

- **`:w`** $\rightarrow$ Save (write) changes to disk.
- **`:w backup.txt`** $\rightarrow$ Save file under a new name.
- **`:q`** $\rightarrow$ Quit (fails if you have unsaved changes).
- **`:q!`** $\rightarrow$ Force quit without saving.
- **`:wq`** or **`:x`** $\rightarrow$ Save changes and quit.

---

## 3. Editing Mastery & Precision Tools

### Search and Replace

#### Searching in the Current Buffer
- **`/pattern` `<Enter>`**: Search forward for `pattern`.
  - With **incremental search** enabled (`:set incsearch`), the cursor jumps to matches live as you type each letter!
- **`?pattern` `<Enter>`**: Search backward for `pattern`.
- **`n`**: Jump to the **next** search match.
- **`N`**: Jump to the **previous** search match.
- **`:nohl`** or **`:noh`**: Turn off match highlighting until the next search.

#### Search & Replace (Substitutions)
The standard substitute command syntax is:
`:[range]s/find/replace/[flags]`

| Command | Action |
|---|---|
| `:s/foo/bar/` | Replace first occurrence of `foo` with `bar` on the current line. |
| `:s/foo/bar/g` | Replace **all** occurrences of `foo` with `bar` on the current line (`g` = global). |
| `:%s/foo/bar/g` | Replace all occurrences of `foo` with `bar` across the **entire file** (`%` = all lines). |
| `:%s/foo/bar/gi` | Case-insensitive replacement across the entire file (`i` = ignore case). |
| `:10,25s/foo/bar/g` | Replace occurrences only between line 10 and line 25. |

---

### Extended Regular Expression Engine

Vi for Windows features an advanced, built-in regular expression engine supporting Vim-compatible precision anchors, shorthands, and character classes:

#### Precision Anchors (`\zs` and `\ze`)
- **`\zs`**: Sets the **start** of the match. Anything matched before `\zs` acts as context and is **not** replaced during substitution.
- **`\ze`**: Sets the **end** of the match. Anything matched after `\ze` acts as context and is **not** replaced.

> [!TIP]
> **Example**: To update a configuration port from `port = 8080;` to `port = 9000;` without re-typing or capturing the surrounding syntax:
> ```vim
> :%s/port = \zs\d\+\ze;/9000/g
> ```
> Only the digits `8080` are substituted with `9000`!

#### Shorthand Character Classes
| Token | Meaning | Equivalent Set |
|---|---|---|
| `\d` | Digit | `[0-9]` |
| `\D` | Non-digit | `[^0-9]` |
| `\w` | Word character | `[a-zA-Z0-9_]` |
| `\W` | Non-word character | `[^a-zA-Z0-9_]` |
| `\s` | Whitespace (space or tab) | `[ \t]` |
| `\S` | Non-whitespace character | `[^ \t]` |
| `\<` | Start of word boundary | Beginning of identifier |
| `\>` | End of word boundary | End of identifier |

#### Quantifiers & Sets
- **`\+`**: Match 1 or more repetitions (e.g. `\d\+` matches one or more consecutive digits).
- **`\?`**: Match 0 or 1 repetition (optional).
- **`*`**: Match 0 or more repetitions.
- **`[...]`**: Explicit character class (e.g. `[a-z_]`, `[0-9A-Fa-f]`).
- **`[^...]`**: Negated character class (e.g. `[^0-9]`).
- **`^` and `$`**: Beginning and end of line anchors.

---

### Mouse & Scroll Wheel Support

Vi for Windows features full ANSI/SGR mouse reporting (`\x1b[?1000h\x1b[?1002h\x1b[?1006h`), bringing modern GUI mouse fluidity into a lightweight console editor:

- **Left-Click Cursor Positioning**: Click anywhere on a buffer line to instantly move the cursor to that character. Works across Normal, Visual, and Insert modes.
- **Scroll Wheel Scrolling**: Rotate your mouse scroll wheel (Wheel Up / Wheel Down) to scroll 3 lines per notch. When working with split windows (`:split` or `:vsplit`), hovering and scrolling over any window automatically focuses and scrolls that specific window!
- **Right-Click Clipboard Paste**: Right-click anywhere in the editor to position the cursor and paste text directly from the Windows OS clipboard (`"+p`).
- **Tabline Click Switching**: When multiple tab pages are open (`:tabnew`), click any tab title in the top row to switch to it immediately.
- **Status Bar Window Activation**: Click the status divider of any split window to activate it.

#### Enabling & Disabling Mouse Reporting
Mouse support is enabled by default. You can toggle or query it at any time:
```vim
:set mouse=a     " Enable mouse support in all modes
:set nomouse     " Disable mouse reporting (leaves mouse handling to terminal)
:set mouse=      " Disable mouse (Vim compatible)
:set mouse?      " Query current mouse status
```

---

### Text Objects: Surgical In-Place Edits

Text objects let you target enclosed blocks of text without having to manually move the cursor to the edges:

$$\text{Format:} \quad [\text{operator}] + [\text{inner/around}] + [\text{target}]$$

- **`i`** = Inner (selects only the content *inside* the delimiters).
- **`a`** = Around (selects the content *including* the delimiters and surrounding whitespace).

| Target | Description | Example: `ci"` | Example: `da(` |
|---|---|---|---|
| **`w`** | Word | `diw` (delete inner word) | `daw` (delete word + space) |
| **`"`** | Double quotes | `ci"` (re-type inside `"..."`) | `da"` (delete `"..."` entirely) |
| **`'`** | Single quotes | `ci'` (re-type inside `'...'`) | `da'` (delete `'...'` entirely) |
| **`` ` ``** | Backticks | `ci`` (re-type inside `` `...` ``) | `da`` (delete `` `...` ``) |
| **`(`** or **`)`** or **`b`** | Parentheses | `di(` (delete inside `(...)`) | `da(` (delete `(...)` and parens) |
| **`[`** or **`]`** | Square brackets | `ci[` (change inside `[...]`) | `da[` (delete `[...]` and brackets) |
| **`{`** or **`}`** or **`B`** | Curly braces | `di{` (delete block inside `{...}`) | `da{` (delete `{...}` and braces) |
| **`<`** or **`>`** | Angle brackets / tags | `ci<` (change inside `<...>`) | `da<` (delete `<...>`) |

> [!TIP]
> Try this: Put your cursor anywhere inside `"connection_string_here"` and type **`ci"`**. Vi automatically deletes the string contents and puts you in Insert mode between the quotes!

---

### Visual Modes & Visual Block Mode (Column Editing)

Vi provides three interactive selection modes:

1. **Characterwise Visual (`v`)**: Select exact characters.
2. **Linewise Visual (`V`)**: Select entire lines.
3. **Visual Block Mode (`Ctrl-V` or `Ctrl-Q`)**: Select a rectangular column of text.

#### Power Feature: Multi-Line Column Insert & Append
Visual Block mode is indispensable for inserting comments, prefixes, or formatting tabular data across many lines simultaneously:

1. Move your cursor to the start of the first line.
2. Press **`Ctrl-V`** (or `Ctrl-Q`).
3. Press **`j`** down to the last line you want to edit.
4. Press **`I`** (capital `i`).
5. Type the text you want prepended to every line (e.g. `# ` or `// `).
6. Press **`<Esc>`**. Vi will automatically replicate your typed text down every single selected line!

#### Column Delete & Replace
- Select the block with **`Ctrl-V`** and motions, then press **`d`** or **`x`** to cut that rectangle out of the file.
- Press **`c`** to delete the block and replace it with typed text across all lines.

---

### Marks & Jumps

Set persistent bookmarks in your file to jump back and forth instantly:

- **`m{a-z}`**: Set a mark at the current cursor position (e.g. `ma` sets mark 'a').
- **`'{a-z}`**: Jump to the start of the line where mark `{a-z}` was set.
- **`` `{a-z} ``**: Jump to the exact character position of mark `{a-z}`.
- **`''`** or **`` `` ``**: Jump back to the position before your last jump.

---

### Macro Recording & Replay

Automate complex repetitive transformations:

1. **Start Recording**: Type `q` followed by a register letter (e.g. `qa` records to register `a`).
2. **Perform the actions**: Type your edits using regular Vi commands.
3. **Stop Recording**: Press `q` in Normal mode.
4. **Replay Macro**: Press `@a` to replay the macro once, or `10@a` to replay it 10 times!
5. **Repeat Last Macro**: Press `@@`.

---

## 4. Vi as a Software Developer's Editor

### Supported Syntax Engines & Language Detection

Vi for Windows features built-in, zero-dependency syntax highlighters that activate automatically upon opening a file:

| Language / Format | Recognized File Extensions | Highlighted Elements |
|---|---|---|
| **TOML** | `.toml`, `Cargo.toml` | Sections (`[...]`), key-value pairs, strings, numbers, booleans, and `#` comments. |
| **SQL** | `.sql` | ANSI & T-SQL statements, queries, transactions, data types, and `--` / `/* */` comments. |
| **XML / HTML / SVG** | `.html`, `.htm`, `.xhtml`, `.xml`, `.xaml`, `.svg`, `.config`, `.manifest`, `.csproj`, `.vcxproj` | Tags, attributes, strings, character entities, `<!--...-->` comments, and `<?...?>` preambles. |
| **YAML** | `.yml`, `.yaml` | Keys, scalar values, document markers (`---`, `...`), `#` comments, and boolean/null literals. |
| **INI / Registry / INF**| `.ini`, `.inf`, `.reg`, `.cfg`, `.properties`, `.gitconfig` | Section headers (`[...]`), `;`/`#` comments, Windows registry headings, key-value assignments. |
| **C / C++** | `.c`, `.h`, `.cpp`, `.hpp`, `.cc`, `.cxx` | Keywords, standard types, preprocessor directives (`#include`), strings, numbers, comments (`//`, `/* */`). |
| **Rust** | `.rs` | Keywords (`fn`, `mut`, `pub`, `impl`, `trait`), types (`String`, `Vec`, `i32`), `#[...]` attribute macros, strings, comments. |
| **Go** | `.go` | Keywords (`func`, `defer`, `chan`, `package`), types, comments, standard strings, and backtick `` `raw strings` ``. |
| **Java** | `.java` | Keywords, standard types, `@` annotations, strings, and comments. |
| **JavaScript / TypeScript** | `.js`, `.mjs`, `.cjs`, `.jsx`, `.ts`, `.tsx` | Keywords, types, template literals (`` `...` ``), strings, and comments. |
| **Python** | `.py`, `.pyw` | `def`, `class`, `@` decorators, imports, `#` comments, numbers, strings. |
| **Zig** | `.zig`, `.zon` | Keywords, types, multiline string literals (`\\`), and `@` built-ins. |
| **Swift** | `.swift` | Keywords, types, `#` compiler directives, and `@` attributes. |
| **Basic** | `.bas`, `.vbs`, `.vb` | Case-insensitive keywords, types, `'` and `REM` comments. |
| **PowerShell** | `.ps1`, `.psm1`, `.psd1` | Cmdlets, control keywords, `$variables`, strings, comments. |
| **Batch / CMD** | `.bat`, `.cmd` | Keywords, commands, `::` and `REM` comments. |
| **Shell** | `.sh`, `.bash`, `.zsh` | Keywords, variables, comments, strings. |
| **JSON** | `.json` | Key names, strings, numbers, boolean literals. |
| **Markdown** | `.md`, `.markdown` | Headings (`#`, `##`), emphasis, list markers. |

To toggle syntax on or off:
```vim
:syntax on
:syntax off
```

---

### Color Schemes

Switch between curated color palettes to match your terminal theme:

```vim
:colorscheme dark        " Default modern dark theme
:colorscheme monokai     " Vibrant Monokai contrast palette
:colorscheme gruvbox     " Warm, retro earthy Gruvbox tones
:colorscheme light       " Clean daylight theme for bright terminals
:colorscheme classic     " High-contrast retro terminal colors
```
*(You can also use the short alias `:colo <name>`)*.

---

### Split Windows (Horizontal & Vertical Viewports)

Work on multiple files or different sections of the same file side-by-side:

```
+-----------------------------------+-----------------------------------+
| src/main.c                        | src/vi.h                          |
|                                   |                                   |
| int main(int argc, char *argv[])  | typedef struct {                  |
| {                                 |     int cx, cy;                   |
|     init_editor();                |     EditorMode mode;              |
|                                   | } Editor;                         |
| [1] src/main.c 1,1                | [2] src/vi.h 1,1                  |
+-----------------------------------+-----------------------------------+
|:                                                                      |
+-----------------------------------------------------------------------+
```

| Command / Shortcut | Action |
|---|---|
| `:split` or `:sp` | Split the current window horizontally. |
| `:vsplit` or `:vs` | Split the current window vertically. |
| `:close` or `:clo` | Close the active split window (`Ctrl-W c`). |
| `:only` or `:on` | Close all other split windows except current (`Ctrl-W o`). |
| **`Ctrl-W w`** | Cycle focus to the next window. |
| **`Ctrl-W p`** | Cycle focus to the previous window. |
| **`Ctrl-W h`** | Move focus to the window on the **left**. |
| **`Ctrl-W l`** | Move focus to the window on the **right**. |
| **`Ctrl-W j`** | Move focus to the window **below**. |
| **`Ctrl-W k`** | Move focus to the window **above**. |
| **`Ctrl-W =`** | Balance all window dimensions equally. |

---

### Tab Pages & Top Tabline (`:tabnew`, `gt`, `gT`)

When working on distinct modules or tasks, tab pages allow you to organize independent viewports within a single Vi instance. Whenever two or more tabs are active, Vi renders a persistent tab bar along the top of your console:

```
[1] main.c | [2] vi.h* | [3] test.py
+-----------------------------------------------------------------------+
| #include "vi.h"                                                       |
|                                                                       |
| int main(int argc, char *argv[])                                      |
```

| Command / Keystroke | Description |
|---|---|
| `:tabnew [file]` / `:tabe [file]` | Create a new tab page (optionally opening `[file]`). |
| `:tabclose` / `:tabc` | Close the current tab page. |
| `:tabonly` / `:tabo` | Close all other tabs except the currently active one. |
| `:tabnext` or **`gt`** | Switch to the next tab page. |
| `:tabprev` or **`gT`** | Switch to the previous tab page. |

> [!TIP]
> In Normal mode, pressing **`gt`** cycles forward through your open tabs, and **`gT`** cycles backward. Each tab maintains its own active split windows and cursor positions!

---

### Side-by-Side Diff Mode & Visual Comparison (`vi -d`, `:diffsplit`)

Vi for Windows includes an embedded, high-performance line comparison engine (LCS algorithm) that visually highlights additions, deletions, and modifications side-by-side with synchronized scrolling.

```
+------------------------------------+------------------------------------+
|+Added line in file 1               |                                    |
| Common base line                   | Common base line                   |
|!ConfigPort = 8080                  |!ConfigPort = 9090                  |
|-Removed legacy line                |                                    |
+------------------------------------+------------------------------------+
```

#### Launching Diff Mode
- **From Command Line**:
  ```powershell
  vi -d config.prod.json config.staging.json
  ```
- **From Within Vi**:
  ```vim
  :e config.prod.json
  :diffsplit config.staging.json
  ```

#### Diff Gutter Markers & Colors
- **`+` (Green)**: Added line in the current buffer.
- **`-` (Red)**: Line deleted relative to the compared buffer.
- **`!` (Cyan / Yellow)**: Modified line with differences between files.

#### Diff Navigation & Synchronization
- **Synchronized Dual-Window Scrolling**: Scrolling up or down (`j`, `k`, `Ctrl-D`, `Ctrl-U`, mouse wheel) in either window automatically moves the other viewport in lockstep.
- **`:diffthis`**: Activate diff mode on the current window.
- **`:diffoff`**: Deactivate diff mode on the current window and restore standard view.
- **`:diffupdate`**: Re-run the diff algorithm after editing lines to update gutter indicators immediately.

---

### Indent-Based Code Folding (`zc`, `zo`, `za`, `zR`, `zM`)

Code folding lets you collapse large functions, classes, or nested blocks so you can navigate complex files effortlessly.

Enable indent folding in Command mode or in your `~/.virc`:
```vim
:set fdm=indent
```

When folded, Vi hides nested lines and renders an informative fold header banner:
```
+-- 42 lines folded: void process_request(HttpRequest *req) ------------
```

| Keystroke | Action |
|---|---|
| **`zc`** | **C**lose fold under cursor. |
| **`zo`** | **O**pen fold under cursor. |
| **`za`** | Toggle fold under cursor (**A**lternate open / closed). |
| **`zM`** | Close all folds across the entire file (**M**inimize). |
| **`zR`** | Open all folds across the entire file (**R**educe / Expand all). |

---

### Project Grep & Quickfix List Navigation (`:grep`, `:copen`)

Search across your entire codebase and step through matching locations with an interactive jump window:

#### 1. Non-Blocking Async Background Grep
Run a multi-file grep across your project files:
```vim
:grep InitEditor
```
- **Asynchronous Execution**: Grep runs in the background on a dedicated Windows worker thread (`CreateThread`). The editor **never freezes**—you can continue typing, navigating, editing other buffers, or opening files while the search executes in real time.
- **Live Animated Spinner**: While searching, the status bar displays a live animated spinner and running match counter:
  ```
  [Grep: "InitEditor" 14 found /]
  ```
- **Zero-Flicker Completion**: When the background grep finishes, the match count is announced on the status bar and the Quickfix list is automatically updated.

#### 2. Open and Navigate the Quickfix Window
```vim
:copen 10        " Open Quickfix window (10 lines high)
:cclose          " Close Quickfix window
```

Inside the Quickfix window:
- Move up and down with **`j`** and **`k`**.
- Press **`<Enter>`** on any match line to jump directly to the target file, line, and column!
- Jump without opening the window:
  - **`:cnext`** or **`:cn`**: Jump to next search result.
  - **`:cprev`** or **`:cp`**: Jump to previous search result.
  - **`:clist`**: List all recorded matches on the command line.

---

### In-Memory Fuzzy File Search (`:find <query>`)

Quickly locate files anywhere in your project tree without knowing their full paths:

```vim
:find buffer.c
:find test_
```
Vi recursively traverses the workspace hierarchy and opens the first matching file directly into an active buffer.

---

### Workspace Sessions (`:mksession`, `:source`, `vi -S`)

Save your entire working environment—including open buffers, split window geometry, tabs, active files, and cursor coordinates—so you can pick up exactly where you left off.

#### Saving a Session
```vim
:mksession                 " Saves to 'Session.vim' in current directory
:mksession myproject.vim   " Saves to custom path
```

#### Restoring a Session
- **From Command Line**:
  ```powershell
  vi -S
  vi -S myproject.vim
  ```
- **From Inside Vi**:
  ```vim
  :source Session.vim
  :so Session.vim
  ```

---

### In-Memory Multi-Buffer Management

Unlike basic command-line editors that force you to work on one file at a time, Vi for Windows manages multiple buffers in memory:

```vim
:ls              " List all open in-memory buffers
:b 2             " Switch to buffer #2
:b main.c        " Switch to buffer matching filename substring 'main.c'
:bn              " Switch to Next buffer
:bp              " Switch to Previous buffer
:bd              " Close (delete) the current buffer
:badd test.py    " Preload 'test.py' into background memory
```

---

### Persistent Undo Histories Across Restarts (`:set undofile`)

By default, editing histories are held in memory during your active editing session. With persistent undo enabled, Vi writes an atomic hidden undo journal (`.{filename}.un~`) alongside your file. When you close Vi, reboot your system, or reopen the file days later, you can still press **`u`** to undo changes made in previous sessions!

Enable persistent undo:
```vim
:set undofile       " Enable persistent undo for current session
:set noundofile     " Disable persistent undo
```
Or add `set undofile` to your `~/.virc` profile.

---

### Buffer-Word Autocompletion (`Ctrl-N` / `Ctrl-P`)

When writing code in **INSERT** or **REPLACE** mode:

- Type the first few characters of any variable, function, or keyword (e.g. `Buff`).
- Press **`Ctrl-N`**: Vi scans the current buffer and all background buffers, instantly proposing completions (e.g. `Buffer`, `BufferState`). Keep pressing `Ctrl-N` to cycle forward.
- Press **`Ctrl-P`**: Cycle backward through completion candidates.

---

### Code Navigation with CTags (`Ctrl-]` / `Ctrl-T`)

Vi integrates with standard `tags` files generated by Universal Ctags or Exuberant Ctags.

#### 1. Generate Tags for Your Repository
From your project's root folder in PowerShell:
```powershell
ctags -R .
```

#### 2. Jump Directly to Symbol Definitions
- Place your cursor on any function name, struct, or type (e.g. `window_split`).
- Press **`Ctrl-]`** (or type `:tag window_split`).
- Vi immediately jumps across files and positions your cursor on the declaration line!
- Press **`Ctrl-T`** (or type `:pop`) to pop back to where you were before the jump.
- Type **`:tags`** to inspect the active jump stack.

---

### Interactive Directory Explorer (`:Explore`, `vi <dir>`)

Open project directories in an interactive browser without leaving Vi:

```vim
:Explore         " Open file explorer in current window
:Sex             " Open file explorer in a new horizontal split
```

Or launch directly from your terminal:
```powershell
vi .
vi src
```

- Navigate files with `j` and `k`.
- Press **`<Enter>`** on a file to open it.
- Press **`<Enter>`** on a folder (or `../`) to navigate into that directory.

---

### Compiling, Testing & External Shell Commands (`:!<cmd>`)

Run your compilers, test runners, or git commands directly from Vi:

```vim
:!clang -Wall -Wextra src/*.c -o vi.exe
:!cargo build --release
:!go test -v ./...
:!pytest
:!git diff
```

---

### Interactive Subshell & Embedded Terminal (`:terminal`, `:sh`)

Need to run a sequence of commands, check git logs, or interact with a CLI utility without exiting Vi? Launch an interactive Windows subshell directly from Vi:

```vim
:terminal        " Launch interactive shell
:term            " Short alias
:sh              " Standard POSIX vi shell command
:shell           " Full alias
```

Vi immediately suspends console raw mode and spawns your default Windows shell (`powershell.exe` or `%COMSPEC%` `cmd.exe`). You can run any interactive scripts, run tests, or view git branches. When you are finished, simply type:
```powershell
exit
```
Vi instantly restores raw terminal mode, redraws your buffers and split windows, and places your cursor exactly where it was before launching the shell.

---

### Code Formatting with Filter Pipes (`:[range]!<cmd>`)

Format code using external CLI formatters without leaving the buffer:

```vim
:%!clang-format         " Format entire C/C++ file with clang-format
:%!rustfmt              " Format entire Rust file
:%!prettier --parser json " Format JSON buffer
:'<,'>!sort             " Sort visually highlighted lines alphabetically
```

---

### Windows System Clipboard Integration (`"+` and `"*`)

Vi for Windows interfaces directly with the native Windows Clipboard API via registers `"+` and `" *`:

- **Copy to Windows Clipboard**:
  - `"+yy`: Copy current line to Windows clipboard.
  - `"+y`: (In visual mode) Copy highlighted selection to Windows clipboard.
  - `"+yi"`: Copy inside quotes to Windows clipboard.
- **Paste from Windows Clipboard**:
  - `"+p`: Paste contents of Windows clipboard after cursor.
  - `"+P`: Paste contents of Windows clipboard before cursor.

---

## 5. Vi as a Windows System Administrator's Tool

### Why Vi on Windows Server & Core?

Windows Server environments (especially **Windows Server Core** and Azure minimal VMs) frequently lack a graphical user interface (GUI). Installing heavy desktop text editors is neither practical nor permitted in hardened infrastructure.

Vi for Windows gives system administrators:
1. **Single Portable Executable**: Keep `vi.exe` on a USB drive, network share, or Sysinternals-style admin folder.
2. **Native Console App**: Runs inside native PowerShell, Command Prompt, or headless OpenSSH sessions.
3. **Low Resource Footprint**: Uses ~5 MB of RAM and zero background daemons.
4. **CRLF Preservation**: Never corrupts Windows line endings or service config files.

---

### Remote Administration over Windows OpenSSH / PowerShell

When connected to a remote server over OpenSSH:
```powershell
ssh administrator@win-server-01.corp
```

Simply launch Vi in the remote shell:
```powershell
vi C:\Windows\System32\drivers\etc\hosts
vi C:\inetpub\wwwroot\web.config
```
Because Vi for Windows uses DEC 2026 synchronized output, remote rendering across SSH is lag-free and visually crisp.

---

### Comparing Config Files Side-by-Side with Diff Mode

Accidentally breaking a production service config (such as an IIS `web.config`, SQL connection string, or `nginx.conf`) can cause downtime. Vi's side-by-side diff mode lets you visually compare the active file against a backup copy before or after edits:

```powershell
vi -d C:\inetpub\wwwroot\web.config C:\inetpub\wwwroot\web.config.bak
```

- Changed lines are flagged with `!` in the gutter.
- Deleted lines in the backup appear with `-`.
- New lines in the active config appear with `+`.
- Scrolling in one window automatically scrolls the other in perfect synchronization.
- To compare an open file against an alternate config while inside Vi:
  ```vim
  :diffsplit C:\inetpub\wwwroot\web.config.staging
  ```

---

### External File Change Detection (`:set autoread`, `:checktime`)

In production environments, files are frequently updated out-of-band by automated deployment pipelines, Git pulls, or background agent processes.

Vi safeguards against accidentally overwriting external updates:

1. **Automatic Detection**: Enable `:set autoread` in Command mode or in your `~/.virc`:
   ```vim
   :set autoread
   ```
2. When Vi detects that the file on disk was modified by an external process, it notifies you and offers to reload the latest contents without losing your place:
   ```text
   File modified externally on disk. Reload? (y/n)
   ```
3. **Manual Check**: You can manually trigger a timestamp check at any time by running:
   ```vim
   :checktime
   ```

---

### Editing System Configuration Files (`hosts`, JSON, XML, INI, TOML, YAML)

#### Example 1: Editing the Windows `hosts` File
Open PowerShell as Administrator:
```powershell
vi C:\Windows\System32\drivers\etc\hosts
```
- Press **`G`** to jump to the bottom of the file.
- Press **`o`** to open a new line.
- Type: `192.168.1.100  internal-api.local`
- Press **`<Esc>`**, then **`:wq`** to save and exit.

#### Example 2: Inspecting and Editing `appsettings.json`
```powershell
vi appsettings.json
```
- Search for the database string: `/ConnectionStrings` `<Enter>`.
- Use **`ci"`** to quickly replace passwords or hostnames inside double quotes.
- Save and exit with **`ZZ`**.

---

### Authoring & Debugging PowerShell Scripts (`.ps1`, `.psm1`)

Vi provides syntax highlighting for PowerShell cmdlets, keywords, and `$variables`.

#### Workflow Tips for PowerShell Developers & Admins:
1. Open your script:
   ```powershell
   vi Deploy-Service.ps1
   ```
2. Auto-indentation is on by default—opening `{` and pressing `<Enter>` automatically indents code blocks.
3. **Execute the script live from within Vi**:
   ```vim
   :!powershell -ExecutionPolicy Bypass -File %
   ```
   *(The `%` symbol automatically expands to the current filename!)*
4. Return to editing immediately after the script finishes running.

---

### Authoring Windows Batch Files (`.bat`, `.cmd`)

Vi recognizes `.bat` and `.cmd` extensions, highlighting `echo`, `goto`, `call`, `rem`, and labels.

Quickly comment out blocks of batch script using **Visual Block Mode**:
1. Place cursor on the first command line.
2. Press **`Ctrl-V`**, press **`j`** down 5 lines.
3. Press **`I`**, type `REM `, and press **`<Esc>`**.
4. All 5 lines are now commented out.

---

### Rapid Log File Inspection & Analysis with Grep and Quickfix

When diagnosing IIS logs, Windows Event exports, or application crash dumps:

1. **Jump to End of Log**: Open a 100,000-line log and press **`G`** to jump to the latest entries instantly with zero delay.
2. **Multi-Log Grep & Quickfix List**: Search across multiple log files at once:
   ```vim
   :grep ERROR
   ```
   Open the matches in the Quickfix list with `:copen 10` and jump between problem points with `<Enter>` or `:cnext` / `:cprev`.
3. **Find Errors Quickly in Current File**:
   ```vim
   /ERROR
   /Exception
   /404
   ```
   Press **`n`** to step through each occurrence.
4. **Filter Log Lines with Native Windows CLI**:
   Extract only lines containing errors into your current buffer:
   ```vim
   :%!findstr /i "fatal exception"
   ```
5. **Sort and Deduplicate IP Addresses**:
   Highlight the IP list in visual line mode (`V`), then pipe through `sort`:
   ```vim
   :'<,'>!sort
   ```

---

### Data Cleaning & Column Extraction with Visual Block Mode

Suppose you have raw server output formatted as:
```text
192.168.1.10   SERVER-APP-01   ONLINE
192.168.1.11   SERVER-APP-02   ONLINE
192.168.1.12   SERVER-APP-03   OFFLINE
192.168.1.13   SERVER-APP-04   ONLINE
```

To delete the IP addresses:
1. Place cursor on row 1, col 1.
2. Press **`Ctrl-V`**.
3. Move right to the end of the IP addresses with **`e`**.
4. Move down to the last row with **`3j`**.
5. Press **`d`**. The IP column is cleanly stripped!

---

### Crash Protection & Swap Recovery on Unstable Connections

If a network glitch drops your SSH session while editing a crucial configuration:

1. Vi has been silently maintaining a real-time recovery journal (`.{filename}.swp`) on disk.
2. Reconnect to the server and open the file again:
   ```powershell
   vi web.config
   ```
3. Type:
   ```vim
   :recover
   ```
4. Vi restores all your unsaved modifications directly from the journal file. Save with `:w` to commit.

---

### Windows File Explorer Context Menu ("Open with Vi")

For workstations and desktop administration, you can register Vi directly in the Windows File Explorer context menu:

```powershell
.\install.ps1 -ContextMenu
```

Now, right-clicking any file in Windows Explorer displays **"Open with Vi"**, instantly opening the target file in a high-speed terminal window.

To unregister the context menu:
```powershell
.\install.ps1 -Uninstall
```

---

## 6. Configuration & Customization (`~/.virc`)

### Creating Your Configuration File

Vi automatically loads settings on startup from `%USERPROFILE%\.virc` (e.g. `C:\Users\YourName\.virc`).

To create or edit your `.virc`:
```powershell
vi ~\.virc
```

---

### Recommended Configuration for Developers

Add the following to your `~/.virc`:

```vim
" ==========================================
" Vi for Windows - Developer Profile
" ==========================================
set number          " Display line numbers
set autoindent      " Match indent of previous line
set incsearch       " Real-time live search match preview
set hlsearch        " Highlight search matches
set autoread        " Detect external changes on disk
set undofile        " Multi-session persistent undo history
set fdm=indent      " Indent-based code folding
set tabstop=4       " 4-space tabs
set shiftwidth=4    " 4-space indent shifts
set showmode        " Display -- INSERT --, -- VISUAL -- indicator
set colorscheme monokai " High-contrast developer theme
```

---

### Recommended Configuration for Sysadmins

Add the following to your `~/.virc`:

```vim
" ==========================================
" Vi for Windows - System Administrator Profile
" ==========================================
set ruler           " Status bar with line, column, and file name
set ignorecase      " Case-insensitive searching for fast log grep
set incsearch       " Live preview of search matches
set autoread        " Detect external updates from scripts/Git
set wrap            " Wrap long log lines on screen
set colorscheme dark" Clean dark theme
```

---

### Complete Options Reference Table

| Option | Short Form | Default | Description |
|---|---|---|---|
| `[no]number` | `nu` / `nonu` | `off` | Display line numbers along the left margin. |
| `[no]ignorecase` | `ic` / `noic` | `off` | Ignore case when searching patterns (`/` and `?`). |
| `[no]incsearch` | `is` / `nois` | `on` | Show search matches interactively while typing query. |
| `[no]wrap` | `wrap` / `nowrap` | `off` | Wrap lines visually instead of horizontal scrolling. |
| `[no]mouse` | `mouse=a` / `nomouse` | `on` | Enable ANSI/SGR mouse click, split scroll, and paste. |
| `[no]hlsearch` | `hls` / `nohls` | `off` | Keep search matches highlighted. Clear with `:nohl`. |
| `[no]autoindent` | `ai` / `noai` | `on` | Copy indentation level from previous line on `<Enter>`. |
| `[no]autoread` | `autoread` / `noautoread` | `off` | Automatically detect and prompt/reload files modified externally. |
| `[no]undofile` | `undofile` / `noundofile` | `off` | Persist multi-level undo history to hidden `.{file}.un~` files. |
| `foldmethod=TYPE` | `fdm=indent\|manual` | `manual` | Code fold calculation method (`indent` or `manual`). |
| `[no]showmode` | `sm` / `nosm` | `on` | Show current mode (`-- INSERT --`, `-- VISUAL --`) on status line. |
| `[no]ruler` | `ruler` / `noruler` | `off` | Show persistent cursor coordinates `(row, col)` bar. |
| `[no]syntax` | `syn` / `nosyn` | `on` | Enable syntax highlighting engine. |
| `[no]compatible`| `cp` / `nocp` | `off` | Toggle strict 1-level undo (`cp`) vs multi-level undo tree (`nocp`). |
| `tabstop=N` | `ts=N` | `4` | Number of spaces that a `<Tab>` character counts for. |
| `shiftwidth=N` | `sw=N` | `4` | Number of spaces to use for each step of `>` and `<` indentation. |
| `colorscheme=NAME`| `colo=NAME` | `dark` | Active theme: `dark`, `light`, `monokai`, `gruvbox`, `classic`. |
| `fileformat=TYPE`| `ff=dos\|unix` | Auto | Toggle line endings: `dos` (`CRLF`) or `unix` (`LF`). |

---

## 7. Emergency Survival Reference Card

Print this card or keep it pinned while learning:

```
================================================================================
                    VI 5.0 FOR WINDOWS SURVIVAL CARD
================================================================================

GETTING OUT OF TROUBLE
  <Esc> <Esc>      Return to Normal Mode safely
  :q! <Enter>      Abandon changes and force quit
  :wq <Enter>      Save and quit (or ZZ)

NAVIGATION (Normal Mode)
  h, j, k, l       Left, Down, Up, Right
  w, b, e          Next word, Previous word, End of word
  0, ^, $          Start of line, First non-blank, End of line
  gg, G            Top of file (line 1), Bottom of file (EOF)
  [num]G           Go to line [num] (e.g. 50G)
  Ctrl-F, Ctrl-B   Page Down, Page Up

INSERTING TEXT
  i, a             Insert before cursor, Append after cursor
  I, A             Insert at start of line, Append at end of line
  o, O             Open line below, Open line above

EDITING & CORRECTIONS
  x, dw, dd        Delete char, Delete word, Delete entire line
  yy, p, P         Yank (copy) line, Paste after, Paste before
  u, Ctrl-R        Undo, Redo (multi-session persistent if :set undofile)
  . (period)       Repeat last modifying command
  ci"              Change inside double quotes (text object)
  da(              Delete around parentheses (text object)

SEARCHING & REPLACING
  /pattern         Search forward (live preview)
  n, N             Next match, Previous match
  :%s/old/new/g    Replace all 'old' with 'new' in entire file
  :grep pattern    Search codebase recursively for pattern
  :copen / :cclose Open / close Quickfix jump list (Enter to jump)
  :cnext / :cprev  Jump to next / previous grep match
  :find <file>     Fuzzy search and open project file

SPLITS, TABS & DIFFS
  :split, :vsplit  Horizontal split, Vertical split
  Ctrl-W w / p     Cycle focus between windows (next / previous)
  Ctrl-W h/j/k/l   Move focus Left, Down, Up, Right
  :close, :only    Close window, Close all other windows
  :tabnew [file]   Open new tab page (gt / gT to cycle tabs)
  :diffsplit file  Side-by-side diff against file with sync scroll
  vi -d f1 f2      Launch CLI side-by-side diff mode

CODE FOLDING
  zc / zo / za     Close fold, Open fold, Toggle fold
  zM / zR          Close all folds, Open all folds (:set fdm=indent)

DEVELOPER & SYSADMIN TOOLS
  Ctrl-V           Visual Block mode (column editing / multi-line insert)
  Ctrl-] , Ctrl-T  Jump to symbol definition (CTags), Pop back
  :Explore, :Sex   Browse folder files and directory tree
  :terminal, :sh   Spawn interactive subshell without exiting Vi
  :mksession [fn]  Save workspace session (vi -S to resume)
  :checktime       Check if file was modified externally on disk
  :!command        Run shell command (e.g. :!cargo test, :!sort)
  :%!formatter     Pipe buffer through CLI tool (e.g. :%!clang-format)
  "+y , "+p        Copy to Windows clipboard, Paste from Windows clipboard
  :recover         Restore buffer from real-time crash journal (.swp)
================================================================================
```
