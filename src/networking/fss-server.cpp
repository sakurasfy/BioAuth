// This is the server side code for FSS which does the evaluation

#include "fss-server.h"

void initializeServer(Fss* fServer, Fss* fClient) {
    fServer->numKeys = fClient->numKeys;
    fServer->aes_keys = (AES_KEY*) malloc(sizeof(AES_KEY)*fClient->numKeys);
    memcpy(fServer->aes_keys, fClient->aes_keys, sizeof(AES_KEY)*fClient->numKeys);
    fServer->numBits = fClient->numBits;
    fServer->numParties = fClient->numParties;
    fServer->prime = fClient->prime;
}

// evaluate whether x satisifies value in function stored in key k

mpz_class evaluateEq(Fss* f, ServerKeyEq *k, uint64_t x) {

    // get num bits to be compared
    uint32_t n = f->numBits;

    // start at the correct LSB
    int xi = getBit(x, (64-n+1));
    unsigned char s[16];
    memcpy(s, k->s[xi], 16);
    unsigned char t = k->t[xi];
    
    unsigned char sArray[32];
    unsigned char temp[2];
    unsigned char out[48];
    for (uint32_t i = 1; i < n+1; i++) {
        if(i!=n) {
            xi = getBit(x, (64-n+i+1));
        } else {
            xi = 0;
        }
        prf(out, s, 48, f->aes_keys, f->numKeys);
        memcpy(sArray, out, 32);
        temp[0] = out[32] % 2;
        temp[1] = out[33] % 2;
        //printf("s: ");
        //printByteArray(s, 16);
        //printf("out: %d %d\n", out[32], out[33]);
        if (i == n) {
            break;
        }
        int xStart = 16 * xi;
        memcpy(s, (unsigned char*) (sArray + xStart), 16);
        for (uint32_t j = 0; j < 16; j++) {
            s[j] = s[j] ^ k->cw[t][i-1].cs[xi][j];
        }
        //printf("After XOR: ");
        //printByteArray(s, 16);
        //printf("%d: t: %d %d, ct: %d, bit: %d\n", i, temp[0], temp[1], k->cw[t][i-1].ct[xi], xi);
        t = temp[xi] ^ k->cw[t][i-1].ct[xi];
    }

    mpz_class ans;
    unsigned char sIntArray[34];
    memcpy(sIntArray, sArray, 32);
    sIntArray[32] = temp[0];
    sIntArray[33] = temp[1];
    mpz_import(ans.get_mpz_t(), 34, 1, sizeof(sIntArray[0]), 0, 0, sIntArray);
    ans = ans * k->w;
    ans = ans % f->prime;
    return ans;
}

// Evaluate whether x < value in function stored in key k

TempEva evaluateLt(Fss* f, ServerKeyLt *k_0, ServerKeyLt *k_1,uint64_t x) {

    TempEva tempp;
    uint32_t n = f->numBits;

    int xi = getBit(x, (64-n+1));
    unsigned char s_0[16];
    memcpy(s_0, k_0->s[xi], 16);
    unsigned char t_0 = k_0->t[xi];
    uint64_t v_0 = k_0->v[xi];

    unsigned char s_1[16];
    memcpy(s_1, k_1->s[xi], 16);
    unsigned char t_1 = k_1->t[xi];
    uint64_t v_1 = k_1->v[xi];

    unsigned char sArray[32];
    unsigned char temp[2];
    unsigned char out[64];
    uint64_t temp_v;
    for (uint32_t i = 1; i < n; i++) {
        if(i!=n) {
            xi = getBit(x, (64-n+i+1));
        } else {
            xi = 0;
        }
        //k_0
        prf(out, s_0, 64, f->aes_keys, f->numKeys);
        memcpy(sArray, out, 32);
        temp[0] = out[32] % 2;
        temp[1] = out[33] % 2;

        temp_v = byteArr2Int64((unsigned char*) (out + 40 + (8*xi)));
        int xStart = 16 * xi;
        memcpy(s_0, (unsigned char*) (sArray + xStart), 16);
        for (uint32_t j = 0; j < 16; j++) {
            s_0[j] = s_0[j] ^ k_0->cw[t_0][i-1].cs[xi][j];
        }
        //printf("%d: t: %d %d, ct: %d, bit: %d\n", i, temp[0], temp[1], k->cw[t][i-1].ct[xi], xi);
        //printf("temp_v: %lld\n", temp_v);
        v_0 = (v_0 + temp_v);
        v_0 = (v_0 + k_0->cw[t_0][i-1].cv[xi]);
        t_0 = temp[xi] ^ k_0->cw[t_0][i-1].ct[xi];

        //k_1
        prf(out, s_1, 64, f->aes_keys, f->numKeys);
        memcpy(sArray, out, 32);
        temp[0] = out[32] % 2;
        temp[1] = out[33] % 2;

        temp_v = byteArr2Int64((unsigned char*)(out + 40 + (8*xi)));
        memcpy(s_1, (unsigned char*)(sArray + xStart), 16);
        for (uint32_t j = 0; j < 16; j++) {
            s_1[j] = s_1[j] ^ k_1->cw[t_1][i-1].cs[xi][j];
        }
        v_1 = (v_1 + temp_v);
        v_1 = (v_1 + k_1->cw[t_1][i-1].cv[xi]);
        t_1 = temp[xi] ^ k_1->cw[t_1][i-1].ct[xi];
    }
    tempp.v_0=v_0;
    tempp.v_1=v_1;
    return tempp;
}


uint64_t evaluateReLu(Fss* f, ServerKeyReLU* k0_0, ServerKeyReLU* k1_0,ServerKeyReLU* k0_1, ServerKeyReLU* k1_1, uint64_t x) {

    uint32_t n = f->numBits;

    uint64_t x0, x1;
    unsigned char tmp[8];
    if (!RAND_bytes(tmp, 8)) {
        printf("Random bytes failed.\n");
        exit(1);
    }
    x0 = byteArr2Int64(tmp);
    x1 = x - x0;


    uint64_t xr0 = x0+k0_0->r_share;
    uint64_t xr1 = x1+k0_0->r_share;

    uint64_t xr = xr0+xr1;

    uint64_t b_0=k0_0->b0_share+k1_0->b0_share;

    uint64_t b_1=k0_0->b1_share+k1_0->b1_share;

       // 评估比较结果
    TempEva temp0 = evaluateLt(f, &k0_0->key_lt, &k1_0->key_lt, xr);
    uint64_t b0_share0 = temp0.v_0;
    uint64_t b0_share1 = temp0.v_1;

    TempEva temp1 = evaluateLt(f, &k0_1->key_lt, &k1_1->key_lt, xr);
    uint64_t b1_share0 = temp1.v_0;
    uint64_t b1_share1 = temp1.v_1;

    // 计算ReLU结果
    uint64_t y0_0 = (b0_share0 * xr + b1_share0);
    uint64_t y0_1 = (b0_share1 * xr + b1_share1);

    // 返回结果（根据协议可能需要返回其中一个或组合）
    return y0_0 + y0_1;
}