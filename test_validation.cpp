#include <iostream>
#include <iomanip>
#include <cstring>

// Mock Arduino types for testing
typedef const char* String;
class IPAddress {
public:
    uint8_t _address[4];
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        _address[0] = a; _address[1] = b; _address[2] = c; _address[3] = d;
    }
    uint8_t operator[](int index) const { return _address[index]; }
};

// Include the ArtNet headers (with path adjustments for testing)
#define ARTNET_ENABLE_WIFI
#include "Artnet/Common.h"
#include "Artnet/ArtPollReply.h"
#include "Artnet/Receiver.h"

using namespace art_net;

class ValidationTester {
public:
    void runAllTests() {
        std::cout << "=== ArtNet Library Validation Tests ===" << std::endl;
        
        testSingleSubnet();
        testCrossSubnet();
        testEdgeCases();
        testRegressionScenarios();
        
        std::cout << "\n=== Validation Complete ===" << std::endl;
        printSummary();
    }

private:
    int tests_passed = 0;
    int tests_failed = 0;
    
    void testSingleSubnet() {
        std::cout << "\n--- Test Case 1: Single Subnet (0-15) ---" << std::endl;
        
        // Create UniverseRegistry and register universes 0-15
        UniverseRegistry<int> registry; // Using int as dummy template parameter
        
        // Register universes 0-15 (Net:0, Subnet:0, Universe:0-15)
        for (uint8_t u = 0; u <= 15; ++u) {
            registry.registerUniverse(0, 0, u); // Net:0, Subnet:0, Universe:0-15
        }
        
        // Generate port mapping
        PortMappingResult mapping = registry.generatePortMapping();
        
        // Validate results
        std::cout << "Registered 16 universes (0-15), got " << (int)mapping.num_ports << " ports" << std::endl;
        
        // Should use 4 ports to cover 16 universes (4 universes per port)
        if (mapping.num_ports == 4) {
            std::cout << "✓ PASS: Correct number of ports (4)" << std::endl;
            tests_passed++;
        } else {
            std::cout << "✗ FAIL: Expected 4 ports, got " << (int)mapping.num_ports << std::endl;
            tests_failed++;
        }
        
        // Check that all ports are properly configured
        bool all_configured = true;
        for (uint8_t i = 0; i < mapping.num_ports; ++i) {
            const auto& port = mapping.ports[i];
            if (port.universe.net != 0 || port.universe.subnet != 0) {
                all_configured = false;
                break;
            }
        }
        
        if (all_configured) {
            std::cout << "✓ PASS: All ports have correct Net:0/Subnet:0" << std::endl;
            tests_passed++;
        } else {
            std::cout << "✗ FAIL: Some ports have incorrect Net/Subnet" << std::endl;
            tests_failed++;
        }
        
        // Test ArtPollReply packet generation
        testArtPollReplyGeneration(mapping, "Single Subnet");
    }
    
    void testCrossSubnet() {
        std::cout << "\n--- Test Case 2: Cross-Subnet (0-19) ---" << std::endl;
        
        UniverseRegistry<int> registry;
        
        // Register universes 0-19 which spans:
        // Net:0/Subnet:0/Universe:0-15 (universes 0-15)  
        // Net:0/Subnet:1/Universe:0-3  (universes 16-19)
        for (uint16_t u = 0; u <= 19; ++u) {
            registry.registerUniverse(u);
        }
        
        PortMappingResult mapping = registry.generatePortMapping();
        
        std::cout << "Registered 20 universes (0-19), got " << (int)mapping.num_ports << " ports" << std::endl;
        
        // Should use 4 ports maximum (Art-Net limit)
        if (mapping.num_ports == 4) {
            std::cout << "✓ PASS: Uses maximum 4 ports for cross-subnet scenario" << std::endl;
            tests_passed++;
        } else {
            std::cout << "✗ FAIL: Expected 4 ports, got " << (int)mapping.num_ports << std::endl;
            tests_failed++;
        }
        
        // Verify cross-subnet coverage
        bool has_subnet0 = false, has_subnet1 = false;
        for (uint8_t i = 0; i < mapping.num_ports; ++i) {
            if (mapping.ports[i].universe.subnet == 0) has_subnet0 = true;
            if (mapping.ports[i].universe.subnet == 1) has_subnet1 = true;
        }
        
        if (has_subnet0 && has_subnet1) {
            std::cout << "✓ PASS: Cross-subnet mapping detected" << std::endl;
            tests_passed++;
        } else {
            std::cout << "✗ FAIL: Cross-subnet mapping not properly handled" << std::endl;
            tests_failed++;
        }
        
        testArtPollReplyGeneration(mapping, "Cross-Subnet");
    }
    
