//by sakara
#include <chrono>
#include <iostream>
#include <iomanip>
#include "networking/fss-common.h"
#include "networking/fss-server.h"
#include "networking/fss-client.h"
#include "com_config.h"

using namespace std;

int main()
{
    auto networks = getNetworkConfigurations();
    
    cout << "=== Secure comparison Party_1 ===" << endl;
    
    for (const auto& net : networks) {
        try {
            printNetworkTestHeader(net);
            setupNetwork(net);
            
            // Set up variables using config constants
            Fss fClient, fServer;
            ServerKeyReLU relu_k0_0;
            ServerKeyReLU relu_k1_0;
            ServerKeyReLU relu_k0_1;
            ServerKeyReLU relu_k1_1;
            
            // Initialize client and server
            initializeClient(&fClient, FSS_Config::FSS_NUM_BITS, FSS_Config::FSS_NUM_PARTIES);
            
            cout << "Generating keys for evaluation..." << endl;
            // Generate keys (in real system, these would come from party 0)
            generateKeyReLu(&fClient, &relu_k0_0, &relu_k0_1, &relu_k1_0, &relu_k1_1);
            
            initializeServer(&fServer, &fClient);
            
            // Test functionality using config constants
            cout << "Testing ReLU functionality..." << endl;
            uint64_t lt_ans0, lt_ans1, lt_fin;
            uint64_t test_input = FSS_Config::TEST_VALUE_A - 1;
            
            lt_ans0 = evaluateReLu(&fServer, &relu_k0_0, &relu_k0_1, &relu_k1_0, &relu_k1_1, test_input);
            lt_ans1 = evaluateReLu(&fServer, &relu_k0_0, &relu_k0_1, &relu_k1_0, &relu_k1_1, test_input);
            lt_fin = lt_ans0 - lt_ans1;
            
            cout << "FSS ReLU test result: " << lt_fin << endl;
            
            cout << "Starting online phase ..." << endl;
            auto t_begin = chrono::high_resolution_clock::now();
            
            size_t rounds = FSS_Config::DEFAULT_ROUNDS;
            for(size_t i = 0; i < rounds; i++) {
                volatile auto x = evaluateReLu(&fServer, &relu_k0_0, &relu_k0_1, &relu_k1_0, &relu_k1_1, i);
                volatile auto y = evaluateReLu(&fServer, &relu_k0_0, &relu_k0_1, &relu_k1_0, &relu_k1_1, i);
            }
            
            auto t_end = chrono::high_resolution_clock::now();
            
            // Calculate performance metrics
            double total_time = chrono::duration<double, milli>(t_end - t_begin).count();
            double online_com = sizeof(uint64_t) * rounds * 3 / (1024.0 * 1024.0); // MB 
            double throughput = (rounds * 2 * 1000.0) / total_time; // evaluations per second
            
            cout << "\n--- Results for " << net.name << " ---" << endl;
            cout << "Rounds: " << rounds << endl;
            cout << "Total time: " << fixed << setprecision(2) << total_time << " ms" << endl;
            cout << "Average time per evaluation: " << fixed << setprecision(3) 
                 << (total_time / (rounds * 2)) << " ms" << endl;
            cout << "Online communication: " << fixed << setprecision(4) << online_com << " MB" << endl;
            //cout << "Throughput: " << fixed << setprecision(1) << throughput << " evaluations/sec" << endl;

            double avg_latency = total_time / rounds; // ms per operation pair
            double time_in_seconds = total_time / 1000.0;
            double effective_bandwidth = (online_com * 8) / time_in_seconds; // Mbps
            double bandwidth_utilization = (effective_bandwidth / net.bandwidth_mbps) * 100.0;
            
            cout << "Server (Party 1) completed function evaluation phase for " << net.name << endl;
            
        } catch (const exception& e) {
            cout << "Error [" << net.name << "]: " << e.what() << endl;
        }
        
        handleTestCleanup();
    }
    
    printTestCompletion();
    
    return 0;
}