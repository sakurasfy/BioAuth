// by sakara

#include "dot_product_config.h"

#include "fake-offline/FakeInputGate.h"
#include "fake-offline/FakeAddGate.h"
#include "fake-offline/FakeCircuit.h"
#include "share/Spdz2kShare.h"
#include "share/Mod2PowN.h"
#include "share/IsSpdz2kShare.h"
#include "fake-offline/FakeParty.h"
#include <chrono>
#include <iostream>

using namespace std;
using namespace bioauth;
using namespace bioauth::experiments::dot_product;


int main() {
    using ShrType = bioauth::Spdz2kShare64;


    FakeParty<ShrType, 2> party(kJobName);
    FakeCircuit<ShrType, 2> circuit(party);
    auto start = std::chrono::high_resolution_clock::now();
    auto a = circuit.input(0, 1, dim);
    auto b = circuit.input(0, dim, 1);
    auto c = circuit.multiply(a, b);
    auto d = circuit.output(c);
    circuit.addEndpoint(d);
    
    circuit.runOffline();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Offline phase took " << duration << " ms." << std::endl;
    std::cout << "Offline comm cost " << party.getTotalOfflineBytesWritten() << " bytes." << std::endl;

    return 0;
}
