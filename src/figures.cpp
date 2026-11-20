#include "figures.h"

#include <algorithm>

using namespace Opt;

const std::vector<CatInfo> &allCats() {
    static const std::vector<CatInfo> list = {
        {Cat::Vehicle, "fahrzeug", "Vehicles"},
        {Cat::Animal, "tier", "Animals"},
        {Cat::Person, "person", "People"},
        {Cat::Angel, "engel", "Angels"},
    };
    return list;
}

namespace {

/* ===================== VEHICLES ===================== */
class Flugzeug : public Figure {
    const Pts tail{-45, -4, -52, -22, -43, -23, -33, -6};
    const Pts cap = Geo::blob({-15.5f, -13, -14.5f, -19, -8, -20.5f, -4.5f, -15.5f, -9, -15});
    const Pts body = Geo::blob({-50, -3, -36, -8, -12, -9.5f, 14, -11, 32, -9, 40, -3, 36, 6, 10, 9, -20, 6, -48, 2});
    const Pts stripe{-34, -1, 24, -1};
    const Pts shield{-1, -10, 3, -17, 6, -10};
    const Pts gear1{4, 6, 9, 16}, gear2{16, 6, 11, 16};
    const Pts strut1{-8, -21, -8, 5}, strut2{17, -21, 17, 5};
    const O oA{.alpha = 0.85f};
    const O oDisc{.noLine = true, .alpha = 0.32f};

public:
    Flugzeug() : Figure("flugzeug", "Biplane", Cat::Vehicle, true, 47, 54, 26, 22, -47, 3, 1, 1, "🛩️") {}
    void bob(const St &st, float out[2]) const override {
        out[0] = S(st.t * 2) * 2.5f;
        out[1] = S(st.t * 1.3f) * 0.035f;
    }
    void draw(Gfx &g, const St &st) const override {
        const float t = st.t;
        g.limb(gear1, 2.2f, C::dgray);
        g.limb(gear2, 2.2f, C::dgray);
        g.circ(10, 17, 4.6f, C::dgray);
        g.circ(10, 17, 1.7f, C::gray, NL);
        g.fill(tail, C::red2);
        const float sw = S(t * 13);
        g.limb({-12, -11, -20, -12 + sw * 1.5f, -28, -10 - sw * 2}, 3.2f, C::yellow, SMOOTH);
        g.circ(-10, -13, 5.4f, C::skin);
        g.fill(cap, C::brown);
        g.rect(-7.5f, -17, 5, 3, C::lblue, D, 1.3f);
        g.fill(body, C::red);
        g.limb(stripe, 2.4f, C::cream, MINOR);
        g.fill(shield, C::lblue, oA);
        g.ell(-42, 0, 10, 2.4f, C::red2);
        g.ell(4, 6, 22, 3.4f, C::cream);
        g.limb(strut1, 1.6f, C::dgray);
        g.limb(strut2, 1.6f, C::dgray);
        g.ell(5, -24, 27, 3.6f, C::cream);
        g.ell(38, -2, 5, 8, C::gray);
        g.ell(44, -2, 2, 15, C::lblue, oDisc);
        g.ell(44, -2, 2.1f, std::abs(Co(t * 38)) * 14 + 1.5f, C::brown2);
        g.circ(44.6f, -2, 2, C::dgray);
    }
};

class Zeppelin : public Figure {
    const Pts fin1{-44, -13, -64, -28, -71, -26, -64, -5}, fin2{-44, 13, -64, 28, -71, 26, -64, 5};
    const Pts s1{-6, 17, -4, 22}, s2{8, 17, 6, 22};
    std::vector<Pts> panels;
    const O oPanel{.noLine = true, .minor = true, .smooth = true};

public:
    Zeppelin() : Figure("zeppelin", "Zeppelin", Cat::Vehicle, true, 62, 72, 29, 31, -69, 5, 0.75f, 1, "🛸", false) {
        for (float x : {-30.0f, -5.0f, 22.0f}) panels.push_back({x + 2, -19, x - 1, 0, x + 2, 19});
    }
    void bob(const St &st, float out[2]) const override {
        out[0] = S(st.t * 1.2f) * 3;
        out[1] = S(st.t * 0.9f) * 0.02f;
    }
    void draw(Gfx &g, const St &st) const override {
        g.fill(fin1, C::red2);
        g.fill(fin2, C::red2);
        g.limb(s1, 1.4f, C::dgray);
        g.limb(s2, 1.4f, C::dgray);
        g.rect(-15, 21, 28, 9, C::wood, D, 3);
        for (float x : {-10.0f, -3.0f, 4.0f}) g.rect(x, 23, 4.5f, 3.5f, C::yellow, NL);
        g.ell(-18, 25.5f, 1.4f, 1 + std::abs(Co(st.t * 30)) * 5, C::dgray);
        g.ell(0, 0, 60, 20, C::silver);
        g.ell(-4, -8, 46, 6.5f, 0xFFE9EDF2, MINOR);
        for (const auto &p : panels) g.limb(p, 1, C::gray2, oPanel);
        g.ell(-63, 0, 11, 3, C::red2);
        g.ell(58, 0, 3.4f, 7, C::gray2);
    }
};

class Auto : public Figure {
    const Pts cabin{-31, -25, -25, -40, -20, -43.5f, 12, -43.5f, 16, -40, 27, -25};
    const Pts win1{-26, -27, -21, -39.5f, -6, -39.5f, -6, -27}, win2{-2, -27, -2, -39.5f, 11, -39.5f, 20, -27};
    const Pts hairS = Geo::blob({1.5f, -33, 2, -38, 7, -38.5f, 10.5f, -35, 6, -35.5f});
    const Pts door{-4, -25, -4, -12}, handle{-12, -21, -8, -21};

public:
    Auto() : Figure("auto", "Small car", Cat::Vehicle, false, 48, 48, 44, 0, -47, -12, 1.1f, 1, "🚗") {}
    void bob(const St &st, float out[2]) const override {
        out[0] = -std::abs(S(st.dist * 0.09f)) * 0.8f;
        out[1] = 0;
    }
    void draw(Gfx &g, const St &st) const override {
        g.fill(cabin, C::teal);
        g.fill(win1, C::lblue);
        g.fill(win2, C::lblue);
        g.circ(6, -32.5f, 4.8f, C::skin);
        g.fill(hairS, C::hair);
        g.rect(-45, -27, 90, 18, C::teal, D, 6);
        g.limb(door, 1, C::teal2, MINOR);
        g.limb(handle, 1.6f, C::teal2, MINOR);
        g.ell(42, -19, 2.8f, 3.6f, C::yellow);
        g.rect(-46, -23, 3, 5.5f, C::red, D, 1);
        g.rect(-48, -13, 9, 4, C::dgray, D, 2);
        g.rect(39, -13, 9, 4, C::dgray, D, 2);
        const float a = st.dist / 9;
        for (float x : {-25.0f, 26.0f}) {
            g.circ(x, -9, 9, C::dgray);
            g.circ(x, -9, 4.4f, C::gray);
            g.circ(x + Co(a) * 2.6f, -9 + S(a) * 2.6f, 1.1f, C::dgray, NL);
        }
    }
};

/* ======================= ANIMALS ======================= */
void storkWing(Gfx &g, float f0, uint32_t col, uint32_t tip) {
    const float ty = -4 - 30 * f0, tx = -14 + 4 * std::abs(f0);
    g.blob({6, -4, tx + 10, ty, tx - 7, ty + 2, -12, -2}, tip);
    const float k = 0.55f;
    g.fill({6, -4, lerpf(6, tx + 10, k), lerpf(-4, ty, k), lerpf(-12, tx - 7, k), lerpf(-2, ty + 2, k), -12, -2}, col, NL);
}

class Storch : public Figure {
    const Pts l1{-14, 4, -40, 7}, l2{-14, 6, -39, 10};
    const Pts tailS{-18, -4, -30, -2, -18, 4}, neck{16, -3, 24, -6, 30, -7};
    const Pts beak{35, -9.5f, 57, -5.5f, 35, -4.5f};
    static constexpr uint32_t wingFar = 0xFFE8ECF1;

public:
    Storch() : Figure("storch", "Stork", Cat::Animal, true, 57, 44, 36, 14, -41, 9, 0.95f, 1, "🕊️") {}
    void bob(const St &st, float out[2]) const override {
        out[0] = S(st.t * 5 + PIf) * 3;
        out[1] = 0;
    }
    void draw(Gfx &g, const St &st) const override {
        storkWing(g, S(st.t * 5 - 0.5f) * 0.9f, wingFar, C::dgray);
        g.limb(l1, 2.3f, C::orange);
        g.limb(l2, 2.3f, C::orange);
        g.fill(tailS, C::white);
        g.ell(0, 0, 22, 9.5f, C::white, D, -0.06f);
        g.limb(neck, 6, C::white, SMOOTH);
        g.circ(31, -7, 5.5f, C::white);
        g.fill(beak, C::orange);
        g.eye(32.8f, -8.4f, 1.3f);
        storkWing(g, S(st.t * 5), C::white, C::dark);
    }
};

class Hummel : public Figure {
    const Pts b1 = Geo::band(0, 0, 17, 12.5f, -10, -5), b2 = Geo::band(0, 0, 17, 12.5f, 1.5f, 6.5f);
    const Pts sting{-16, -1.5f, -23, 1, -16, 3.5f}, ant{16, -8, 18, -15, 23, -18};
    const O oW1{.alpha = 0.55f}, oW2{.alpha = 0.6f};

public:
    Hummel() : Figure("hummel", "Bumblebee", Cat::Animal, true, 25, 24, 20, 15, -22, 1, 0.9f, 0.85f, "🐝") {}
    void bob(const St &st, float out[2]) const override {
        out[0] = S(st.t * 3) * 6;
        out[1] = S(st.t * 3 + 1) * 0.09f;
    }
    void draw(Gfx &g, const St &st) const override {
        const float t = st.t;
        g.ell(-5, -13, 7, 4.5f, C::lblue, oW1, -0.6f + S(t * 42) * 0.4f);
        for (int i = 0; i <= 2; i++) {
            const float x = -6.0f + 6 * i;
            g.limb({x, 9, x - 2 + S(t * 8 + i) * 0.8f, 14}, 1.6f, C::dark);
        }
        g.ell(0, 0, 17, 12.5f, C::yellow);
        g.fill(b1, C::dark, NL);
        g.fill(b2, C::dark, NL);
        g.fill(sting, C::dark);
        g.limb(ant, 1.4f, C::dark, SMOOTH);
        g.circ(23.5f, -18.2f, 1.9f, C::dark);
        g.circ(16, -1, 8, C::dark);
        g.circ(19.5f, -3, 2.8f, C::white, NL);
        g.eye(20.3f, -3, 1.4f);
        g.circ(21.5f, 1.5f, 1.6f, C::pink, MINOR);
        g.ell(-2, -14, 8.5f, 5, C::lblue, oW2, -0.3f + S(t * 42 + 1) * 0.45f);
    }
};

void dogLeg(Gfx &g, float x, float ph, uint32_t col) {
    const float sw = S(ph) * 5, lift = std::max(0.0f, Co(ph)) * 3;
    g.limb({x, -13, x + sw * 0.5f, -7, x + sw, -1.5f - lift}, 4.6f, col);
}

class Dackel : public Figure {
    const Pts pole{-8, -22, -10, -54};
    const Pts harness = Geo::band(0, -17, 28, 8.5f, -13, -8.5f);
    const Pts collar{21.5f, -28, 23.5f, -18};

public:
    Dackel() : Figure("dackel", "Dachshund", Cat::Animal, false, 47, 40, 58, 0, -10, -55, 1, 1, "🐕") {}
    void bob(const St &st, float out[2]) const override {
        out[0] = -std::abs(S(st.dist / 7)) * 1.4f;
        out[1] = 0;
    }
    void draw(Gfx &g, const St &st) const override {
        const float ph = st.dist / 7, t = st.t;
        dogLeg(g, -20, ph + PIf, C::brown2);
        dogLeg(g, 18, ph, C::brown2);
        g.limb({-26, -19, -33, -25, -37, -31 + S(t * 12) * 3}, 3, C::brown, SMOOTH);
        g.limb(pole, 1.7f, C::wood);
        g.circ(-10, -55.5f, 2.1f, C::gold);
        g.ell(0, -17, 28, 8.5f, C::brown);
        g.fill(harness, C::red, NL);
        dogLeg(g, -16, ph, C::brown);
        dogLeg(g, 22, ph + PIf, C::brown);
        g.ell(30, -25, 9, 7.5f, C::brown, D, -0.15f);
        g.ell(38, -22, 7, 4.5f, C::brown);
        g.circ(44.5f, -23, 2.2f, C::dark);
        g.blob({25, -30, 30, -29.5f, 29.5f, -18 + S(ph) * 1.5f, 24, -17 + S(ph) * 1.5f}, C::brown2);
        g.eye(32.4f, -27.4f, 1.4f);
        g.limb(collar, 2.6f, C::red);
    }
};

/* ====================== PEOPLE ====================== */
class Person : public Figure {
    const uint32_t shirt, pants, hair;
    const QString kind;
    const uint32_t capCol;
    const uint32_t pantsFar = Col::dark(pants, 0.22f);
    const uint32_t shirtFar = Col::dark(shirt, 0.2f);
    const Pts torso = Geo::blob({-6, -28, 6, -28, 7, -45, 3, -50.5f, -3, -50.5f, -7, -45});
    const Pts armFar{1, -46, 6, -44, 10, -49}, armNear{1, -46, 4, -38.5f, 10, -37};
    const Pts neck{1, -50, 1.5f, -53};
    const Pts ponyCap = Geo::blob({-5, -56, -6, -62, 0, -65.5f, 7, -64.5f, 9.5f, -60, 3, -61.5f, -1.5f, -58});
    const Pts hatHair = Geo::blob({-4, -54, -5, -60, 0, -61, -1, -57});
    const Pts hatBand{-4.5f, -65.5f, 8.5f, -65.5f};
    const Pts capHair = Geo::blob({-5.5f, -57, -5, -62, 1, -65, 7, -63.5f, 9, -60});

