// One line on what the service is doing, in the Omarchy language. Shared by
// the service (the bar panel shows its answer) and the settings window.
#pragma once

#include "settings.h"

#include <QJsonObject>
#include <QString>

QString statusText(const Settings &set, const QJsonObject &status, bool running);
