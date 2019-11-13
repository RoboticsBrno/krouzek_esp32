#include <cstddef>
#include <algorithm>
#include <assert.h>

// Kontejnery v C++ jsou klasický příklad využití šablon.
// Zde je ukázková implementace std::vector, dynamického pole.

template<class T>
class Viktor {
public:
    Viktor() {
        m_size = 0;
        m_capacity = 16;
        m_data = new T[m_capacity];
    }

    ~Viktor() {
        delete[] m_data;
    }

    void push_back(T value) {
        growIfNeeded();
        m_data[m_size++] = value;
    }

    T pop_back() {
        assert(m_size > 0); // spadne, pokud je m_size == 0
        return m_data[m_size--];
    }

    void clear() {
        m_size = 0;
    }

    void resize(size_t newSize, T value = T()) {
        if(newSize > m_size) {
            growIfNeeded(newSize - m_size);
            std::fill(m_data + m_size, m_data + newSize, value);
        }
        m_size = newSize;
    }

    T& operator[](size_t idx) {
        return m_data[idx];
    }

private:
    void growIfNeeded(size_t neededSpace = 1) {
        if(m_size + neededSpace > m_capacity) {
            m_capacity *= 2;
            T *newData = new T[m_capacity];
            std::copy(m_data, m_data+m_size, newData);
            delete[] m_data;
            m_data = newData;
        }
    }

    T *m_data;
    size_t m_size;
    size_t m_capacity;
};

static void testViktor() {
    Viktor<float> vf;

    vf.push_back(4.f);
    vf.push_back(10.f);

    vf.resize(10);

    vf.pop_back();

    vf.clear();
}
