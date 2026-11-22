#include "gfx.h"

#include <QFontDatabase>
#include <QFontMetricsF>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

namespace {
constexpr float PI_F = 3.14159265358979f;
inline float deg(float rad) { return rad * 180.0f / PI_F; }
// Kotlin's roundToInt rounds halves up; std::lround rounds them away from zero.
inline float roundK(float v) { return std::floor(v + 0.5f); }
}

// ---------- Styles ----------
const std::vector<StyleInfo> &allStyles() {
    static const std::vector<StyleInfo> list = {
        {Style::Cartoon, "cartoon", "Cartoon", "Clear colours, dark outline. Easy to read everywhere."},
        {Style::Pixel, "pixel", "Pixel art", "Chunky pixels like old game consoles."},
        {Style::Paper, "paper", "Paper cut-out", "Like coloured paper with soft shadows."},
        {Style::Silhouette, "silhouette", "Silhouette", "Black silhouettes, text cut out of the banner."},
        {Style::Doodle, "doodle", "Doodle", "Wobbly pencil lines and crayon hatching."},
        {Style::Neon, "neon", "Neon", "Glowing outlines. Best in dark mode."},
        {Style::Aquarell, "aquarell", "Watercolour", "Soft, translucent washes of colour."},
        {Style::Kupferstich, "kupferstich", "Copper engraving", "Sepia hatching like old book prints."},
        {Style::Comic, "comic", "Comic", "Bold outlines and halftone dots."},
        {Style::Knete, "knete", "Clay", "Rounded and shiny like modelling clay."},
        {Style::Emoji, "emoji", "Emoji", "Your system's emoji as figures."},
    };
    return list;
}

bool styleFromId(const QString &id, Style *out) {
    for (const auto &s : allStyles())
        if (id == QLatin1String(s.id)) {
            *out = s.style;
            return true;
        }
    return false;
}

QString styleId(Style st) {
    for (const auto &s : allStyles())
        if (s.style == st) return QString::fromLatin1(s.id);
    return QStringLiteral("cartoon");
}

// ---------- Colours ----------
namespace Col {
uint32_t withAlpha(uint32_t col, float a) {
    a = std::clamp(a, 0.0f, 1.0f);
    return (col & 0xFFFFFF) | (uint32_t(int(a * 255)) << 24);
}

uint32_t mix(uint32_t a, uint32_t b, float k) {
    auto ch = [&](int s) {
        return std::clamp(int(((a >> s) & 255) * (1 - k) + ((b >> s) & 255) * k), 0, 255);
    };
    return (0xFFu << 24) | (uint32_t(ch(16)) << 16) | (uint32_t(ch(8)) << 8) | uint32_t(ch(0));
}

float lum(uint32_t col) {
    return (((col >> 16) & 255) * 0.3f + ((col >> 8) & 255) * 0.59f + (col & 255) * 0.11f) / 255.0f;
}
}

