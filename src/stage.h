// One flyby: figure plus banner, measured once and drawn per frame. Ported
// from the Android app's gfx/Stage.kt. A train lives in world coordinates
// spanning every screen; each screen draws it shifted by its own offset.
#pragma once

#include "banners.h"

#include <QImage>
#include <QPainter>
#include <cmath>

struct Train {
    const Figure *fig = nullptr;
    const BannerKind *kind = nullptr;
    Layout L;
    Style style = Style::Cartoon;
    float s = 1, bh = 0;
    int d = 1;
    float v = 1, x0 = 0, x1 = 0, P = 2;
    // Room above and below the lane in px, including the margin.
    float up = 0, down = 0;
    // How far the train reaches ahead of and behind the figure's origin.
    float front = 0, behind = 0;

    float height() const { return up + down; }
    // Duration in seconds.
    float duration() const { return std::abs(x1 - x0) / v; }
    // World x range covered at time el, in px.
    void span(float el, float *lo, float *hi) const;
};

class Stage {
public:
    explicit Stage(float dp = 1) : m_dp(dp) {}

    Train makeTrain(const Figure &fig, const QString &text, Style style, float sizeDp, float bhDp, float speedDp,
                    int dir, float width, const std::vector<const Figure *> &persons, const Rnd &rnd = {});

    // Draws the flyby at time el (s) onto a transparent w×h target whose
    // origin is world x = offX. laneY is the lane in target coordinates.
    // Returns true once the train has left.
    bool render(QPainter &p, float w, float h, const Train &tr, float el, float laneY, float offX = 0);

private:
    bool drawInto(Gfx &g, const Train &tr, float el, float laneY, float P, float offX);

    float m_dp;
    QImage m_lo, m_loR;
};
