# Vi 5.0 for Windows

An ultra-responsive, zero-dependency release of **Vi** (version 5.0.0-2026), natively compiled for Windows Server 2022/2025 and Windows 10/11 x64.

📖 **Comprehensive Documentation**: See [**Vi User Guide (`vi-user-guide.md`)**](vi-user-guide.md) for a complete beginner walkthrough, developer workflows, and Windows system administration guide.

---

## Highlights

- **Pure C11 & Windows Console Virtual Terminal Sequences**: Single, self-contained binary (`vi.exe`, ~350 KB) with no external runtime or DLL dependencies beyond standard Windows system libraries (`user32.dll`).
- **Mouse & Scroll Wheel Support**: Full SGR mouse reporting (`:set mouse=a` / `:set nomouse`). Click to position cursor in any mode, scroll wheel up/down to scroll split windows smoothly, right-click to paste from Windows clipboard (`"+p`), and click tabline to switch tabs.
- **Extended Regular Expression Engine**: Advanced Vim-compatible regex support with `\zs` (match start) and `\ze` (match end) precision anchors, character classes (`\d`, `\D`, `\w`, `\W`, `\s`, `\S`), quantifiers (`\+`, `\?`, `*`), word boundaries (`\<`, `\>`), and character sets (`[...]`, `[^...]`).
- **Non-Blocking Async Background Grep**: Multithreaded `:grep <pattern>` running on background worker threads with an animated status bar spinner (`[Grep: "pattern" N found /]`) and auto-populated Quickfix list without UI freezing.
- **Side-by-Side Diff Mode**: Real-time LCS line comparison (`vi -d file1 file2`, `:diffsplit <file>`, `:diffthis`, `:diffoff`) with gutter change markers (`+`, `-`, `!`) and synchronized dual-window scrolling.
- **Tab Pages**: Multi-tab document management (`:tabnew [file]`, `:tabclose`, `:tabnext`, `:tabprev`, `:tabonly`, `gt`, `gT`) with a sleek top tabline.
- **Indent-Based Code Folding**: Collapse and expand code blocks (`zc`, `zo`, `za`, `zR`, `zM`, `:set fdm=indent`) with visual fold banners.
- **Persistent Multi-Session Undo**: Optional disk-persisted undo histories (`.{filename}.un~`, `:set undofile`) surviving editor restarts.
- **Workspace Session Persistence**: Save and restore complete editor states, window layouts, buffers, tabs, and cursor positions (`:mksession [path]`, `:source <path>`, `vi -S [path]`).
- **External File Modification Detection**: Automatically detect external disk modifications (`:set autoread`, `:checktime`) with non-destructive reload prompts.
- **Interactive Subshell & Terminal**: Launch a nested Windows shell directly inside Vi without losing editor state (`:terminal`, `:sh`).
- **In-Memory Fuzzy File Search**: Rapid recursive project file finder (`:find <pattern>`).
- **Windows Explorer Context Menu**: Seamless integration with the Windows Shell via `install.ps1 -ContextMenu` ("Open with Vi").
- **Vi Grammar**: Full support for `[count][operator][count][motion]` compositions (e.g. `d3w`, `y$`, `5j`, `c2e`, `>ip`).
- **Text Objects**: Inner and around objects for words (`iw`, `aw`), quotes (`i"`, `a"`, `i'`, `a'`, `i```, `a```), brackets (`i(`, `a(`, `i[`, `a[`, `i{`, `a{`, `i<`, `a<`).
- **Visual Block Mode**: Rectangular text selection (`Ctrl-V` / `Ctrl-Q`), block delete, block yank, block change, and block column insert/append (`I`, `A`).
- **Multi-Window Viewport Splits**: Horizontal and vertical splits (`:split`, `:vsplit`, `:close`, `:only`), window cycling (`Ctrl-W w`, `Ctrl-W p`), directional navigation (`Ctrl-W h/j/k/l`), and balancing (`Ctrl-W =`).
- **CTags Symbol Navigation**: Seamless cross-file code navigation using tags files (`Ctrl-]` to jump to definition, `Ctrl-T` to pop back, `:tag <symbol>`, `:pop`, `:tags`).
- **Integrated Directory Explorer**: Interactive netrw-style directory navigation (`:Explore`, `:Sex`, `:e <dir>`, `vi <dir>`) with file sizes, parent directory traversal (`../`), and opening files.
- **External Shell Filter Pipes**: Filter buffer ranges through native shell commands (e.g. `:%!sort`, `:'<,'>!findstr /v "temp"`).
- **Incremental Search**: Real-time live match preview as you type in search mode (`:set incsearch` / `:set is`).
- **5 Configurable Color Schemes**: Built-in curated themes (`:colorscheme <dark|light|monokai|gruvbox|classic>`).
- **Soft Line Wrapping Toggle**: Toggle virtual soft wrapping without modifying buffer lines (`:set wrap` / `:set nowrap`).
- **In-Memory Multi-Buffer Manager**: Edit multiple files simultaneously in RAM without disk re-reads (`:ls`, `:b`, `:bn`, `:bp`, `:bd`, `:badd`).
- **High-Performance Log Loading**: Geometric buffer capacity allocation ($O(N)$ dynamic scaling) for instant opening of large server log files.
- **Buffer-Word Autocompletion**: Zero-dependency `Ctrl-N` / `Ctrl-P` keyword autocompletion in Insert mode, scanning all active buffers.
- **Crash Recovery & Auto-Save Swap Journaling**: Real-time hidden swap files (`.{filename}.swp`) and `:recover` command.
- **CLI Startup Options & Read-Only View Mode**: Jump directly to line (`vi +142`), search pattern (`vi +/error`), or open in read-only mode (`vi -R` or `view`).
- **Read-Only File Protection**: Automatic detection of Windows `FILE_ATTRIBUTE_READONLY`, active `[RO]` status indicator, and write protection preventing accidental changes unless explicitly overridden (`:w!`).
- **Rich Syntax Highlighting**: Real-time syntax highlighting for 23 languages and formats:
  - **TOML** (`.toml`, `Cargo.toml`) with sections, key-value pairs, strings, booleans, numbers, and comments
  - **SQL** (`.sql`) with ANSI & T-SQL statements, queries, transactions, data types, and `--` / `/* */` comments
  - **XML / HTML / SVG** (`.html`, `.htm`, `.xhtml`, `.xml`, `.xaml`, `.svg`, `.config`, `.manifest`, `.wxs`, `.csproj`, `.vcxproj`) with tags, attributes, entities, `<!--...-->`, and `<?...?>`
  - **YAML** (`.yml`, `.yaml`) with keys, scalar values, document markers (`---`, `...`), `#` comments, and boolean/null literals
  - **INI / Windows Registry / INF** (`.ini`, `.inf`, `.reg`, `.cfg`, `.properties`, `.gitconfig`) with `[...]` sections, `;`/`#` comments, registry headers, and `key = value` pairs
  - **C / C++** (`.c`, `.h`, `.cpp`, `.hpp`, `.cc`, `.cxx`)
  - **Rust** (`.rs`) with keyword, type, comment, and `#[...]` attribute support
  - **Go** (`.go`) with keyword, type, comment, string, and backtick `` `...` `` raw strings
  - **Java** (`.java`) with keywords, types, `@` annotations, strings, and comments
  - **COBOL** (`.cbl`, `.cob`, `.cpy`) with case-insensitive verbs, fixed-format (col 7/1) & free-format (`*>`) comments, and hyphenated keywords
  - **Fortran** (`.f`, `.for`, `.f90`, `.f95`, `.f03`, `.f08`) with fixed-form (`c`, `*`), free-form (`!`), and `#` preprocessor support
  - **Pascal** (`.pas`, `.pp`, `.inc`, `.p`) with case-insensitive keywords, types, `//`, `{ ... }`, `(* ... *)`, and `{$...}` directives
  - **Swift** (`.swift`) with keywords, types, `#` compiler directives, and `@` attributes
  - **Objective-C** (`.m`, `.mm`) with `@interface`, `@property`, `@end`, `@"..."` strings, types, and `#import` preprocessor support
  - **JavaScript / TypeScript** (`.js`, `.mjs`, `.cjs`, `.jsx`, `.ts`, `.tsx`) with keywords, types, and template literals (`` `...` ``)
  - **Python** (`.py`, `.pyw`) with keywords, built-in types, and `@` decorator support
  - **Zig** (`.zig`, `.zon`) with keywords, types, multiline string literals (`\\`), and `@` built-ins
  - **Basic** (`.bas`, `.vbs`, `.vb`) with case-insensitive keywords/types, `'`, and `REM` comments
  - **PowerShell** (`.ps1`, `.psm1`, `.psd1`) with cmdlet keywords and `$var` highlighting
  - **Batch / CMD** (`.bat`, `.cmd`) with commands, `::`, and `REM` comments
  - **Shell** (`.sh`, `.bash`, `.zsh`)
  - **JSON** (`.json`)
  - **Markdown** (`.md`, `.markdown`)
