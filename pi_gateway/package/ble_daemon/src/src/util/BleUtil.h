#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace ble::util {

constexpr bool is_hex(char c) {
    return ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F'));
}

constexpr uint8_t to_hex(char c) {
    if ((c >= '0') && (c <= '9'))
        return c - '0';
    if ((c >= 'a') && (c <= 'f'))
        return (c - 'a') + 10;
    if ((c >= 'A') && (c <= 'F'))
        return (c - 'A') + 10;
    return 0;
}

static inline std::string addr_to_str(const uint8_t* addr) {
    char buf[18];
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    return std::string(buf);
}

template <std::size_t N_OUT = 6, std::size_t N_IN = 18>
constexpr std::array<uint8_t, N_OUT> address_from(char const (&address_in)[N_IN]) {
    std::array<uint8_t, N_OUT> address_out{};
    int j = 0;
    for (int i = (N_OUT - 1); i >= 0; i--) {
        uint8_t byte_value = 0;

        if (!is_hex(address_in[j]))
            j++;
        byte_value = (to_hex(address_in[j]) << 4);
        j++;
        byte_value |= to_hex(address_in[j]);
        j++;

        address_out[i] = byte_value;
    }
    return address_out;
}

namespace uuid {

template <std::size_t STRLEN>
constexpr std::array<uint8_t, 16> from_string(char const (&value)[STRLEN]) {
    std::array<uint8_t, 16> p{};
    int j = 0;
    for (int i = 15; i >= 0; i--) {
        if (value[j] == '-') j++;
        if (!is_hex(value[j])) j++;
        
        uint8_t byte_value = (to_hex(value[j]) << 4);
        j++;
        byte_value |= to_hex(value[j]);
        j++;

        p[i] = byte_value;
    }
    return p;
}

} // namespace uuid

} // namespace ble::util
