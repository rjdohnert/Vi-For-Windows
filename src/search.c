#include "vi.h"
#include "search.h"
#include "screen.h"

#define MAX_REGEX_TOKENS 256

typedef enum {
    TOK_CHAR,           /* literal character */
    TOK_DOT,            /* . (any character except \0) */
    TOK_DIGIT,          /* \d (0-9) */
    TOK_NON_DIGIT,      /* \D (not 0-9) */
    TOK_WORD,           /* \w (isalnum or _) */
    TOK_NON_WORD,       /* \W (not word) */
    TOK_SPACE,          /* \s (whitespace) */
    TOK_NON_SPACE,      /* \S (non-whitespace) */
    TOK_CCLASS,         /* [...] positive character class */
    TOK_NCCLASS,        /* [^...] negated character class */
    TOK_BOL,            /* ^ start of line */
    TOK_EOL,            /* $ end of line */
    TOK_BOW,            /* \< word start */
    TOK_EOW,            /* \> word end */
    TOK_START_OF_MATCH, /* \zs set match start */
    TOK_END_OF_MATCH    /* \ze set match end */
} RegexTokenType;

typedef enum {
    QUANT_ONE,          /* exactly 1 */
    QUANT_STAR,         /* 0 or more (*) */
    QUANT_PLUS,         /* 1 or more (\+ or +) */
    QUANT_QUESTION      /* 0 or 1 (\? or ?) */
} RegexQuantType;

typedef struct {
    RegexTokenType type;
    RegexQuantType quant;
    char ch;
    unsigned char cclass[32]; /* 256-bit character class bitmap */
} RegexToken;

static bool is_word_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static bool single_char_matches(const RegexToken *tok, char c, bool ic) {
    if (c == '\0') return false;
    switch (tok->type) {
        case TOK_CHAR:
            if (ic) return tolower((unsigned char)tok->ch) == tolower((unsigned char)c);
            return tok->ch == c;
        case TOK_DOT:
            return true;
        case TOK_DIGIT:
            return isdigit((unsigned char)c) != 0;
        case TOK_NON_DIGIT:
            return !isdigit((unsigned char)c);
        case TOK_WORD:
            return is_word_char(c);
        case TOK_NON_WORD:
            return !is_word_char(c);
        case TOK_SPACE:
            return isspace((unsigned char)c) != 0;
        case TOK_NON_SPACE:
            return !isspace((unsigned char)c);
        case TOK_CCLASS: {
            unsigned char cl = (unsigned char)tolower((unsigned char)c);
            unsigned char cu = (unsigned char)toupper((unsigned char)c);
            if (ic) {
                return ((tok->cclass[cl / 8] & (1 << (cl % 8))) != 0) ||
                       ((tok->cclass[cu / 8] & (1 << (cu % 8))) != 0);
            }
            return (tok->cclass[(unsigned char)c / 8] & (1 << ((unsigned char)c % 8))) != 0;
        }
        case TOK_NCCLASS: {
            unsigned char cl = (unsigned char)tolower((unsigned char)c);
            unsigned char cu = (unsigned char)toupper((unsigned char)c);
            if (ic) {
                return !(((tok->cclass[cl / 8] & (1 << (cl % 8))) != 0) ||
                         ((tok->cclass[cu / 8] & (1 << (cu % 8))) != 0));
            }
            return (tok->cclass[(unsigned char)c / 8] & (1 << ((unsigned char)c % 8))) == 0;
        }
        default:
            return false;
    }
}

