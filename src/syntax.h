#ifndef SYNTAX_H
#define SYNTAX_H

#include "vi.h"

typedef enum {
    HL_NORMAL = 0,
    HL_KEYWORD,
    HL_TYPE,
    HL_STRING,
    HL_NUMBER,
    HL_COMMENT,
    HL_PREPROC,
    HL_HEADING,
    HL_DIFF_CHANGE,
    HL_DIFF_ADD,
    HL_DIFF_DELETE
} SyntaxHighlight;

typedef enum {
    FILETYPE_NONE,
    FILETYPE_C,
    FILETYPE_POWERSHELL,
    FILETYPE_BATCH,
    FILETYPE_PYTHON,
    FILETYPE_SHELL,
    FILETYPE_JSON,
    FILETYPE_MARKDOWN,
    FILETYPE_BASIC,
    FILETYPE_RUST,
    FILETYPE_GO,
    FILETYPE_JAVA,
    FILETYPE_COBOL,
    FILETYPE_FORTRAN,
    FILETYPE_PASCAL,
    FILETYPE_SWIFT,
    FILETYPE_OBJC,
    FILETYPE_JAVASCRIPT,
    FILETYPE_ZIG,
    FILETYPE_XML,
    FILETYPE_YAML,
    FILETYPE_INI,
    FILETYPE_TOML,
    FILETYPE_SQL
} FileType;

void syntax_detect_filetype(void);
void syntax_highlight_row(int filerow, unsigned char *hl_types);
const char *syntax_color_code(unsigned char hl_type);
bool syntax_set_colorscheme(const char *name);
const char *syntax_get_colorscheme(void);

#endif /* SYNTAX_H */
