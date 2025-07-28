#include <chrono>
#include "fss-common.h"
#include "fss-server.h"
#include "fss-client.h"

int main()
{
    // Set up variables
    uint64_t a = 3;
    uint64_t b_0 = 1;
    uint64_t b_1 = -1;
    Fss fClient, fServer;
    ServerKeyEq k0;
    ServerKeyEq k1;


    ServerKeyReLU relu_k0_0;
    ServerKeyReLU relu_k1_0;
    ServerKeyReLU relu_k0_1;
    ServerKeyReLU relu_k1_1;
    
    initializeClient(&fClient, 10, 2);
    auto t_begin = std::chrono::high_resolution_clock::now();
    size_t rounds = 256;//set  rounds
    for(size_t i=0; i<rounds; i++) {
        generateKeyReLu(&fClient, &relu_k0_0, &relu_k0_1,&relu_k1_0,&relu_k1_1);
        generateKeyReLu(&fClient, &relu_k0_0, &relu_k0_1,&relu_k1_0,&relu_k1_1);
    }
    std::cout<<std::endl;
    std::cout<<std::endl;
    std::cout<<std::endl;
    std::cout<<"length="<<rounds<<std::endl;
    auto t_end = std::chrono::high_resolution_clock::now();
    std::cout << "----------offline: -----------" <<std::endl;
    std::cout<<"time="<<std::chrono::duration<double, std::milli>(t_end - t_begin).count()<< " ms" << endl;
    double offline_com = sizeof(ServerKeyReLU)*rounds*12/(1024.0*1024.0); // MB
    std::cout << "offline communication:" <<  offline_com  << " MB" <<std::endl;

    initializeServer(&fServer, &fClient);
    uint64_t lt_ans0, lt_ans1, lt_fin;

    lt_ans0= evaluateReLu(&fServer, &relu_k0_0, &relu_k0_1,&relu_k1_0,&relu_k1_1, (a-1));
    lt_ans1= evaluateReLu(&fServer, &relu_k0_0, &relu_k0_1,&relu_k1_0,&relu_k1_1, (a-1));
    lt_fin = lt_ans0 - lt_ans1;
    lt_fin = lt_ans0 - lt_ans1;
    //cout << "FSS Lt Match (should be non-zero): " << lt_fin<< endl;


  
    t_begin = std::chrono::high_resolution_clock::now();
    //for(size_t i=0; i<rounds; i++) {
     //   volatile auto x = evaluateEq(&fServer, &k0, i);
    //}
    for(size_t i=0; i<rounds; i++) {
        volatile auto x = evaluateReLu(&fServer, &relu_k0_0, &relu_k0_1,&relu_k1_0,&relu_k1_1, i);
        volatile auto y = evaluateReLu(&fServer, &relu_k0_0, &relu_k0_1,&relu_k1_0,&relu_k1_1, i);
    }

    t_end = std::chrono::high_resolution_clock::now();
    std::cout << "--------------online:----------- " <<std::endl;
    std::cout<<"time="<<std::chrono::duration<double, std::milli>(t_end - t_begin).count()<< " ms" << endl;
    double online_com = sizeof(ServerKeyLt)*rounds/(1024.0*1024.0); // MB
    std::cout << "online communication:" <<  online_com  << " MB" <<std::endl;
    return 1;
}
