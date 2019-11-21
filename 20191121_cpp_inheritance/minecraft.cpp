#include <string>

class Block {
public:
    Block() {}

    void vykresli() {
        // ...
    }

    void poKliknuti() {
        // ...
    }

    virtual bool lzeVytezit() { return true; }

    virtual std::string barva() = 0;

};

class TravnatyBlock : public Block {
    TravnatyBlock() : Block() { }


    std::string barva() { return "green"; }
};

class DiamantovyBlock : public Block {
    DiamantovyBlock() : Block() { }

    bool lzeVytezit() { return false; }

    std::string barva() { return "white"; }
};
