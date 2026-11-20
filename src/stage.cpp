#include "stage.h"

#include <QRandomGenerator>
#include <algorithm>

void Train::span(float el, float *lo, float *hi) const {
    const float fx = x0 + d * v * el;
    if (d > 0) {
        *lo = fx - behind;
        *hi = fx + front;
    } else {
        *lo = fx - front;
        *hi = fx + behind;
    }
}

Train Stage::makeTrain(const Figure &fig, const QString &text, Style style, float sizeDp, float bhDp, float speedDp,
                       int dir, float width, const std::vector<const Figure *> &persons, const Rnd &rnd0) {
    const Rnd rnd = rnd0 ? rnd0 : Rnd([] { return float(QRandomGenerator::global()->generateDouble()); });
    const BannerKind &kind = BannerKind::of(fig);
    const float size = sizeDp * m_dp;
    const float s = size / 60.0f * (&kind == &Banners::transparent() ? 1.0f : fig.k);
    float bh = bhDp * m_dp;
    const float P = std::max(2.0f, std::floor(bh * 0.6f / 8.0f + 0.5f));
    if (style == Style::Pixel) {
        const float fac = &kind == &Banners::wimpel()      ? 0.44f
                        : &kind == &Banners::anhaenger()   ? 0.58f
                        : &kind == &Banners::transparent() ? 0.56f
                                                           : 0.6f;
        bh = std::max(bh, 8 * P / fac + 2);
    }
    const int d = dir == 0 ? (rnd() < 0.5f ? 1 : -1) : dir;
    QImage scratch(1, 1, QImage::Format_ARGB32_Premultiplied);
    QPainter mp(&scratch);
    Gfx g(mp, style, 0, m_dp, P);
    const std::vector<const Figure *> pool = persons.empty() ? std::vector<const Figure *>{&fig} : persons;

    Train tr;
    tr.fig = &fig;
    tr.kind = &kind;
    // Collections clip their sayings when they are read; this is the backstop
    // for every other way text gets here.
    tr.L = kind.layout(g, clipText(text), bh, s, pool, rnd, fig);
    tr.style = style;
    tr.s = s;
    tr.bh = bh;
    tr.d = d;
    tr.P = P;
    const float margin = 10 * m_dp + 8 * s;
    const auto ext = kind.extent(fig, s, bh);
    tr.front = fig.front * s + 8 * m_dp;
    tr.behind = std::max(fig.back * s, -fig.towX * s + tr.L.len) + 8 * m_dp;
    tr.v = speedDp * m_dp * fig.spd;
    tr.x0 = d > 0 ? -tr.front : width + tr.front;
    tr.x1 = d > 0 ? width + tr.behind : -tr.behind;
    tr.up = ext[0] + margin;
    tr.down = ext[1] + (fig.air ? margin : 4 * m_dp);
    return tr;
}

bool Stage::render(QPainter &p, float w, float h, const Train &tr, float el, float laneY, float offX) {
    switch (tr.style) {
    case Style::Pixel: {
        // The low-resolution grid is laid out in device pixels, so each art
        // pixel is a whole number of screen pixels even at fractional scales.
        const qreal dpr = p.device() ? p.device()->devicePixelRatio() : 1.0;
        const int pd = std::max(1, int(std::lround(tr.P * dpr)));
        const float P = float(pd / dpr);
        const int lw = std::max(1, int(std::ceil(w * dpr / pd))), lh = std::max(1, int(std::ceil(h * dpr / pd)));
        if (m_lo.width() != lw || m_lo.height() != lh) {
            m_lo = QImage(lw, lh, QImage::Format_ARGB32_Premultiplied);
            m_loR = QImage(lw, lh, QImage::Format_ARGB32_Premultiplied);
        }
        m_lo.fill(Qt::transparent);
        m_loR.fill(Qt::transparent);
        bool done;
        {
            QPainter lc(&m_lo), rc(&m_loR);
            for (QPainter *q : {&lc, &rc}) {
                q->scale(1 / P, 1 / P);
                q->translate(-offX, 0);
            }
            Gfx g(lc, Style::Pixel, el, m_dp, P, &rc);
            g.ox = offX;
            done = drawInto(g, tr, el, laneY, P, offX);
        }
        Gfx::outline(m_lo);
        p.save();
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
        const QRectF target(0, 0, lw * P, lh * P);
        p.drawImage(target, m_loR);
        p.drawImage(target, m_lo);
        p.restore();
        return done;
    }
    case Style::Paper:
    case Style::Aquarell: {
        p.save();
        p.translate(-offX, 0);
        Gfx g(p, tr.style, el, m_dp);
        g.ox = offX;
        const bool done = drawInto(g, tr, el, laneY, 1, offX);
        p.restore();
        Gfx::grain(p, w, h);
        return done;
    }
    default: {
        p.save();
        p.translate(-offX, 0);
        Gfx g(p, tr.style, el, m_dp);
        g.ox = offX;
        const bool done = drawInto(g, tr, el, laneY, 1, offX);
        p.restore();
        return done;
    }
    }
}

bool Stage::drawInto(Gfx &g, const Train &tr, float el, float laneY, float P, float offX) {
    const bool pix = tr.style == Style::Pixel;
    const float tA = pix ? std::floor(el * 10) / 10 : el;
    float fx = tr.x0 + tr.d * tr.v * el;
    if (pix) fx = std::floor((fx - offX) / P + 0.5f) * P + offX;
    const float distPx = tr.v * tA;
    const float s = tr.s;
    const Figure &fig = *tr.fig;
    const int d = tr.d;
    St st;
    st.t = tA;
    st.dist = distPx / s;
    float bob[2];
    fig.bob(st, bob);
    const float rot = bob[1] * d;
    const float gy = laneY + bob[0] * s;
    if (tr.kind == &Banners::transparent()) {
        tr.kind->draw(g, tr.L, fx + d * 10 * s, laneY, d, tA, tr.bh, s, laneY, distPx);
    } else {
        const float tx = d * fig.towX * s, ty = fig.towY * s;
        const float ax = fx + tx * std::cos(rot) - ty * std::sin(rot);
        const float ay = gy + tx * std::sin(rot) + ty * std::cos(rot);
        tr.kind->draw(g, tr.L, ax, ay, d, tA, tr.bh, s, laneY, distPx);
        if (tr.style == Style::Emoji) {
            const float hgt = (fig.top + fig.bottom) * s;
            const float cy = fig.air ? gy + (fig.bottom - fig.top) * s / 2 : gy - fig.top * s / 2;
            g.emoji(fig.emoji, fx, cy, hgt * 0.95f, (d > 0) == fig.emojiFacesLeft);
        } else {
            g.push(fx, gy, rot, d * s, s);
            fig.draw(g, st);
            g.pop();
        }
    }
    return d > 0 ? fx >= tr.x1 : fx <= tr.x1;
}
