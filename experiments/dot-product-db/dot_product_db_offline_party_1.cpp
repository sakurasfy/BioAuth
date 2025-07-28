//by sakara
//offline simulation for Party 1
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

constexpr int PARTY_ID = 1;
constexpr uint64_t FIELD_SIZE = (1ULL << 61) - 1; 

class RealisticSPDZEstimator {
private:
    static constexpr size_t BGV_CIPHERTEXT_SIZE = 16 * 1024;     
    static constexpr size_t PACKING_SIZE = 1024;                
    static constexpr size_t ZK_PROOF_SIZE = 32 * 1024;          
    static constexpr size_t MAC_AUTH_SIZE = 64;                 
    static constexpr size_t SACRIFICE_RATIO = 10;               
    static constexpr size_t PROTOCOL_OVERHEAD = 1024;           
    
public:
    static size_t estimateTripleGeneration(size_t num_triples) {
        size_t total = 0;
        
        size_t ciphertexts_needed = (num_triples + PACKING_SIZE - 1) / PACKING_SIZE;
        total += ciphertexts_needed * BGV_CIPHERTEXT_SIZE * 2;

        total += ciphertexts_needed * ZK_PROOF_SIZE * 2;

        total += 5 * PROTOCOL_OVERHEAD;
        
        return total;
    }
    
    static size_t estimateSacrificePhase(size_t num_triples) {
        size_t sacrifice_count = num_triples / SACRIFICE_RATIO;
        size_t total = 0;

        total += sacrifice_count * BGV_CIPHERTEXT_SIZE * 6;
        
        // MAC
        total += sacrifice_count * MAC_AUTH_SIZE * 4;
        
        return total;
    }
    
    static size_t estimateSquarePairs(size_t count) {
        size_t total = 0;
        
        size_t ciphertexts_needed = (count + PACKING_SIZE - 1) / PACKING_SIZE;
        total += ciphertexts_needed * BGV_CIPHERTEXT_SIZE * 2;
        
        total += count * MAC_AUTH_SIZE * 2;
        
        return total;
    }
    
    static size_t estimateBitTriples(size_t count) {
        return count * 512; 
    }
    
    static size_t estimateInputMasks(size_t count) {
        size_t total = 0;
        
        size_t ciphertexts_needed = (count + PACKING_SIZE - 1) / PACKING_SIZE;
        total += ciphertexts_needed * BGV_CIPHERTEXT_SIZE * 2;
        
        // MAC
        total += count * MAC_AUTH_SIZE * 2;
        
        return total;
    }
    
    static size_t estimateGlobalVerification(size_t total_values) {
        size_t total = 0;
        
        total += 8 * BGV_CIPHERTEXT_SIZE * 2; 
        
        total += 8 * 1024 * 2;
        
        return total;
    }
    
