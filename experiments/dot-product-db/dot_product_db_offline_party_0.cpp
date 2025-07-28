//by sakara
//offline simulation for Party 0
#include "dot_product_db_config.h"
#include "networking/Party.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <thread>
#include <random>
#include <iomanip>
#include <unordered_set>
#include <algorithm>

using namespace std;
using namespace bioauth;
using namespace bioauth::experiments::dot_product;

constexpr int PARTY_ID = 0;
constexpr uint64_t FIELD_SIZE = (1ULL << 61) - 1; 

struct NetworkConfig {
    std::string name;
    int delay_ms;       
    int bandwidth_mbps; 
    std::string description;
    
    NetworkConfig(const std::string& n, int delay, int bw, const std::string& desc)
        : name(n), delay_ms(delay), bandwidth_mbps(bw), description(desc) {}
};

struct Triple {
    uint64_t a_share, b_share, c_share;
    uint64_t mac_a, mac_b, mac_c;
    
    Triple(uint64_t a, uint64_t b, uint64_t c, uint64_t ma, uint64_t mb, uint64_t mc)
        : a_share(a), b_share(b), c_share(c), mac_a(ma), mac_b(mb), mac_c(mc) {}
};

struct SquarePair {
    uint64_t r_share, r_squared_share;
    uint64_t mac_r, mac_r_squared;
    
    SquarePair(uint64_t r, uint64_t r2, uint64_t mr, uint64_t mr2)
        : r_share(r), r_squared_share(r2), mac_r(mr), mac_r_squared(mr2) {}
};

struct BitTriple {
    uint64_t a_bit, b_bit, c_bit;  // bits in {0,1}
    uint64_t mac_a, mac_b, mac_c;
    
    BitTriple(uint64_t a, uint64_t b, uint64_t c, uint64_t ma, uint64_t mb, uint64_t mc)
        : a_bit(a), b_bit(b), c_bit(c), mac_a(ma), mac_b(mb), mac_c(mc) {}
};

struct InputMask {
    uint64_t mask_share;
    uint64_t mac_share;
    
    InputMask(uint64_t mask, uint64_t mac) : mask_share(mask), mac_share(mac) {}
};

uint64_t field_add(uint64_t a, uint64_t b) {
    return (a + b) % FIELD_SIZE;
}

uint64_t field_mul(uint64_t a, uint64_t b) {
    return ((__uint128_t)a * b) % FIELD_SIZE;
}

uint64_t field_sub(uint64_t a, uint64_t b) {
    return (a >= b) ? (a - b) : (FIELD_SIZE - (b - a));
}

