#include <confidant/authentication/account_credentials.h>
#include <string.h>

#define CREDENTIALS_OPSLIMIT crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_SENSITIVE
#define CREDENTIALS_MEMLIMIT crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_SENSITIVE

#ifndef WIN32
#define strcpy_s(dest, size, src) strcpy(dest, src)
#define strcat_s(dest, size, src) strcat(dest, src)
#endif

bool GenerateKeyPairSeedFromPassword( SeedKeySecureMemory& seedKeyOut, const SaltSecureMemory& salt, const SecureMemoryBase& password )
{
	auto saltBytes = salt.lock();
	auto passwordBytes = password.lock();

	auto seedBytes = seedKeyOut.lock( SecureMemoryBase::Write );

	if( crypto_pwhash_scryptsalsa208sha256( seedBytes, seedKeyOut.getSize(), passwordBytes, strlen(passwordBytes), saltBytes, CREDENTIALS_OPSLIMIT, CREDENTIALS_MEMLIMIT ) != 0 )
	{
		return false;
	}

	return true;
}

bool GenerateSigningKeyPairFromSeed( SigningPrivateKeySecureMemory& privateKeyOut, SigningPublicKeySecureMemory& publicKeyOut, const SeedKeySecureMemory& seedKey )
{
	auto privateKeyOutBytes = privateKeyOut.lock( SecureMemoryBase::Write );
	auto publicKeyOutBytes = publicKeyOut.lock( SecureMemoryBase::Write );
	auto seedBytes = seedKey.lock();

	if( crypto_sign_seed_keypair( publicKeyOutBytes, privateKeyOutBytes, seedBytes ) != 0 )
	{
		return false;
	}

	return true;
}

bool GenerateEncryptingKeyPairFromSeed( EncryptingPrivateKeySecureMemory& privateKeyOut, EncryptingPublicKeySecureMemory& publicKeyOut, const SeedKeySecureMemory& seedKey )
{
	auto privateKeyOutBytes = privateKeyOut.lock( SecureMemoryBase::Write );
	auto publicKeyOutBytes = publicKeyOut.lock( SecureMemoryBase::Write );
	auto seedBytes = seedKey.lock();

	if( crypto_box_seed_keypair( publicKeyOutBytes, privateKeyOutBytes, seedBytes ) != 0 )
	{
		return false;
	}

	return true;
}
