#include "vi.h"
#include "syntax.h"

static FileType s_filetype = FILETYPE_NONE;

static const char *C_KEYWORDS[] = {
    "if", "else", "while", "for", "do", "return", "switch", "case", "break",
    "continue", "default", "goto", "struct", "union", "enum", "typedef", "sizeof",
    "static", "extern", "const", "volatile", "inline", "register", "auto", NULL
};

static const char *C_TYPES[] = {
    "void", "char", "short", "int", "long", "float", "double", "signed", "unsigned",
    "bool", "size_t", "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t",
    "uint32_t", "int64_t", "uint64_t", "DWORD", "HANDLE", "FILE", "Line", "Editor",
    "true", "false", "NULL", NULL
};

static const char *PS_KEYWORDS[] = {
    "function", "param", "if", "else", "elseif", "foreach", "for", "while",
    "switch", "return", "exit", "try", "catch", "finally", "throw", "in",
    "Write-Host", "Write-Error", "Write-Output", "Get-ChildItem", "New-Item",
    "Remove-Item", "Copy-Item", "Test-Path", "Join-Path", "Split-Path", NULL
};

static const char *PY_KEYWORDS[] = {
    "def", "class", "import", "from", "as", "if", "elif", "else", "while",
    "for", "in", "try", "except", "finally", "raise", "return", "yield",
    "lambda", "with", "pass", "is", "not", "and", "or", "async", "await",
    "global", "nonlocal", "assert", "del", "match", "case", NULL
};

static const char *PY_TYPES[] = {
    "int", "float", "complex", "str", "bytes", "bytearray", "bool", "list",
    "dict", "set", "frozenset", "tuple", "range", "object", "type", "self",
    "cls", "super", "print", "len", "open", "isinstance", "issubclass",
    "enumerate", "zip", "map", "filter", "any", "all", "sum", "min", "max",
    "abs", "None", "True", "False", NULL
};

static const char *BAT_KEYWORDS[] = {
    "echo", "set", "if", "else", "for", "in", "do", "goto", "call", "exit",
    "pause", "cd", "dir", "copy", "del", "rem", "not", "exist", "errorlevel", NULL
};

static const char *BASIC_KEYWORDS[] = {
    "dim", "sub", "function", "end", "if", "then", "else", "elseif", "for", "to",
    "step", "next", "while", "wend", "do", "loop", "until", "print", "input",
    "as", "select", "case", "return", "exit", "const", "byval", "byref", "call",
    "goto", "on", "error", "resume", "set", "let", "mod", "and", "or", "not", NULL
};

static const char *BASIC_TYPES[] = {
    "string", "integer", "long", "single", "double", "boolean", "byte", "variant",
    "object", "true", "false", "nothing", "null", NULL
};

static const char *RUST_KEYWORDS[] = {
    "fn", "let", "mut", "struct", "enum", "impl", "trait", "pub", "use", "mod",
    "match", "if", "else", "while", "for", "loop", "return", "break", "continue",
    "crate", "super", "self", "Self", "where", "async", "await", "unsafe", "type",
    "dyn", "ref", "move", "const", "static", "extern", "as", "in", NULL
};

static const char *RUST_TYPES[] = {
    "i8", "i16", "i32", "i64", "i128", "isize", "u8", "u16", "u32", "u64",
    "u128", "usize", "f32", "f64", "bool", "char", "str", "String", "Vec",
    "Option", "Result", "Some", "None", "Ok", "Err", "Box", "Rc", "Arc",
    "true", "false", NULL
};

static const char *GO_KEYWORDS[] = {
    "package", "import", "func", "return", "var", "const", "type", "struct",
    "interface", "if", "else", "switch", "case", "default", "for", "range",
    "break", "continue", "go", "defer", "select", "chan", "map", "goto",
    "fallthrough", NULL
};

static const char *GO_TYPES[] = {
    "int", "int8", "int16", "int32", "int64", "uint", "uint8", "uint16", "uint32",
    "uint64", "uintptr", "float32", "float64", "complex64", "complex128", "string",
    "bool", "byte", "rune", "error", "any", "nil", "true", "false", "make", "new",
    "len", "cap", "append", "copy", "delete", "panic", "recover", NULL
};

static const char *JAVA_KEYWORDS[] = {
    "abstract", "assert", "break", "case", "catch", "class", "const",
    "continue", "default", "do", "else", "enum", "extends", "final",
    "finally", "for", "goto", "if", "implements", "import", "instanceof",
    "interface", "native", "new", "package", "private", "protected",
    "public", "return", "static", "strictfp", "super", "switch",
    "synchronized", "this", "throw", "throws", "transient", "try",
    "void", "volatile", "while", "record", "sealed", "non-sealed",
    "permits", "yield", "var", NULL
};

static const char *JAVA_TYPES[] = {
    "boolean", "byte", "char", "short", "int", "long", "float", "double",
    "Boolean", "Byte", "Character", "Short", "Integer", "Long", "Float", "Double",
    "String", "Object", "Class", "List", "ArrayList", "Map", "HashMap",
    "Set", "HashSet", "Optional", "Collection", "Arrays", "Collections",
    "Exception", "Throwable", "RuntimeException", "true", "false", "null", NULL
};

static const char *COBOL_KEYWORDS[] = {
    "identification", "division", "environment", "data", "working-storage",
    "procedure", "program-id", "section", "pic", "picture", "value", "values",
    "usage", "comp", "comp-1", "comp-2", "comp-3", "comp-4", "comp-5",
    "binary", "display", "is", "are", "perform", "thru", "through",
    "varying", "until", "times", "with", "test", "before", "after",
    "if", "then", "else", "end-if", "end-perform", "end-evaluate",
    "end-read", "end-write", "end-call", "end-compute", "evaluate",
    "when", "other", "move", "to", "add", "subtract", "multiply",
    "divide", "compute", "into", "giving", "remainder", "by", "open",
    "close", "read", "write", "rewrite", "delete", "start", "input",
    "output", "input-output", "extend", "call", "using", "returning",
    "cancel", "exit", "program", "stop", "run", "goback", "copy",
    "replace", "accept", "initialize", "inspect", "string", "unstring",
    "set", "up", "down", "search", "all", "at", "end", "not", "and",
    "or", "greater", "less", "equal", "than", "occurs", "depending",
    "on", "indexed", "index", "redefines", "renames", NULL
};