    static size_t estimateZKProofOverhead() {
        return 20 * ZK_PROOF_SIZE * 2; 
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
    uint64_t a_bit, b_bit, c_bit;
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

class SPDZCommunicationTester {
private:
    Party& party;
    std::mt19937_64 gen;
    size_t total_communication = 0;
    
public:
    SPDZCommunicationTester(Party& p) : party(p), gen(std::random_device{}()) {}
    
    // 
    void handleMACKeyExchange() {
        
        std::uniform_int_distribution<uint64_t> dis;
        
        // 
        uint64_t remote_key_share = party.Receive<uint64_t>(0);
        
        uint64_t local_key_share = dis(gen);
        party.Send(0, local_key_share);
        
        uint64_t recv_test_value = party.Receive<uint64_t>(0);
        uint64_t recv_test_mac = party.Receive<uint64_t>(0);
        
        uint64_t test_value = dis(gen);
        uint64_t test_mac = dis(gen);
        party.Send(0, test_value);
        party.Send(0, test_mac);
        
        total_communication += 4 * sizeof(uint64_t);
    }
    
    // 
    void handleTripleGeneration(size_t count) {
        
        std::uniform_int_distribution<uint64_t> dis;
        
        for (size_t i = 0; i < count; i++) {
            auto received_triple = party.ReceiveVec<uint64_t>(0, 6);

            std::vector<uint64_t> local_triple(6);
            for (auto& val : local_triple) val = dis(gen);
            party.SendVec(0, local_triple);
            
            total_communication += 12 * sizeof(uint64_t);
        }
        
    }
    
    // 
    void handleSacrificePhase(size_t total_triples) {
        
        std::uniform_int_distribution<uint64_t> dis;
        size_t num_to_sacrifice = std::max(1UL, total_triples / 10);
        
        for (size_t i = 0; i < num_to_sacrifice; i++) {
            // 
            uint64_t remote_a = party.Receive<uint64_t>(0);
            uint64_t remote_b = party.Receive<uint64_t>(0);
            uint64_t remote_c = party.Receive<uint64_t>(0);
            
            // 
            party.Send(0, dis(gen)); // a_share
            party.Send(0, dis(gen)); // b_share
            party.Send(0, dis(gen)); // c_share
            
            total_communication += 6 * sizeof(uint64_t);
        }
        
    }
    
    // 
    void handleSquarePairs(size_t count) {
        
        std::uniform_int_distribution<uint64_t> dis;
        
        for (size_t i = 0; i < count; i++) {
            auto received_pair = party.ReceiveVec<uint64_t>(0, 4);
            
            std::vector<uint64_t> local_pair(4);
            for (auto& val : local_pair) val = dis(gen);
            party.SendVec(0, local_pair);
            
            total_communication += 8 * sizeof(uint64_t);
        }
        
    }
    
    void handleBitTriples(size_t count) {
        std::uniform_int_distribution<uint64_t> dis;
        
        for (size_t i = 0; i < count; i++) {
            auto received_bits = party.ReceiveVec<uint64_t>(0, 6);
            
            std::vector<uint64_t> local_bits(6);
            for (auto& val : local_bits) val = dis(gen);
            party.SendVec(0, local_bits);
            
            total_communication += 12 * sizeof(uint64_t);
        }
    }
    
    //
    void handleInputMasks(size_t count) {
       
        std::uniform_int_distribution<uint64_t> dis;
        
        for (size_t i = 0; i < count; i++) {
            uint64_t recv_mask = party.Receive<uint64_t>(0);
            uint64_t recv_mac = party.Receive<uint64_t>(0);

            party.Send(0, dis(gen)); // mask
            party.Send(0, dis(gen)); // mac
            
            total_communication += 4 * sizeof(uint64_t);
        }
        
    }
    
    // 
    void handleBatchMACVerification() {
        
        std::uniform_int_distribution<uint64_t> dis;
        
        // 
        auto remote_challenges = party.ReceiveVec<uint64_t>(0, 10);
        
        // 
        std::vector<uint64_t> challenges(10);
        for (auto& c : challenges) {
            c = dis(gen);
            party.Send(0, c);
        }
        
        uint64_t recv_combined_value = party.Receive<uint64_t>(0);
        uint64_t recv_combined_mac = party.Receive<uint64_t>(0);

        party.Send(0, dis(gen)); // combined_value
        party.Send(0, dis(gen)); // combined_mac
        
        total_communication += 24 * sizeof(uint64_t);
    }
    
    void handleZKProof() {
        
        std::uniform_int_distribution<uint64_t> dis;
        
        for (int round = 0; round < 3; round++) {
            uint64_t recv_commitment = party.Receive<uint64_t>(0);
            
            uint64_t recv_challenge = party.Receive<uint64_t>(0);
            party.Send(0, dis(gen)); // challenge
            
            uint64_t recv_response = party.Receive<uint64_t>(0);
            party.Send(0, dis(gen)); // response
            
            total_communication += 5 * sizeof(uint64_t);
        }
        
    }

    void runCommunicationTest() {

        size_t num_triples = dim * dbsize;  
        size_t num_squares = dbsize;        
        size_t num_bits = dim * 64;         
        size_t num_input_masks = dim + dbsize * dim; 
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        handleMACKeyExchange();
        handleTripleGeneration(num_triples);
        handleSacrificePhase(num_triples);
        handleSquarePairs(num_squares);
        handleBitTriples(num_bits);
        handleInputMasks(dim);
        handleInputMasks(dbsize * dim);
        handleBatchMACVerification();
        handleZKProof();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        double mb = total_communication / (1024.0 * 1024.0);
        
        size_t realistic_comm = RealisticSPDZEstimator::calculateTotalRealistic(dim, dbsize);
        double realistic_mb = realistic_comm / (1024.0 * 1024.0);
        double realistic_gb = realistic_mb / 1024.0;
        
        std::cout << "\noffline complete:" << std::endl;
        //std::cout << "  simulated communication: " << std::fixed << std::setprecision(2) << mb << " MB" << std::endl;
        //std::cout << "  realistic communiciation: " << std::fixed << std::setprecision(2) 
                  //<< realistic_mb << " MB (" << realistic_gb << " GB)" << std::endl;
        //std::cout << "  time: " << duration << " ms" << std::endl;
    }
    
    size_t getTotalCommunication() const { return total_communication; }
};

int main(int argc, char* argv[]) {
    int base_port = (argc >= 2) ? std::stoi(argv[1]) : 8000;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "=== Party 0 offline ===" << std::endl;
    std::cout << "vector length: " << dim << std::endl;
    std::cout << "database size: " << dbsize << std::endl;
    std::cout << std::endl;
    
    const std::vector<std::string> network_names = {
       "LAN", "MAN", "WAN"
    };
    
    for (size_t test = 0; test < network_names.size(); test++) {
        try {
            
            
            Party party(PARTY_ID, 2, base_port);
            SPDZCommunicationTester tester(party);
            
            auto start_time = std::chrono::high_resolution_clock::now();
            tester.runCommunicationTest();
            auto end_time = std::chrono::high_resolution_clock::now();
            
            auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            uint64_t party_bytes = party.bytes_sent();
            double party_mb = party_bytes / (1024.0 * 1024.0);
            double total_mb = tester.getTotalCommunication() / (1024.0 * 1024.0);
            
            size_t realistic_comm = RealisticSPDZEstimator::calculateTotalRealistic(dim, dbsize);
            double realistic_mb = realistic_comm / (1024.0 * 1024.0);
            double realistic_gb = realistic_mb / 1024.0;
            
            double throughput = (total_duration > 0) ? party_mb / (total_duration / 1000.0) : 0;
            double realistic_throughput = (total_duration > 0) ? realistic_mb / (total_duration / 1000.0) : 0;
            
            std::cout  << network_names[test] << std::endl;
            std::cout << "  time: " << total_duration << " ms" << std::endl;
            std::cout << "  simulated communication: " << std::setprecision(2) << total_mb << " MB" << std::endl;
            std::cout << "  realistic estimate communication: " << std::setprecision(2) 
                      << realistic_mb << " MB (" << realistic_gb << " GB)" << std::endl;
            std::cout << std::string(50, '-') << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "Error [" << network_names[test] << "]: " << e.what() << std::endl;
        }
        
        if (test < network_names.size() - 1) {
            std::cout << "wait for next testing..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    /*std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "=== Party 1  ===" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    size_t total_realistic = RealisticSPDZEstimator::calculateTotalRealistic(dim, dbsize);
    double total_realistic_gb = total_realistic / (1024.0 * 1024.0 * 1024.0);
    std::cout << "  vector length: " << dim << std::endl;
    std::cout << "  database size: " << dbsize << std::endl;
    std::cout << std::endl;
    
    std::cout << "realistic communiciation: " << std::fixed << std::setprecision(2) 
              << total_realistic_gb << " GB" << std::endl;
    std::cout << std::endl;*/

    
    return 0;
}