static int compile_regex(const char *pat, RegexToken *tokens, int max_tokens) {
    int num_toks = 0;
    int i = 0;

    while (pat[i] != '\0' && num_toks < max_tokens) {
        RegexToken *tok = &tokens[num_toks];
        memset(tok, 0, sizeof(RegexToken));
        tok->quant = QUANT_ONE;

        if (pat[i] == '\\') {
            if (pat[i + 1] == 'z' && pat[i + 2] == 's') {
                tok->type = TOK_START_OF_MATCH;
                i += 3;
                num_toks++;
                continue;
            } else if (pat[i + 1] == 'z' && pat[i + 2] == 'e') {
                tok->type = TOK_END_OF_MATCH;
                i += 3;
                num_toks++;
                continue;
            } else if (pat[i + 1] == 'd') {
                tok->type = TOK_DIGIT;
                i += 2;
            } else if (pat[i + 1] == 'D') {
                tok->type = TOK_NON_DIGIT;
                i += 2;
            } else if (pat[i + 1] == 'w') {
                tok->type = TOK_WORD;
                i += 2;
            } else if (pat[i + 1] == 'W') {
                tok->type = TOK_NON_WORD;
                i += 2;
            } else if (pat[i + 1] == 's') {
                tok->type = TOK_SPACE;
                i += 2;
            } else if (pat[i + 1] == 'S') {
                tok->type = TOK_NON_SPACE;
                i += 2;
            } else if (pat[i + 1] == '<') {
                tok->type = TOK_BOW;
                i += 2;
                num_toks++;
                continue;
            } else if (pat[i + 1] == '>') {
                tok->type = TOK_EOW;
                i += 2;
                num_toks++;
                continue;
            } else if (pat[i + 1] == '+' && num_toks > 0) {
                tokens[num_toks - 1].quant = QUANT_PLUS;
                i += 2;
                continue;
            } else if (pat[i + 1] == '?' && num_toks > 0) {
                tokens[num_toks - 1].quant = QUANT_QUESTION;
                i += 2;
                continue;
            } else if (pat[i + 1] != '\0') {
                tok->type = TOK_CHAR;
                tok->ch = pat[i + 1];
                i += 2;
            } else {
                tok->type = TOK_CHAR;
                tok->ch = '\\';
                i++;
            }
        } else if (pat[i] == '^' && i == 0) {
            tok->type = TOK_BOL;
            i++;
            num_toks++;
            continue;
        } else if (pat[i] == '$' && pat[i + 1] == '\0') {
            tok->type = TOK_EOL;
            i++;
            num_toks++;
            continue;
        } else if (pat[i] == '.') {
            tok->type = TOK_DOT;
            i++;
        } else if (pat[i] == '[') {
            i++;
            bool negate = false;
            if (pat[i] == '^') {
                negate = true;
                i++;
            }
            tok->type = negate ? TOK_NCCLASS : TOK_CCLASS;
            if (pat[i] == ']') {
                unsigned char c = (unsigned char)pat[i++];
                tok->cclass[c / 8] |= (1 << (c % 8));
            }
            while (pat[i] != '\0' && pat[i] != ']') {
                if (pat[i + 1] == '-' && pat[i + 2] != '\0' && pat[i + 2] != ']') {
                    unsigned char start = (unsigned char)pat[i];
                    unsigned char end = (unsigned char)pat[i + 2];
                    if (start > end) { unsigned char tmp = start; start = end; end = tmp; }
                    for (unsigned int c = start; c <= end; c++) {
                        tok->cclass[c / 8] |= (1 << (c % 8));
                    }
                    i += 3;
                } else {
                    unsigned char c = (unsigned char)pat[i++];
                    tok->cclass[c / 8] |= (1 << (c % 8));
                }
            }
            if (pat[i] == ']') i++;
        } else if (pat[i] == '*' && num_toks > 0) {
            tokens[num_toks - 1].quant = QUANT_STAR;
            i++;
            continue;
        } else if (pat[i] == '+' && num_toks > 0) {
            tokens[num_toks - 1].quant = QUANT_PLUS;
            i++;
            continue;
        } else if (pat[i] == '?' && num_toks > 0) {
            tokens[num_toks - 1].quant = QUANT_QUESTION;
            i++;
            continue;
        } else {
            tok->type = TOK_CHAR;
            tok->ch = pat[i++];
        }

        /* Check quantifier directly following token */
        if (pat[i] == '\\' && pat[i + 1] == '+') {
            tok->quant = QUANT_PLUS;
            i += 2;
        } else if (pat[i] == '\\' && pat[i + 1] == '?') {
            tok->quant = QUANT_QUESTION;
            i += 2;
        } else if (pat[i] == '*') {
            tok->quant = QUANT_STAR;
            i++;
        }

        num_toks++;
    }

    return num_toks;
}