static const char *COBOL_TYPES[] = {
    "space", "spaces", "zero", "zeros", "zeroes", "quote", "quotes",
    "high-value", "high-values", "low-value", "low-values",
    "alphanumeric", "numeric", "alphabetic", "true", "false", NULL
};

static const char *FORTRAN_KEYWORDS[] = {
    "program", "module", "subroutine", "function", "end", "contains",
    "use", "implicit", "none", "call", "return", "entry", "include",
    "if", "then", "else", "elseif", "endif", "enddo", "do", "while",
    "cycle", "exit", "select", "case", "where", "elsewhere", "forall",
    "allocate", "deallocate", "nullify", "read", "write", "print",
    "open", "close", "inquire", "rewind", "backspace", "format",
    "intent", "in", "out", "inout", "dimension", "parameter",
    "allocatable", "pointer", "target", "public", "private", "interface",
    "type", "generic", "pure", "elemental", "recursive", "only",
    "operator", "assignment", "data", "save", "common", "block",
    "endprogram", "endmodule", "endsubroutine", "endfunction", "endtype",
    "endselect", "endwhere", "endforall", "endinterface", NULL
};

static const char *FORTRAN_TYPES[] = {
    "integer", "real", "double", "precision", "complex", "character",
    "logical", "kind", "len", "true", "false", "null", NULL
};

static const char *PASCAL_KEYWORDS[] = {
    "program", "unit", "interface", "implementation", "uses", "begin",
    "end", "var", "const", "type", "procedure", "function", "if",
    "then", "else", "case", "of", "for", "to", "downto", "do", "while",
    "repeat", "until", "with", "record", "array", "set", "file",
    "class", "object", "constructor", "destructor", "property",
    "inherited", "override", "virtual", "abstract", "reintroduce",
    "overload", "try", "finally", "except", "on", "raise", "break",
    "continue", "exit", "goto", "label", "inline", "operator",
    "packed", "initialization", "finalization", "resourcestring",
    "exports", "threadvar", "self", NULL
};

static const char *PASCAL_TYPES[] = {
    "integer", "cardinal", "shortint", "smallint", "longint", "int64",
    "byte", "word", "longword", "qword", "real", "single", "double",
    "extended", "comp", "currency", "boolean", "bytebool", "wordbool",
    "longbool", "char", "ansichar", "widechar", "string", "ansistring",
    "widestring", "unicodestring", "shortstring", "pointer", "pchar",
    "text", "textfile", "nil", "true", "false", "and", "or", "not",
    "xor", "shl", "shr", "div", "mod", "in", NULL
};

static const char *SWIFT_KEYWORDS[] = {
    "func", "let", "var", "class", "struct", "enum", "protocol",
    "extension", "init", "deinit", "subscript", "typealias",
    "associatedtype", "import", "public", "private", "fileprivate",
    "internal", "open", "static", "final", "mutating", "nonmutating",
    "override", "lazy", "weak", "unowned", "inout", "throws",
    "rethrows", "async", "await", "if", "else", "guard", "switch",
    "case", "default", "for", "in", "while", "repeat", "break",
    "continue", "return", "defer", "fallthrough", "do", "catch",
    "throw", "try", "where", "as", "is", "self", "Self", "super",
    "actor", "some", "any", "isolated", "nonisolated", "macro", NULL
};

static const char *SWIFT_TYPES[] = {
    "Int", "Int8", "Int16", "Int32", "Int64", "UInt", "UInt8",
    "UInt16", "UInt32", "UInt64", "Float", "Double", "Float80",
    "Bool", "String", "Character", "Array", "Dictionary", "Set",
    "Optional", "Any", "AnyObject", "Void", "Result", "Task",
    "true", "false", "nil", NULL
};

static const char *OBJC_KEYWORDS[] = {
    "@interface", "@implementation", "@end", "@protocol", "@property",
    "@synthesize", "@dynamic", "@public", "@protected", "@private",
    "@package", "@try", "@catch", "@finally", "@throw", "@selector",
    "@encode", "@synchronized", "@autoreleasepool", "@class",
    "@compatibility_alias", "if", "else", "while", "for", "do",
    "return", "switch", "case", "break", "continue", "default", "goto",
    "struct", "union", "enum", "typedef", "sizeof", "static", "extern",
    "const", "volatile", "inline", "self", "super", "nil", "Nil",
    "YES", "NO", "strong", "weak", "assign", "copy", "readonly",
    "readwrite", "nonatomic", "atomic", NULL
};

static const char *OBJC_TYPES[] = {
    "void", "char", "short", "int", "long", "float", "double", "signed",
    "unsigned", "bool", "size_t", "id", "Class", "SEL", "IMP", "BOOL",
    "instancetype", "NSInteger", "NSUInteger", "CGFloat", "NSTimeInterval",
    "NSString", "NSArray", "NSDictionary", "NSSet", "NSNumber", "NSData",
    "NSObject", "NSError", "NSURL", "NSMutableArray", "NSMutableDictionary",
    "true", "false", "NULL", NULL
};

static const char *JS_KEYWORDS[] = {
    "function", "var", "let", "const", "return", "if", "else",
    "switch", "case", "default", "break", "continue", "for", "while",
    "do", "in", "of", "try", "catch", "finally", "throw", "class",
    "extends", "constructor", "static", "super", "this", "new",
    "delete", "typeof", "instanceof", "void", "import", "export",
    "from", "as", "async", "await", "yield", "debugger", "with",
    "type", "interface", "declare", "implements", "enum", "namespace",
    "package", "private", "protected", "public", "readonly", NULL
};

