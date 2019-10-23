
// Chceme použít RBControl
#include "RBControl.hpp"

// už nemusíme psát rb::
using namespace rb;
using namespace std;

int x = 4;

struct motor {
    int rychlost;
    bool dopredu;
};

class LepsiMotor {
public:
    // konstruktor
    LepsiMotor(bool prohod) {
        prohod_polaritu = prohod;
        pwm = new uint8_t[64];
    }

    // Destruktor
    ~LepsiMotor() {
        delete[] pwm;
    }

    void setRychlost(int val) {
        rychlost = val;
    }

    int &gerRychlost() {
        return rychlost;
    }

private:
    int rychlost;
    bool dopredu;

    bool prohod_polaritu;
    uint8_t *pwm;
    double *pole;
};

LepsiMotor *motorova() {
    int x;

    // na stacku
    LepsiMotor m2(false);
    //m2.rychlost = 50;
    m2.setRychlost(50);

    // na heapu (haldě)
    auto *m3 = new LepsiMotor(true);
    return m3;
}

void setup() {

    auto *m3 = motorova();
    m3->setRychlost(50);

    int& r = m3->gerRychlost();

    delete m3;

    motor m;
    m.rychlost = 50;
    m.dopredu = false;


    int x = 4;

    int *y = &x;
    *y = 6;
    // x = 6
    y = NULL;

    int& ref = x;
    ref = 7;
    // x == 7

    auto& man = Manager::get();

    // BIT OR 0b00010001
    // 0b00000001 | 0b00010000 =  0b00010001

    // BIT AND
    // 0b00010001 & 0b00010000 = 0b00010000
    // 0b00010001 & 0b00000100 = 0b00000000
    // if((disabled & MAN_DISABLE_MOTOR_FAILSAFE) != 0)
    man.install(
        ManagerInstallFlags::MAN_DISABLE_BATTERY_MANAGEMENT |
        ManagerInstallFlags::MAN_DISABLE_MOTOR_FAILSAFE
    );

    man.schedule(500, kazdouPulVterinu);

    int counter = 0;
    bool on = false;
    auto f = [&]()-> bool {
        printf("pulvterina %d!\n", counter);
        counter++;

        man.leds().blue(counter%2 == 1);

        auto& l = man.leds();
        l.red(on);
        on = !on;
        return true;
    };

    man.schedule(500, f);
}

int counter = 0;

bool kazdouPulVterinu() {
    printf("pulvterina!\n");
    return true; // spoustet porad dokola, false == jednou
}
