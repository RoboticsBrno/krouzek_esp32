#include <vector>
#include <list>
#include <cstdlib>
#include <algorithm>

// Začneme s kódem bez templatu (šablon)
// Umělý příklad dvou metod, které sečtou dvě čísla - jednou jsou to inty, podruhé floaty.
// Museli jsme napsat dvě metody.
int sectiInt(int a, int b) {
    return a + b;
}

float sectiFloat(float a, float b) {
    return a + b;
}


// Takto vypada šablonová funkce, která dělá to stejné
template<typename TypPromennych> // muzete videt i template<class TypPromennych> - funguje stejne
TypPromennych secti(TypPromennych a, TypPromennych b) {
    return a + b;
}
// Všimněte si kódu: template<typename TypPromennych>
// TypPromennych je zastupné jméno pro typ, se kterým je funkce zavolána.
// Obvykle se používá jen T, zde rozepsáno na TypPromennych pro názornost.
// Když je šablonová funkce takto zapsaná, nic neudělá - ani se nezkompiluje,
// musí ji teprv nějaký kód volat, aby se to stalo.
// Když je odněkud volaná, kompilátor podle šablony vygeneruje opravdovou funkci
// a tu zkompiluje. Tedy když zavoláme následující:
static void testSecti() {
    secti(4, 5);
    secti(4.f, 5.f);
}
// Kompilátor vygeneruje 2x funkci secti, jednou s int a podruhé s float. Vygenerované
// funkce budou vypadat přesně tak, jako ty dvě ručně napsané na začátku souboru.



// Specializace šablon: můžete pro vybrané datové typy napsat ručně tzv. specializaci,
// která se pro daný typ použije místo toho, aby kompilátor generoval podle šablony.
template<>
double secti(double a, double b) {
    return a + b + 10;
}

static void testSpecializace() {
    secti(4, 4);     // 8
    secti(4.f, 4.f); // 8.f
    secti(4.0, 4.0); // 18.0
}
// Zde kompilátor dvakrát použil šablonu a po tom pro typ double použil naši zlomyslnou
// specializaci, která k výsledku přičetla 10


// V případě, kdy je šablonový typ pouze vracený z funkce, kompilátor nemůže
// sám vydedukovat, co to má být za typ. Je třeba bu pomoci zapsáním typu do <>
// mezi jménem šablonové funkce a první závorku, viz testReturnTyp:
template<class T>
T nahodneCislo() {
    return rand();
}

static void testReturnTyp() {
    auto ix = nahodneCislo<int>();   // ix je int
    auto fx = nahodneCislo<float>(); // fx je float
}


// šablony mohou mít více typových parametrů:
template<class T, class U>
T min(T a, U b) {
    return a < b ? a : b;
}


// šablony mohou mít i netypové parametry:
template<class T, int N>
T nahodnyMedian() {
    T buffer[N];
    for(int i = 0; i < N; ++i) {
        buffer[i] = rand();
    }

    std::sort(buffer, buffer+N);

    return buffer[N/2];
}

static void testMedian() {
    nahodnyMedian<int, 256>(); // Vrati median z 256 nahodnych cisel
    nahodnyMedian<float, 1024>(); // Vrati median 1024 nahodnych floatu

    nahodnyMedian<int, -1>(); // nezkompiluje se
}

