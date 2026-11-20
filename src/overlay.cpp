#include "overlay.h"

#include <LayerShellQt/Window>
#include <QPainter>
#include <QScreen>

Overlay::Overlay(QScreen *screen, const Train &train, const QElapsedTimer &clock, float offX, int top, int height)
    : m_stage(1), m_train(train), m_clock(clock), m_offX(offX) {
    // Click-through: Qt gives a window that is transparent for input an empty
    // Wayland input region, so the pointer reaches whatever lies underneath.
    setFlags(Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    setScreen(screen);
    setTitle("Spruchband");
    resize(screen->geometry().width(), height);

    auto *layer = LayerShellQt::Window::get(this);
    layer->setLayer(LayerShellQt::Window::LayerOverlay);
    layer->setScope("spruchband");
    layer->setScreen(screen);
    layer->setWantsToBeOnActiveScreen(false);
    layer->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorLeft |
                                                    LayerShellQt::Window::AnchorRight));
    // -1: measured from the screen edge, ignoring the bar's reserved space.
    layer->setExclusiveZone(-1);
    layer->setMargins(QMargins(0, top, 0, 0));
    layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
    layer->setActivateOnShow(false);
    layer->setDesiredSize(QSize(0, height));
}

void Overlay::setLive(bool live) {
    if (live == m_live) return;
    m_live = live;
    update();
}

void Overlay::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(QRect(QPoint(), size()), Qt::transparent);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    if (!m_live) return;
    const float el = float(m_clock.nsecsElapsed() / 1e9);
    m_stage.render(p, width(), height(), m_train, el, m_train.up, m_offX);
    // The next frame, paced by the compositor's frame callbacks.
    update();
}