// ---------- Geometry ----------
namespace Geo {
Pts ell(float cx, float cy, float rx, float ry, float rot, int n) {
    Pts out(n * 2);
    const float c = std::cos(rot), s = std::sin(rot);
    for (int i = 0; i < n; i++) {
        const float a = float(i) / n * 2 * PI_F;
        const float x = std::cos(a) * rx, y = std::sin(a) * ry;
        out[i * 2] = cx + x * c - y * s;
        out[i * 2 + 1] = cy + x * s + y * c;
    }
    return out;
}

Pts rect(float x, float y, float w, float h, float r0) {
    if (r0 <= 0) return {x, y, x + w, y, x + w, y + h, x, y + h};
    const float r = std::min({r0, w / 2, h / 2});
    Pts out(4 * 6 * 2);
    int k = 0;
    auto arc = [&](float cx, float cy, float a0) {
        for (int i = 0; i <= 5; i++) {
            const float a = a0 + i / 5.0f * (PI_F / 2);
            out[k++] = cx + std::cos(a) * r;
            out[k++] = cy + std::sin(a) * r;
        }
    };
    arc(x + w - r, y + r, -PI_F / 2);
    arc(x + w - r, y + h - r, 0);
    arc(x + r, y + h - r, PI_F / 2);
    arc(x + r, y + r, PI_F);
    return out;
}

// Catmull-Rom curve through the control points.
Pts blob(const Pts &p, bool closed, int seg) {
    const int n = int(p.size()) / 2;
    const int last = closed ? n : n - 1;
    Pts out(last * seg * 2 + (closed ? 0 : 2));
    auto px = [&](int i) { return closed ? p[((i + n) % n) * 2] : p[std::clamp(i, 0, n - 1) * 2]; };
    auto py = [&](int i) { return closed ? p[((i + n) % n) * 2 + 1] : p[std::clamp(i, 0, n - 1) * 2 + 1]; };
    int k = 0;
    for (int i = 0; i < last; i++) {
        for (int s = 0; s < seg; s++) {
            const float t = float(s) / seg, t2 = t * t, t3 = t2 * t;
            auto cr = [&](float a, float b, float c, float d) {
                return 0.5f * (2 * b + (-a + c) * t + (2 * a - 5 * b + 4 * c - d) * t2 + (-a + 3 * b - 3 * c + d) * t3);
            };
            out[k++] = cr(px(i - 1), px(i), px(i + 1), px(i + 2));
            out[k++] = cr(py(i - 1), py(i), py(i + 1), py(i + 2));
        }
    }
    if (!closed) {
        out[k++] = p[(n - 1) * 2];
        out[k] = p[(n - 1) * 2 + 1];
    }
    return out;
}

// Stripe of an ellipse between x1 and x2 (bumblebee).
Pts band(float cx, float cy, float rx, float ry, float x1, float x2, int n) {
    Pts out((n + 1) * 4);
    for (int i = 0; i <= n; i++) {
        const float x = x1 + (x2 - x1) * i / n;
        const float h = ry * std::sqrt(std::max(0.0f, 1 - (x / rx) * (x / rx)));
        out[i * 2] = cx + x;
        out[i * 2 + 1] = cy - h;
        const int j = (2 * n + 1 - i) * 2;
        out[j] = cx + x;
        out[j + 1] = cy + h;
    }
    return out;
}

Pts star(float cx, float cy, float r, float inner) {
    Pts out(16);
    for (int i = 0; i < 8; i++) {
        const float a = i / 8.0f * 2 * PI_F - PI_F / 2;
        const float rr = i % 2 == 1 ? r * inner : r;
        out[i * 2] = cx + std::cos(a) * rr;
        out[i * 2 + 1] = cy + std::sin(a) * rr;
    }
    return out;
}

float rnd(float n) {
    const float x = std::sin(n * 127.1f + 311.7f) * 43758.547f;
    return x - std::floor(x);
}
}

// ---------- Fonts ----------
Fonts &Fonts::get() {
    static Fonts fonts;
    return fonts;
}

Fonts::Fonts() {
    struct Def {
        const char *file;
        int weight;
        bool italic;
    };
    // Same files and weights as the Android app's Fonts class.
    const Def defs[Count] = {
        {"fredoka.ttf", 650, false},  {"caveat.ttf", 700, false},    {"tiltneon.ttf", 0, false},
        {"pressstart2p.ttf", 0, false}, {"cormorant_italic.ttf", 700, true}, {"permanentmarker.ttf", 0, false},
        {"bangers.ttf", 0, false},     {"imfell_italic.ttf", 0, true},
    };
    for (int i = 0; i < Count; i++) {
        const int id = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/") + defs[i].file);
        const QStringList families = id >= 0 ? QFontDatabase::applicationFontFamilies(id) : QStringList();
        QFont f(families.value(0, QStringLiteral("sans-serif")));
        f.setPixelSize(int(REF));
        f.setHintingPreference(QFont::PreferNoHinting);
        f.setItalic(defs[i].italic);
        if (defs[i].weight) {
            f.setWeight(QFont::Weight(defs[i].weight));
            f.setVariableAxis(QFont::Tag("wght"), defs[i].weight);
        }
        m_fonts[i] = f;
        const QFontMetricsF fm(f);
        m_ascent[i] = fm.ascent();
        m_descent[i] = fm.descent();
    }
}

const QPainterPath &Fonts::path(Id id, const QString &s) {
    const QString key = QString::number(id) + QChar(0x1f) + s;
    auto it = m_paths.constFind(key);
    if (it != m_paths.constEnd()) return *it;
    if (m_paths.size() > 4000) m_paths.clear();
    QPainterPath p;
    p.addText(0, 0, m_fonts[id], s);
    return *m_paths.insert(key, p);
}

