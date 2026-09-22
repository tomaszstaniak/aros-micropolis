#include "micropolis.h"
#include "../budget/test-callback.h"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace {
bool trackSprites = false;
void *liveSprites[32] = {};
std::size_t liveSpriteCount = 0;

void forgetAllocation(void *pointer)
{
    for (std::size_t i = 0; i < liveSpriteCount; ++i) {
        if (liveSprites[i] == pointer) {
            liveSprites[i] = liveSprites[--liveSpriteCount];
            return;
        }
    }
}
}

void *operator new(std::size_t size)
{
    void *pointer = std::malloc(size);
    if (!pointer) {
        throw std::bad_alloc();
    }
    if (trackSprites && size == sizeof(SimSprite)) {
        assert(liveSpriteCount < sizeof(liveSprites) / sizeof(liveSprites[0]));
        liveSprites[liveSpriteCount++] = pointer;
    }
    return pointer;
}

void operator delete(void *pointer) noexcept
{
    forgetAllocation(pointer);
    std::free(pointer);
}

void operator delete(void *pointer, std::size_t) noexcept
{
    ::operator delete(pointer);
}

int main()
{
    Micropolis *city = new Micropolis();
    city->setCallback(new TestCallback(), emscripten::val());
    city->init();

    trackSprites = true;
    SimSprite *train = city->newSprite("train", SPRITE_TRAIN, 32, 32);
    SimSprite *ship = city->newSprite("ship", SPRITE_SHIP, 64, 64);
    city->newSprite("monster", SPRITE_MONSTER, 96, 96);
    trackSprites = false;

    assert(train != nullptr);
    assert(ship != nullptr);
    city->destroySprite(ship); // cover the free-list as well as active sprites
    assert(liveSpriteCount == 3);

    delete city;
    assert(liveSpriteCount == 0);
    std::puts("RESULT: 1/1 passed");
}
