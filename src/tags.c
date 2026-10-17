#include "vi.h"
#include "tags.h"
#include "buffer.h"
#include "search.h"
#include "screen.h"

static FILE *find_tags_file(char *out_path, size_t max_len) {
    const char *paths[] = { "tags", "../tags", "../../tags" };
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        FILE *fp = fopen(paths[i], "r");
        if (fp) {
            strncpy(out_path, paths[i], max_len - 1);
            out_path[max_len - 1] = '\0';
            return fp;
        }
    }
    return NULL;
}

bool tags_jump(const char *symbol) {
    if (!symbol || *symbol == '\0') {
        set_status("E426: tag not found");
        return false;
    }

    char tags_path[MAX_PATH];
    FILE *fp = find_tags_file(tags_path, sizeof(tags_path));
    if (!fp) {
        set_status("E433: No tags file");
        return false;
    }

    char line_buf[1024];
    bool found = false;
    char target_file[MAX_PATH] = {0};
    char target_pat[512] = {0};

    while (fgets(line_buf, sizeof(line_buf), fp)) {
        if (line_buf[0] == '!' || line_buf[0] == '\0') continue; /* Skip ctags metadata */

        char *nl = strchr(line_buf, '\n');
        if (nl) *nl = '\0';
        char *cr = strchr(line_buf, '\r');
        if (cr) *cr = '\0';

        char *tag = strtok(line_buf, "\t");
        char *file = strtok(NULL, "\t");
        char *ex_cmd = strtok(NULL, "\t");

        if (tag && file && ex_cmd && strcmp(tag, symbol) == 0) {
            strncpy(target_file, file, sizeof(target_file) - 1);
            strncpy(target_pat, ex_cmd, sizeof(target_pat) - 1);
            found = true;
            break;
        }
    }
    fclose(fp);

    if (!found) {
        set_status("E426: tag not found: %s", symbol);
        return false;
    }

    /* Push current location to tag stack */
    if (E.tag_stack_top < MAX_TAG_STACK) {
        TagStackEntry *entry = &E.tag_stack[E.tag_stack_top++];
        strncpy(entry->filename, E.filename ? E.filename : "", sizeof(entry->filename) - 1);
        entry->filename[sizeof(entry->filename) - 1] = '\0';
        entry->cy = E.cy;
        entry->cx = E.cx;
    }

    /* Open target file if different */
    if (!E.filename || strcmp(E.filename, target_file) != 0) {
        editor_open(target_file);
    }

    /* Jump to line or pattern */
    if (isdigit((unsigned char)target_pat[0])) {
        int line_no = atoi(target_pat);
        E.cy = min(max(0, line_no - 1), E.num_lines - 1);
        E.cx = 0;
    } else {
        /* Pattern search (regex or literal) */
        char clean_pat[512];
        strncpy(clean_pat, target_pat, sizeof(clean_pat) - 1);
        clean_pat[sizeof(clean_pat) - 1] = '\0';

        char *sem = strstr(clean_pat, ";\"");
        if (sem) *sem = '\0';

        int plen = (int)strlen(clean_pat);
        while (plen > 0 && (clean_pat[plen - 1] == '/' || clean_pat[plen - 1] == '$' || clean_pat[plen - 1] == '\r' || clean_pat[plen - 1] == '\n')) {
            clean_pat[--plen] = '\0';
        }
        if (clean_pat[0] == '/') {
            memmove(clean_pat, clean_pat + 1, plen);
            plen--;
        }

        const char *search_target = clean_pat;
        if (clean_pat[0] == '^') search_target = clean_pat + 1;

        for (int r = 0; r < E.num_lines; r++) {
            if (regex_search_line(clean_pat, E.lines[r].chars, false, NULL) != NULL ||
                strstr(E.lines[r].chars, search_target) != NULL) {
                E.cy = r;
                E.cx = 0;
                break;
            }
        }
    }

    set_status("tag 1 of 1 in %s", target_file);
    return true;
}

bool tags_pop(void) {
    if (E.tag_stack_top <= 0) {
        set_status("E427: tag stack empty");
        return false;
    }

    TagStackEntry *entry = &E.tag_stack[--E.tag_stack_top];
    if (entry->filename[0] != '\0' && (!E.filename || strcmp(E.filename, entry->filename) != 0)) {
        editor_open(entry->filename);
    }
    E.cy = min(max(0, entry->cy), E.num_lines - 1);
    E.cx = min(max(0, entry->cx), (E.num_lines > 0) ? E.lines[E.cy].len : 0);
    set_status("popped to %s (line %d)", entry->filename[0] ? entry->filename : "[No Name]", E.cy + 1);
    return true;
}

void tags_list(void) {
    if (E.tag_stack_top == 0) {
        set_status("tag stack empty");
        return;
    }
    char buf[256];
    int pos = snprintf(buf, sizeof(buf), "Tag stack (%d): ", E.tag_stack_top);
    for (int i = E.tag_stack_top - 1; i >= 0 && pos < (int)sizeof(buf) - 30; i--) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%s:%d ",
                        E.tag_stack[i].filename, E.tag_stack[i].cy + 1);
    }
    set_status("%s", buf);
}
