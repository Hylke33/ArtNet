#pragma once
#ifndef ARTNET_ARTPOLLREPLY_H
#define ARTNET_ARTPOLLREPLY_H

#include "Common.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace art_net {
namespace art_poll_reply {

static constexpr size_t NUM_POLLREPLY_PUBLIC_PORT_LIMIT {4};

union Packet
{
    struct
    {
        uint8_t id[8];
        uint8_t op_code_l;
        uint8_t op_code_h;
        uint8_t ip[4];
        uint8_t port_l;
        uint8_t port_h;
        uint8_t ver_h;
        uint8_t ver_l;
        uint8_t net_sw;
        uint8_t sub_sw;
        uint8_t oem_h;
        uint8_t oem_l;
        uint8_t ubea_ver;
        uint8_t status_1;
        uint8_t esta_man_l;
        uint8_t esta_man_h;
        uint8_t short_name[18];
        uint8_t long_name[64];
        uint8_t node_report[64];
        uint8_t num_ports_h;
        uint8_t num_ports_l;
        uint8_t port_types[NUM_POLLREPLY_PUBLIC_PORT_LIMIT];
        uint8_t good_input[NUM_POLLREPLY_PUBLIC_PORT_LIMIT];
        uint8_t good_output[NUM_POLLREPLY_PUBLIC_PORT_LIMIT];
        uint8_t sw_in[NUM_POLLREPLY_PUBLIC_PORT_LIMIT];
        uint8_t sw_out[NUM_POLLREPLY_PUBLIC_PORT_LIMIT];
        uint8_t sw_video;
        uint8_t sw_macro;
        uint8_t sw_remote;
        uint8_t spare[3];
        uint8_t style;
        uint8_t mac[6];
        uint8_t bind_ip[4];
        uint8_t bind_index;
        uint8_t status_2;
        uint8_t filler[26];
    };
    uint8_t b[239];
};

struct Config
{
    uint16_t oem {0x00FF};      // OemUnknown https://github.com/tobiasebsen/ArtNode/blob/master/src/Art-NetOemCodes.h
    uint16_t esta_man {0x0000}; // ESTA manufacturer code
    uint8_t status1 {0x00};     // Unknown / Normal
    uint8_t status2 {0x08};     // sACN capable
    String short_name {"Arduino ArtNet"};
    String long_name {"Arduino ArtNet Protocol by hideakitai/ArtNet"};
    String node_report {""};
    // Four universes from Device to Controller
    // NOTE: Only low 4 bits of the universes
    // NOTE: Upper 11 bits of the universes will be
    //       shared with the subscribed universe (net, subnet)
    // e.g.) If you have subscribed universe 0x1234,
    //       you can set the device to controller universes
    //       from 0x1230 to 0x123F (sw_in will be from 0x0 to 0xF).
    //       So, I recommned subscribing only in the range of
    //       0x1230 to 0x123F if you want to set the sw_in.
    // REF: Please refer the Art-Net spec for the detail.
    //      https://art-net.org.uk/downloads/art-net.pdf
    uint8_t sw_in[4] {0};
};

// Forward declaration for PortMappingResult
struct PortMappingResult;

inline Packet generatePacketFrom(const IPAddress &my_ip, const uint8_t my_mac[6], const PortMappingResult& port_mapping, const Config &metadata)
{
    Packet r;
    for (size_t i = 0; i < ID_LENGTH; i++) {
        r.id[i] = static_cast<uint8_t>(ARTNET_ID[i]);
    }
    r.op_code_l = ((uint16_t)OpCode::PollReply >> 0) & 0x00FF;
    r.op_code_h = ((uint16_t)OpCode::PollReply >> 8) & 0x00FF;
    memcpy(r.mac, my_mac, 6);
    for (size_t i = 0; i < 4; ++i) {
        r.ip[i] = my_ip[i];
    }
    r.port_h = (DEFAULT_PORT >> 8) & 0xFF;
    r.port_l = (DEFAULT_PORT >> 0) & 0xFF;
    r.ver_h = (PROTOCOL_VER >> 8) & 0x00FF;
    r.ver_l = (PROTOCOL_VER >> 0) & 0x00FF;
    r.oem_h = (metadata.oem >> 8) & 0xFF; //
    r.oem_l = (metadata.oem >> 0) & 0xFF;
    r.ubea_ver = 0;     // UBEA not programmed
    r.status_1 = metadata.status1;
    r.esta_man_h = (metadata.esta_man >> 8) & 0xFF;
    r.esta_man_l = (metadata.esta_man >> 0) & 0xFF;
    memset(r.short_name, 0, 18);
    memset(r.long_name, 0, 64);
    memset(r.node_report, 0, 64);
    memcpy(r.short_name, metadata.short_name.c_str(), metadata.short_name.length());
    memcpy(r.long_name, metadata.long_name.c_str(), metadata.long_name.length());
    memcpy(r.node_report, metadata.node_report.c_str(), metadata.node_report.length());
    r.num_ports_h = 0; // Reserved
    r.num_ports_l = port_mapping.num_ports;
    
    // Initialize all port arrays to zero
    memset(r.sw_in, 0, 4);
    memset(r.sw_out, 0, 4);
    memset(r.port_types, 0, 4);
    memset(r.good_input, 0, 4);
    memset(r.good_output, 0, 4);
    
    // Configure ports based on PortMappingResult
    // Use primary universe for net/subnet if available, otherwise default to 0
    uint16_t primary_universe = port_mapping.num_ports > 0 ? port_mapping.ports[0].universe.universe15bit : 0;
    r.net_sw = (primary_universe >> 8) & 0x7F;
    r.sub_sw = (primary_universe >> 4) & 0x0F;
    
    // Configure each port up to the reported number of ports
    for (uint8_t i = 0; i < port_mapping.num_ports && i < 4; ++i) {
        const auto& port = port_mapping.ports[i];
        
        // Set sw_in and sw_out based on port configuration
        r.sw_in[i] = port.sw_in & 0x0F;
        r.sw_out[i] = port.sw_out & 0x0F;
        
        // Configure port types based on input/output capabilities
        if (port.input_enabled && port.output_enabled) {
            r.port_types[i] = 0xC0;   // I/O available by DMX512
        } else if (port.input_enabled) {
            r.port_types[i] = 0x80;   // Input available by DMX512
        } else if (port.output_enabled) {
            r.port_types[i] = 0x40;   // Output available by DMX512
        } else {
            r.port_types[i] = 0x00;   // Port disabled
        }
        
        // Set good input/output status
        r.good_input[i] = port.input_enabled ? 0x80 : 0x00;    // Data received without error if enabled
        r.good_output[i] = port.output_enabled ? 0x80 : 0x00;  // Data transmitted without error if enabled
    }
    
    // If no ports configured, still populate sw_in from metadata for backward compatibility
    if (port_mapping.num_ports == 0) {
        for (size_t i = 0; i < 4; ++i) {
            r.sw_in[i] = metadata.sw_in[i] & 0x0F;
        }
    }
    
    r.sw_video = 0;   // Video display shows local data
    r.sw_macro = 0;   // No support for macro key inputs
    r.sw_remote = 0;  // No support for remote trigger inputs
    memset(r.spare, 0x00, 3);
    r.style = 0x00;  // A DMX to / from Art-Net device
    for (size_t i = 0; i < 4; ++i) {
        r.bind_ip[i] = my_ip[i];
    }
    r.bind_index = 0;
    r.status_2 = metadata.status2;
    memset(r.filler, 0x00, 26);

    return r;
}

} // namespace art_poll_reply
} // namespace art_net

using ArtPollReplyConfig = art_net::art_poll_reply::Config;

#endif // ARTNET_ARTPOLLREPLY_H
