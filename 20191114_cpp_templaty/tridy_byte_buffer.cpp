#include <stdint.h>
#include <cstddef>
#include <algorithm>
#include <string.h>
#include <assert.h>
#include <string>

#include <driver/uart.h>

// Další případ, na který můžete narazit, je převod datových typů na jejich
// reprezentaci v bytech.  Byte buffer se může hodit například na posílání přes
// seriovou linku, kdy musíte posílat jen byty.

class ByteBuffer {
public:
    ByteBuffer() {
        m_size = 0;
        m_readPosition = 0;
        m_capacity = 16;
        m_data = new uint8_t[m_capacity];
    }

    ~ByteBuffer() {
        delete[] m_data;
    }

    template<typename T>
    void write(T value);

    template<typename T>
    T read();

    void setReadPos(size_t newPos) {
        assert(newPos <= m_size);
        m_readPosition = newPos;
    }

    size_t size() const { return m_size; }

    void clear() {
        m_size = 0;
        m_readPosition = 0;
    }

    void reserve(size_t sizeAtLeast) {
        if(sizeAtLeast > m_capacity)
            growIfNeeded(sizeAtLeast - m_capacity);
    }

    void resize(size_t newSize) {
        if(newSize > m_size) {
            growIfNeeded(newSize - m_size);
        }
        m_size = newSize;
        m_readPosition = 0;
    }

    uint8_t *data() { return m_data; }

private:
    // Strategie: Zkus zvětšit kapacitu 2x, pokud nestačí, použij požadovanou velikost jako kapacitu.
    void growIfNeeded(size_t neededExtraSpace = 1) {
        const size_t neededSpace = m_size + neededExtraSpace;
        if(neededSpace <= m_capacity)
            return;

        m_capacity *= 2;
        if(neededSpace > m_capacity)
            m_capacity = neededSpace;

        auto *newData = new uint8_t[m_capacity];
        std::copy(m_data, m_data+m_size, newData);
        delete[] m_data;
        m_data = newData;
    }

    uint8_t *m_data;
    size_t m_size;
    size_t m_capacity;
    size_t m_readPosition;
};

// implementace šablonových METOD musí být vždy dostupné pro kompilátor. Pokud tedy
// třídu rozdělíte na .h a .cpp soubor, implementace, kterou byste normálně dali do .cpp,
// musíte u šablon dát do .h. Můžete to udělat takto, prostě pod class{ ... } definici
// hodíte do .h i implementaci:
template<typename T>
void ByteBuffer::write(T value) {
    growIfNeeded(sizeof(T));
    memcpy((void*)(m_data + m_size), (void*)&value, sizeof(T));
    m_size += sizeof(T);
}

// Specializace write() pro stringy
template<>
void ByteBuffer::write(const std::string& value) {
    growIfNeeded(value.size() + 1);
    memcpy((void*)(m_data + m_size), (void*)value.c_str(), value.size() + 1);
    m_size += value.size() + 1;
}

template<typename T>
T ByteBuffer::read() {
    assert(m_readPosition + sizeof(T) <= m_size);

    T result;
    memcpy((void*)&result, (void*)(m_data + m_readPosition), sizeof(T));
    m_readPosition += sizeof(T);
    return result;
}

// Specializace read() pro stringy
template<>
std::string ByteBuffer::read() {
    assert(m_readPosition < m_size);

    uint8_t *end = std::find(m_data + m_readPosition, m_data + m_size, 0);
    std::string result(m_data + m_readPosition, end);

    m_readPosition += result.size();
    if(end != m_data + m_size)
        ++m_readPosition; // for the 0 terminator character
    return result;
}


static void testByteBuffer() {
    // Na jedne strane zapiseme a posleme...
    ByteBuffer tx_buff;
    tx_buff.write(42);
    tx_buff.write(int16_t(24));
    tx_buff.write("testovaci string");

    char tx_packet_size = tx_buff.size();
    uart_write_bytes(UART_NUM_1, &tx_packet_size, 1); // velikost následujících dat
    uart_write_bytes(UART_NUM_1, (char*)tx_buff.data(), tx_buff.size());

    // na druhe prijmeme
    uint8_t rx_packet_size;
    uart_read_bytes(UART_NUM_1, &rx_packet_size, 1, portMAX_DELAY); // kolik nám toho přijde?

    ByteBuffer rx_buff;
    rx_buff.resize(rx_packet_size);

    uart_read_bytes(UART_NUM_1, rx_buff.data(), rx_packet_size, portMAX_DELAY);

    rx_buff.read<int>(); // 42
    rx_buff.read<int16_t>(); // 24
    rx_buff.read<std::string>(); // "testovaci string"
}
