#include "save-replace.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

// A failed lookup is not proof that a name is free (permissions, I/O).
static int pathState(const char *path) {
    struct stat st;
    if (lstat(path, &st) == 0) return 1;
    return errno == ENOENT ? 0 : -1;
}

static bool unusedName(const char *path, const char *suffix,
                       char *out, size_t capacity) {
    for (unsigned i = 0; i < 1000; ++i) {
        int n = i ? snprintf(out, capacity, "%s.%s.%u", path, suffix, i)
                  : snprintf(out, capacity, "%s.%s", path, suffix);
        if (n < 0 || static_cast<size_t>(n) >= capacity) return false;
        int state = pathState(out);
        if (state < 0) return false;
        if (state == 0) return true;
    }
    return false;
}

bool makeAuxNames(const char *path, char *tmp, size_t tmpSize,
                  char *bak, size_t bakSize) {
    return path && *path && unusedName(path, "tmp", tmp, tmpSize) &&
           unusedName(path, "bak", bak, bakSize);
}

bool replaceWithNewFile(const char *path, const char *tmp,
                        char *feedback, size_t feedbackSize) {
    struct stat st;
    char bak[512];
    int oldState = pathState(path);
    if (strcmp(path, tmp) == 0 || oldState < 0 ||
        lstat(tmp, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size == 0 ||
        (oldState && (lstat(path, &st) != 0 || !S_ISREG(st.st_mode))) ||
        !unusedName(path, "bak", bak, sizeof bak)) {
        snprintf(feedback, feedbackSize,
                 "SAVE FAILED: replacement refused; target untouched; new city: %s", tmp);
        return false;
    }

    // Keep every existing recovery copy. In particular, an older .bak is
    // NOT a backup of the current main file. Never remove the latter.
    if (oldState && rename(path, bak) != 0) {
        snprintf(feedback, feedbackSize,
                 "SAVE FAILED: previous city: %s; new city: %s", path, tmp);
        return false;
    }
    if (rename(tmp, path) != 0) {
        const char *previous = path;
        if (oldState && rename(bak, path) != 0) previous = bak;
        snprintf(feedback, feedbackSize,
                 "SAVE FAILED: previous city: %s; new city: %s", previous, tmp);
        return false;
    }
    snprintf(feedback, feedbackSize, "saved: %s", path);
    return true;
}
