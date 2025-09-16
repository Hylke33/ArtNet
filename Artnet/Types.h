#pragma once
#ifndef ARTNET_TYPES_H
#define ARTNET_TYPES_H

#include "Common.h"
#include <stdint.h>

namespace art_net {

// Core data structures for universe subscription tracking

struct UniverseDescriptor {
    uint8_t net;        // 7-bit net (0-127)
    uint8_t subnet;     // 4-bit subnet (0-15)
    uint8_t universe;   // 4-bit universe (0-15)
    uint16_t universe15bit;  // 15-bit combined universe identifier
    
    UniverseDescriptor() : net(0), subnet(0), universe(0), universe15bit(0) {}
    
    UniverseDescriptor(uint8_t n, uint8_t s, uint8_t u)
        : net(n), subnet(s), universe(u), universe15bit(((uint16_t)n << 8) | ((uint16_t)s << 4) | (uint16_t)u) {}
    
    UniverseDescriptor(uint16_t universe_15bit)
        : universe15bit(universe_15bit), net((universe_15bit >> 8) & 0x7F),
          subnet((universe_15bit >> 4) & 0x0F), universe(universe_15bit & 0x0F) {}
    
    bool operator<(const UniverseDescriptor& other) const {
        return universe15bit < other.universe15bit;
    }
    
    bool operator==(const UniverseDescriptor& other) const {
        return universe15bit == other.universe15bit;
    }
};

struct PortConfiguration {
    uint8_t port_index;         // Physical port index (0-3 for standard Art-Net)
    UniverseDescriptor universe; // Universe assigned to this port
    bool input_enabled;         // Port configured for input
    bool output_enabled;        // Port configured for output
    uint8_t sw_in;             // Input universe setting for ArtPollReply
    uint8_t sw_out;            // Output universe setting for ArtPollReply
    
    PortConfiguration() : port_index(0), input_enabled(false), output_enabled(false), sw_in(0), sw_out(0) {}
    
    PortConfiguration(uint8_t idx, const UniverseDescriptor& univ, bool in, bool out)
        : port_index(idx), universe(univ), input_enabled(in), output_enabled(out),
          sw_in(univ.universe), sw_out(univ.universe) {}
};

struct PortMappingResult {
    uint8_t num_ports;                    // Number of ports to report
    PortConfiguration ports[4];           // Port configurations (max 4 ports)
    bool has_subscriptions;               // True if any universe subscriptions exist
    
    PortMappingResult() : num_ports(0), has_subscriptions(false) {
        // Initialize all ports
        for (uint8_t i = 0; i < 4; ++i) {
            ports[i] = PortConfiguration();
        }
    }
};

} // namespace art_net

#endif // ARTNET_TYPES_H