- **Seamless Windows Clipboard Bridge**: Access the OS clipboard directly via `"+y`, `"+p`, `"*y`, `"*p`.
- **CRLF & LF Preservation**: Transparently detects and preserves Windows DOS (`CRLF`) and UNIX (`LF`) line endings.
- **Multi-Level Undo & Vi Toggle**: Default multi-level tree undo/redo (`u`, `Ctrl-R`), with 1-level toggle mode via `:set cp`.
- **Macros**: Record keystrokes into named registers (`q{a-z}`) and replay with `@a` or `@@`.
- **Flexible Startup Profiles**: Auto-loads configuration from `%USERPROFILE%\_virc`, `.virc`, `_vimrc`, or `.exrc`, supporting both `"` and `#` comments.

## Command-Line Usage

```powershell
vi [options] [file...]
view [file...]
```

| Flag / Option | Description | Example |
|---|---|---|
| `-d file1 file2` | Open two files in side-by-side split Diff mode with synchronized scrolling | `vi -d web.config web.config.bak` |
| `-S [session]` | Load and restore workspace session state from file (defaults to `Session.vim`) | `vi -S` or `vi -S myproj.session` |
| `+<line>` | Open file and jump directly to specified line number | `vi +142 C:\Logs\cluster.log` |
| `+/<pattern>` | Open file and jump directly to first match for pattern | `vi +/FATAL application.log` |
| `-R` or `-r` | Read-only mode: prevents accidental `:w` unless `:w!` forced | `vi -R C:\Windows\System32\drivers\etc\hosts` |
| `view <file>` | Dedicated read-only viewer alias (`view.exe`) | `view web.config` |
| `-v`, `--version` | Print Vi version and architecture information | `vi -v` |
| `-h`, `--help` | Display command-line help reference | `vi -h` |