    void legs(Gfx &g, float ph, uint32_t col) const {
        const float a1 = S(ph) * 0.45f, bend = std::max(0.0f, Co(ph)) * 0.75f, a2 = a1 - bend;
        const float kx = 14 * S(a1), ky = -28 + 14 * Co(a1);
        const float fx = kx + 14 * S(a2), fy = ky + 14 * Co(a2);
        g.limb({0, -28, kx, ky, fx, fy}, 5.4f, col);
        g.ell(fx + 2.2f, fy - 0.6f, 3.8f, 2.2f, C::dark, D, -a2 * 0.5f);
    }

public:
    Person(const char *id, const char *name, const char *emoji, uint32_t shirt, uint32_t pants, uint32_t hair,
           const char *kind, uint32_t cap = 0, float k = 1)
        : Figure(id, name, Cat::Person, false, 14, 13, 72, 0, 10, -38, 0.8f, k, emoji), shirt(shirt), pants(pants),
          hair(hair), kind(QString::fromLatin1(kind)), capCol(cap) {}
    void bob(const St &st, float out[2]) const override {
        const float a = S(st.dist / 9) * 0.45f;
        out[0] = 28 * (1 - Co(a));
        out[1] = 0;
    }
    void draw(Gfx &g, const St &st) const override {
        const float ph = st.dist / 9;
        legs(g, ph + PIf, pantsFar);
        g.limb(armFar, 4.6f, shirtFar);
        g.circ(10, -49, 2.5f, C::skin2);
        if (kind == "hat") g.rect(-13, -48, 8, 17, C::green, D, 2.5f);
        g.fill(torso, shirt);
        legs(g, ph, pants);
        g.limb(neck, 4, C::skin);
        if (kind == "pony") {
            const float sw = S(ph * 2) * 1.5f;
            g.limb({-5, -61, -10.5f, -58 + sw, -12, -52 + sw}, 4.2f, hair, SMOOTH);
        }
        g.circ(2, -57, 7, C::skin);
        g.eye(6, -57.8f, 1.1f);
        g.circ(6.6f, -54.3f, 1.4f, C::pink, MINOR);
        if (kind == "pony") {
            g.fill(ponyCap, hair);
        } else if (kind == "hat") {
            g.fill(hatHair, hair);
            g.rect(-4.5f, -72, 13, 9, C::brown, D, 3.5f);
            g.ell(2, -63, 10.5f, 2.2f, C::brown2);
            g.limb(hatBand, 1.6f, C::red, MINOR);
        } else if (kind == "cap") {
            g.fill(capHair, hair);
            g.ell(10, -60.6f, 5, 1.6f, capCol);
            g.ell(1.5f, -61.8f, 7.3f, 4.2f, capCol);
        }
    }
    void drawFront(Gfx &g, const St &) const override {
        g.limb(armNear, 4.8f, shirt);
        g.circ(10.5f, -37, 2.6f, C::skin);
    }
};

/* ======================= ANGELS ======================= */
const Pts &puttoWingPts() {
    static const Pts p = Geo::blob({0, 0, -4, -10, -12, -18, -21, -19, -17, -14, -23, -11, -17, -7, -20, -2, -10, 1});
    return p;
}
void puttoWing(Gfx &g, uint32_t col) {
    static const Pts line{-5, -4, -15, -12};
    g.fill(puttoWingPts(), col);
    g.limb(line, 0.9f, 0xFFC9D4E2, MINOR);
}

class Putto : public Figure {
    const Pts armBack{0, -2, -6, 4, -9, 9}, leg1{-4, 10, -12, 16, -19, 13}, leg2{1, 12, -5, 20, -12, 21};
    const Pts smile{9.5f, -9.5f, 12, -8.4f, 14, -9.8f}, armFront{4, -2, 11, 3, 17, 1};
    const Pts curls{1, -23, 4.2f, 7, -25, 4.5f, 13, -22.5f, 4, -2, -17, 3.8f, 16, -18, 3};
    const O oSmile{.noLine = true, .minor = true, .smooth = true};
    static constexpr uint32_t drape = 0xFF9CC9E8, wingFar = 0xFFE3E9F1;

public:
    Putto() : Figure("putto", "Cherub", Cat::Angel, true, 22, 27, 30, 23, 17, 1, 0.85f, 1, "👼", false) {}
    void bob(const St &st, float out[2]) const override {
        out[0] = S(st.t * 3) * 4;
        out[1] = S(st.t * 1.5f) * 0.05f;
    }
    void draw(Gfx &g, const St &st) const override {
        const float t = st.t, wr = -0.25f + S(t * 9) * 0.5f;
        g.push(-4, -6, wr - 0.15f);
        puttoWing(g, wingFar);
        g.pop();
        g.limb(armBack, 5, C::skin);
        g.limb(leg1, 6.5f, C::skin, SMOOTH);
        g.limb(leg2, 6.5f, C::skin, SMOOTH);
        g.ell(0, 3, 11, 13, C::skin, D, 0.2f);
        g.blob({-10, 4, 8, 7, 7, 14, -4, 15.5f, -14, 13, -25, 15 + S(t * 6) * 2.5f, -18, 8}, drape);
        g.circ(7, -14, 10, C::skin);
        for (size_t i = 0; i + 2 < curls.size(); i += 3) g.circ(curls[i], curls[i + 1], curls[i + 2], C::blond);
        g.circ(13, -10, 2.6f, C::pink, MINOR);
        g.eye(12, -15, 1.4f);
        g.limb(smile, 0.9f, C::dark, oSmile);
        g.push(-3, -5, wr);
        puttoWing(g, C::white);
        g.pop();
        g.limb(armFront, 5, C::skin);
        g.circ(17, 1, 2.6f, C::skin);
    }
};

const Pts &bigWingPts() {
    static const Pts p = Geo::blob({0, 0, -6, -16, -18, -32, -36, -44, -52, -47, -45, -39, -53, -33, -43, -27, -49, -20,
                                    -37, -15, -41, -7, -25, -4, -12, 4});
    return p;
}
void bigWing(Gfx &g, uint32_t col, uint32_t line) {
    static const Pts l1{-10, -6, -34, -28}, l2{-16, -2, -40, -18};
    g.fill(bigWingPts(), col);
    g.limb(l1, 1, line, MINOR);
    g.limb(l2, 1, line, MINOR);
}

class Erzengel : public Figure {
    const Pts sash{8, -20, -4, 2, -12, 22}, foot{-2, 35, -8, 41};
    const Pts hairBack = Geo::blob({-2, -38, -12, -33, -16, -21, -6, -24});
    const Pts hairTop = Geo::blob({-5, -31, -4, -38, 4, -39.5f, 9.5f, -35, 3, -35.5f, -1, -30});
    const Pts trumpet{8, -28, 33, -33}, bell{32, -34.5f, 43, -39.5f, 43, -26.5f, 32, -31.5f};
    const Pts armFront{2, -18, 10, -20, 17.5f, -29}, armBack{0, -16, -4, -4, -6, 6};
    static constexpr uint32_t wingFar = 0xFFDDE4EE, wingFarL = 0xFFBCC7D6, wingNearL = 0xFFD3DBE6;

public:
    Erzengel() : Figure("erzengel", "Archangel", Cat::Angel, true, 44, 54, 66, 42, -6, 8, 0.8f, 1, "😇", false) {}
    void bob(const St &st, float out[2]) const override {
        out[0] = S(st.t * 2) * 4;
        out[1] = S(st.t * 1.1f) * 0.03f;
    }
    void draw(Gfx &g, const St &st) const override {
        const float t = st.t, wr = S(t * 3) * 0.28f;
        g.push(-5, -18, wr - 0.18f);
        bigWing(g, wingFar, wingFarL);
        g.pop();
        g.blob({6, -22, 11, -4, 14, 22, 6, 34, -8, 36, -22, 38 + S(t * 4) * 2, -31, 30 + S(t * 4 + 1) * 3, -14, 12, -8, -20},
               C::robe);
        g.limb(sash, 3, C::blue, SMOOTH);
        g.limb(foot, 3, C::skin);
        g.fill(hairBack, C::gold);
        g.circ(2, -30, 8, C::skin);
        g.fill(hairTop, C::gold);
        g.ring(0, -45, 9, 2.6f, 1.8f, C::gold);
        g.eye(6.6f, -31, 1.3f);
        g.limb(trumpet, 2.6f, C::gold);
        g.fill(bell, C::gold);
        g.limb(armFront, 5.5f, C::robe);
        g.circ(17.5f, -29.5f, 2.8f, C::skin);
        g.push(-2, -16, wr);
        bigWing(g, C::white, wingNearL);
        g.pop();
        g.limb(armBack, 5.5f, C::robe);
        g.circ(-6, 7.5f, 2.9f, C::skin);
    }
};

} // namespace

