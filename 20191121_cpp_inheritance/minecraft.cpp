#include <string>

class Block {
    friend class Critter;
public:
    Block() {}

    void vykresli() {
        // ...
    }

    void poKliknuti() {
        if(!lzeVytezit())
            return;
        // ...
    }

    virtual bool lzeVytezit() { return true; }

    virtual std::string barva() = 0;

private:
    float x, y, z;
};

class TravnatyBlock : public Block {
public:
    TravnatyBlock() : Block() { }

    std::string barva() { return "green"; }
};

class DiamantovyBlock : public Block {
public:
    DiamantovyBlock() : Block() { }

    bool lzeVytezit() { return false; }

    std::string barva() { return "white"; }
};

class Critter {
public:
    void hraj(TravnatyBlock b) {
        b.x = 4;
    }

};


static void neco() {
    TravnatyBlock b;
    b.lzeVytezit();
    b.x = 4;
}