void setupNetwork(const NetworkConfig& config) {
    if (config.delay_ms == 0) {
        system("sudo tc qdisc del dev lo root 2>/dev/null");
        return;
    }
    
    std::string cmd = 
        "sudo tc qdisc del dev lo root 2>/dev/null; "
        "sudo tc qdisc add dev lo root handle 1: netem delay " + 
        std::to_string(config.delay_ms) + "ms; "
        "sudo tc qdisc add dev lo parent 1:1 handle 10: tbf rate " + 
        std::to_string(config.bandwidth_mbps) + "mbit burst 32kbit latency 50ms";
    
    system(cmd.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

void cleanupNetwork() {
    system("sudo tc qdisc del dev lo root 2>/dev/null");
}

// realistic ommuniciation
class RealisticSPDZEstimator {
private:
    static constexpr size_t BGV_CIPHERTEXT_SIZE = 16 * 1024;     // per ciphertext
    static constexpr size_t PACKING_SIZE = 1024;                
    static constexpr size_t ZK_PROOF_SIZE = 32 * 1024;          // ZK proof
    static constexpr size_t MAC_AUTH_SIZE = 64;                 // MAC authentication
    static constexpr size_t SACRIFICE_RATIO = 10;               // sacrifice for verification
    static constexpr size_t PROTOCOL_OVERHEAD = 1024;           // overhead per round
    
public:
    static size_t estimateTripleGeneration(size_t num_triples) {
        size_t total = 0;
        
        // BGV
        size_t ciphertexts_needed = (num_triples + PACKING_SIZE - 1) / PACKING_SIZE;
        total += ciphertexts_needed * BGV_CIPHERTEXT_SIZE * 2;
        
        // ZK
        total += ciphertexts_needed * ZK_PROOF_SIZE * 2;
        
        // 5 rounds
        total += 5 * PROTOCOL_OVERHEAD;
        
        return total;
    }
    
    static size_t estimateSacrificePhase(size_t num_triples) {
        size_t sacrifice_count = num_triples / SACRIFICE_RATIO;
        size_t total = 0;
        
        // open 
        total += sacrifice_count * BGV_CIPHERTEXT_SIZE * 6;
        
        // MAC check
        total += sacrifice_count * MAC_AUTH_SIZE * 4;
        
        return total;
    }
    
    static size_t estimateSquarePairs(size_t count) {
        size_t total = 0;
        
        //square pair
        size_t ciphertexts_needed = (count + PACKING_SIZE - 1) / PACKING_SIZE;
        total += ciphertexts_needed * BGV_CIPHERTEXT_SIZE * 2;
        
        // MAC authentication
        total += count * MAC_AUTH_SIZE * 2;
        
        return total;
    }
    
    static size_t estimateBitTriples(size_t count) {

        return count * 512; // based on MASCOT
    }
    
    static size_t estimateInputMasks(size_t count) {
        size_t total = 0;
        
        // input mask
        size_t ciphertexts_needed = (count + PACKING_SIZE - 1) / PACKING_SIZE;
        total += ciphertexts_needed * BGV_CIPHERTEXT_SIZE * 2;
        
        // MAC authentication
        total += count * MAC_AUTH_SIZE * 2;
        
        return total;
    }
    
    static size_t estimateGlobalVerification(size_t total_values) {
        size_t total = 0;
        
        // global MAC check
        total += 8 * BGV_CIPHERTEXT_SIZE * 2; // 8 rounds
        
        // random challenges and responses
        total += 8 * 1024 * 2;
        
        return total;
    }
    
    static size_t estimateZKProofOverhead() {
        // ZK overhead
        return 20 * ZK_PROOF_SIZE * 2; // 20 rounds
    }
    
    static size_t calculateTotalRealistic(size_t dim, size_t dbsize) {
        size_t total = 0;
        
        size_t num_triples = dim * dbsize;
        size_t num_squares = dbsize;
        size_t num_bits = dim * 64;
        size_t num_input_masks = dim + dbsize * dim;
        
        total += estimateTripleGeneration(num_triples);
        total += estimateSacrificePhase(num_triples);
        total += estimateSquarePairs(num_squares);
        total += estimateBitTriples(num_bits);
        total += estimateInputMasks(num_input_masks);
        total += estimateGlobalVerification(num_triples + num_input_masks);
        total += estimateZKProofOverhead();
        
        return total;
    }
};

class SPDZOfflineSimulator {
private:
    Party& party;
    std::mt19937_64 gen;
    uint64_t mac_key;

    size_t total_communication = 0;
    int verification_rounds = 0;
    
public:
    SPDZOfflineSimulator(Party& p) : party(p), gen(std::random_device{}()) {}
    
    // establish and distribute mac keys
    void establishMACKey() {
        
        std::uniform_int_distribution<uint64_t> dis(1, FIELD_SIZE-1);
        uint64_t local_key_share = dis(gen);
        
        // exchange
        party.Send(1, local_key_share);
        uint64_t remote_key_share = party.Receive<uint64_t>(1);
        
        // compute global mac key
        mac_key = field_add(local_key_share, remote_key_share);
        
        // key authentication protocol (simplified)
        uint64_t test_value = dis(gen);
        uint64_t test_mac = field_mul(test_value, mac_key);
        party.Send(1, test_value);
        party.Send(1, test_mac);
        
        uint64_t recv_test_value = party.Receive<uint64_t>(1);
        uint64_t recv_test_mac = party.Receive<uint64_t>(1);
        
        total_communication += 4 * sizeof(uint64_t);
        //std::cout << " complete" << std::endl;
    }
    
    // generate multiplication triples
    std::vector<Triple> generateMultiplicationTriples(size_t count) {
        
        std::vector<Triple> triples;
        std::uniform_int_distribution<uint64_t> dis(0, FIELD_SIZE-1);
        
        // simulate SHE-based triplet generation
        for (size_t i = 0; i < count; i++) {
            uint64_t a = dis(gen);
            uint64_t b = dis(gen);
            uint64_t c = field_mul(a, b); 
            
            // generate MACs
            uint64_t mac_a = field_mul(a, mac_key);
            uint64_t mac_b = field_mul(b, mac_key);
            uint64_t mac_c = field_mul(c, mac_key);
            
            triples.emplace_back(a, b, c, mac_a, mac_b, mac_c);
            
            // send
            std::vector<uint64_t> triple_data = {a, b, c, mac_a, mac_b, mac_c};
            party.SendVec(1, triple_data);
            
            // receive
            auto received_triple = party.ReceiveVec<uint64_t>(1, 6);
            
            total_communication += 12 * sizeof(uint64_t);
        }
        
        //std::cout << " complete" << std::endl;
        return triples;
    }
    
    // Sacrifice phase communication (simplified, no actual verification)
    void sacrificePhase(size_t total_triples) {
        
        std::uniform_int_distribution<uint64_t> dis(0, FIELD_SIZE-1);
        size_t num_to_sacrifice = std::max(1UL, total_triples / 10);
        
        for (size_t i = 0; i < num_to_sacrifice; i++) {
            // send
            party.Send(1, dis(gen)); // a_share
            party.Send(1, dis(gen)); // b_share  
            party.Send(1, dis(gen)); // c_share
            
            // receive
            uint64_t remote_a = party.Receive<uint64_t>(1);
            uint64_t remote_b = party.Receive<uint64_t>(1);
            uint64_t remote_c = party.Receive<uint64_t>(1);
            
            total_communication += 6 * sizeof(uint64_t);
        }
        
        verification_rounds++;

    }
    
    // generate square pairs
    std::vector<SquarePair> generateSquarePairs(size_t count) {
        
        std::vector<SquarePair> pairs;
        std::uniform_int_distribution<uint64_t> dis(0, FIELD_SIZE-1);
        
        for (size_t i = 0; i < count; i++) {
            uint64_t r = dis(gen);
            uint64_t r_squared = field_mul(r, r);
            
            uint64_t mac_r = field_mul(r, mac_key);
            uint64_t mac_r_squared = field_mul(r_squared, mac_key);
            
            pairs.emplace_back(r, r_squared, mac_r, mac_r_squared);

            std::vector<uint64_t> pair_data = {r, r_squared, mac_r, mac_r_squared};
            party.SendVec(1, pair_data);
            auto received_pair = party.ReceiveVec<uint64_t>(1, 4);
            
            total_communication += 8 * sizeof(uint64_t);
        }
        
        return pairs;
    }
    
    // generate bit triples
    std::vector<BitTriple> generateBitTriples(size_t count) {
        
        std::vector<BitTriple> bit_triples;
        std::uniform_int_distribution<uint64_t> bit_dis(0, 1);
        
        for (size_t i = 0; i < count; i++) {
            uint64_t a_bit = bit_dis(gen);
            uint64_t b_bit = bit_dis(gen);
            uint64_t c_bit = a_bit & b_bit; // AND operation
            
            uint64_t mac_a = field_mul(a_bit, mac_key);
            uint64_t mac_b = field_mul(b_bit, mac_key);
            uint64_t mac_c = field_mul(c_bit, mac_key);
            
            bit_triples.emplace_back(a_bit, b_bit, c_bit, mac_a, mac_b, mac_c);
            
            std::vector<uint64_t> bit_data = {a_bit, b_bit, c_bit, mac_a, mac_b, mac_c};
            party.SendVec(1, bit_data);
            auto received_bits = party.ReceiveVec<uint64_t>(1, 6);
            
            total_communication += 12 * sizeof(uint64_t);
        }
        
        return bit_triples;
    }
    
    // generate input mask
    std::vector<InputMask> generateInputMasks(size_t count) {
        
        std::vector<InputMask> masks;
        std::uniform_int_distribution<uint64_t> dis(0, FIELD_SIZE-1);
        
        for (size_t i = 0; i < count; i++) {
            uint64_t mask = dis(gen);
            uint64_t mac = field_mul(mask, mac_key);
            
            masks.emplace_back(mask, mac);
            
            party.Send(1, mask);
            party.Send(1, mac);
            
            uint64_t recv_mask = party.Receive<uint64_t>(1);
            uint64_t recv_mac = party.Receive<uint64_t>(1);
            
            total_communication += 4 * sizeof(uint64_t);
        }
        
        return masks;
    }
    
    // batch MAC verification
    void batchMACVerification() {

        std::uniform_int_distribution<uint64_t> dis(0, FIELD_SIZE-1);
        
        // generate random challenges
        std::vector<uint64_t> challenges(10);
        for (auto& c : challenges) {
            c = dis(gen);
            party.Send(1, c);
        }
        
        // receive challenges
        auto remote_challenges = party.ReceiveVec<uint64_t>(1, 10);
        
        // compute linear combinations
        uint64_t combined_value = 0, combined_mac = 0;
        for (const auto& challenge : challenges) {
            uint64_t test_val = dis(gen);
            combined_value = field_add(combined_value, field_mul(challenge, test_val));
            combined_mac = field_add(combined_mac, field_mul(challenge, field_mul(test_val, mac_key)));
        }
        
        party.Send(1, combined_value);
        party.Send(1, combined_mac);
        
        uint64_t recv_combined_value = party.Receive<uint64_t>(1);
        uint64_t recv_combined_mac = party.Receive<uint64_t>(1);
        
        total_communication += 24 * sizeof(uint64_t);
        verification_rounds++;
    }
    
    // similuate ZK proof
    void zkProofSimulation() {
        
        std::uniform_int_distribution<uint64_t> dis(0, FIELD_SIZE-1);
        
        // Fiat-Shamir
        for (int round = 0; round < 3; round++) {
            // vommitment
            uint64_t commitment = dis(gen);
            party.Send(1, commitment);
            
            // challenge
            uint64_t challenge = dis(gen);
            party.Send(1, challenge);
            uint64_t recv_challenge = party.Receive<uint64_t>(1);
            
            // response
            uint64_t response = field_add(commitment, field_mul(challenge, dis(gen)));
            party.Send(1, response);
            uint64_t recv_response = party.Receive<uint64_t>(1);
            
            total_communication += 5 * sizeof(uint64_t);
        }
        
        verification_rounds++;
    }
    
    // offline
    void runCompleteOfflinePhase() {
        
        // Calculate the amount of preprocessing data required
        size_t num_triples = dim * dbsize;  // inner product
        size_t num_squares = dbsize;        // comparision
        size_t num_bits = dim * 64;         // bit
        size_t num_input_masks = dim + dbsize * dim; // mask
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        establishMACKey();
        
        auto triples = generateMultiplicationTriples(num_triples);
        sacrificePhase(num_triples);
        
        auto squares = generateSquarePairs(num_squares);
        auto bit_triples = generateBitTriples(num_bits);
        auto query_masks = generateInputMasks(dim);
        auto db_masks = generateInputMasks(dbsize * dim);
        
        batchMACVerification();
        zkProofSimulation();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        double mb = total_communication / (1024.0 * 1024.0);
        
        // realistic communication estimate
        size_t realistic_comm = RealisticSPDZEstimator::calculateTotalRealistic(dim, dbsize);
        double realistic_mb = realistic_comm / (1024.0 * 1024.0);
        double realistic_gb = realistic_mb / 1024.0;
        
        std::cout << "\noffline phase complete:" << std::endl;
        //std::cout << " simulated communication: " << std::fixed << std::setprecision(2) << mb << " MB" << std::endl;
        //std::cout << " realistic estimate communication: " << std::fixed << std::setprecision(2) 
                  //<< realistic_mb << " MB (" << realistic_gb << " GB)" << std::endl;
        //std::cout << "  time: " << duration << " ms" << std::endl;
    }
    
    size_t getTotalCommunication() const { return total_communication; }
};

int main(int argc, char* argv[]) {
    int base_port = (argc >= 2) ? std::stoi(argv[1]) : 8000;
    
    std::vector<NetworkConfig> networks = {
        NetworkConfig("LAN", 1, 1000, "LAN: RTT 2ms, 1Gbps"),
        NetworkConfig("MAN", 10, 100, "MAN: RTT 20ms, 100Mbps"),
        NetworkConfig("WAN", 50, 50, "WAN: RTT 100ms, 50Mbps"),
    };
    
    std::cout << "=== Party 0 offline ===" << std::endl;
    std::cout << "vector length: " << dim << std::endl;
    std::cout << "database size: " << dbsize << std::endl;
    std::cout << std::endl;
    
    for (const auto& net : networks) {
        try {
            setupNetwork(net);
            
            Party party(PARTY_ID, 2, base_port);
            SPDZOfflineSimulator simulator(party);
            
            
            auto start_time = std::chrono::high_resolution_clock::now();
            simulator.runCompleteOfflinePhase();
            auto end_time = std::chrono::high_resolution_clock::now();
            
            auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            double mb = simulator.getTotalCommunication() / (1024.0 * 1024.0);
            
            // realistic communication cost
            size_t realistic_comm = RealisticSPDZEstimator::calculateTotalRealistic(dim, dbsize);
            double realistic_mb = realistic_comm / (1024.0 * 1024.0);
            double realistic_gb = realistic_mb / 1024.0;
            
            double throughput = (total_duration > 0) ? mb / (total_duration / 1000.0) : 0;
            double realistic_throughput = (total_duration > 0) ? realistic_mb / (total_duration / 1000.0) : 0;
            
            std::cout << net.name 
                      << " (delay: " << net.delay_ms << "ms, bandwidth: " << net.bandwidth_mbps << "Mbps)" << std::endl;
            
            std::cout << " time: " << total_duration << " ms" << std::endl;
            std::cout << " simulated communication : " << std::setprecision(2) << mb << " MB" << std::endl;
            std::cout << " realistic communication : " << std::setprecision(2) 
                      << realistic_mb << " MB (" << realistic_gb << " GB)" << std::endl;
   
            
        } catch (const std::exception& e) {
            std::cout << "Error [" << net.name << "]: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    cleanupNetwork();
    
    return 0;
}