---

## Buffer & File Management

| Command | Action |
|---|---|
| `:ls` or `:buffers` | List all open in-memory buffers with ID, status, and filename |
| `:b <id\|name>` | Switch to buffer by numeric ID or filename substring |
| `:bn` or `:bnext` | Switch to next buffer in memory |
| `:bp` or `:bprev` | Switch to previous buffer in memory |
| `:bd` or `:bdelete` | Delete current buffer (errors if dirty) |
| `:bd!` or `:bdelete!` | Force delete buffer, discarding unsaved modifications |
| `:badd <file>` | Add file to the buffer table |
| `:e <file>` | Open file in a new buffer or switch to it if already open |
| `:e!` | Reload current buffer from disk, discarding unsaved modifications |
| `:recover` | Restore buffer contents from disk swap file (`.{filename}.swp`) |
| `:w [file]` | Write buffer to disk (prevented if read-only) |
| `:w! [file]` | Force write buffer to disk (overrides read-only mode) |
| `:wq` or `:x` | Write buffer to disk and exit |
| `:wq!` or `:x!` | Force write buffer to disk and exit |
| `:q` / `:q!` | Quit (force quit without saving) |
| `:version` or `:ve` | Show Vi version (`Vi 5.0.0-2026`) |

## Split Windows & Navigation

