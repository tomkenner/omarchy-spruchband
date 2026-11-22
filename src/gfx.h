// Drawing core, ported from the Android app's gfx/Gfx.kt and gfx/Geo.kt.
// Figures and banners only know fill/limb/cord/text; what that looks like is
// decided here, per style. World coordinates are logical pixels.
#pragma once

#include <QFont>
#include <QHash>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QString>
#include <cstdint>
#include <vector>

using Pts = std::vector<float>; // flat x0,y0,x1,y1,…

enum class Style { Cartoon, Pixel, Paper, Silhouette, Doodle, Neon, Aquarell, Kupferstich, Comic, Knete, Emoji };

struct StyleInfo {
    Style style;
    const char *id;
    const char *name;
    const char *hint;
};

// In the order the settings list them.
const std::vector<StyleInfo> &allStyles();
bool styleFromId(const QString &id, Style *out);
QString styleId(Style s);

// Drawing options for one shape. Fields stay in this order so designated
// initializers (O{.noLine = true, .alpha = .5f}) work.
struct O {
    bool noLine = false;
    bool minor = false;
    float alpha = 1;
    bool cloth = false;
    uint32_t neon = 0;
    float lw = 2;
    bool smooth = false;
};

namespace Opt {
inline const O D{};
inline const O NL{.noLine = true};
inline const O MINOR{.noLine = true, .minor = true};
inline const O SMOOTH{.smooth = true};
}

namespace Col {
constexpr uint32_t INK = 0xFF231D2E;
constexpr uint32_t PAPER = 0xFFF7F1E3;
constexpr uint32_t PENCIL = 0xFF2B2A33;
constexpr uint32_t SEPIA = 0xFF3B2A1A;
constexpr uint32_t PARCH = 0xFFF4EAD5;

uint32_t withAlpha(uint32_t col, float a);
uint32_t mix(uint32_t a, uint32_t b, float k);
inline uint32_t light(uint32_t col, float k) { return mix(col, 0xFFFFFFFF, k); }
inline uint32_t dark(uint32_t col, float k) { return mix(col, 0xFF000000, k); }
float lum(uint32_t col);
inline QColor q(uint32_t argb) { return QColor::fromRgba(argb); }
}

namespace Geo {
Pts ell(float cx, float cy, float rx, float ry, float rot = 0, int n = 28);
Pts rect(float x, float y, float w, float h, float r0 = 0);
Pts blob(const Pts &p, bool closed = true, int seg = 6);
Pts band(float cx, float cy, float rx, float ry, float x1, float x2, int n = 8);
Pts star(float cx, float cy, float r, float inner = 0.38f);
float rnd(float n);
}

enum class FontRole { Default, Marker, Antiqua };

// The fonts bundled with the Android app (SIL OFL / Apache), loaded from the
// resources once, plus the system's colour emoji font.
class Fonts {
public:
    enum Id { Fredoka, Caveat, Neon, Pixel, Antiqua, Marker, Comic, Fell, Count };
    static Fonts &get();
    const QFont &font(Id id) const { return m_fonts[id]; }
    // Outline of s at the reference size, cached; scale by size / REF.
    const QPainterPath &path(Id id, const QString &s);
    qreal advance(Id id, const QString &s);
    qreal ascent(Id id) const { return m_ascent[id]; }
    qreal descent(Id id) const { return m_descent[id]; }
    static constexpr qreal REF = 100;

private:
    Fonts();
    QFont m_fonts[Count];
    qreal m_ascent[Count] = {};
    qreal m_descent[Count] = {};
    QHash<QString, QPainterPath> m_paths;
    QHash<QString, qreal> m_advances;
};

class Gfx {
public:
    Gfx(QPainter &c, Style st, float t, float dp, float P = 1, QPainter *rope = nullptr);

    QPainter &c;
    const Style st;
    const float t;
    const float dp;
    const float P;
    // World x of the canvas origin; pixel art snaps to its grid from here.
    float ox = 0;

    // n screen dp in local units.
    float px(float v) const { return v * dp / m_scales[m_depth]; }

    void push(float x, float y, float rot = 0, float sx = 1) { push(x, y, rot, sx, sx); }
    void push(float x, float y, float rot, float sx, float sy);
    void pop();

    void fill(const Pts &p, uint32_t col, const O &o = Opt::D);
    void ell(float cx, float cy, float rx, float ry, uint32_t col, const O &o = Opt::D, float rot = 0);
    void circ(float cx, float cy, float r, uint32_t col, const O &o = Opt::D);
    void rect(float x, float y, float w, float h, uint32_t col, const O &o = Opt::D, float r = 0);
    void blob(const Pts &p, uint32_t col, const O &o = Opt::D);
    void limb(const Pts &p0, float w, uint32_t col, const O &o = Opt::D);
    void ring(float cx, float cy, float rx, float ry, float w, uint32_t col);
    void cord(const Pts &p, float w, uint32_t col);
    void eye(float x, float y, float r);
    void starF(float cx, float cy, float r, uint32_t col);
    void shade(const Pts &p, float a);

    float measure(const QString &s, float size, FontRole role = FontRole::Default);
    void text(const QString &s, float x0, float y0, float size, uint32_t col, float rot = 0,
              FontRole role = FontRole::Default, uint32_t neon = 0xFFFF4FD8);
    void emoji(const QString &e, float x, float y, float size, bool mirror, float rot = 0);

    // Paper texture over everything drawn so far (target must start transparent).
    static void grain(QPainter &c, float w, float h);
    // Pixel art post-processing: a 1-px outline around all shapes.
    static void outline(QImage &img);

private:
    const QPainterPath &build(const Pts &p, bool closed = true);
    Pts wob(const Pts &p, float amp, int salt, bool closed, bool boilOn = true) const;
    void fillPath(const QPainterPath &p, uint32_t col);
    void strokePath(const QPainterPath &p, uint32_t col, float w, QPainter *cv = nullptr);
    void hatch(const Pts &p, float spacing, uint32_t col, float w, int dir = 1);
    Fonts::Id face(FontRole role) const;
    float fontSize(float size) const;

    QPainter *m_rope;
    bool m_aa;
    QPainterPath m_path;
    int m_n = 0;
    float m_boil;
    float m_scales[32];
    int m_depth = 0;
};