namespace Figures {
const std::vector<const Figure *> &all() {
    static const Flugzeug flugzeug;
    static const Zeppelin zeppelin;
    static const Auto car;
    static const Storch storch;
    static const Hummel hummel;
    static const Dackel dackel;
    static const Person walker("spaziergaengerin", "Walker", "🚶‍♀️", 0xFFE8705A, 0xFF3C5A8C, C::hair, "pony");
    static const Person hiker("wanderer", "Hiker", "🚶", 0xFFC9453D, 0xFF9B8A62, 0xFF6B5440, "hat");
    static const Person child("kind", "Child", "🧒", 0xFFF2C14A, 0xFF4A7FB8, 0xFF8A5A2B, "cap", 0xFF3D7FC4, 0.74f);
    static const Putto putto;
    static const Erzengel erzengel;
    static const std::vector<const Figure *> list = {&flugzeug, &zeppelin, &car,  &storch, &hummel,  &dackel,
                                                     &walker,   &hiker,    &child, &putto, &erzengel};
    return list;
}

const Figure *byId(const QString &id) {
    for (const Figure *f : all())
        if (f->id == id) return f;
    return nullptr;
}

std::vector<const Figure *> persons() {
    std::vector<const Figure *> out;
    for (const Figure *f : all())
        if (f->cat == Cat::Person) out.push_back(f);
    return out;
}
}