| Command / Key | Action |
|---|---|
| `:split` or `:sp` | Split active window horizontally (`Ctrl-W s`) |
| `:vsplit` or `:vs` | Split active window vertically (`Ctrl-W v`) |
| `:close` or `:clo` | Close active split window (`Ctrl-W c`) |
| `:only` or `:on` | Close all other split windows (`Ctrl-W o`) |
| `Ctrl-W w` | Cycle cursor focus to next split window |
| `Ctrl-W p` | Cycle cursor focus to previous split window |
| `Ctrl-W h / j / k / l` | Directionally move cursor Left, Down, Up, Right across windows |
| `Ctrl-W =` | Balance all window heights equally |

---

## Tab Pages

| Command / Key | Action |
|---|---|
| `:tabnew [file]` / `:tabe` | Open a new tab page (optionally opening file) |
| `:tabclose` / `:tabc` | Close current tab page |
| `:tabnext` / `gt` | Switch to next tab page |
| `:tabprev` / `gT` | Switch to previous tab page |
| `:tabonly` / `:tabo` | Close all other tab pages except current |

---

## Diff Mode & File Change Detection

| Command / Flag | Action |
|---|---|
| `vi -d file1 file2` | CLI: Open two files in side-by-side Diff split with synchronized scrolling |
| `:diffsplit <file>` | Open file in vertical split and start diff comparison against current buffer |
| `:diffthis` | Enable diff comparison mode on the active window |
| `:diffoff` | Disable diff mode on current window |
| `:diffupdate` | Recalculate LCS differences and update gutter change signs (`+`, `-`, `!`) |
| `:checktime` | Check if current buffer file was modified externally on disk |
| `:set autoread` | Automatically check and prompt/reload files modified outside Vi |

---

## Project Search, Quickfix List & Fuzzy Finder

| Command / Key | Action |
|---|---|
| `:grep <pattern>` | Search project files recursively using native `findstr /s /n /i` |
| `:copen [height]` | Open the Quickfix list window (defaults to 8 lines) |
| `:cclose` | Close the Quickfix window |
| `:cnext` / `:cn` | Jump to next match in Quickfix list |
| `:cprev` / `:cp` | Jump to previous match in Quickfix list |
| `:clist` | List all quickfix matches on the command line |
| `<Enter>` *(in Quickfix)* | Jump directly to file, line, and column under cursor |
| `:find <query>` | In-memory recursive fuzzy file search in project tree |

---

## Indent-Based Code Folding

| Command / Key | Action |
|---|---|
| `:set fdm=indent` | Enable indentation-based code folding |
| `:set fdm=manual` | Disable automatic indent fold calculation |
| `zc` | Close fold under cursor |
| `zo` | Open fold under cursor |
| `za` | Toggle fold under cursor (open $\leftrightarrow$ close) |
| `zR` | Open all folds across the entire buffer |
| `zM` | Close all folds across the entire buffer |

---

## Workspace Sessions, Subshell & Persistent Undo

| Command / Option | Action |
|---|---|
| `:mksession [path]` / `:mks` | Save active workspace session (buffers, windows, tabs, cursor) |
| `:source <path>` / `:so` | Restore workspace session from file |
| `vi -S [path]` | CLI: Launch Vi and restore specified session (defaults to `Session.vim`) |
| `:terminal` / `:sh` | Suspend Vi and open interactive Windows shell (`powershell` / `cmd`) |
| `:set undofile` | Enable persistent multi-session undo journal files (`.{file}.un~`) |
| `:set noundofile` | Disable persistent undo file generation |

