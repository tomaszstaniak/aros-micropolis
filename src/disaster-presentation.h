#ifndef MICROPOLIS_DISASTER_PRESENTATION_H
#define MICROPOLIS_DISASTER_PRESENTATION_H
#include "micropolis.h"
#include <cstdint>
#include <vector>

// Presentation time advances once per 100ms frontend timer event, independently
// of simulation speed. Offsets never alter the engine map or its random stream.
class EarthquakePresentation {
    int remaining_ = 0;
    int phase_ = 0;
public:
    void start(int strength) {
        int duration = strength < 300 ? 300 : strength > 1000 ? 1000 : strength;
        remaining_ = (duration + 99) / 100;
        phase_ = 0;
    }
    bool tick() {
        if (!remaining_) return false;
        --remaining_;
        ++phase_;
        return true; // Includes the final redraw that restores the origin.
    }
    int x() const { return active() ? ((phase_ & 1) ? -2 : 2) : 0; }
    int y() const { return active() ? ((phase_ & 2) ? -1 : 1) : 0; }
    bool active() const { return remaining_ > 0; }
    void clear() { remaining_ = phase_ = 0; }
};

class ScenarioPresentation {
public:
    enum Result { None, Won, Lost };
private:
    Result pending_ = None;
    bool received_ = false;
public:
    // The engine sends victory as a message without didWinGame. Queue it here
    // so the frontend can open its dialog after the engine batch has returned.
    bool message(int id) {
        if (received_ || (id != MESSAGE_SCENARIO_WON && id != MESSAGE_SCENARIO_LOST)) return false;
        pending_ = id == MESSAGE_SCENARIO_WON ? Won : Lost;
        received_ = true;
        return true;
    }
    Result take() {
        Result result = pending_;
        pending_ = None;
        return result;
    }
    // Call when replacing the current city/scenario, including an ordinary load.
    void reset() { pending_ = None; received_ = false; }
};
// Shift the completed image, including overlays, without feeding modified
// pixels back into later reads. Scratch must be separate from the framebuffer.
inline void shiftEarthquakeFrame(uint32_t *pixels, int width, int height,
                                int dx, int dy, std::vector<uint32_t>& scratch) {
    if (!pixels || width <= 0 || height <= 0 || (!dx && !dy)) return;
    const size_t w = static_cast<size_t>(width), h = static_cast<size_t>(height);
    if (h > scratch.max_size() / w) return;
    scratch.assign(pixels, pixels + w * h);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Wider signed arithmetic also keeps out-of-range offsets safe.
            const int64_t sourceX = static_cast<int64_t>(x) - dx;
            const int64_t sourceY = static_cast<int64_t>(y) - dy;
            pixels[static_cast<size_t>(y) * w + static_cast<size_t>(x)] =
                sourceX >= 0 && sourceX < width && sourceY >= 0 && sourceY < height
                ? scratch[static_cast<size_t>(sourceY) * w + static_cast<size_t>(sourceX)]
                : 0xff000000u;
        }
    }
}
#endif
