#include <Arduino.h>

#include <map>

#include "range.h"

void setup() {
    delay(1000);

    std::vector<int> vektor = { 2, 3, 4, 6, 8, 9, 9, 0, 0 };
    int poleIntu[] = { 2, 3, 4, 6, 8, 9, 9, 0, 0 };

    std::string string = "Robotka!";

    printf("\nProchazeni strigu pomoci iteratoru:\n");
    {
        for (auto itr = string.begin(); itr != string.end(); ++itr) {
            printf("  %c\n", *itr);
        }
    }

    printf("\nKolekce mivaji i reverzni iteratory:\n");
    for (auto itr = string.rbegin(); itr != string.rend(); ++itr) {
        printf("  %c\n", *itr);
    }

    printf("\n\nPointery jsou vlastne taky iteratory:\n");
    {
        // sizeof vrací velikost v bytech, ale počítání s pointery probíhá po velikosti
        // prvku, tj. poleIntu += 1 ve skutečnosti posune o 4 byty (velikost intu).
        // Musíme tedy vydělit velikost v bytech velikostí integeru, abychom dostali počet prvků.
        const auto pocetPrvku = sizeof(poleIntu) / sizeof(int);

        int* begin = poleIntu;
        int* end = poleIntu + pocetPrvku;
        for (int* itr = begin; itr != end; ++itr) {
            printf("  %d\n", *itr);
        }
    }

    printf("\n\nPouzivani v STD algoritmech:\n"); // https://en.cppreference.com/w/cpp/algorithm
    {
        std::vector<int>::iterator itr = std::find(vektor.begin(), vektor.end(), 6);
        printf("   nalezeno na pozici: %d\n", itr - vektor.begin());
    }

    {
        std::string::iterator itr = std::find(string.begin(), string.end(), 'X');
        bool nalezeno = itr != string.end();
        printf("   Existuje v '%s' X? %s\n", string.c_str(), nalezeno ? "true" : "false");
    }

    {
        std::sort(string.begin(), string.end());
        printf("   Serazeny string: %s\n", string.c_str());
    }

    printf("\n\nMazani z STD kontejneru:\n");
    {
        for (auto itr = vektor.begin(); itr != vektor.end(); /* zde neni pricteni! */) {
            if (*itr == 9) {
                printf("    mazu %d na pozici %d\n", *itr, itr - vektor.begin());
                itr = vektor.erase(itr); // erase vrati iterator ukazujici *za* smazany prvek
            } else {
                ++itr;
            }
        }
    }

    printf("\n\nMapy a iterátory:\n");
    {
        std::map<std::string, int> mapa {
            { "b", 8 },
            { "d", 16 },
            { "a", 4 },
            { "e", 23 },
            { "c", 15 },
        };

        for (auto itr = mapa.begin(); itr != mapa.end(); ++itr) {
            std::pair<std::string, int> par = *itr;

            // std::pair je vlastně tato struktura:
            // struct {
            //     std::string first;
            //     int second;
            // };

            printf("  %s => %d\n", par.first.c_str(), par.second);

            // Jde to i jednodušeji
            // (*itr).first == itr->first
            printf("    %s => %d\n", itr->first.c_str(), itr->second);
        }
    }

    printf("\n\nPriklad vlastniho iteratoru:");
    testRange();
}

void loop() {}