static const char *JS_TYPES[] = {
    "Object", "Array", "String", "Number", "Boolean", "Symbol", "BigInt",
    "Function", "Promise", "Map", "Set", "WeakMap", "WeakSet", "Date",
    "RegExp", "Error", "JSON", "Math", "console", "window", "document",
    "global", "process", "any", "string", "number", "boolean", "unknown",
    "never", "true", "false", "null", "undefined", "NaN", "Infinity", NULL
};

static const char *ZIG_KEYWORDS[] = {
    "const", "var", "fn", "pub", "export", "extern", "return", "if",
    "else", "switch", "while", "for", "break", "continue", "defer",
    "errdefer", "struct", "enum", "union", "error", "opaque", "try",
    "catch", "orelse", "unreachable", "async", "await", "suspend",
    "resume", "comptime", "inline", "noinline", "threadlocal", "test",
    "usingnamespace", "and", "or", "align", "volatile", "allowzero",
    "asm", "callconv", "anytype", "anyframe", NULL
};

static const char *ZIG_TYPES[] = {
    "void", "bool", "noreturn", "type", "anyerror", "i8", "u8", "i16",
    "u16", "i32", "u32", "i64", "u64", "i128", "u128", "isize", "usize",
    "f16", "f32", "f64", "f80", "f128", "c_char", "c_short", "c_ushort",
    "c_int", "c_uint", "c_long", "c_ulong", "c_longlong", "c_ulonglong",
    "true", "false", "null", "undefined", NULL
};

static const char *YAML_TYPES[] = {
    "true", "false", "yes", "no", "on", "off", "null", "~",
    "True", "False", "Yes", "No", "On", "Off", "Null", NULL
};

static const char *SQL_KEYWORDS[] = {
    "select", "insert", "update", "delete", "from", "where", "join", "left",
    "right", "inner", "outer", "full", "cross", "on", "group", "by", "order",
    "having", "limit", "offset", "top", "distinct", "union", "all", "as", "into",
    "values", "set", "create", "alter", "drop", "table", "view", "index", "trigger",
    "procedure", "function", "database", "schema", "primary", "key", "foreign",
    "references", "check", "default", "constraint", "cascade", "and", "or", "not",
    "in", "is", "null", "like", "between", "exists", "case", "when", "then", "else",
    "end", "begin", "commit", "rollback", "transaction", "grant", "revoke", NULL
};

static const char *SQL_TYPES[] = {
    "int", "integer", "tinyint", "smallint", "bigint", "float", "real", "double",
    "decimal", "numeric", "char", "varchar", "nchar", "nvarchar", "text", "ntext",
    "date", "time", "datetime", "datetime2", "timestamp", "blob", "boolean", "bool",
    "binary", "varbinary", "uniqueidentifier", "json", "xml", NULL
};

static const char *TOML_KEYWORDS[] = {
    "true", "false", "inf", "nan", NULL
};

static bool is_separator_ft(char c, FileType ft) {
    if (isspace((unsigned char)c) || c == '\0') return true;
    if (ft == FILETYPE_COBOL && c == '-') return false;
    return strchr(",.()+-/*=~%<>[];:!&|^?{}\\", c) != NULL;
}

void syntax_detect_filetype(void) {
    s_filetype = FILETYPE_NONE;
    if (!E.filename) return;

    const char *dot = strrchr(E.filename, '.');
    if (!dot) return;

    if (_stricmp(dot, ".c") == 0 || _stricmp(dot, ".h") == 0 ||
        _stricmp(dot, ".cpp") == 0 || _stricmp(dot, ".hpp") == 0 ||
        _stricmp(dot, ".cc") == 0 || _stricmp(dot, ".cxx") == 0) {
        s_filetype = FILETYPE_C;
    } else if (_stricmp(dot, ".ps1") == 0 || _stricmp(dot, ".psm1") == 0 ||
               _stricmp(dot, ".psd1") == 0) {
        s_filetype = FILETYPE_POWERSHELL;
    } else if (_stricmp(dot, ".bat") == 0 || _stricmp(dot, ".cmd") == 0) {
        s_filetype = FILETYPE_BATCH;
    } else if (_stricmp(dot, ".py") == 0 || _stricmp(dot, ".pyw") == 0) {
        s_filetype = FILETYPE_PYTHON;
    } else if (_stricmp(dot, ".sh") == 0 || _stricmp(dot, ".bash") == 0 ||
               _stricmp(dot, ".zsh") == 0) {
        s_filetype = FILETYPE_SHELL;
    } else if (_stricmp(dot, ".json") == 0) {
        s_filetype = FILETYPE_JSON;
    } else if (_stricmp(dot, ".md") == 0 || _stricmp(dot, ".markdown") == 0) {
        s_filetype = FILETYPE_MARKDOWN;
    } else if (_stricmp(dot, ".bas") == 0 || _stricmp(dot, ".vbs") == 0 ||
               _stricmp(dot, ".vb") == 0) {
        s_filetype = FILETYPE_BASIC;
    } else if (_stricmp(dot, ".rs") == 0) {
        s_filetype = FILETYPE_RUST;
    } else if (_stricmp(dot, ".go") == 0) {
        s_filetype = FILETYPE_GO;
    } else if (_stricmp(dot, ".java") == 0) {
        s_filetype = FILETYPE_JAVA;
    } else if (_stricmp(dot, ".cbl") == 0 || _stricmp(dot, ".cob") == 0 ||
               _stricmp(dot, ".cpy") == 0) {
        s_filetype = FILETYPE_COBOL;
    } else if (_stricmp(dot, ".f") == 0 || _stricmp(dot, ".for") == 0 ||
               _stricmp(dot, ".f90") == 0 || _stricmp(dot, ".f95") == 0 ||
               _stricmp(dot, ".f03") == 0 || _stricmp(dot, ".f08") == 0) {
        s_filetype = FILETYPE_FORTRAN;
    } else if (_stricmp(dot, ".pas") == 0 || _stricmp(dot, ".pp") == 0 ||
               _stricmp(dot, ".inc") == 0 || _stricmp(dot, ".p") == 0) {
        s_filetype = FILETYPE_PASCAL;
    } else if (_stricmp(dot, ".swift") == 0) {
        s_filetype = FILETYPE_SWIFT;
    } else if (_stricmp(dot, ".m") == 0 || _stricmp(dot, ".mm") == 0) {
        s_filetype = FILETYPE_OBJC;
    } else if (_stricmp(dot, ".js") == 0 || _stricmp(dot, ".mjs") == 0 ||
               _stricmp(dot, ".cjs") == 0 || _stricmp(dot, ".jsx") == 0 ||
               _stricmp(dot, ".ts") == 0 || _stricmp(dot, ".tsx") == 0) {
        s_filetype = FILETYPE_JAVASCRIPT;
    } else if (_stricmp(dot, ".zig") == 0 || _stricmp(dot, ".zon") == 0) {
        s_filetype = FILETYPE_ZIG;
    } else if (_stricmp(dot, ".xml") == 0 || _stricmp(dot, ".config") == 0 ||
               _stricmp(dot, ".manifest") == 0 || _stricmp(dot, ".xaml") == 0 ||
               _stricmp(dot, ".svg") == 0 || _stricmp(dot, ".html") == 0 ||
               _stricmp(dot, ".htm") == 0 || _stricmp(dot, ".csproj") == 0 ||
               _stricmp(dot, ".vbproj") == 0 || _stricmp(dot, ".props") == 0 ||
               _stricmp(dot, ".targets") == 0 || _stricmp(dot, ".nuspec") == 0 ||
               _stricmp(dot, ".resx") == 0 || _stricmp(dot, ".plist") == 0) {
        s_filetype = FILETYPE_XML;
    } else if (_stricmp(dot, ".yaml") == 0 || _stricmp(dot, ".yml") == 0) {
        s_filetype = FILETYPE_YAML;
    } else if (_stricmp(dot, ".ini") == 0 || _stricmp(dot, ".inf") == 0 ||
               _stricmp(dot, ".reg") == 0) {
        s_filetype = FILETYPE_INI;
    } else if (_stricmp(dot, ".toml") == 0) {
        s_filetype = FILETYPE_TOML;
    } else if (_stricmp(dot, ".sql") == 0) {
        s_filetype = FILETYPE_SQL;
    }
}

