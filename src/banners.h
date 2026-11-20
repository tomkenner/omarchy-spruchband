// The five banner kinds, ported from the Android app's gfx/Banners.kt. The
// kind follows the figure's category.
#pragma once

#include "figures.h"

#include <QChar>
#include <QString>
#include <array>
#include <functional>
#include <vector>

// Measurements of one banner for one flyby, computed once.
struct Layout {
    QString text;
    float size = 0, textW = 0;
    std::vector<float> cx;
    float pad = 0, rope = 0, cloth = 0, len = 0, board = 0, bar = 0, lead = 0;
    std::vector<QChar> itemCh;
    std::vector<float> itemO;
    float pw = 0, content = 0, str = 0;
    int n = 0;
    float sp = 0;
    std::vector<const Figure *> carriers;
};

using Rnd = std::function<float()>;

// The longest saying a banner carries, in UTF-16 code units. Every train is
// laid out from clipped text (Stage::makeTrain), so measuring and drawing a
// banner stay bounded whatever a collection file holds.
constexpr qsizetype MAX_SAYING = 200;
// s cut at a character boundary to at most max code units, ending in "…"
// when anything was cut.
QString clipText(const QString &s, qsizetype max = MAX_SAYING);

class BannerKind {
public:
    virtual ~BannerKind() = default;
    virtual const char *name() const = 0; // English; translated where it is shown
    virtual Layout layout(Gfx &g, const QString &text, float bh, float s, const std::vector<const Figure *> &pool,
                          const Rnd &rnd, const Figure &fig) const = 0;
    virtual void draw(Gfx &g, const Layout &L, float ax, float ay, int d, float t, float bh, float s, float ground,
                      float dist) const = 0;
    // Room needed above (0) and below (1) the lane, in px.
    virtual std::array<float, 2> extent(const Figure &fig, float s, float bh) const = 0;

    static const BannerKind &of(const Figure &f);
};

namespace Banners {
const BannerKind &schlepp();
const BannerKind &anhaenger();
const BannerKind &wimpel();
const BannerKind &transparent();
const BannerKind &banderole();
}
