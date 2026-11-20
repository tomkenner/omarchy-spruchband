#include "preview.h"

#include <QPainter>
#include <algorithm>

StagePreview::StagePreview(QQuickItem *parent) : QQuickPaintedItem(parent), m_stage(1) {
    setAntialiasing(true);
    setFillColor(Qt::transparent);
    m_frame.setInterval(16);
    connect(&m_frame, &QTimer::timeout, this, &StagePreview::onFrame);
    m_gap.setSingleShot(true);
    m_gap.setInterval(500);
    connect(&m_gap, &QTimer::timeout, this, [this] { next(); });
}

void StagePreview::geometryChange(const QRectF &n, const QRectF &o) {
    QQuickPaintedItem::geometryChange(n, o);
    // The first flyby waits until the item has a size to fly across.
    if (!m_train && !m_gap.isActive() && n.width() > 0 && n.height() > 0) m_gap.start(100);
}

void StagePreview::next(const QString &style) {
    m_gap.stop();
    m_train.reset();
    if (width() <= 0 || height() <= 0) return;
    m_set.sync();
    const Figure *fig = m_picker.figure();
    if (!fig) {
        update();
        return;
    }
    const QString text = m_picker.text().value_or(QStringLiteral("Spruchband"));
    Style st = m_picker.style();
    if (!style.isEmpty()) styleFromId(style, &st);
    m_train = m_stage.makeTrain(*fig, text, st, m_set.size(), m_set.band(), m_set.speed(), m_set.direction(),
                                float(width()), m_picker.persons());
    const float h = float(height());
    m_lane = fig->air ? std::max(m_train->up + (h - m_train->height()) / 2, std::min(m_train->up, h * 0.6f))
                      : h - m_train->down;
    m_clock.start();
    m_frame.start();
}

void StagePreview::onFrame() {
    if (!m_train) return;
    if (m_clock.nsecsElapsed() / 1e9 > m_train->duration()) {
        m_train.reset();
        m_frame.stop();
        m_gap.start();
    }
    update();
}

void StagePreview::paint(QPainter *painter) {
    if (!m_train) return;
    m_stage.render(*painter, float(width()), float(height()), *m_train, float(m_clock.nsecsElapsed() / 1e9), m_lane);
}
