#include <confidant/authentication/account_credentials.h>
#include <string.h>

#define CREDENTIALS_OPSLIMIT crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE * 4
#define CREDENTIALS_MEMLIMIT crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE * 4

bool GenerateKeyPairSeedFromCredentials( SeedKeySecureMemory& seedKeyOut, const SaltSecureMemory& salt, const SecureMemoryBase& username, const SecureMemoryBase& password )
{
	auto saltBytes = salt.lock();
	auto usernameBytes = username.lock();
	auto passwordBytes = password.lock();

	SecureMemoryBase combinedKey( username.getSize() + password.getSize() );
	auto combinedKeyBytes = combinedKey.lock( SecureMemoryBase::Write );

	strcpy_s( combinedKeyBytes, combinedKey.getSize(), usernameBytes );
	strcat_s( combinedKeyBytes, combinedKey.getSize(), passwordBytes );

	auto seedBytes = seedKeyOut.lock( SecureMemoryBase::Write );

	if( crypto_pwhash_scryptsalsa208sha256( seedBytes, seedKeyOut.getSize(), combinedKeyBytes, strlen(combinedKeyBytes), saltBytes, CREDENTIALS_OPSLIMIT, CREDENTIALS_MEMLIMIT ) != 0 )
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
