#include "vi.h"
#include "explorer.h"
#include "buffer.h"
#include "screen.h"

bool is_directory(const char *path) {
    if (!path || *path == '\0') return false;
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return false;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool explorer_open(const char *path) {
    char full_path[MAX_PATH];
    if (GetFullPathNameA(path, sizeof(full_path), full_path, NULL) == 0) {
        strncpy(full_path, path, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    }
    int fp_len = (int)strlen(full_path);
    while (fp_len > 3 && (full_path[fp_len - 1] == '\\' || full_path[fp_len - 1] == '/')) {
        full_path[--fp_len] = '\0';
    }

    char buf_title[MAX_PATH + 32];
    snprintf(buf_title, sizeof(buf_title), "[Explorer: %s]", full_path);

    if (E.cur_buf_idx >= 0 && E.cur_buf_idx < E.num_buffers) {
        bool is_exp = (E.filename && strncmp(E.filename, "[Explorer: ", 11) == 0);
        bool is_empty_unnamed = (!E.dirty && E.filename == NULL && E.num_lines <= 1 &&
                                 (E.num_lines == 0 || E.lines[0].len == 0));
        if (!is_exp && !is_empty_unnamed && E.num_buffers < MAX_BUFFERS) {
            buffer_save_current_state();
            int exp_idx = -1;
            for (int i = 0; i < E.num_buffers; i++) {
                if (E.buffers[i].filename && strncmp(E.buffers[i].filename, "[Explorer: ", 11) == 0) {
                    exp_idx = i;
                    break;
                }
            }
            if (exp_idx >= 0) {
                buffer_load_state(exp_idx);
            } else {
                int new_idx = E.num_buffers++;
                Buffer *nb = &E.buffers[new_idx];
                memset(nb, 0, sizeof(Buffer));
                nb->id = E.next_buffer_id++;
                nb->filename = _strdup(buf_title);
                nb->loaded = true;
                E.cur_buf_idx = new_idx;
                if (E.cur_win_idx >= 0 && E.cur_win_idx < E.num_windows) {
                    E.windows[E.cur_win_idx].buf_idx = new_idx;
                }
                E.lines = NULL;
                E.num_lines = 0;
                E.undo_stack.top = 0;
                E.redo_stack.top = 0;
                E.u_line_backup = NULL;
                E.u_line_cy = -1;
                memset(E.marks, 0, sizeof(E.marks));
            }
        }
    } else if (E.num_buffers == 0) {
        E.num_buffers = 1;
        E.cur_buf_idx = 0;
        memset(&E.buffers[0], 0, sizeof(Buffer));
        E.buffers[0].id = E.next_buffer_id++;
        E.buffers[0].loaded = true;
    }

    /* Clear current buffer lines */
    while (E.num_lines > 0) delete_row(0);

    /* Set buffer filename to explorer path indicator */
    free(E.filename);
    E.filename = _strdup(buf_title);
    if (E.cur_buf_idx >= 0 && E.cur_buf_idx < E.num_buffers) {
        free(E.buffers[E.cur_buf_idx].filename);
        E.buffers[E.cur_buf_idx].filename = _strdup(buf_title);
    }

    char header[512];
    insert_row(0, "\" ====================================================================", 70);
    snprintf(header, sizeof(header), "\" Directory: %s", full_path);
    insert_row(1, header, (int)strlen(header));
    insert_row(2, "\" Press <Enter> to open file/folder, '-' for parent directory", 60);
    insert_row(3, "\" ====================================================================", 70);
    insert_row(4, "../", 3);

    char search_pattern[MAX_PATH + 16];
    snprintf(search_pattern, sizeof(search_pattern), "%s\\*", full_path);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_pattern, &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        /* First pass: directories */
        do {
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                char item[MAX_PATH + 16];
                snprintf(item, sizeof(item), "%s/", fd.cFileName);
                insert_row(E.num_lines, item, (int)strlen(item));
            }
        } while (FindNextFileA(hFind, &fd));

        /* Second pass: files */
        FindClose(hFind);
        hFind = FindFirstFileA(search_pattern, &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    char item[MAX_PATH + 64];
                    unsigned long long sz = ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
                    if (sz < 1024) {
                        snprintf(item, sizeof(item), "%-32s  %llu B", fd.cFileName, sz);
                    } else if (sz < 1024 * 1024) {
                        snprintf(item, sizeof(item), "%-32s  %llu KB", fd.cFileName, sz / 1024);
                    } else {
                        snprintf(item, sizeof(item), "%-32s  %llu MB", fd.cFileName, sz / (1024 * 1024));
                    }
                    insert_row(E.num_lines, item, (int)strlen(item));
                }
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
    }

    E.cy = 4;
    E.cx = 0;
    E.dirty = false;
    buffer_save_current_state();
    set_status("%s (Browse with j/k, <Enter> to open)", buf_title);
    return true;
}

