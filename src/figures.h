// The eleven figures, ported from the Android app's gfx/Figure.kt and
// gfx/Figures2.kt. A figure faces right. Origin: fliers at the body centre,
// walkers on the ground.
#pragma once

#include "gfx.h"

#include <QString>
#include <cmath>
#include <vector>

// Time and distance travelled (in figure units) of one flyby.
struct St {
    float t = 0;
    float dist = 0;
};

enum class Cat { Vehicle, Animal, Person, Angel };

struct CatInfo {
    Cat cat;
    const char *id;
    const char *name;
};
const std::vector<CatInfo> &allCats();

class Figure {
public:
    Figure(const char *id, const char *name, Cat cat, bool air, float front, float back, float top, float bottom,
           float towX, float towY, float spd, float k, const char *emoji, bool emojiFacesLeft = true)
        : id(QString::fromLatin1(id)), name(name), emoji(QString::fromUtf8(emoji)), cat(cat), air(air), front(front),
          back(back), top(top), bottom(bottom), towX(towX), towY(towY), spd(spd), k(k), emojiFacesLeft(emojiFacesLeft) {}
    virtual ~Figure() = default;

    // out[0] = vertical offset (units), out[1] = rotation (rad)
    virtual void bob(const St &st, float out[2]) const = 0;
    virtual void draw(Gfx &g, const St &st) const = 0;
    virtual void drawFront(Gfx &, const St &) const {}

    const QString id;
    const char *name; // English; translated where it is shown
    const QString emoji;
    const Cat cat;
    const bool air;
    const float front, back, top, bottom, towX, towY, spd, k;
    const bool emojiFacesLeft;
    // Tow point in the Emoji style, relative to the emoji's centre in emoji sizes (facing right);
    // NaN = same as towX/towY. The emoji is narrower than the drawn figure, so the banner would hang in the air.
    float emojiTowX = NAN, emojiTowY = 0;
};

namespace Figures {
const std::vector<const Figure *> &all();
const Figure *byId(const QString &id);
std::vector<const Figure *> persons();
}

inline float S(float x) { return std::sin(x); }
inline float Co(float x) { return std::cos(x); }
constexpr float PIf = 3.14159265358979f;
inline float lerpf(float a, float b, float k) { return a + (b - a) * k; }

// Gait for the Emoji style, which has no legs: walkers rock and hop in the same step rhythm the drawn
// figures move their legs in. out[0] = lift (fraction of the emoji size, negative = up),
// out[1] = tilt (rad). Flyers hover anyway and stay at 0.
inline void emojiGait(const Figure &fig, const St &st, float out[2]) {
    out[0] = out[1] = 0;
    if (fig.air) return;
    if (fig.cat == Cat::Person) {
        const float ph = st.dist / 9;
        out[0] = -std::abs(S(ph)) * 0.06f;
        out[1] = S(ph) * 0.13f;
    } else if (fig.cat == Cat::Animal) {
        const float ph = st.dist / 7;
        out[0] = -std::abs(S(ph)) * 0.07f;
        out[1] = Co(ph) * 0.06f;
    } else {
        out[0] = -std::abs(S(st.dist * 0.09f)) * 0.02f;
    }
}

namespace C {
constexpr uint32_t red = 0xFFE5534B, red2 = 0xFFB93A35, cream = 0xFFFFF4DC, yellow = 0xFFF6C344;
constexpr uint32_t orange = 0xFFEC7A3C, blue = 0xFF3D7FC4, lblue = 0xFFA9DCF5, teal = 0xFF2FA39B;
constexpr uint32_t teal2 = 0xFF22807A, green = 0xFF4F9D5D, brown = 0xFFA8683F, brown2 = 0xFF7C4A2C;
constexpr uint32_t wood = 0xFF6D4A30, skin = 0xFFF3C3A0, skin2 = 0xFFE3A27D, gray = 0xFFB8C0CA;
constexpr uint32_t gray2 = 0xFF8E98A5, dgray = 0xFF4A4F5C, white = 0xFFFFFFFF, pink = 0xFFF59AB0;
constexpr uint32_t gold = 0xFFE9B43B, dark = 0xFF2A2235, silver = 0xFFCFD5DD, robe = 0xFFF3EEE4;
constexpr uint32_t hair = 0xFF5B3A25, blond = 0xFFF0C85C, rope = 0xFF5A4632;
}
