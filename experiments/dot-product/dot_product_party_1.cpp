// by sakara

#include "dot_product_config.h"

#include "share/Spdz2kShare.h"
#include "protocols/Circuit.h"
#include "utils/print_vector.h"
#include "utils/rand.h"

using namespace std;
using namespace bioauth;
using namespace bioauth::experiments::dot_product;

int main() {
    using ShrType = Spdz2kShare64;
    using ClearType = ShrType::ClearType;
    //vector<ClearType> vec(dim, 1);  

    PartyWithFakeOffline<ShrType> party(1, 2, 5050, kJobName);
    Circuit<ShrType> circuit(party);

    auto a = circuit.input(0, 1, dim);
    auto b = circuit.input(1, dim, 1);
    auto c = circuit.multiply(a, b);
    auto d = circuit.output(c);
    circuit.addEndpoint(d);

    std::vector<ClearType> vec_b(dim);
    std::generate(vec_b.begin(), vec_b.end(), [] { return getRand<ClearType>(); });
    b->setInput(vec_b);

    circuit.readOfflineFromFile();
    circuit.runOnlineWithBenckmark();

    circuit.printStats();


    return 0;
}