static bool match_tokens_here(const RegexToken *toks, int num_toks, int tidx,
                             const char *line_start, const char *cur, bool ic,
                             const char **out_zs, const char **out_ze) {
    if (tidx == num_toks) {
        if (!*out_ze) *out_ze = cur;
        return true;
    }

    const RegexToken *tok = &toks[tidx];

    /* Zero-width assertions & match markers */
    if (tok->type == TOK_START_OF_MATCH) {
        *out_zs = cur;
        return match_tokens_here(toks, num_toks, tidx + 1, line_start, cur, ic, out_zs, out_ze);
    }
    if (tok->type == TOK_END_OF_MATCH) {
        *out_ze = cur;
        return match_tokens_here(toks, num_toks, tidx + 1, line_start, cur, ic, out_zs, out_ze);
    }
    if (tok->type == TOK_BOL) {
        if (cur == line_start) {
            return match_tokens_here(toks, num_toks, tidx + 1, line_start, cur, ic, out_zs, out_ze);
        }
        return false;
    }
    if (tok->type == TOK_EOL) {
        if (*cur == '\0') {
            return match_tokens_here(toks, num_toks, tidx + 1, line_start, cur, ic, out_zs, out_ze);
        }
        return false;
    }
    if (tok->type == TOK_BOW) {
        bool at_word_start = is_word_char(*cur) && (cur == line_start || !is_word_char(cur[-1]));
        if (at_word_start) {
            return match_tokens_here(toks, num_toks, tidx + 1, line_start, cur, ic, out_zs, out_ze);
        }
        return false;
    }
    if (tok->type == TOK_EOW) {
        bool at_word_end = (cur > line_start && is_word_char(cur[-1])) && !is_word_char(*cur);
        if (at_word_end) {
            return match_tokens_here(toks, num_toks, tidx + 1, line_start, cur, ic, out_zs, out_ze);
        }
        return false;
    }

    /* Quantifier handling for single character tokens */
    if (tok->quant == QUANT_ONE) {
        if (single_char_matches(tok, *cur, ic)) {
            return match_tokens_here(toks, num_toks, tidx + 1, line_start, cur + 1, ic, out_zs, out_ze);
        }
        return false;
    }

    if (tok->quant == QUANT_QUESTION) {
        /* Greedy: try 1 char match first, then 0 chars */
        if (single_char_matches(tok, *cur, ic)) {
            const char *saved_zs = *out_zs;
            const char *saved_ze = *out_ze;
            if (match_tokens_here(toks, num_toks, tidx + 1, line_start, cur + 1, ic, out_zs, out_ze)) {
                return true;
            }
            *out_zs = saved_zs;
            *out_ze = saved_ze;
        }
        return match_tokens_here(toks, num_toks, tidx + 1, line_start, cur, ic, out_zs, out_ze);
    }

    if (tok->quant == QUANT_STAR || tok->quant == QUANT_PLUS) {
        int max_match = 0;
        while (cur[max_match] != '\0' && single_char_matches(tok, cur[max_match], ic)) {
            max_match++;
        }
        int min_match = (tok->quant == QUANT_PLUS) ? 1 : 0;
        for (int k = max_match; k >= min_match; k--) {
            const char *saved_zs = *out_zs;
            const char *saved_ze = *out_ze;
            if (match_tokens_here(toks, num_toks, tidx + 1, line_start, cur + k, ic, out_zs, out_ze)) {
                return true;
            }
            *out_zs = saved_zs;
            *out_ze = saved_ze;
        }
        return false;
    }

    return false;
}

const char *regex_search_line(const char *pat, const char *text, bool ic, int *match_len) {
    if (!pat || !text) return NULL;
    if (pat[0] == '\0') {
        if (match_len) *match_len = 0;
        return text;
    }

    RegexToken tokens[MAX_REGEX_TOKENS];
    int num_toks = compile_regex(pat, tokens, MAX_REGEX_TOKENS);
    if (num_toks == 0) return NULL;

    /* If pattern begins with BOL anchor ^, only try at start of text */
    if (tokens[0].type == TOK_BOL) {
        const char *zs = text;
        const char *ze = NULL;
        if (match_tokens_here(tokens, num_toks, 0, text, text, ic, &zs, &ze)) {
            if (!ze) ze = text;
            if (match_len) *match_len = (int)(ze - zs);
            return zs;
        }
        return NULL;
    }

    const char *t = text;
    do {
        const char *zs = t;
        const char *ze = NULL;
        if (match_tokens_here(tokens, num_toks, 0, text, t, ic, &zs, &ze)) {
            if (!ze) ze = t;
            if (match_len) *match_len = (int)(ze - zs);
            return zs;
        }
    } while (*t++ != '\0');

    return NULL;
}

void editor_search(bool forward) {
    if (E.num_lines == 0 || strlen(E.last_search) == 0) return;
    int cur = E.cy;
    int start_col = forward ? E.cx + 1 : E.cx - 1;

    for (int step = 0; step < E.num_lines; step++) {
        int delta = forward ? step : -step;
        int row = ((cur + delta) % E.num_lines + E.num_lines) % E.num_lines;
        char *line = E.lines[row].chars;
        const char *pos = NULL;

        if (step == 0) {
            if (forward && start_col < E.lines[row].len) {
                pos = regex_search_line(E.last_search, &line[start_col], E.opt.ignorecase, NULL);
            } else if (!forward && start_col >= 0) {
                for (int i = start_col; i >= 0; i--) {
                    if (regex_search_line(E.last_search, &line[i], E.opt.ignorecase, NULL) == &line[i]) {
                        pos = &line[i];
                        break;
                    }
                }
            }
        } else {
            pos = regex_search_line(E.last_search, line, E.opt.ignorecase, NULL);
            if (!forward && pos) {
                const char *last = pos;
                const char *next = pos + 1;
                while (*next && (next = regex_search_line(E.last_search, next, E.opt.ignorecase, NULL))) {
                    last = next;
                    next++;
                }
                pos = last;
            }
        }

        if (pos) {
            E.mark_prev.y = E.cy;
            E.mark_prev.x = E.cx;
            E.mark_prev.set = true;
            E.cy = row;
            E.cx = (int)(pos - line);
            return;
        }
    }
    set_status("Pattern not found: %s", E.last_search);
}