bool explorer_handle_enter(void) {
    if (!E.filename || strncmp(E.filename, "[Explorer: ", 11) != 0) return false;
    if (E.cy < 0 || E.cy >= E.num_lines) return false;

    char line_copy[512];
    strncpy(line_copy, E.lines[E.cy].chars, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';

    if (line_copy[0] == '\"' || line_copy[0] == '\0') return true; /* Skip comments */

    /* Extract base path from buffer title: "[Explorer: <base_path>]" */
    char base_path[MAX_PATH] = {0};
    const char *start = E.filename + 11;
    const char *end = strrchr(start, ']');
    if (end) {
        int len = (int)(end - start);
        strncpy(base_path, start, len);
        base_path[len] = '\0';
    } else {
        strcpy(base_path, ".");
    }

    /* Check parent navigation */
    if (strcmp(line_copy, "../") == 0) {
        char parent[MAX_PATH];
        snprintf(parent, sizeof(parent), "%s\\..", base_path);
        explorer_open(parent);
        return true;
    }

    /* Extract target name (trim size suffix) */
    char name[MAX_PATH] = {0};
    if (line_copy[strlen(line_copy) - 1] == '/') {
        /* Directory */
        strncpy(name, line_copy, sizeof(name) - 1);
        int nlen = (int)strlen(name);
        if (nlen > 0 && name[nlen - 1] == '/') name[nlen - 1] = '\0';
        char full_dir[MAX_PATH];
        snprintf(full_dir, sizeof(full_dir), "%s\\%s", base_path, name);
        explorer_open(full_dir);
        return true;
    } else {
        /* File */
        char *space = strstr(line_copy, "  ");
        if (space) *space = '\0';
        while (strlen(line_copy) > 0 && line_copy[strlen(line_copy) - 1] == ' ') {
            line_copy[strlen(line_copy) - 1] = '\0';
        }
        char full_file[MAX_PATH];
        snprintf(full_file, sizeof(full_file), "%s\\%s", base_path, line_copy);
        editor_open(full_file);
        return true;
    }
}

static bool find_rec(const char *dir, const char *query, char *out_path, int out_size, int depth) {
    if (depth > 8) return false;
    char spec[MAX_PATH];
    snprintf(spec, sizeof(spec), "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(spec, &fd);
    if (h == INVALID_HANDLE_VALUE) return false;

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
        if (strcmp(fd.cFileName, ".git") == 0 || strcmp(fd.cFileName, ".gemini") == 0) continue;

        char full[MAX_PATH];
        snprintf(full, sizeof(full), "%s\\%s", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (find_rec(full, query, out_path, out_size, depth + 1)) {
                FindClose(h);
                return true;
            }
        } else {
            char lower_name[MAX_PATH], lower_q[MAX_PATH];
            int i;
            for (i = 0; fd.cFileName[i] && i < MAX_PATH - 1; i++) lower_name[i] = (char)tolower((unsigned char)fd.cFileName[i]);
            lower_name[i] = '\0';
            for (i = 0; query[i] && i < MAX_PATH - 1; i++) lower_q[i] = (char)tolower((unsigned char)query[i]);
            lower_q[i] = '\0';

            if (strstr(lower_name, lower_q) != NULL) {
                strncpy(out_path, full, out_size - 1);
                out_path[out_size - 1] = '\0';
                FindClose(h);
                return true;
            }
        }
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    return false;
}

bool explorer_find_file(const char *query, char *out_path, int out_size) {
    if (!query || *query == '\0') return false;
    return find_rec(".", query, out_path, out_size, 0);
}

