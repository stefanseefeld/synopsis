
typedef struct rpk {
    int    bits;
} RSA_PUBLIC_KEY;

void testFunc( RSA_PUBLIC_KEY* publicKey )
{
    struct rpk        pk2;
    RSA_PUBLIC_KEY    pk3;
    RSA_PUBLIC_KEY*   pk4;
    
    publicKey->bits = 4;

    pk2.bits = 3;
    pk3.bits = 2;
    pk4->bits = 1;
}