qreal Fonts::advance(Id id, const QString &s) {
    const QString key = QString::number(id) + QChar(0x1f) + s;
    auto it = m_advances.constFind(key);
    if (it != m_advances.constEnd()) return *it;
    if (m_advances.size() > 8000) m_advances.clear();
    return *m_advances.insert(key, QFontMetricsF(m_fonts[id]).horizontalAdvance(s));
}

// ---------- Gfx ----------
Gfx::Gfx(QPainter &c_, Style st_, float t_, float dp_, float P_, QPainter *rope)
    : c(c_), st(st_), t(t_), dp(dp_), P(P_), m_rope(rope ? rope : &c_), m_aa(st_ != Style::Pixel),
      m_boil(std::floor(t_ * 7)) {
    m_scales[0] = 1;
    c.setRenderHint(QPainter::Antialiasing, m_aa);
    m_rope->setRenderHint(QPainter::Antialiasing, m_aa);
    m_path.setFillRule(Qt::WindingFill);
}

void Gfx::push(float x, float y, float rot, float sx, float sy) {
    c.save();
    c.translate(x, y);
    if (rot != 0) c.rotate(deg(rot));
    c.scale(sx, sy);
    m_depth++;
    m_scales[m_depth] = m_scales[m_depth - 1] * std::abs(sx);
}

void Gfx::pop() {
    c.restore();
    m_depth--;
}

const QPainterPath &Gfx::build(const Pts &p, bool closed) {
    m_path.clear();
    m_path.setFillRule(Qt::WindingFill);
    if (p.size() < 2) return m_path;
    m_path.moveTo(p[0], p[1]);
    for (size_t i = 2; i + 1 < p.size(); i += 2) m_path.lineTo(p[i], p[i + 1]);
    if (closed) m_path.closeSubpath();
    return m_path;
}

Pts Gfx::wob(const Pts &p, float amp, int salt, bool closed, bool boilOn) const {
    const float s = m_n * 97.0f + salt * 13.0f + (boilOn ? m_boil * 1013.0f : 0.0f);
    const int m = int(p.size()) / 2;
    Pts out(p.size() + (closed ? 2 : 0));
    for (int i = 0; i < m; i++) {
        out[i * 2] = p[i * 2] + (Geo::rnd(s + i * 3.1f) - 0.5f) * amp;
        out[i * 2 + 1] = p[i * 2 + 1] + (Geo::rnd(s + i * 7.7f + 5.0f) - 0.5f) * amp;
    }
    if (closed) {
        out[m * 2] = out[0] + amp * 0.4f;
        out[m * 2 + 1] = out[1] + amp * 0.3f;
    }
    return out;
}

void Gfx::fillPath(const QPainterPath &p, uint32_t col) { c.fillPath(p, Col::q(col)); }