---

## CTags, Directory Explorer & Filter Pipes

| Command / Key | Action |
|---|---|
| `Ctrl-]` or `:tag <sym>` | Jump directly to definition of symbol under cursor |
| `Ctrl-T` or `:pop` | Pop tag stack and return to previous jump point |
| `:tags` | Display current CTags navigation call stack |
| `:Explore` or `:E` | Open interactive Directory Explorer in current window |
| `:Sex` | Open Directory Explorer in a new horizontal split |
| `vi <dir>` or `:e <dir>` | Open directory path directly into Directory Explorer |
| `:[range]!<command>` | Pipe range through external shell command (e.g. `:%!sort`) |

---

## Editor Options & Color Schemes

| Option Command | Action |
|---|---|
| `:set readonly` / `:set ro` | Enable read-only protection on active buffer |
| `:set noreadonly` / `:set noro` | Clear read-only protection on active buffer |
| `:set incsearch` / `:set is` | Enable live incremental search match preview |
| `:set noincsearch` / `:set nois` | Disable incremental search match preview |
| `:set wrap` / `:set nowrap` | Toggle soft line wrapping without mutating buffer |
| `:set mouse=a` / `:set nomouse` | Enable / disable ANSI/SGR mouse click, scroll, and paste |
| `:set autoread` / `:set noautoread` | Toggle automatic external file change detection |
| `:set undofile` / `:set noundofile` | Toggle persistent disk-backed undo histories |
| `:set fdm=indent` / `fdm=manual` | Set fold method: `indent` or `manual` |
| `:set number` / `:set nu` | Display line numbers |
| `:set nonumber` / `:set nonu` | Hide line numbers |
| `:set list` / `:set nolist` | Toggle display of hidden characters (tabs as `^I`, trailing `$`) |
| `:colorscheme <theme>` | Set color scheme: `dark`, `light`, `monokai`, `gruvbox`, `classic` |
| `:colo <theme>` | Short alias for `:colorscheme` |

---

## Autocompletion

While in **INSERT** or **REPLACE** mode:
- **`Ctrl-N`**: Look up and cycle forward through word completions matching the prefix before cursor.
- **`Ctrl-P`**: Cycle backward through completions.
- Words are dynamically harvested across the current buffer and all loaded background buffers.

---

## Building and Installing

### Prerequisites
- Windows Server 2022/2025 Core and Desktop + Windows 10/11 x64
- Any C compiler: **Clang** (recommended), **GCC** (MinGW-w64), or **MSVC** (`cl.exe`).

### Build
Run the PowerShell build script:
```powershell
.\build.ps1
```
Or use the batch script:
```cmd
build.bat
```

### Installation

#### Current User (No Admin Rights Needed)
```powershell
.\install.ps1
```
Installs `vi.exe` and `view.exe` to `%LOCALAPPDATA%\Programs\vi` and adds it to your User `PATH`.

#### System-Wide / Windows Server Core (All Users & Remote SSH / WinRM)
```powershell
.\install.ps1 -AllUsers
```
Installs `vi.exe` and `view.exe` to `%ProgramFiles%\vi` and registers it in the Machine `PATH` so all administrators, remote OpenSSH sessions, and PowerShell remoting endpoints have immediate access to `vi` and `view`.

#### Windows Explorer Context Menu ("Open with Vi")
```powershell
.\install.ps1 -ContextMenu
```
Integrates Vi into the Windows File Explorer right-click context menu under `HKCU\Software\Classes\*\shell\Open with Vi`, enabling you to right-click any file in Windows Explorer and open it immediately in Vi.

#### Uninstall
```powershell
.\install.ps1 -Uninstall
# Or for system-wide uninstall:
.\install.ps1 -AllUsers -Uninstall
```