typedef enum {
    SCHEME_DARK = 0,
    SCHEME_LIGHT,
    SCHEME_MONOKAI,
    SCHEME_GRUVBOX,
    SCHEME_CLASSIC
} ColorScheme;

static ColorScheme s_colorscheme = SCHEME_DARK;

bool syntax_set_colorscheme(const char *name) {
    if (!name || *name == '\0') return false;
    if (_stricmp(name, "dark") == 0) s_colorscheme = SCHEME_DARK;
    else if (_stricmp(name, "light") == 0) s_colorscheme = SCHEME_LIGHT;
    else if (_stricmp(name, "monokai") == 0) s_colorscheme = SCHEME_MONOKAI;
    else if (_stricmp(name, "gruvbox") == 0) s_colorscheme = SCHEME_GRUVBOX;
    else if (_stricmp(name, "classic") == 0) s_colorscheme = SCHEME_CLASSIC;
    else return false;
    strncpy(E.opt.colorscheme, name, sizeof(E.opt.colorscheme) - 1);
    E.opt.colorscheme[sizeof(E.opt.colorscheme) - 1] = '\0';
    return true;
}

const char *syntax_get_colorscheme(void) {
    switch (s_colorscheme) {
        case SCHEME_LIGHT: return "light";
        case SCHEME_MONOKAI: return "monokai";
        case SCHEME_GRUVBOX: return "gruvbox";
        case SCHEME_CLASSIC: return "classic";
        default: return "dark";
    }
}

const char *syntax_color_code(unsigned char hl_type) {
    if (s_colorscheme == SCHEME_LIGHT) {
        switch (hl_type) {
            case HL_KEYWORD: return "\x1b[34m";    /* Blue */
            case HL_TYPE:    return "\x1b[32m";    /* Dark Green */
            case HL_STRING:  return "\x1b[31m";    /* Red / Brown */
            case HL_NUMBER:  return "\x1b[35m";    /* Purple */
            case HL_COMMENT: return "\x1b[90m";    /* Gray */
            case HL_PREPROC: return "\x1b[36m";    /* Cyan */
            case HL_HEADING: return "\x1b[1;30m";  /* Bold Black */
            case HL_DIFF_CHANGE: return "\x1b[33m"; /* Yellow */
            case HL_DIFF_ADD:    return "\x1b[32m"; /* Green */
            case HL_DIFF_DELETE: return "\x1b[31m"; /* Red */
            default:         return "\x1b[0m";
        }
    } else if (s_colorscheme == SCHEME_MONOKAI) {
        switch (hl_type) {
            case HL_KEYWORD: return "\x1b[38;5;197m"; /* Vibrant Pink */
            case HL_TYPE:    return "\x1b[38;5;81m";  /* Light Blue/Cyan */
            case HL_STRING:  return "\x1b[38;5;186m"; /* Soft Yellow */
            case HL_NUMBER:  return "\x1b[38;5;141m"; /* Lavender */
            case HL_COMMENT: return "\x1b[38;5;242m"; /* Slate */
            case HL_PREPROC: return "\x1b[38;5;208m"; /* Orange */
            case HL_HEADING: return "\x1b[1;38;5;197m";
            case HL_DIFF_CHANGE: return "\x1b[38;5;221m"; /* Yellow */
            case HL_DIFF_ADD:    return "\x1b[38;5;148m"; /* Bright Green */
            case HL_DIFF_DELETE: return "\x1b[38;5;197m"; /* Red */
            default:         return "\x1b[0m";
        }
    } else if (s_colorscheme == SCHEME_GRUVBOX) {
        switch (hl_type) {
            case HL_KEYWORD: return "\x1b[38;5;208m"; /* Gruvbox Orange */
            case HL_TYPE:    return "\x1b[38;5;214m"; /* Gruvbox Yellow */
            case HL_STRING:  return "\x1b[38;5;142m"; /* Gruvbox Green */
            case HL_NUMBER:  return "\x1b[38;5;175m"; /* Gruvbox Purple */
            case HL_COMMENT: return "\x1b[38;5;245m"; /* Gruvbox Gray */
            case HL_PREPROC: return "\x1b[38;5;108m"; /* Gruvbox Aqua */
            case HL_HEADING: return "\x1b[1;38;5;214m";
            case HL_DIFF_CHANGE: return "\x1b[38;5;214m"; /* Yellow */
            case HL_DIFF_ADD:    return "\x1b[38;5;142m"; /* Green */
            case HL_DIFF_DELETE: return "\x1b[38;5;167m"; /* Red */
            default:         return "\x1b[0m";
        }
    } else if (s_colorscheme == SCHEME_CLASSIC) {
        switch (hl_type) {
            case HL_KEYWORD: return "\x1b[33m";    /* Yellow */
            case HL_TYPE:    return "\x1b[36m";    /* Cyan */
            case HL_STRING:  return "\x1b[32m";    /* Green */
            case HL_NUMBER:  return "\x1b[31m";    /* Red */
            case HL_COMMENT: return "\x1b[34m";    /* Blue */
            case HL_PREPROC: return "\x1b[35m";    /* Magenta */
            case HL_HEADING: return "\x1b[1;37m";  /* Bold White */
            case HL_DIFF_CHANGE: return "\x1b[33m"; /* Yellow */
            case HL_DIFF_ADD:    return "\x1b[32m"; /* Green */
            case HL_DIFF_DELETE: return "\x1b[31m"; /* Red */
            default:         return "\x1b[0m";
        }
    } else {
        switch (hl_type) {
            case HL_KEYWORD: return "\x1b[36m";    /* Cyan */
            case HL_TYPE:    return "\x1b[32m";    /* Green */
            case HL_STRING:  return "\x1b[33m";    /* Yellow */
            case HL_NUMBER:  return "\x1b[35m";    /* Magenta */
            case HL_COMMENT: return "\x1b[90m";    /* Bright Black / Gray */
            case HL_PREPROC: return "\x1b[94m";    /* Bright Blue */
            case HL_HEADING: return "\x1b[1;37m";  /* Bold White */
            case HL_DIFF_CHANGE: return "\x1b[33m"; /* Yellow */
            case HL_DIFF_ADD:    return "\x1b[32m"; /* Green */
            case HL_DIFF_DELETE: return "\x1b[31m"; /* Red */
            default:         return "\x1b[0m";
        }
    }
}