void Gfx::strokePath(const QPainterPath &p, uint32_t col, float w, QPainter *cv) {
    QPen pen(Col::q(col), std::max(w, 0.01f), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    (cv ? cv : &c)->strokePath(p, pen);
}

static void bbox(const Pts &p, float b[4]) {
    b[0] = b[1] = 1e9f;
    b[2] = b[3] = -1e9f;
    for (size_t i = 0; i + 1 < p.size(); i += 2) {
        b[0] = std::min(b[0], p[i]);
        b[2] = std::max(b[2], p[i]);
        b[1] = std::min(b[1], p[i + 1]);
        b[3] = std::max(b[3], p[i + 1]);
    }
}

// Parallel lines at an angle, clipped to the shape. dir = 1 or -1.
void Gfx::hatch(const Pts &p, float spacing, uint32_t col, float w, int dir) {
    if (spacing <= 0) return;
    float b[4];
    bbox(p, b);
    const float h = b[3] - b[1];
    c.save();
    c.setClipPath(build(p), Qt::IntersectClip);
    c.setPen(QPen(Col::q(col), w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (float x = b[0] - h; x < b[2] + h; x += spacing) {
        if (dir > 0) c.drawLine(QPointF(x, b[3]), QPointF(x + h, b[1]));
        else c.drawLine(QPointF(x, b[1]), QPointF(x + h, b[3]));
    }
    c.restore();
}

void Gfx::fill(const Pts &p, uint32_t col, const O &o) {
    m_n++;
    if (o.minor && (st == Style::Silhouette || st == Style::Neon || st == Style::Pixel)) return;
    const float a = o.alpha;
    switch (st) {
    case Style::Cartoon:
    case Style::Emoji:
        fillPath(build(p), a < 1 ? Col::withAlpha(col, a) : col);
        if (!o.noLine) strokePath(m_path, Col::INK, px(o.lw));
        break;
    case Style::Paper:
        c.save();
        c.translate(px(0.8f), px(1.6f));
        fillPath(build(p), 0x3D32230F);
        c.restore();
        fillPath(build(p), a < 1 ? Col::withAlpha(col, std::min(1.0f, a + 0.2f)) : col);
        break;
    case Style::Silhouette:
        if (a < 0.5f && !o.cloth) return;
        fillPath(build(p), Col::INK);
        break;
    case Style::Pixel:
        if (a < 0.5f) return;
        fillPath(build(p), col);
        break;
    case Style::Doodle: {
        build(p);
        if (a >= 0.5f || o.cloth) fillPath(m_path, 0xFFFFFDF6);
        hatch(p, px(o.cloth ? 6.0f : 3.6f), Col::withAlpha(col, o.cloth ? 0.35f : 0.8f * a), px(1.3f));
        const float amp = px(1.6f);
        strokePath(build(wob(p, amp, 1, true), false), Col::PENCIL, px(1.3f));
        if (p.size() > 12) strokePath(build(wob(p, amp, 2, true), false), Col::withAlpha(Col::PENCIL, 0.5f), px(1.3f));
        break;
    }
    case Style::Neon: {
        const uint32_t nc = o.neon != 0 ? o.neon : col;
        if (a < 0.5f) {
            strokePath(build(p), Col::withAlpha(nc, 0.6f), px(1));
            return;
        }
        fillPath(build(p), 0xEB0E0A22);
        strokePath(m_path, Col::withAlpha(nc, 0.25f), px(6));
        strokePath(m_path, Col::light(nc, 0.35f), px(1.7f));
        break;
    }
    case Style::Aquarell:
        if (a >= 0.5f || o.cloth) fillPath(build(p), 0xD9FFFDF8);
        fillPath(build(wob(p, px(2.2f), 1, true, false), false), Col::withAlpha(col, 0.45f * a));
        fillPath(build(wob(p, px(2.6f), 2, true, false), false), Col::withAlpha(col, 0.38f * a));
        strokePath(m_path, Col::withAlpha(Col::dark(col, 0.25f), 0.35f * a), px(1));
        break;
    case Style::Kupferstich: {
        fillPath(build(p), Col::mix(col, Col::PARCH, 0.72f));
        const float l = Col::lum(col);
        if (!o.cloth && a >= 0.5f) {
            if (l < 0.8f) hatch(p, px(2.6f), Col::withAlpha(Col::SEPIA, 0.55f), px(0.7f), 1);
            if (l < 0.5f) hatch(p, px(2.6f), Col::withAlpha(Col::SEPIA, 0.5f), px(0.7f), -1);
            if (l < 0.28f) hatch(p, px(1.6f), Col::withAlpha(Col::SEPIA, 0.45f), px(0.7f), 1);
        } else if (o.cloth) {
            hatch(p, px(5), Col::withAlpha(Col::SEPIA, 0.12f), px(0.6f), 1);
        }
        if (!o.noLine || o.cloth) strokePath(build(p), Col::SEPIA, px(1.1f));
        break;
    }
    case Style::Comic:
        fillPath(build(p), a < 1 ? Col::withAlpha(col, a) : col);
        if (a >= 0.5f && !o.cloth && p.size() > 12) {
            float b[4];
            bbox(p, b);
            const float sp = px(4.6f), r = px(0.95f);
            c.save();
            c.setClipPath(build(p), Qt::IntersectClip);
            c.setPen(Qt::NoPen);
            c.setBrush(Col::q(Col::withAlpha(Col::dark(col, 0.35f), 0.7f)));
            float y = b[1] + (b[3] - b[1]) * 0.45f;
            int row = 0;
            while (y < b[3]) {
                float x = b[0] + (row % 2 == 1 ? sp / 2 : 0.0f);
                while (x < b[2]) {
                    c.drawEllipse(QPointF(x, y), r, r);
                    x += sp;
                }
                y += sp * 0.87f;
                row++;
            }
            c.restore();
        }
        if (!o.noLine) strokePath(build(p), Col::INK, px(3));
        break;
    case Style::Knete:
        c.save();
        c.translate(px(1.4f), px(2.4f));
        fillPath(build(p), 0x40000000);
        c.restore();
        fillPath(build(p), a < 1 ? Col::withAlpha(col, a) : col);
        if (a >= 0.5f) {
            float b[4];
            bbox(p, b);
            const float w = b[2] - b[0], h = b[3] - b[1], r = std::max(w, h);
            if (r > 0) {
                QRadialGradient shine(QPointF(b[0] + w * 0.35f, b[1] + h * 0.3f), r * 0.7f);
                shine.setColorAt(0, Col::q(0x80FFFFFF));
                shine.setColorAt(1, Col::q(0x00FFFFFF));
                c.fillPath(m_path, shine);
                QRadialGradient shadow(QPointF(b[0] + w * 0.45f, b[1] + h * 0.4f), r * 0.75f);
                shadow.setColorAt(0, Col::q(0x00000000));
                shadow.setColorAt(0.55, Col::q(0x00000000));
                shadow.setColorAt(1, Col::q(0x38000000));
                c.fillPath(m_path, shadow);
            }
        }
        break;
    }
}

void Gfx::ell(float cx, float cy, float rx, float ry, uint32_t col, const O &o, float rot) {
    fill(Geo::ell(cx, cy, rx, ry, rot), col, o);
}
void Gfx::circ(float cx, float cy, float r, uint32_t col, const O &o) {
    fill(Geo::ell(cx, cy, r, r, 0, r < 4 ? 14 : 26), col, o);
}
void Gfx::rect(float x, float y, float w, float h, uint32_t col, const O &o, float r) {
    fill(Geo::rect(x, y, w, h, r), col, o);
}
void Gfx::blob(const Pts &p, uint32_t col, const O &o) { fill(Geo::blob(p), col, o); }

// A line with width (legs, arms, poles).
void Gfx::limb(const Pts &p0, float w, uint32_t col, const O &o) {
    m_n++;
    if (o.minor && (st == Style::Silhouette || st == Style::Neon)) return;
    const Pts p = o.smooth ? Geo::blob(p0, false, 5) : p0;
    build(p, false);
    switch (st) {
    case Style::Cartoon:
    case Style::Emoji:
        if (!o.noLine) strokePath(m_path, Col::INK, w + px(4));
        strokePath(m_path, col, w);
        break;
    case Style::Paper:
        c.save();
        c.translate(px(0.8f), px(1.6f));
        strokePath(m_path, 0x3D32230F, w);
        c.restore();
        strokePath(m_path, col, w);
        break;
    case Style::Silhouette:
        strokePath(m_path, Col::INK, w);
        break;
    case Style::Pixel:
        strokePath(m_path, col, std::max(w, P / m_scales[m_depth]));
        break;
    case Style::Doodle:
        strokePath(m_path, Col::withAlpha(col, 0.55f), w);
        strokePath(build(wob(p, px(1.4f), 3, false), false), Col::PENCIL, px(1.3f));
        break;
    case Style::Neon: {
        const uint32_t nc = o.neon != 0 ? o.neon : col;
        strokePath(m_path, Col::withAlpha(nc, 0.22f), w + px(5));
        strokePath(m_path, Col::light(nc, 0.4f), std::max(px(1.8f), w * 0.35f));
        break;
    }
    case Style::Aquarell:
        strokePath(m_path, Col::withAlpha(col, 0.55f), w);
        strokePath(build(wob(p, px(1.6f), 3, false, false), false), Col::withAlpha(col, 0.35f), w * 0.8f);
        break;
    case Style::Kupferstich:
        if (!o.noLine) strokePath(m_path, Col::SEPIA, w + px(2.2f));
        strokePath(m_path, Col::mix(col, Col::PARCH, 0.72f), w);
        break;
    case Style::Comic:
        if (!o.noLine) strokePath(m_path, Col::INK, w + px(5));
        strokePath(m_path, col, w);
        break;
    case Style::Knete:
        c.save();
        c.translate(px(1.2f), px(2));
        strokePath(m_path, 0x40000000, w);
        c.restore();
        strokePath(m_path, col, w);
        if (w > px(2.5f)) {
            c.save();
            c.translate(-w * 0.14f, -w * 0.16f);
            strokePath(m_path, Col::withAlpha(Col::light(col, 0.5f), 0.7f), w * 0.3f);
            c.restore();
        }
        break;
    }
}

void Gfx::ring(float cx, float cy, float rx, float ry, float w, uint32_t col) {
    Pts e = Geo::ell(cx, cy, rx, ry);
    e.push_back(e[0]);
    e.push_back(e[1]);
    limb(e, w, col);
}

// Rope or string in world coordinates (w in dp). Pixel art draws it on its own
// layer, without the outline.
void Gfx::cord(const Pts &p, float w, uint32_t col) {
    QPainter *cv = st == Style::Pixel ? m_rope : &c;
    switch (st) {
    case Style::Doodle:
        strokePath(build(wob(p, px(1.2f), 5, false), false), Col::PENCIL, px(1.2f));
        break;
    case Style::Neon:
        build(p, false);
        strokePath(m_path, 0x40A0DCFF, px(4));
        strokePath(m_path, 0xFFBFEFFF, px(1.2f));
        break;
    default: {
        build(p, false);
        const uint32_t cc = st == Style::Silhouette ? Col::INK : st == Style::Kupferstich ? Col::SEPIA : col;
        strokePath(m_path, cc, st == Style::Pixel ? P : px(w), cv);
    }
    }
}

void Gfx::eye(float x, float y, float r) {
    switch (st) {
    case Style::Neon:
        fill(Geo::ell(x, y, r * 0.8f, r * 0.8f, 0, 10), 0xFFFFFFFF, Opt::NL);
        break;
    case Style::Pixel: {
        const float s = P / m_scales[m_depth];
        c.fillRect(QRectF(x - s / 2, y - s / 2, s, s), Col::q(Col::INK));
        break;
    }
    default:
        fillPath(build(Geo::ell(x, y, r, r * 1.1f, 0, 12)),
                 st == Style::Silhouette ? Col::PAPER : st == Style::Kupferstich ? Col::SEPIA : Col::INK);
        if (st == Style::Cartoon || st == Style::Comic || st == Style::Knete)
            fillPath(build(Geo::ell(x + r * 0.35f, y - r * 0.4f, r * 0.38f, r * 0.38f, 0, 8)), 0xFFFFFFFF);
    }
}

void Gfx::starF(float cx, float cy, float r, uint32_t col) {
    fill(Geo::star(cx, cy, r), col, O{.noLine = true, .neon = col});
}

// Shadow of a fold in the cloth (flat styles only).
void Gfx::shade(const Pts &p, float a) {
    if (st != Style::Cartoon && st != Style::Paper && st != Style::Comic && st != Style::Knete && st != Style::Emoji) return;
    fillPath(build(p), Col::withAlpha(0xFF3C2850, a));
}

// ---------- Text ----------
Fonts::Id Gfx::face(FontRole role) const {
    switch (st) {
    case Style::Pixel: return Fonts::Pixel;
    case Style::Doodle: return Fonts::Caveat;
    case Style::Neon: return Fonts::Neon;
    case Style::Kupferstich: return Fonts::Fell;
    case Style::Comic: return role == FontRole::Antiqua ? Fonts::Antiqua : Fonts::Comic;
    default:
        return role == FontRole::Marker ? Fonts::Marker : role == FontRole::Antiqua ? Fonts::Antiqua : Fonts::Fredoka;
    }
}

float Gfx::fontSize(float size) const {
    switch (st) {
    case Style::Pixel: {
        // Press Start 2P is only crisp at multiples of 8 pixels.
        const float low = std::max(8.0f, roundK(size / P / 8.0f) * 8);
        return low * P;
    }
    case Style::Doodle: return size * 1.25f;
    case Style::Kupferstich: return size * 1.1f;
    case Style::Comic: return size * 1.08f;
    default: return size;
    }
}

float Gfx::measure(const QString &s, float size, FontRole role) {
    return float(Fonts::get().advance(face(role), s)) * fontSize(size) / float(Fonts::REF);
}

void Gfx::text(const QString &s, float x0, float y0, float size, uint32_t col, float rot, FontRole role, uint32_t neon) {
    Fonts &fonts = Fonts::get();
    const Fonts::Id id = face(role);
    const float k = fontSize(size) / float(Fonts::REF);
    float x = x0, y = y0;
    if (st == Style::Pixel) {
        x = roundK((x - ox) / P) * P + ox;
        y = roundK(y / P) * P;
    }
    const QPainterPath &gp = fonts.path(id, s);
    const float adv = float(fonts.advance(id, s)) * k;
    const float base = float(fonts.ascent(id) - fonts.descent(id)) / 2 * k + (st == Style::Pixel ? P / 2 : 0.0f);
    c.save();
    c.translate(x, y);
    if (rot != 0 && st != Style::Pixel) c.rotate(deg(rot));
    c.translate(-adv / 2, base);
    c.scale(k, k);
    if (st == Style::Neon) {
        // Android draws a blurred shadow layer; two wide translucent strokes
        // give the same glow without blurring every frame.
        const float r = 12.0f * dp / 2.5f / k;
        strokePath(gp, Col::withAlpha(neon, 0.16f), r * 2);
        strokePath(gp, Col::withAlpha(neon, 0.32f), r);
        fillPath(gp, Col::light(neon, 0.45f));
        fillPath(gp, 0xCCFFFFFF);
    } else {
        const uint32_t color = st == Style::Silhouette ? Col::PAPER
                             : st == Style::Doodle      ? Col::PENCIL
                             : st == Style::Kupferstich ? Col::SEPIA
                                                        : col;
        if (st == Style::Comic) strokePath(gp, Col::INK, size * 0.12f / k);
        fillPath(gp, color);
    }
    c.restore();
}

// Emoji as the figure (Emoji style).
void Gfx::emoji(const QString &e, float x, float y, float size, bool mirror, float rot) {
    QFont f(QStringLiteral("Noto Color Emoji"));
    f.setPixelSize(std::max(1, int(roundK(size))));
    const QFontMetricsF fm(f);
    c.save();
    c.translate(x, y);
    // Rotate about the bottom edge so the emoji rocks from one foot to the other.
    if (rot != 0) {
        c.translate(0, size * 0.42f);
        c.rotate(deg(rot));
        c.translate(0, -size * 0.42f);
    }
    if (mirror) c.scale(-1, 1);
    c.setFont(f);
    c.setPen(Qt::black);
    c.drawText(QPointF(-fm.horizontalAdvance(e) / 2, (fm.ascent() - fm.descent()) / 2), e);
    c.restore();
}

void Gfx::grain(QPainter &c, float w, float h) {
    static const QImage noise = [] {
        QImage img(96, 96, QImage::Format_ARGB32);
        auto *rng = QRandomGenerator::global();
        for (int y = 0; y < 96; y++) {
            auto *line = reinterpret_cast<QRgb *>(img.scanLine(y));
            for (int x = 0; x < 96; x++) {
                const int v = 120 + int(rng->generateDouble() * 120);
                line[x] = qRgba(v, v, v, 26);
            }
        }
        return img;
    }();
    c.save();
    c.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    c.fillRect(QRectF(0, 0, w, h), QBrush(noise));
    c.restore();
}

void Gfx::outline(QImage &img) {
    const int w = img.width(), h = img.height();
    const int stride = int(img.bytesPerLine() / 4);
    auto *px = reinterpret_cast<QRgb *>(img.bits());
    std::vector<uint8_t> m(size_t(w) * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) m[size_t(y) * w + x] = qAlpha(px[y * stride + x]) > 0;
    const QRgb ink = Col::INK;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const size_t i = size_t(y) * w + x;
            if (m[i]) continue;
            if ((x > 0 && m[i - 1]) || (x < w - 1 && m[i + 1]) || (y > 0 && m[i - w]) || (y < h - 1 && m[i + w]))
                px[y * stride + x] = ink;
        }
}
