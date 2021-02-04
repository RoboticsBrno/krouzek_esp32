#pragma once

#include <iterator>
#include <stdio.h>
#include <vector>

using std::iterator;
using std::iterator_traits;

class Range {
public:
    class const_iterator {
        friend class Range;

        typedef std::iterator_traits<const const_iterator*> __traits_type;

    public:
        typedef const_iterator iterator_type;
        typedef typename __traits_type::iterator_category iterator_category;
        typedef typename __traits_type::value_type value_type;
        typedef typename __traits_type::difference_type difference_type;
        typedef typename __traits_type::reference reference;
        typedef typename __traits_type::pointer pointer;

        // Dereference na hodnotu
        const int& operator*() const { return m_pos; }
        int const* operator->() const { return &m_pos; }

        // Prefix increment
        const_iterator& operator++() {
            m_pos += m_range.m_step;
            return *this;
        }

        // Postfix increment
        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator!=(const const_iterator& b) const { return m_pos < b.m_range.m_stop; };

        size_t operator-(const const_iterator& b) const {
            if (m_pos > b.m_pos && &m_range == &b.m_range) {
                return (this->m_pos - b.m_pos) / m_range.m_step;
            } else {
                return 0;
            }
        }

    private:
        const_iterator(const Range& range, int pos)
            : m_range(range)
            , m_pos(pos) {}

        const Range& m_range;
        int m_pos;
    };

    Range(int start, int stop, int step = 1) {
        m_start = start;
        m_stop = stop;
        m_step = step;
    }

    const_iterator begin() const { return const_iterator(*this, m_start); }
    const_iterator end() const { return const_iterator(*this, m_start + m_stop); }

    size_t size() const { return (m_stop - m_start) / m_step; }

private:
    int m_start;
    int m_stop;
    int m_step;
};

void testRange() {
    Range r(0, 10, 2);

    printf("  Vypis z range:\n");
    for (auto itr = r.begin(); itr != r.end(); ++itr) {
        printf("    %d\n", *itr);
    }

    printf("\n  Kopirovani z range\n");
    std::vector<int> vektor(r.size());

    std::copy(r.begin(), r.end(), vektor.begin());

    for (auto n : vektor) {
        printf("    %d\n", n);
    }
}
