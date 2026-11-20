// The live preview at the top of the settings window: one flyby after another
// with the current settings, drawn by the same Stage as the overlay.
#pragma once

#include "settings.h"
#include "stage.h"

#include <QElapsedTimer>
#include <QQuickPaintedItem>
#include <QTimer>
#include <optional>

class StagePreview : public QQuickPaintedItem {
    Q_OBJECT
public:
    explicit StagePreview(QQuickItem *parent = nullptr);

    // Starts the next flyby now, in the given style if one is named.
    Q_INVOKABLE void next(const QString &style = {});
    void paint(QPainter *painter) override;

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void onFrame();

    Settings m_set;
    Picker m_picker{m_set};
    Stage m_stage;
    std::optional<Train> m_train;
    QElapsedTimer m_clock;
    QTimer m_frame, m_gap;
    float m_lane = 0;
};
