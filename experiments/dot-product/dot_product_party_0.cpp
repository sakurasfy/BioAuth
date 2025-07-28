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

    PartyWithFakeOffline<ShrType> party(0, 2, 5050, kJobName);
    //std::cout<<"offline-------"<<std::endl;
    Circuit<ShrType> circuit(party);

    auto a = circuit.input(0, 1, dim);
    auto b = circuit.input(1, dim, 1);
    auto c = circuit.multiply(a, b);
    auto d = circuit.output(c);
    circuit.addEndpoint(d);

    std::vector<ClearType> vec_a(dim);
    std::generate(vec_a.begin(), vec_a.end(), [] { return getRand<ClearType>(); });
    a->setInput(vec_a);
    //b->setInput(vec);

    circuit.readOfflineFromFile();
    //std::cout<<"before-------------"<<endl;
    //circuit.printStats();
    circuit.runOnlineWithBenckmark();
    //std::cout<<"after-------------"<<endl;
    circuit.printStats();


    return 0;
}