void syntax_highlight_row(int filerow, unsigned char *hl_types) {
    if (filerow < 0 || filerow >= E.num_lines) return;
    Line *line = &E.lines[filerow];
    int len = line->len;
    memset(hl_types, HL_NORMAL, len);

    if (s_filetype == FILETYPE_NONE) return;

    /* Markdown Heading */
    if (s_filetype == FILETYPE_MARKDOWN && len > 0 && line->chars[0] == '#') {
        for (int i = 0; i < len; i++) hl_types[i] = HL_HEADING;
        return;
    }

    int i = 0;
    bool in_string = false;
    char str_delim = 0;

    /* Batch REM / :: comment check */
    if (s_filetype == FILETYPE_BATCH) {
        int nonspace = 0;
        while (nonspace < len && isspace((unsigned char)line->chars[nonspace])) nonspace++;
        if (nonspace < len - 1 && line->chars[nonspace] == ':' && line->chars[nonspace + 1] == ':') {
            for (int k = nonspace; k < len; k++) hl_types[k] = HL_COMMENT;
            return;
        }
        if (nonspace <= len - 3 && _strnicmp(&line->chars[nonspace], "rem", 3) == 0 &&
            (nonspace + 3 == len || isspace((unsigned char)line->chars[nonspace + 3]))) {
            for (int k = nonspace; k < len; k++) hl_types[k] = HL_COMMENT;
            return;
        }
    }

    /* COBOL fixed-format comments (col 7 or col 1) */
    if (s_filetype == FILETYPE_COBOL) {
        if (len > 6 && (line->chars[6] == '*' || line->chars[6] == '/')) {
            for (int k = 0; k < len; k++) hl_types[k] = HL_COMMENT;
            return;
        }
        if (len > 0 && (line->chars[0] == '*' || line->chars[0] == '/')) {
            for (int k = 0; k < len; k++) hl_types[k] = HL_COMMENT;
            return;
        }
    }

    /* Fortran fixed-format comments (col 1 c, C, *, !) */
    if (s_filetype == FILETYPE_FORTRAN && len > 0) {
        char c0 = line->chars[0];
        if (c0 == 'c' || c0 == 'C' || c0 == '*' || c0 == '!') {
            for (int k = 0; k < len; k++) hl_types[k] = HL_COMMENT;
            return;
        }
    }

    /* C/C++, Objective-C, and Fortran Preprocessor directive */
    if (s_filetype == FILETYPE_C || s_filetype == FILETYPE_OBJC || s_filetype == FILETYPE_FORTRAN) {
        int nonspace = 0;
        while (nonspace < len && isspace((unsigned char)line->chars[nonspace])) nonspace++;
        if (nonspace < len && line->chars[nonspace] == '#') {
            for (int k = nonspace; k < len; k++) hl_types[k] = HL_PREPROC;
            return;
        }
    }

    while (i < len) {
        char c = line->chars[i];

        /* String processing */
        if (in_string) {
            hl_types[i++] = HL_STRING;
            if (c == '\\' && i < len) {
                hl_types[i++] = HL_STRING;
                continue;
            }
            if (c == str_delim) in_string = false;
            continue;
        }

        /* Block comments */
        if ((s_filetype == FILETYPE_C || s_filetype == FILETYPE_RUST || s_filetype == FILETYPE_GO ||
             s_filetype == FILETYPE_JAVA || s_filetype == FILETYPE_SWIFT || s_filetype == FILETYPE_OBJC ||
             s_filetype == FILETYPE_JAVASCRIPT || s_filetype == FILETYPE_ZIG || s_filetype == FILETYPE_SQL) &&
            c == '/' && i + 1 < len && line->chars[i + 1] == '*') {
            hl_types[i++] = HL_COMMENT;
            hl_types[i++] = HL_COMMENT;
            while (i < len) {
                if (line->chars[i] == '*' && i + 1 < len && line->chars[i + 1] == '/') {
                    hl_types[i++] = HL_COMMENT;
                    hl_types[i++] = HL_COMMENT;
                    break;
                }
                hl_types[i++] = HL_COMMENT;
            }
            continue;
        }

        /* Line comments // */
        if ((s_filetype == FILETYPE_C || s_filetype == FILETYPE_RUST || s_filetype == FILETYPE_GO ||
             s_filetype == FILETYPE_JAVA || s_filetype == FILETYPE_SWIFT || s_filetype == FILETYPE_OBJC ||
             s_filetype == FILETYPE_JAVASCRIPT || s_filetype == FILETYPE_ZIG || s_filetype == FILETYPE_PASCAL) &&
            c == '/' && i + 1 < len && line->chars[i + 1] == '/') {
            while (i < len) hl_types[i++] = HL_COMMENT;
            break;
        }

        /* SQL line comments -- */
        if (s_filetype == FILETYPE_SQL && c == '-' && i + 1 < len && line->chars[i + 1] == '-') {
            while (i < len) hl_types[i++] = HL_COMMENT;
            break;
        }

        /* Line comments # */
        if ((s_filetype == FILETYPE_PYTHON || s_filetype == FILETYPE_SHELL ||
             s_filetype == FILETYPE_POWERSHELL || s_filetype == FILETYPE_YAML ||
             s_filetype == FILETYPE_TOML) && c == '#') {
            while (i < len) hl_types[i++] = HL_COMMENT;
            break;
        }

        /* INI comments ; and # */
        if (s_filetype == FILETYPE_INI && (c == ';' || c == '#')) {
            while (i < len) hl_types[i++] = HL_COMMENT;
            break;
        }

        /* XML comments <!-- ... --> */
        if (s_filetype == FILETYPE_XML && c == '<' && i + 3 < len &&
            line->chars[i + 1] == '!' && line->chars[i + 2] == '-' && line->chars[i + 3] == '-') {
            while (i < len) {
                hl_types[i] = HL_COMMENT;
                if (line->chars[i] == '-' && i + 2 < len && line->chars[i + 1] == '-' && line->chars[i + 2] == '>') {
                    hl_types[i + 1] = HL_COMMENT;
                    hl_types[i + 2] = HL_COMMENT;
                    i += 3;
                    break;
                }
                i++;
            }
            continue;
        }

        /* XML processing instructions / DTD */
        if (s_filetype == FILETYPE_XML && c == '<' && i + 1 < len &&
            (line->chars[i + 1] == '?' || line->chars[i + 1] == '!')) {
            while (i < len) {
                hl_types[i] = HL_PREPROC;
                if (line->chars[i] == '>') { i++; break; }
                i++;
            }
            continue;
        }

        /* XML tags <tag or </tag */
        if (s_filetype == FILETYPE_XML && c == '<') {
            hl_types[i++] = HL_KEYWORD;
            if (i < len && line->chars[i] == '/') hl_types[i++] = HL_KEYWORD;
            while (i < len && (isalnum((unsigned char)line->chars[i]) || line->chars[i] == '_' ||
                               line->chars[i] == '-' || line->chars[i] == ':' || line->chars[i] == '.')) {
                hl_types[i++] = HL_KEYWORD;
            }
            continue;
        }
        if (s_filetype == FILETYPE_XML && (c == '>' || (c == '/' && i + 1 < len && line->chars[i + 1] == '>'))) {
            if (c == '/') hl_types[i++] = HL_KEYWORD;
            if (i < len) hl_types[i++] = HL_KEYWORD;
            continue;
        }

        /* XML entity &name; */
        if (s_filetype == FILETYPE_XML && c == '&') {
            int k = i + 1;
            while (k < len && k < i + 10 && line->chars[k] != ';' && !isspace((unsigned char)line->chars[k])) k++;
            if (k < len && line->chars[k] == ';') {
                while (i <= k) hl_types[i++] = HL_PREPROC;
                continue;
            }
        }

        /* XML attribute name before = */
        if (s_filetype == FILETYPE_XML && (isalpha((unsigned char)c) || c == '_' || c == ':')) {
            int k = i;
            while (k < len && (isalnum((unsigned char)line->chars[k]) || line->chars[k] == '_' ||
                               line->chars[k] == '-' || line->chars[k] == ':')) k++;
            int eq = k;
            while (eq < len && isspace((unsigned char)line->chars[eq])) eq++;
            if (eq < len && line->chars[eq] == '=') {
                while (i < k) hl_types[i++] = HL_TYPE;
                continue;
            }
        }

        /* YAML document markers --- or ... */
        if (s_filetype == FILETYPE_YAML && (strncmp(&line->chars[i], "---", 3) == 0 || strncmp(&line->chars[i], "...", 3) == 0)) {
            while (i < len && !isspace((unsigned char)line->chars[i])) hl_types[i++] = HL_PREPROC;
            continue;
        }

        /* YAML key before colon */
        if (s_filetype == FILETYPE_YAML && (i == 0 || isspace((unsigned char)line->chars[i - 1]) || line->chars[i - 1] == '-')) {
            int k = i;
            while (k < len && (isalnum((unsigned char)line->chars[k]) || line->chars[k] == '_' || line->chars[k] == '-')) k++;
            if (k > i && k < len && line->chars[k] == ':' && (k + 1 == len || isspace((unsigned char)line->chars[k + 1]))) {
                while (i < k) hl_types[i++] = HL_KEYWORD;
                hl_types[i++] = HL_KEYWORD;
                continue;
            }
        }

        /* INI section header [Section] */
        if (s_filetype == FILETYPE_INI && c == '[') {
            while (i < len) {
                hl_types[i] = HL_KEYWORD;
                if (line->chars[i] == ']') { i++; break; }
                i++;
            }
            continue;
        }

        /* Registry file header */
        if (s_filetype == FILETYPE_INI && i == 0 &&
            (_strnicmp(&line->chars[i], "Windows Registry Editor", 23) == 0 ||
             _strnicmp(&line->chars[i], "REGEDIT", 7) == 0)) {
            while (i < len) hl_types[i++] = HL_PREPROC;
            break;
        }

        /* Registry dword/hex value */
        if (s_filetype == FILETYPE_INI && (_strnicmp(&line->chars[i], "dword:", 6) == 0 || _strnicmp(&line->chars[i], "hex:", 4) == 0)) {
            while (i < len && !isspace((unsigned char)line->chars[i]) && line->chars[i] != ';') {
                hl_types[i++] = HL_NUMBER;
            }
            continue;
        }

        /* INI / REG key before = */
        if (s_filetype == FILETYPE_INI && (i == 0 || isspace((unsigned char)line->chars[i - 1]))) {
            int k = i;
            while (k < len && line->chars[k] != '=' && line->chars[k] != ';' && line->chars[k] != '#') k++;
            if (k > i && k < len && line->chars[k] == '=') {
                while (i < k) {
                    if (line->chars[i] == '"') {
                        hl_types[i++] = HL_STRING;
                        while (i < k && line->chars[i] != '"') hl_types[i++] = HL_STRING;
                        if (i < k) hl_types[i++] = HL_STRING;
                    } else {
                        hl_types[i] = isspace((unsigned char)line->chars[i]) ? HL_NORMAL : HL_TYPE;
                        i++;
                    }
                }
                continue;
            }
        }

        /* TOML section header [table] or [[array_of_tables]] */
        if (s_filetype == FILETYPE_TOML && c == '[') {
            while (i < len) {
                hl_types[i] = HL_HEADING;
                if (line->chars[i] == ']' && (i + 1 == len || line->chars[i + 1] != ']')) { i++; break; }
                i++;
            }
            continue;
        }

        /* TOML key before = */
        if (s_filetype == FILETYPE_TOML && (i == 0 || isspace((unsigned char)line->chars[i - 1]))) {
            int k = i;
            while (k < len && (isalnum((unsigned char)line->chars[k]) || line->chars[k] == '_' || line->chars[k] == '-')) k++;
            int eq = k;
            while (eq < len && isspace((unsigned char)line->chars[eq])) eq++;
            if (k > i && eq < len && line->chars[eq] == '=') {
                while (i < k) hl_types[i++] = HL_KEYWORD;
                continue;
            }
        }

        /* Basic comments */
        if (s_filetype == FILETYPE_BASIC && (c == '\'' || (i <= len - 4 && _strnicmp(&line->chars[i], "rem ", 4) == 0))) {
            while (i < len) hl_types[i++] = HL_COMMENT;
            break;
        }

        /* COBOL inline comments *> */
        if (s_filetype == FILETYPE_COBOL && c == '*' && i + 1 < len && line->chars[i + 1] == '>') {
            while (i < len) hl_types[i++] = HL_COMMENT;
            break;
        }

        /* Fortran inline comments ! */
        if (s_filetype == FILETYPE_FORTRAN && c == '!') {
            while (i < len) hl_types[i++] = HL_COMMENT;
            break;
        }

        /* Pascal comments { ... }, compiler directives {$...}, and (* ... *) */
        if (s_filetype == FILETYPE_PASCAL && c == '{') {
            bool is_directive = (i + 1 < len && line->chars[i + 1] == '$');
            unsigned char hl = is_directive ? HL_PREPROC : HL_COMMENT;
            while (i < len) {
                hl_types[i] = hl;
                if (line->chars[i] == '}') { i++; break; }
                i++;
            }
            continue;
        }
        if (s_filetype == FILETYPE_PASCAL && c == '(' && i + 1 < len && line->chars[i + 1] == '*') {
            hl_types[i++] = HL_COMMENT;
            hl_types[i++] = HL_COMMENT;
            while (i < len) {
                if (line->chars[i] == '*' && i + 1 < len && line->chars[i + 1] == ')') {
                    hl_types[i++] = HL_COMMENT;
                    hl_types[i++] = HL_COMMENT;
                    break;
                }
                hl_types[i++] = HL_COMMENT;
            }
            continue;
        }

        /* Zig multiline string literal \\ */
        if (s_filetype == FILETYPE_ZIG && c == '\\' && i + 1 < len && line->chars[i + 1] == '\\') {
            while (i < len) hl_types[i++] = HL_STRING;
            break;
        }

        /* Objective-C @"string" */
        if (s_filetype == FILETYPE_OBJC && c == '@' && i + 1 < len && line->chars[i + 1] == '"') {
            hl_types[i++] = HL_STRING;
            in_string = true;
            str_delim = '"';
            hl_types[i++] = HL_STRING;
            continue;
        }

        /* Check for string start */
        if (c == '"' || c == '\'' ||
            ((s_filetype == FILETYPE_GO || s_filetype == FILETYPE_JAVASCRIPT) && c == '`')) {
            in_string = true;
            str_delim = c;
            hl_types[i++] = HL_STRING;
            continue;
        }

        /* Rust attribute macro #[...] */
        if (s_filetype == FILETYPE_RUST && c == '#' && i + 1 < len && line->chars[i + 1] == '[') {
            while (i < len && line->chars[i] != ']') hl_types[i++] = HL_PREPROC;
            if (i < len) hl_types[i++] = HL_PREPROC;
            continue;
        }

        /* Swift compiler directives #if, #available, etc. */
        if (s_filetype == FILETYPE_SWIFT && c == '#' && i + 1 < len && isalpha((unsigned char)line->chars[i + 1])) {
            hl_types[i++] = HL_PREPROC;
            while (i < len && isalnum((unsigned char)line->chars[i])) hl_types[i++] = HL_PREPROC;
            continue;
        }

        /* Built-ins / Decorators / Annotations starting with @ (Zig, Java, Python, Swift) */
        if ((s_filetype == FILETYPE_ZIG || s_filetype == FILETYPE_JAVA ||
             s_filetype == FILETYPE_PYTHON || s_filetype == FILETYPE_SWIFT) &&
            c == '@' && i + 1 < len && (isalpha((unsigned char)line->chars[i + 1]) || line->chars[i + 1] == '_')) {
            hl_types[i++] = HL_PREPROC;
            while (i < len && (isalnum((unsigned char)line->chars[i]) || line->chars[i] == '_' || line->chars[i] == '.')) {
                hl_types[i++] = HL_PREPROC;
            }
            continue;
        }

        /* Objective-C @directives and keywords starting with @ */
        if (s_filetype == FILETYPE_OBJC && c == '@' && i + 1 < len && isalpha((unsigned char)line->chars[i + 1])) {
            hl_types[i++] = HL_KEYWORD;
            while (i < len && (isalnum((unsigned char)line->chars[i]) || line->chars[i] == '_')) {
                hl_types[i++] = HL_KEYWORD;
            }
            continue;
        }

        /* Check for numbers */
        if (isdigit((unsigned char)c) && (i == 0 || is_separator_ft(line->chars[i - 1], s_filetype))) {
            hl_types[i++] = HL_NUMBER;
            while (i < len && (isalnum((unsigned char)line->chars[i]) || line->chars[i] == '.' ||
                               line->chars[i] == '_' || line->chars[i] == 'x' || line->chars[i] == 'X')) {
                hl_types[i++] = HL_NUMBER;
            }
            continue;
        }

        /* PowerShell Variable $var */
        if (s_filetype == FILETYPE_POWERSHELL && c == '$' && i + 1 < len && isalnum((unsigned char)line->chars[i + 1])) {
            hl_types[i++] = HL_TYPE;
            while (i < len && (isalnum((unsigned char)line->chars[i]) || line->chars[i] == '_' || line->chars[i] == ':')) {
                hl_types[i++] = HL_TYPE;
            }
            continue;
        }

        /* Check for keywords and types at word boundary */
        if (i == 0 || is_separator_ft(line->chars[i - 1], s_filetype)) {
            const char **kw_list = NULL;
            const char **type_list = NULL;
            bool case_sensitive = true;

            switch (s_filetype) {
                case FILETYPE_C:
                    kw_list = C_KEYWORDS;
                    type_list = C_TYPES;
                    break;
                case FILETYPE_POWERSHELL:
                    kw_list = PS_KEYWORDS;
                    case_sensitive = false;
                    break;
                case FILETYPE_PYTHON:
                    kw_list = PY_KEYWORDS;
                    type_list = PY_TYPES;
                    break;
                case FILETYPE_BATCH:
                    kw_list = BAT_KEYWORDS;
                    case_sensitive = false;
                    break;
                case FILETYPE_BASIC:
                    kw_list = BASIC_KEYWORDS;
                    type_list = BASIC_TYPES;
                    case_sensitive = false;
                    break;
                case FILETYPE_RUST:
                    kw_list = RUST_KEYWORDS;
                    type_list = RUST_TYPES;
                    break;
                case FILETYPE_GO:
                    kw_list = GO_KEYWORDS;
                    type_list = GO_TYPES;
                    break;
                case FILETYPE_JAVA:
                    kw_list = JAVA_KEYWORDS;
                    type_list = JAVA_TYPES;
                    break;
                case FILETYPE_COBOL:
                    kw_list = COBOL_KEYWORDS;
                    type_list = COBOL_TYPES;
                    case_sensitive = false;
                    break;
                case FILETYPE_FORTRAN:
                    kw_list = FORTRAN_KEYWORDS;
                    type_list = FORTRAN_TYPES;
                    case_sensitive = false;
                    break;
                case FILETYPE_PASCAL:
                    kw_list = PASCAL_KEYWORDS;
                    type_list = PASCAL_TYPES;
                    case_sensitive = false;
                    break;
                case FILETYPE_SWIFT:
                    kw_list = SWIFT_KEYWORDS;
                    type_list = SWIFT_TYPES;
                    break;
                case FILETYPE_OBJC:
                    kw_list = OBJC_KEYWORDS;
                    type_list = OBJC_TYPES;
                    break;
                case FILETYPE_JAVASCRIPT:
                    kw_list = JS_KEYWORDS;
                    type_list = JS_TYPES;
                    break;
                case FILETYPE_ZIG:
                    kw_list = ZIG_KEYWORDS;
                    type_list = ZIG_TYPES;
                    break;
                case FILETYPE_YAML:
                    type_list = YAML_TYPES;
                    break;
                case FILETYPE_SQL:
                    kw_list = SQL_KEYWORDS;
                    type_list = SQL_TYPES;
                    case_sensitive = false;
                    break;
                case FILETYPE_TOML:
                    kw_list = TOML_KEYWORDS;
                    break;
                default:
                    break;
            }

            bool matched = false;
            if (kw_list) {
                for (int k = 0; kw_list[k]; k++) {
                    int klen = (int)strlen(kw_list[k]);
                    if (i + klen <= len) {
                        bool cmp = case_sensitive ? (strncmp(&line->chars[i], kw_list[k], klen) == 0)
                                                  : (_strnicmp(&line->chars[i], kw_list[k], klen) == 0);
                        if (cmp && (i + klen == len || is_separator_ft(line->chars[i + klen], s_filetype))) {
                            for (int m = 0; m < klen; m++) hl_types[i + m] = HL_KEYWORD;
                            i += klen;
                            matched = true;
                            break;
                        }
                    }
                }
            }
            if (!matched && type_list) {
                for (int k = 0; type_list[k]; k++) {
                    int klen = (int)strlen(type_list[k]);
                    if (i + klen <= len) {
                        bool cmp = case_sensitive ? (strncmp(&line->chars[i], type_list[k], klen) == 0)
                                                  : (_strnicmp(&line->chars[i], type_list[k], klen) == 0);
                        if (cmp && (i + klen == len || is_separator_ft(line->chars[i + klen], s_filetype))) {
                            for (int m = 0; m < klen; m++) hl_types[i + m] = HL_TYPE;
                            i += klen;
                            matched = true;
                            break;
                        }
                    }
                }
            }
            if (matched) continue;
        }

        i++;
    }
}
