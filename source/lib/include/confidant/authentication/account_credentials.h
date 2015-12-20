#pragma once

#include <confidant/common/secure_memory.h>

typedef SecureMemory< crypto_pwhash_scryptsalsa208sha256_SALTBYTES > SaltSecureMemory;
typedef SecureMemory< crypto_sign_SECRETKEYBYTES > SigningPrivateKeySecureMemory;
typedef SecureMemory< crypto_sign_PUBLICKEYBYTES > SigningPublicKeySecureMemory;

typedef SecureMemory< crypto_box_SECRETKEYBYTES > EncryptingPrivateKeySecureMemory;
typedef SecureMemory< crypto_box_PUBLICKEYBYTES > EncryptingPublicKeySecureMemory;

typedef SecureMemory< crypto_box_SEEDBYTES > SeedKeySecureMemory;

bool GenerateKeyPairSeedFromCredentials( SeedKeySecureMemory& seedKey, const SaltSecureMemory& salt, const SecureMemoryBase& username, const SecureMemoryBase& password );
bool GenerateSigningKeyPairFromSeed( SigningPrivateKeySecureMemory& privateKeyOut, SigningPublicKeySecureMemory& publicKeyOut, const SeedKeySecureMemory& seedKey );
bool GenerateEncryptingKeyPairFromSeed( EncryptingPrivateKeySecureMemory& privateKeyOut, EncryptingPublicKeySecureMemory& publicKeyOut, const SeedKeySecureMemory& seedKey );