    void testEdgeCases() {
        std::cout << "\n--- Test Case 3: Edge Cases ---" << std::endl;
        
        // Test 3a: No subscriptions
        {
            UniverseRegistry<int> registry;
            PortMappingResult mapping = registry.generatePortMapping();
            
            if (mapping.num_ports == 1 && mapping.ports[0].universe.universe15bit == 0) {
                std::cout << "✓ PASS: No subscriptions defaults to universe 0" << std::endl;
                tests_passed++;
            } else {
                std::cout << "✗ FAIL: No subscriptions handling incorrect" << std::endl;
                tests_failed++;
            }
        }
        
        // Test 3b: Universe 0 only
        {
            UniverseRegistry<int> registry;
            registry.registerUniverse(0);
            PortMappingResult mapping = registry.generatePortMapping();
            
            if (mapping.num_ports == 1 && mapping.ports[0].universe.universe15bit == 0) {
                std::cout << "✓ PASS: Universe 0 only works correctly" << std::endl;
                tests_passed++;
            } else {
                std::cout << "✗ FAIL: Universe 0 only handling incorrect" << std::endl;
                tests_failed++;
            }
        }
        
        // Test 3c: Sparse universes
        {
            UniverseRegistry<int> registry;
            registry.registerUniverse(5);
            registry.registerUniverse(67);
            registry.registerUniverse(128);
            PortMappingResult mapping = registry.generatePortMapping();
            
            if (mapping.num_ports == 3) {
                std::cout << "✓ PASS: Sparse universes (5, 67, 128) mapped correctly" << std::endl;
                tests_passed++;
            } else {
                std::cout << "✗ FAIL: Sparse universes not handled correctly" << std::endl;
                tests_failed++;
            }
        }
    }
    
    void testRegressionScenarios() {
        std::cout << "\n--- Test Case 4: Regression Testing ---" << std::endl;
        
        // Test that the UniverseRegistry properly tracks subscriptions
        UniverseRegistry<int> registry;
        
        // Add some universes
        registry.registerUniverse(1);
        registry.registerUniverse(2);
        registry.registerUniverse(3);
        
        if (registry.getActiveUniverseCount() == 3) {
            std::cout << "✓ PASS: Registry tracks universe count correctly" << std::endl;
            tests_passed++;
        } else {
            std::cout << "✗ FAIL: Registry universe count tracking broken" << std::endl;
            tests_failed++;
        }
        
        // Test unregister functionality
        registry.unregisterUniverse(2);
        if (registry.getActiveUniverseCount() == 2) {
            std::cout << "✓ PASS: Universe unregistration works" << std::endl;
            tests_passed++;
        } else {
            std::cout << "✗ FAIL: Universe unregistration broken" << std::endl;
            tests_failed++;
        }
        
        // Test clear functionality
        registry.clear();
        if (registry.getActiveUniverseCount() == 0) {
            std::cout << "✓ PASS: Registry clear works" << std::endl;
            tests_passed++;
        } else {
            std::cout << "✗ FAIL: Registry clear broken" << std::endl;
            tests_failed++;
        }
    }
    
    void testArtPollReplyGeneration(const PortMappingResult& mapping, const std::string& scenario) {
        // Test ArtPollReply packet generation
        IPAddress test_ip(192, 168, 1, 100);
        uint8_t test_mac[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
        
        art_poll_reply::Config config;
        config.short_name = "Test Node";
        config.long_name = "Test ArtNet Node for Validation";
        
        art_poll_reply::Packet packet = art_poll_reply::generatePacketFrom(test_ip, test_mac, mapping, config);
        
        std::cout << scenario << " ArtPollReply Analysis:" << std::endl;
        std::cout << "  Num Ports: " << (int)packet.num_ports_l << std::endl;
        std::cout << "  Net: " << (int)packet.net_sw << ", Subnet: " << (int)packet.sub_sw << std::endl;
        
        // Validate packet structure
        bool packet_valid = true;
        
        // Check OpCode
        uint16_t opcode = (packet.op_code_h << 8) | packet.op_code_l;
        if (opcode != (uint16_t)OpCode::PollReply) {
            packet_valid = false;
            std::cout << "  ✗ FAIL: Incorrect OpCode" << std::endl;
        }
        
        // Check number of ports matches mapping
        if (packet.num_ports_l != mapping.num_ports) {
            packet_valid = false;
            std::cout << "  ✗ FAIL: Port count mismatch" << std::endl;
        }
        
        // Check port configurations
        for (uint8_t i = 0; i < mapping.num_ports && i < 4; ++i) {
            if (packet.sw_in[i] != (mapping.ports[i].universe.universe & 0x0F)) {
                packet_valid = false;
                std::cout << "  ✗ FAIL: sw_in[" << (int)i << "] incorrect" << std::endl;
            }
        }
        
        if (packet_valid) {
            std::cout << "  ✓ PASS: ArtPollReply packet structure valid" << std::endl;
            tests_passed++;
        } else {
            std::cout << "  ✗ FAIL: ArtPollReply packet validation failed" << std::endl;
            tests_failed++;
        }
    }
    
    void printSummary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Tests Passed: " << tests_passed << std::endl;
        std::cout << "Tests Failed: " << tests_failed << std::endl;
        std::cout << "Total Tests: " << (tests_passed + tests_failed) << std::endl;
        
        if (tests_failed == 0) {
            std::cout << "🎉 ALL TESTS PASSED - Implementation appears correct!" << std::endl;
        } else {
            std::cout << "⚠️  SOME TESTS FAILED - Issues found that need attention" << std::endl;
        }
    }
};

int main() {
    ValidationTester tester;
    tester.runAllTests();
    return 0;
}