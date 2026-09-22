#ifndef MICROPOLIS_UNSAVED_CITY_H
#define MICROPOLIS_UNSAVED_CITY_H
// "Unsaved" means: saving now would write a different city file than the
// last successful save or load. That covers edits, simulated time, budget
// and saved options without tracking every engine write path by hand.
#include <cstdint>
#include <cstdio>
#include <functional>

// FNV-1a over what saveFile writes. 0 means the fingerprint is unknown,
// which callers must treat as modified.
template<class City>
uint64_t cityFingerprint(City &city,const char *scratchPath) {
    // Pause/run changes simSpeed, which saveFile records; that is not an edit.
    const auto speed=city.simSpeed;
    city.simSpeed=0;
    const bool saved=city.saveFile(scratchPath);
    city.simSpeed=speed;
    if(!saved){remove(scratchPath);return 0;}
    FILE *f=fopen(scratchPath,"rb");
    if(!f){remove(scratchPath);return 0;}
    uint64_t hash=1469598103934665603ULL;
    int c;size_t n=0;
    while((c=fgetc(f))!=EOF){hash=(hash^(uint8_t)c)*1099511628211ULL;++n;}
    const bool ok=!ferror(f);
    fclose(f);remove(scratchPath);
    return ok && n>0 && hash!=0 ? hash : 0;
}

struct UnsavedCity {
    uint64_t baseline=0;
    bool modified(uint64_t current) const { return current==0 || baseline==0 || current!=baseline; }
};

// Requester answers, in EasyRequest order for "Save|Discard|Cancel".
enum class UnsavedChoice { Cancel=0, Save=1, Discard=2 };

// Replacing or closing a modified city needs an explicit answer. A failed
// or cancelled save keeps the current city.
inline bool confirmLeavingCity(bool modified,
                               const std::function<UnsavedChoice()> &ask,
                               const std::function<bool()> &save) {
    if(!modified) return true;
    switch(ask()) {
        case UnsavedChoice::Discard: return true;
        case UnsavedChoice::Save: return save();
        default: return false;
    }
}
#endif
