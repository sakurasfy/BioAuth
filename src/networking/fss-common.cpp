#include "fss-common.h"

AES_KEY* prf(unsigned char* out, unsigned char* key, uint64_t in_size, AES_KEY* aes_keys, uint32_t numKeys) {
    AES_KEY* temp_keys = aes_keys;

    // Do Matyas–Meyer–Oseas one-way compression function using different AES keys to get desired output length
    uint32_t num_keys_required = in_size / 16;
    if (num_keys_required > numKeys) {
        free(temp_keys);
        temp_keys = (AES_KEY*) malloc(sizeof(AES_KEY) * num_keys_required); 
        for (int i = 0; i < num_keys_required; i++) {
            unsigned char rand_bytes[16];
            if (!RAND_bytes(rand_bytes, 16)) {
                printf("Random bytes failed.\n");
            }

            // Use OpenSSL standard AES setup
            AES_set_encrypt_key(rand_bytes, 128, &(temp_keys[i]));
        }
    }

    for (int i = 0; i < num_keys_required; i++) {
        // Use OpenSSL standard AES encryption
        AES_encrypt(key, out + (i * 16), &temp_keys[i]);
    }

    for (int i = 0; i < in_size; i++) {
        out[i] = out[i] ^ key[i % 16];
    }

    return temp_keys;
}