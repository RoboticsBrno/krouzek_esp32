#include <cstddef>

// Třídy mohou být také šablonové. Funguje to téměř stejně jako metody.
// Zde je příklad klasického pole obaleného v šablonové třídě,
// podobně funguje std::array ze základní knihovny

template<class T, size_t N>
class NasePole {
public:
    NasePole(): m_data{ 0 } {

    }

    size_t size() const { return N; }

    T& at(size_t idx) {
        return m_data[idx];
    }

    T& operator[](size_t idx) {
        return m_data[idx];
    }

    T* data() {
        return &m_data;
    }

private:
    T m_data[N];
};

static void testNasePole() {
    NasePole<int, 56> pole; // Stejné jako int pole[56]
    pole[4] = 42;
    pole.size(); // == 56
}


// Třídy lze také specializovat, a to klidně částečně - zde jen jeden z parametrů:
template<size_t N>
class NasePole <bool, N> {
public:
    NasePole(): m_data({ 0 }) {

    }

    size_t size() const { return N; }

    bool& at(size_t idx) {
        return m_data[idx];
    }

    bool& operator[](size_t idx) {
        return m_data[idx];
    }

    bool* data() {
        return &m_data;
    }

private:
    bool m_data[N];
};
