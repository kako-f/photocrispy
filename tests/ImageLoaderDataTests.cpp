#include "ImageLoader.h"

#include <cassert>
#include <cstdint>

static void previewDataUses8BitPixels()
{
    ImageData image;
    image.kind = ImageKind::Preview;
    image.is16Bit = false;
    image.channels = 3;
    image.pixels8 = { 1, 2, 3 };

    assert(image.kind == ImageKind::Preview);
    assert(!image.is16Bit);
    assert(image.channels == 3);
    assert(image.pixels8.size() == 3);
    assert(image.pixels16.empty());
}

static void fullDataUses16BitPixels()
{
    ImageData image;
    image.kind = ImageKind::Full;
    image.is16Bit = true;
    image.channels = 3;
    image.pixels16 = { 1024, 2048, 4096 };

    assert(image.kind == ImageKind::Full);
    assert(image.is16Bit);
    assert(image.channels == 3);
    assert(image.pixels16.size() == 3);
    assert(image.pixels8.empty());
}

int main()
{
    previewDataUses8BitPixels();
    fullDataUses16BitPixels();
    return 0;
}
