#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <string>
#include "../../src/save-replace.h"

namespace fs = std::filesystem;
static int renameCount, failMask, failures, checks;
// Fail specific filesystem boundaries; all other operations use real files.
extern "C" int rename(const char *a, const char *b) {
    ++renameCount;
    if (failMask & (1 << renameCount)) { errno = EIO; return -1; }
    return renameat(AT_FDCWD, a, AT_FDCWD, b);
}
static void check(bool ok, const char *name) {
    ++checks; if (!ok) ++failures;
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
}
static void put(const fs::path &p, const char *s) {
    std::ofstream out(p); out << s; out.close();
    if (!out) { perror("fixture"); exit(2); }
}
static std::string get(const fs::path &p) {
    std::ifstream in(p); return {std::istreambuf_iterator<char>(in), {}};
}
int main() {
    char root[] = "/tmp/micropolis-save-XXXXXX";
    if (!mkdtemp(root)) return 2;
    for (int mask : {0, 2, 4, 12}) {
        auto dir = fs::path(root) / std::to_string(mask); fs::create_directory(dir);
        auto path = (dir / "city.cty").string();
        auto tmp = path + ".tmp", bak = path + ".bak";
        put(path, "LATEST"); put(bak, "OLDER"); put(tmp, "NEW");
        renameCount = 0; failMask = mask;
        char feedback[2048];
        bool ok = replaceWithNewFile(path.c_str(), tmp.c_str(), feedback, sizeof feedback);
        check(ok == (mask == 0), "replacement reports injected failure");
        check(get(bak) == "OLDER", "existing recovery backup survives");
        bool latest = false;
        for (const auto &entry : fs::directory_iterator(dir))
            if (get(entry.path()) == "LATEST") latest = true;
        check(latest, "latest previous save survives every rename boundary");
        check(get(ok ? path : tmp) == "NEW", "new city survives replacement failure");
        if (mask == 4) check(get(path) == "LATEST", "failed promotion restores main file");
        if (mask == 12) check(std::strstr(feedback, ".bak.1") != nullptr,
                              "failed rollback names previous city's recovery file");
        if (!ok) check(std::strstr(feedback, tmp.c_str()) != nullptr,
                        "failure names new city's recovery file");
        char nextTmp[512], nextBak[512];
        failMask = 0;
        check(makeAuxNames(path.c_str(), nextTmp, sizeof nextTmp, nextBak, sizeof nextBak),
              "next save can select unused auxiliary names");
        check(!fs::exists(nextTmp), "next save does not truncate a recovered temporary city");
        check(!fs::exists(nextBak), "backup name does not alias existing backup");
    }
    // Fresh save and recovery from an orphaned backup both retain all cities.
    for (bool orphan : {false, true}) {
        auto path = (fs::path(root) / (orphan ? "orphan" : "fresh")).string();
        auto tmp = path + ".tmp", bak = path + ".bak";
        put(tmp, "NEW"); if (orphan) put(bak, "ONLY-PREVIOUS");
        char feedback[2048]; failMask = 0;
        check(replaceWithNewFile(path.c_str(), tmp.c_str(), feedback, sizeof feedback),
              "fresh or orphaned save can be promoted");
        check(get(path) == "NEW", "promoted city contents match");
        if (orphan) check(get(bak) == "ONLY-PREVIOUS", "orphaned backup remains recoverable");
    }
    char tmp[256], bak[256]; std::string longName(255, 'a');
    check(!makeAuxNames(longName.c_str(), tmp, sizeof tmp, bak, sizeof bak),
          "truncated auxiliary path is rejected");
    {
        auto path = (fs::path(root) / (std::string(240, 'b') + ".cty")).string();
        char tmp[512], bak[512], feedback[2048];
        check(makeAuxNames(path.c_str(), tmp, sizeof tmp, bak, sizeof bak),
              "long but fitting paths remain usable");
        put(path, "PREVIOUS"); put(tmp, "NEW"); failMask=0;
        check(replaceWithNewFile(path.c_str(), tmp, feedback, sizeof feedback),
              "long path replacement succeeds");
        check(get(path)=="NEW" && get(bak)=="PREVIOUS", "long path preserves both versions");
    }
    auto p = (fs::path(root) / "alias").string(); put(p, "ONLY-COPY");
    char feedback[2048]; failMask = 0;
    check(!replaceWithNewFile(p.c_str(), p.c_str(), feedback, sizeof feedback),
          "aliased target and temporary file rejected");
    check(get(p) == "ONLY-COPY", "alias refusal preserves only copy");
    fs::remove_all(root);
    printf("RESULT: %d/%d passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
