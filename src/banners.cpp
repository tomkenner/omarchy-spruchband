#include "banners.h"

#include <algorithm>
#include <cmath>

using namespace Opt;

namespace {

constexpr float PI2 = 2 * PIf;
constexpr uint32_t BANNER_RED = 0xFFD23A32;
constexpr uint32_t CLOTH = 0xFFFFFDF7;

void measureChars(Gfx &g, Layout &L, float size, FontRole role = FontRole::Default) {
    L.size = size;
    L.cx.assign(L.text.size(), 0);
    float prev = 0;
    for (qsizetype i = 0; i < L.text.size(); i++) {
        const float w = g.measure(L.text.left(i + 1), size, role);
        L.cx[i] = (prev + w) / 2;
        prev = w;
    }
    L.textW = prev;
}

Pts edges(const Pts &mx, const Pts &my, float top, float bot) {
    const size_t n = mx.size();
    Pts out(n * 4);
    for (size_t i = 0; i < n; i++) {
        out[i * 2] = mx[i];
        out[i * 2 + 1] = my[i] - top;
    }
    for (size_t i = 0; i < n; i++) {
        const size_t j = n - 1 - i;
        out[(n + i) * 2] = mx[j];
        out[(n + i) * 2 + 1] = my[j] + bot;
    }
    return out;
}

Pts pl(const Pts &xs, const Pts &ys, float off) {
    Pts out(xs.size() * 2);
    for (size_t i = 0; i < xs.size(); i++) {
        out[i * 2] = xs[i];
        out[i * 2 + 1] = ys[i] + off;
    }
    return out;
}

/* ---------- Tow banner (plane, zeppelin) ---------- */
class Schlepp : public BannerKind {
public:
    const char *name() const override { return "Tow banner"; }
    Layout layout(Gfx &g, const QString &text, float bh, float, const std::vector<const Figure *> &, const Rnd &,
                  const Figure &) const override {
        Layout L;
        L.text = text;
        measureChars(g, L, bh * 0.6f);
        L.pad = bh * 0.55f;
        L.rope = std::max(36.0f * g.dp / 2.6f, bh * 2.1f);
        L.cloth = L.textW + L.pad * 2;
        L.len = L.rope + bh * 0.35f + L.cloth + bh * 0.9f;
        return L;
    }
    std::array<float, 2> extent(const Figure &fig, float s, float bh) const override {
        return {std::max(fig.top * s, bh * 0.8f), std::max(fig.bottom * s, bh * 1.35f)};
    }
    void draw(Gfx &g, const Layout &L, float ax, float ay, int d, float t, float bh, float, float, float) const override {
        const float qx = ax - d * L.rope, qy = ay + bh * 0.25f;
        Pts rope(22);
        for (int i = 0; i <= 10; i++) {
            const float k = i / 10.0f;
            rope[i * 2] = ax + (qx - ax) * k;
            rope[i * 2 + 1] = ay + (qy - ay) * k + S(PIf * k) * bh * 0.16f;
        }
        g.cord(rope, 1.4f, C::rope);
        const float px = qx - d * bh * 0.35f, py = qy;
        g.cord({qx, qy, px, py - bh * 0.55f}, 1, C::rope);
        g.cord({qx, qy, px, py + bh * 0.55f}, 1, C::rope);
        const int n = std::max(6, int(std::ceil(L.cloth / 9)));
        auto yAt = [&](float u) {
            return py + bh * (0.02f + 0.16f * std::pow(std::max(u / L.cloth, 0.0f), 1.4f)) * S(u / (bh * 5.5f) * PI2 - t * 7);
        };
        Pts mx(n + 1), my(n + 1);
        for (int i = 0; i <= n; i++) {
            const float u = L.cloth * i / n;
            mx[i] = px - d * u;
            my[i] = yAt(u);
        }
        const float h = bh / 2;
        g.fill(edges(mx, my, h, h), CLOTH, O{.cloth = true, .neon = 0xFF5FF2FF});
        for (int i = 0; i < n; i++) {
            const float sl = (my[i + 1] - my[i]) / std::max(1.0f, std::abs(mx[i + 1] - mx[i]));
            if (sl > 0.04f)
                g.shade({mx[i], my[i] - h, mx[i + 1], my[i + 1] - h, mx[i + 1], my[i + 1] + h, mx[i], my[i] + h},
                        std::min(0.22f, sl * 0.9f));
        }
        if (g.st != Style::Silhouette && g.st != Style::Neon) {
            g.limb(pl(mx, my, -h * 0.78f), bh * 0.07f, BANNER_RED, NL);
            g.limb(pl(mx, my, h * 0.78f), bh * 0.07f, BANNER_RED, NL);
        }
        g.limb({px, py - h * 1.15f, px, py + h * 1.15f}, std::max(2.4f, bh * 0.08f), C::dgray);
        g.circ(px, py + h * 1.2f, bh * 0.08f, C::dgray);
        const float ex = mx[n], ey = my[n], fl = S(t * 9) * bh * 0.12f;
        g.fill({ex, ey - h * 0.7f, ex - d * bh * 0.85f, ey + fl, ex, ey + h * 0.7f}, BANNER_RED, O{.neon = 0xFFFF4F7A});
        const float left = d > 0 ? px - L.cloth : px;
        for (qsizetype i = 0; i < L.text.size(); i++) {
            const QChar ch = L.text[i];
            if (ch == ' ') continue;
            const float x = left + L.pad + L.cx[i];
            const float u = d > 0 ? px - x : x - px;
            float rot = std::atan2(yAt(u + 2) - yAt(u - 2), float(-d * 4));
            if (d > 0) rot += PIf;
            g.text(QString(ch), x, yAt(u), L.size, 0xFFC8302C, rot);
        }
    }
};

/* ---------- Trailer with billboard (car) ---------- */
class Anhaenger : public BannerKind {
public:
    const char *name() const override { return "Billboard trailer"; }
    Layout layout(Gfx &g, const QString &text, float bh, float s, const std::vector<const Figure *> &, const Rnd &,
                  const Figure &) const override {
        Layout L;
        L.text = text;
        L.size = bh * 0.58f;
        L.textW = g.measure(text, L.size);
        L.board = L.textW + bh * 1.2f;
        L.bar = bh * 0.5f + 12 * s;
        L.len = L.bar + L.board + 6;
        return L;
    }
    std::array<float, 2> extent(const Figure &fig, float s, float bh) const override {
        return {std::max(fig.top * s, 24 * s + bh * 1.3f), 2.0f};
    }
    void draw(Gfx &g, const Layout &L, float ax, float ay, int d, float t, float bh, float s, float ground,
              float dist) const override {
        const float bump = S(dist * 0.07f) * 0.8f;
        const float front = ax - d * L.bar, back = front - d * L.board;
        const float x0 = std::min(front, back), x1 = std::max(front, back);
        const float chTop = ground - 19 * s + bump, chH = 7 * s;
        g.limb({ax, ay, front, chTop + chH / 2}, 3 * s, C::dgray);
        const int nAx = std::max(1, int(std::floor(L.board / (bh * 9) + 0.5f)));
        const float r = 7.5f * s;
        for (int i = 0; i < nAx; i++) {
            const float x = x0 + L.board * (i + 1) / (nAx + 1);
            g.circ(x, ground - r, r, C::dgray);
            g.circ(x, ground - r, r * 0.45f, C::gray);
            const float a = dist / r;
            g.circ(x + Co(a) * r * 0.28f, ground - r + S(a) * r * 0.28f, r * 0.12f, C::dgray, NL);
        }
        g.rect(x0, chTop, L.board, chH, C::red2, D, 2);
        const float bH = bh * 1.2f, bTop = chTop - 5 * s - bH;
        for (float x : {x0 + bh * 0.5f, x1 - bh * 0.5f}) g.limb({x, chTop, x, bTop + bH}, 2.4f * s, C::dgray);
        g.rect(x0, bTop, L.board, bH, C::yellow, O{.neon = 0xFFFFD34F}, 4);
        g.rect(x0 + bh * 0.12f, bTop + bh * 0.12f, L.board - bh * 0.24f, bH - bh * 0.24f, CLOTH, NL, 2);
        const int nb = int(std::floor(L.board / (bh * 0.7f)));
        const O minor{.noLine = true, .minor = true};
        for (int i = 0; i < nb; i++) {
            const bool on = (int(std::floor(t * 4)) + i) % 3 == 0;
            g.circ(x0 + L.board * (i + 0.5f) / nb, bTop + bh * 0.06f, bh * 0.07f, on ? 0xFFFFF3A8 : C::orange, minor);
        }
        g.text(L.text, (x0 + x1) / 2, bTop + bH / 2, L.size, 0xFF1F2A44, 0, FontRole::Default, 0xFF5FF2FF);
    }
};

/* ---------- Bunting (animals) ---------- */
constexpr uint32_t PEN[] = {0xFFE5534B, 0xFFF6C344, 0xFF3D7FC4, 0xFF4F9D5D, 0xFFF59AB0, 0xFF7D62B3};
constexpr uint32_t PEN_FG[] = {0xFFFFFFFF, 0xFF2A2235, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF2A2235, 0xFFFFFFFF};

class Wimpel : public BannerKind {
public:
    const char *name() const override { return "Bunting"; }
    Layout layout(Gfx &, const QString &text, float bh, float, const std::vector<const Figure *> &, const Rnd &,
                  const Figure &) const override {
        Layout L;
        L.text = text.toUpper();
        L.pw = bh * 0.82f;
        const float gap = bh * 0.1f, sp = bh * 0.45f;
        float x = 0;
        for (const QChar c : L.text) {
            if (c == ' ') {
                x += sp;
                continue;
            }
            L.itemCh.push_back(c);
            L.itemO.push_back(x + L.pw / 2);
            x += L.pw + gap;
        }
        L.lead = bh * 1.1f;
        L.content = x;
        L.str = L.lead + x + bh * 0.4f;
        L.len = L.str + bh;
        L.size = bh * 0.44f;
        return L;
    }
    std::array<float, 2> extent(const Figure &fig, float s, float bh) const override {
        const float ty = fig.towY * s;
        return {std::max(fig.top * s, -(ty - bh * 2.0f)), std::max(std::max(fig.bottom * s, ty + bh * 1.55f), 2.0f)};
    }
    void draw(Gfx &g, const Layout &L, float ax, float ay, int d, float t, float bh, float, float, float) const override {
        const float ls = L.str, dd = bh * 0.38f;
        auto px = [&](float u) { return ax - d * u; };
        auto py = [&](float u) {
            return ay + dd * S(PIf * std::min(1.0f, u / ls)) + bh * 0.14f * S(u / (bh * 6) * PI2 - t * 4) * (u / ls);
        };
        Pts str(82);
        for (int i = 0; i <= 40; i++) {
            const float u = ls * i / 40;
            str[i * 2] = px(u);
            str[i * 2 + 1] = py(u);
        }
        const float ex = px(ls), ey = py(ls), bx = ex - d * bh * 0.25f, by = ey - bh * 1.35f;
        g.cord({ex, ey, ex - d * bh * 0.1f, ey - bh * 0.45f + S(t * 3) * 2, bx, by + bh * 0.52f}, 1, C::rope);
        g.cord(str, 1.3f, C::rope);
        for (size_t i = 0; i < L.itemCh.size(); i++) {
            const float u = d > 0 ? L.lead + L.content - L.itemO[i] : L.lead + L.itemO[i];
            const float ul = u + d * L.pw / 2, ur = u - d * L.pw / 2;
            const float lx = px(ul), ly = py(ul), rx = px(ur), ry = py(ur);
            const float mx = (lx + rx) / 2, my = (ly + ry) / 2;
            const float slope = std::atan2(ry - ly, rx - lx);
            const float sw = S(t * 3.2f + i * 0.7f) * 0.1f + slope * 0.5f;
            const float apx = mx - S(sw) * bh, apy = my + Co(sw) * bh;
            const uint32_t bg = PEN[i % 6], fg = PEN_FG[i % 6];
            g.fill({lx, ly, rx, ry, apx, apy}, bg, O{.neon = bg});
            g.text(QString(L.itemCh[i]), mx + (apx - mx) * 0.34f, my + (apy - my) * 0.34f, L.size, fg, slope,
                   FontRole::Default, bg);
        }
        g.ell(bx, by, bh * 0.42f, bh * 0.52f, C::red, O{.neon = 0xFFFF5A7A});
        g.fill({bx - bh * 0.07f, by + bh * 0.58f, bx + bh * 0.07f, by + bh * 0.58f, bx, by + bh * 0.48f}, C::red2, NL);
        g.ell(bx - bh * 0.14f, by - bh * 0.2f, bh * 0.08f, bh * 0.14f, C::white,
              O{.noLine = true, .minor = true, .alpha = 0.7f}, 0.4f);
    }
};

/* ---------- Protest placard (people) ---------- */
class Transparent : public BannerKind {
public:
    const char *name() const override { return "Placard"; }
    Layout layout(Gfx &g, const QString &text, float bh, float s, const std::vector<const Figure *> &pool,
                  const Rnd &rnd, const Figure &fig) const override {
        Layout L;
        L.text = text;
        measureChars(g, L, bh * 0.56f, FontRole::Marker);
        L.cloth = L.textW + bh * 1.4f;
        const float span = std::max(bh * 7, 170.0f * g.dp / 2.6f);
        L.n = std::max(2, int(std::ceil(L.cloth / span)) + 1);
        L.carriers.push_back(&fig);
        for (int i = 0; i < L.n - 1; i++)
            L.carriers.push_back(pool[std::clamp(int(rnd() * pool.size()), 0, int(pool.size()) - 1)]);
        L.sp = L.cloth / (L.n - 1);
        L.len = L.cloth + 16 * s;
        return L;
    }
    std::array<float, 2> extent(const Figure &, float s, float bh) const override { return {82 * s + bh * 1.65f, 2.0f}; }
    void draw(Gfx &g, const Layout &L, float ax, float, int d, float t, float bh, float s, float ground,
              float dist) const override {
        auto poleX = [&](int i) { return ax - d * i * L.sp; };
        const float bot = ground - 82 * s, top = bot - bh * 1.3f;
        const float x0 = std::min(poleX(0), poleX(L.n - 1)), x1 = std::max(poleX(0), poleX(L.n - 1));
        auto frac = [&](float x) {
            const float f = (x - x0) / L.sp;
            return f - std::floor(f);
        };
        auto yT = [&](float x) { return top + S(PIf * frac(x)) * bh * 0.08f; };
        auto yB = [&](float x) {
            return bot + S(PIf * frac(x)) * bh * 0.2f + S(x * 0.045f / (g.dp / 2.6f) + t * 3) * bh * 0.05f;
        };
        const int n = std::max(2, int(std::ceil((x1 - x0) / 8)));
        Pts shape((n + 1) * 4);
        for (int i = 0; i <= n; i++) {
            const float x = x0 + (x1 - x0) * i / n;
            shape[i * 2] = x;
            shape[i * 2 + 1] = yT(x);
            const int j = 2 * n + 1 - i;
            shape[j * 2] = x;
            shape[j * 2 + 1] = yB(x);
        }
        const auto &people = L.carriers;
        St st;
        float bob[2];
        // Poles behind the cloth
        for (size_t i = 0; i < people.size(); i++) {
            const Figure *f = people[i];
            const float fs = s * f->k, pxp = poleX(int(i));
            st.t = t;
            st.dist = dist / fs + i * 13;
            f->bob(st, bob);
            g.limb({pxp, ground - 30 * fs + bob[0] * fs, pxp, top - bh * 0.18f}, 2.6f * s, C::wood);
            g.circ(pxp, top - bh * 0.22f, 2.4f * s, C::gold);
        }
        g.fill(shape, 0xFFFAFAFA, O{.cloth = true, .neon = 0xFFFFD34F});
        for (qsizetype i = 0; i < L.text.size(); i++) {
            const QChar ch = L.text[i];
            if (ch == ' ') continue;
            const float x = x0 + bh * 0.7f + L.cx[i];
            g.text(QString(ch), x, (yT(x) + yB(x)) / 2, L.size, 0xFFC9302C, S(x * 0.045f + t * 3) * 0.03f, FontRole::Marker);
        }
        for (size_t i = 0; i < people.size(); i++) {
            const Figure *f = people[i];
            const float fs = s * f->k;
            st.t = t;
            st.dist = dist / fs + i * 13;
            f->bob(st, bob);
            if (g.st == Style::Emoji) {
                g.emoji(f->emoji, poleX(int(i)) - d * 11 * fs, ground - 31 * fs, 60 * fs, (d > 0) == f->emojiFacesLeft);
            } else {
                g.push(poleX(int(i)) - d * 10 * fs, ground + bob[0] * fs, 0, d * fs, fs);
                f->draw(g, st);
                f->drawFront(g, st);
                g.pop();
            }
        }
    }
};

/* ---------- Scroll (angels) ---------- */
class Banderole : public BannerKind {
public:
    const char *name() const override { return "Scroll"; }
    Layout layout(Gfx &g, const QString &text, float bh, float s, const std::vector<const Figure *> &, const Rnd &,
                  const Figure &fig) const override {
        Layout L;
        L.text = text;
        measureChars(g, L, bh * 0.62f, FontRole::Antiqua);
        L.lead = (fig.towX + fig.back) * s + bh * 0.6f;
        L.cloth = L.lead + L.textW + bh * 0.9f;
        L.len = L.cloth + bh * 0.8f;
        return L;
    }
    std::array<float, 2> extent(const Figure &fig, float s, float bh) const override {
        return {std::max(fig.top * s, -fig.towY * s + bh * 1.6f), std::max(fig.bottom * s, fig.towY * s + bh * 1.6f)};
    }
    void draw(Gfx &g, const Layout &L, float ax, float ay, int d, float t, float bh, float, float, float) const override {
        auto amp = [&](float u) { return bh * 0.42f * std::min(1.0f, u / (bh * 2.2f)); };
        auto yAt = [&](float u) {
            return ay + amp(u) * S(u / (bh * 7.5f) * PI2 - t * 2.2f + 1) + bh * 0.45f * std::min(1.0f, u / L.lead);
        };
        const int n = std::max(8, int(std::ceil(L.cloth / 7)));
        Pts mx(n + 1), my(n + 1), top((n + 1) * 2), bot((n + 1) * 2);
        for (int i = 0; i <= n; i++) {
            const float u = L.cloth * i / n, x = ax - d * u, y = yAt(u);
            const float dy = (yAt(u + 1) - yAt(u - 1)) / 2, len = std::hypot(1.0f, dy);
            float ux = -dy / len, uy = -d / len;
            if (uy > 0) {
                ux = -ux;
                uy = -uy;
            }
            const float h = bh * 0.5f * (0.75f + 0.25f * std::min(1.0f, u / (bh * 1.5f)));
            mx[i] = x;
            my[i] = y;
            top[i * 2] = x + ux * h;
            top[i * 2 + 1] = y + uy * h;
            bot[i * 2] = x - ux * h;
            bot[i * 2 + 1] = y - uy * h;
        }
        Pts shape((n + 1) * 4);
        for (int i = 0; i <= n; i++) {
            shape[i * 2] = top[i * 2];
            shape[i * 2 + 1] = top[i * 2 + 1];
            const int j = 2 * n + 1 - i;
            shape[j * 2] = bot[i * 2];
            shape[j * 2 + 1] = bot[i * 2 + 1];
        }
        g.fill(shape, 0xFFF3E3B5, O{.cloth = true, .neon = 0xFFFFD34F});
        for (int i = 0; i < n; i++) {
            const float sl = (my[i + 1] - my[i]) / std::max(1.0f, std::abs(mx[i + 1] - mx[i]));
            if (std::abs(sl) > 0.05f && (sl > 0) == (d > 0))
                g.shade({top[i * 2], top[i * 2 + 1], top[i * 2 + 2], top[i * 2 + 3], bot[i * 2 + 2], bot[i * 2 + 3],
                         bot[i * 2], bot[i * 2 + 1]},
                        std::min(0.2f, std::abs(sl) * 0.4f));
        }
        if (g.st != Style::Silhouette && g.st != Style::Neon) {
            Pts e1(top.size()), e2(bot.size());
            for (int i = 0; i <= n; i++) {
                e1[i * 2] = top[i * 2];
                e1[i * 2 + 1] = top[i * 2 + 1] + (my[i] - top[i * 2 + 1]) * 0.12f;
                e2[i * 2] = bot[i * 2];
                e2[i * 2 + 1] = bot[i * 2 + 1] + (my[i] - bot[i * 2 + 1]) * 0.12f;
            }
            g.limb(e1, bh * 0.05f, 0xFFC99A2E, NL);
            g.limb(e2, bh * 0.05f, 0xFFC99A2E, NL);
        }
        const float r = bh * 0.42f, cx = mx[n] - d * r * 0.4f, cy = my[n] + r * 0.2f;
        g.circ(cx, cy, r, 0xFFE2CD94, O{.neon = 0xFFFFD34F});
        g.ring(cx, cy, r * 0.5f, r * 0.5f, std::max(1.2f, bh * 0.04f), 0xFFB98A3A);
        g.circ(cx, cy, r * 0.16f, 0xFFB98A3A, NL);
        const float left = d > 0 ? ax - L.lead - L.textW : ax + L.lead;
        for (qsizetype i = 0; i < L.text.size(); i++) {
            const QChar ch = L.text[i];
            if (ch == ' ') continue;
            const float x = left + L.cx[i], u = d > 0 ? ax - x : x - ax;
            float rot = std::atan2(yAt(u + 2) - yAt(u - 2), float(-d * 4));
            if (d > 0) rot += PIf;
            g.text(QString(ch), x, yAt(u), L.size, 0xFF6B2A1A, rot, FontRole::Antiqua, 0xFFFFD34F);
        }
        for (int j = 0; j < 5; j++) {
            const float u = L.lead * 0.5f + (L.cloth - L.lead * 0.5f) * (j + 0.5f) / 5 + S(t + j) * bh * 0.3f;
            const float tw = 0.5f + 0.5f * S(t * 4 + j * 1.7f);
            if (tw > 0.15f) g.starF(ax - d * u, yAt(u) - bh * (0.95f + 0.25f * (j % 2)), bh * 0.2f * tw, C::gold);
        }
    }
};

} // namespace

namespace Banners {
const BannerKind &schlepp() { static const Schlepp k; return k; }
const BannerKind &anhaenger() { static const Anhaenger k; return k; }
const BannerKind &wimpel() { static const Wimpel k; return k; }
const BannerKind &transparent() { static const Transparent k; return k; }
const BannerKind &banderole() { static const Banderole k; return k; }
}

const BannerKind &BannerKind::of(const Figure &f) {
    switch (f.cat) {
    case Cat::Vehicle: return f.air ? Banners::schlepp() : Banners::anhaenger();
    case Cat::Animal: return Banners::wimpel();
    case Cat::Person: return Banners::transparent();
    case Cat::Angel: return Banners::banderole();
    }
    return Banners::schlepp();
}
