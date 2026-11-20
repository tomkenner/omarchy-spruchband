// One transparent, click-through strip across one screen, on the layer shell's
// overlay layer. Every screen gets its own strip for a flyby; all of them draw
// the same train, each shifted by where its screen sits in the world.
#pragma once

#include "stage.h"

#include <QElapsedTimer>
#include <QRasterWindow>

class Overlay : public QRasterWindow {
public:
    Overlay(QScreen *screen, const Train &train, const QElapsedTimer &clock, float offX, int top, int height);

    // Live strips repaint every frame; the others stay clear and idle.
    void setLive(bool live);
    float offX() const { return m_offX; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    Stage m_stage;
    const Train &m_train;
    const QElapsedTimer &m_clock;
    const float m_offX;
    bool m_live = false;
};
