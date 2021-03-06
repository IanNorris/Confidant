
#include <confidant/authentication.h>
#include <confidant/common/random.h>
#include <confidant/common/base64.h>

#include "input.h"
#include <string>
#include <fstream>

#include "restclient-cpp/restclient.h"
#include "restclient-cpp/meta.h"
#include <json/json.h>
#include <string.h> // string was already #include'd above, string.h is required for strlen and memcmp on linux

#if defined( _MSC_VER )
	#define PATH_SEPARATOR '\\'
	#pragma comment(lib, "libconfidant.lib")
#else
	#define PATH_SEPARATOR '/'
#endif

int PostJsonQuery( bool secure, const std::string& server, const std::string& command, const Json::Value& queryValues, Json::Value resultValues )
{
	std::string tTargetPath = ( secure ? "https://" : "http://") + server + command;
	
	Json::FastWriter writer;
	RestClient::response response = RestClient::post( tTargetPath, "text/json", writer.write( queryValues ) );
			
	bool success = false;
	if( response.code == 200 )
	{
		Json::Reader reader;
		if( !reader.parse( response.body, resultValues ) )
		{
			return 0;
		}
	}

	return response.code;
}

bool PostJsonQueryWithErrorHandling( bool secure, const std::string& server, const std::string& command, const Json::Value& queryValues, Json::Value resultValues )
{
	int responseCode = PostJsonQuery( secure, server, command, queryValues, resultValues );
	switch( responseCode )
	{
		case 0:
			std::cerr << "Server returned a malformed response." << std::endl;
			return false;

		case 200:
			return true;

		default:
			std::cerr << "Server returned error code " << responseCode << "." << std::endl;
			return false;
	}
}

bool ReadBinaryFileIntoSecureMemory( const std::string& filename, SecureMemoryBase& memoryOut, bool silentFail )
{
	std::ifstream saltFile( filename, std::ifstream::binary );
	if( saltFile )
	{
		saltFile.seekg( 0, std::ios::end );
		fpos_t fileSize = saltFile.tellg().seekpos();
		saltFile.seekg( 0, std::ios::beg );

		if( memoryOut.getSize() == 0 )
		{
			memoryOut.allocate( (size_t)fileSize );
		}
		else if( memoryOut.getSize() != fileSize )
		{
			std::cerr << "File " << filename << "does not match the expected file size of " << memoryOut.getSize() << "bytes , it is actually " << fileSize << " bytes." << std::endl;
			return false;
		}		

		auto memoryOutBytes = memoryOut.lock( SecureMemoryBase::Write );
		saltFile.read( memoryOutBytes, fileSize );
	}
	else
	{
		if( !silentFail )
		{
			std::cerr << "The file " << filename << " was not found, unable to load identity." << std::endl;
		}
		return false;
	}

	return true;
}

bool WriteSecureMemoryToFile( const std::string& filename, const SecureMemoryBase& content )
{
	std::ofstream file( filename, std::ofstream::binary );
	if( !file )
	{
		std::cerr << "Unable to write to file " << filename << std::endl;
		return false;
	}

	file.write( content.lock(), content.getSize() );
	file.close();

	return true;
}

bool DecryptAuthenticated( const SecureMemoryBase& encryptedMemory, const SeedKeySecureMemory& key, const NonceSecureMemory& nonce, SecureMemoryBase& plainTextOut )
{
	plainTextOut.allocate( 1 + encryptedMemory.getSize() - crypto_secretbox_MACBYTES );
	auto plainTextBytes = plainTextOut.lock( SecureMemoryBase::Write );

	auto nonceBytes = nonce.lock();
	auto keyBytes = key.lock();

	if( crypto_secretbox_open_easy( plainTextBytes, encryptedMemory.lock(), encryptedMemory.getSize(), nonceBytes, keyBytes ) != 0 )
	{
		return false;
	}
	((char*)plainTextBytes)[ encryptedMemory.getSize() - crypto_secretbox_MACBYTES ] = 0;

	return true;
}

bool EncryptAuthenticated( const char* plainTextBytes, size_t plainTextLength, const SeedKeySecureMemory& key, const NonceSecureMemory& nonce, SecureMemoryBase& cipherTextOut )
{
	cipherTextOut.allocate( crypto_secretbox_MACBYTES + plainTextLength );
	auto cipherTextBytes = cipherTextOut.lock( SecureMemoryBase::Write );

	auto nonceBytes = nonce.lock();
	auto keyBytes = key.lock();

	if( crypto_secretbox_easy( cipherTextBytes, (const unsigned char*)plainTextBytes, plainTextLength, nonceBytes, keyBytes ) != 0 )
	{
		return false;
	}

	return true;
}

bool EncryptAuthenticated( const std::string& plainText, const SeedKeySecureMemory& key, const NonceSecureMemory& nonce, SecureMemoryBase& plainTextOut )
{
	return EncryptAuthenticated( plainText.c_str(), plainText.size(), key, nonce, plainTextOut ); 
}

bool EncryptAuthenticated( const SecureMemoryBase& plainText, const SeedKeySecureMemory& key, const NonceSecureMemory& nonce, SecureMemoryBase& plainTextOut )
{
	return EncryptAuthenticated( plainText.lock(), plainText.getSize(), key, nonce, plainTextOut ); 
}

bool JsonStringValueToSecureMemory( const Json::Value& jsonValue, const char* keyName, SecureMemoryBase& memoryOut )
{
	SecureMemoryBase b64EncodedEncryptedNonce;
	Json::Value jsonValueOfKey = jsonValue.get( keyName, Json::Value(false) );

	if( !jsonValueOfKey.isString() )
	{
		return false;
	}

	const char* jsonValueOfKeyAsString = jsonValueOfKey.asCString();
	if( !jsonValueOfKeyAsString )
	{
		return false;
	}

	size_t stringLength = strlen(jsonValueOfKeyAsString);
			
	memoryOut.allocate( stringLength + 1 );
	memcpy( memoryOut.lock( SecureMemoryBase::Write ), jsonValueOfKeyAsString, stringLength );

	return true;
}

bool JsonStringValueToBase64DecodedSecureMemory( const Json::Value& jsonValue, const char* keyName, SecureMemoryBase& memoryOut )
{
	SecureMemoryBase encodedMemory;
	if( !JsonStringValueToSecureMemory( jsonValue, keyName, encodedMemory ) )
	{
		return false;
	}

	base64_decode( memoryOut, encodedMemory );

	return true;
}

int mainInternal( int argc, char* argv[] )
{
	if( argc < 2 )
	{
		std::cerr << "USAGE: confidant-cl <identity-directory>." << std::endl;
		std::cerr << "If this is your first time using a Confidant service," << std::endl;
		std::cerr << "specify an existing empty directory to receive your keys." << std::endl;
		exit(1);
	}

	if( sodium_init() == -1 )
	{
		std::cerr << "Failed to initialise libsodium." << std::endl;

		return 1;
	}

	std::cout << "--Confidant Client V0.01--" << std::endl;

	std::string identityPath = argv[1];
	if( identityPath.length() > 1 && identityPath[ identityPath.length() - 1 ] != PATH_SEPARATOR )
	{
		identityPath.append( 1, PATH_SEPARATOR );
	}

	Json::Value root;
	Json::Value identities;

	NonceSecureMemory nonce;
	auto nonceBytes = nonce.lock(SecureMemoryBase::Write);

	SeedKeySecureMemory currentIdentitySeed;
	std::string currentIdentity = "";

	SaltSecureMemory passwordSalt;
	SecureMemoryBase identityFileMemory;
	if( ReadBinaryFileIntoSecureMemory( identityPath + "confidant", identityFileMemory, true ) )
	{
		//Read the salt
		if( !ReadBinaryFileIntoSecureMemory( identityPath + "salt", passwordSalt, false ) )
		{
			return 1;
		}

		//Read the nonce
		if( !ReadBinaryFileIntoSecureMemory( identityPath + "nonce", nonce, false ) )
		{
			return 1;
		}
		
		SecureMemory<4096> password;
		std::cout << "Enter password (hidden): ";
		SecureReadConsole( password, false );

		SeedKeySecureMemory passwordKey;
		auto passwordKeyBytes = passwordKey.lock();
		if( !GenerateKeyPairSeedFromPassword( passwordKey, passwordSalt, password ) )
		{
			std::cerr << "Unable to generate keypair." << std::endl;
			return 3;
		}

		SecureMemoryBase plainText;
		if( !DecryptAuthenticated( identityFileMemory, passwordKey, nonce, plainText ) )
		{
			std::cerr << "Unable to decrypt identity file." << std::endl;
			return 4;
		}

		auto plainTextBytes = plainText.lock();
		Json::Reader reader;
		if( !reader.parse( (const char*)plainTextBytes, root ) )
		{
			std::cerr << "Unable to parse identity." << std::endl;
			return 5;
		}
		identities = root[ "identities" ]; 
	}
	else
	{
		std::cout << "No existing identity was found. You will now be guided through" << std::endl; 
		std::cout << "the process to create a new one. First you will be asked to" << std::endl;
		std::cout << "enter a password that will be used to encrypt all your identities" << std::endl;
		std::cout << "when they are saved to disk." << std::endl << std::endl;
		
		std::cout << "***NOTE*** if you forget this password" << std::endl;
		std::cout << "you will be PERMENANTLY locked out from all your identities" << std::endl;
		std::cout << "it is not possible to recover this password or any identities" << std::endl;
		std::cout << "that may be attached to it." << std::endl << std::endl;
		
		SecureMemory<4096> password;
		SecureMemory<4096> passwordAgain;

		while( true )
		{
			std::cout << "Enter password (hidden): ";
			SecureReadConsole( password, false );

			std::cout << "Re-enter password (hidden): ";
			SecureReadConsole( passwordAgain, false );

			if( SecureMemoryCompare( password, passwordAgain ) )
			{
				break;
			}

			std::cerr << "Passwords did not match. Please try again." << std::endl;
		}

		std::cout << "Your identity is now being created. This may take a long time on slow devices." << std::endl;

		FillBufferWithRandomBytes( passwordSalt );
		auto passwordSaltBytes = passwordSalt.lock();
		
		SeedKeySecureMemory passwordKey;
		auto passwordKeyBytes = passwordKey.lock();
		if( !GenerateKeyPairSeedFromPassword( passwordKey, passwordSalt, password ) )
		{
			std::cerr << "Unable to generate keypair." << std::endl;
			return 3;
		}

		SeedKeySecureMemory identitySeed;
		FillBufferWithRandomBytes( identitySeed );

		ToHexSecureMemory identitySeedHex( identitySeed );
		auto identitySeedHexBytes = identitySeedHex.lock();

		std::cout << "Next you should name your identity." << std::endl;
		std::cout << "The name is only visible to you but will make it easier for you if you" << std::endl;
		std::cout << "decide to have multiple identities later. Examples: Personal, Work." << std::endl << std::endl;

		std::cout << "Enter identity name: ";

		std::string identityName;
		std::getline( std::cin, identityName );

		FillBufferWithRandomBytes( nonce );

		if( !WriteSecureMemoryToFile( identityPath + "nonce", nonce ) )
		{
			return 4;
		}
		
		//TODO: How do we stop Json::Value leaving junk in memory? Custom allocator?

		identities[ identityName ] = (const char*)identitySeedHexBytes;
		root[ "identities" ] = identities;

		Json::FastWriter writer;
		std::string identityJSON = writer.write( root );

		SecureMemoryBase cipherText;
		if( !EncryptAuthenticated( identityJSON, passwordKey, nonce, cipherText ) )
		{
			std::cerr << "Unable to encrypt identity." << std::endl;
			return 4;
		}

		//Erase the plain text from memory, we don't need it again
		sodium_memzero( (void* const)identityJSON.c_str(), identityJSON.length() );

		if( !WriteSecureMemoryToFile( identityPath + "confidant", cipherText ) )
		{
			return 4;
		}

		if( !WriteSecureMemoryToFile( identityPath + "salt", passwordSalt ) )
		{
			return 4;
		}

		std::cout << "Identity successfully written to disc." << std::endl;
	}

	auto children = identities.getMemberNames();
	
	int identityChoice = -1;
	if( children.size() == 0 )
	{
		std::cerr << "No valid identity found." << std::endl;
		return 5;
	}
	else if( children.size() == 1 )
	{
		identityChoice = 0;
	}
	else
	{
		while( true )
		{
			std::cout << "Please pick the identity number you would like to act as:" << std::endl;

			size_t identityCount = children.size();
			for( size_t identity = 0; identity < identityCount; identity++ )
			{
				std::cout << "[" << identity << "] " << children[ identity ];
			}

			std::cout << "Identity: ";

			size_t identityChoice = (size_t)-1;
			std::cin >> identityChoice;
			
			if( identityChoice < 0 || identityChoice >= children.size() )
			{
				std::cerr << identityChoice << " is not a valid idenitity." << std::endl;
			}
			else
			{
				break;
			}
		}
	}

	if( identityChoice == -1 )
	{
		return 6;
	}

	std::string identitySeedHex = identities[ children[ identityChoice ] ].asString();
	currentIdentity = children[ identityChoice ];

	//Save the identity seed for later
	auto currentIdentitySeedBytes = currentIdentitySeed.lock( SecureMemoryBase::Write );

	size_t bytesWritten = 0;
	if( sodium_hex2bin( currentIdentitySeedBytes, currentIdentitySeed.getSize(), identitySeedHex.c_str(), identitySeedHex.size(), nullptr, &bytesWritten, nullptr ) != 0 || bytesWritten != currentIdentitySeed.getSize() )
	{
		std::cerr << "Failed to decode identity." << std::endl;
		return 8;
	}

	SigningPrivateKeySecureMemory currentSigningPrivateKey;
	SigningPublicKeySecureMemory currentSigningPublicKey;

	EncryptingPrivateKeySecureMemory currentEncryptingPrivateKey;
	EncryptingPublicKeySecureMemory currentEncryptingPublicKey;

	if( !GenerateSigningKeyPairFromSeed( currentSigningPrivateKey, currentSigningPublicKey, currentIdentitySeed ) )
	{
		std::cerr << "Failed to decode identity." << std::endl;
		return 8;
	}

	if( !GenerateEncryptingKeyPairFromSeed( currentEncryptingPrivateKey, currentEncryptingPublicKey, currentIdentitySeed ) )
	{
		std::cerr << "Failed to decode identity." << std::endl;
		return 8;
	}

	SecureMemoryBase b64PublicSigningKey;
	SecureMemoryBase b64PublicEncryptingKey;

	base64_encode( b64PublicSigningKey, currentSigningPublicKey );
	base64_encode( b64PublicEncryptingKey, currentEncryptingPublicKey );

	auto signPK = b64PublicSigningKey.lock( SecureMemoryBase::Write );
	auto encryptPK = b64PublicEncryptingKey.lock( SecureMemoryBase::Write );

	{
		char* signPKEnd = signPK;
		signPKEnd = &signPKEnd[ b64PublicSigningKey.getSize() - 1 ];
		while( *signPKEnd == '\0' || *signPKEnd == '=' ) signPKEnd--;
		*(signPKEnd+1) = '\0';
	}

	{
		char* encryptPKEnd = encryptPK;
		encryptPKEnd = &encryptPKEnd[ b64PublicEncryptingKey.getSize() - 1 ];
		while( *encryptPKEnd == '\0' || *encryptPKEnd == '=' ) encryptPKEnd--;
		*(encryptPKEnd+1) = '\0';
	}

	std::cout << currentIdentity << " public message key: " << (const char*)signPK << std::endl;
	std::cout << currentIdentity << " public signing key: " << (const char*)encryptPK << std::endl;

	bool connected = false;
	std::string serverName;

	while( true )
	{
		std::string tCommand;
		std::cout << "[" << currentIdentity << "]# ";
		std::getline( std::cin, tCommand );

		if( tCommand.compare( "quit" ) == 0 )
		{
			break;
		}
		else if( tCommand.compare( "connect" ) == 0 )
		{
			std::cout << "Enter server name: ";
			std::getline( std::cin, serverName );

			Json::Value query;
			query[ "publicKey" ] = (const char*)encryptPK;

			Json::Value sessionInfo;

			if( PostJsonQueryWithErrorHandling( false, serverName, "/authenticate/begin", query, sessionInfo ) )
			{
				std::cout << "Connected to server." << std::endl;
				connected = true;
			}

			SecureMemoryBase encryptedNonce;
			if( !JsonStringValueToBase64DecodedSecureMemory( sessionInfo, "encryptedNonce", encryptedNonce ) )
			{
				std::cerr << "Server did not send an encrypted nonce in a format the client can read." << std::endl;
				return 500;
			}

			NonceSecureMemory blockNonce;
			if( !JsonStringValueToBase64DecodedSecureMemory( sessionInfo, "blockNonce", blockNonce ) )
			{
				std::cerr << "Server did not send a block nonce in a format the client can read." << std::endl;
				return 500;
			}


			//TODO: Its expecting a seed block, that's surely wrong?

			SecureMemoryBase nonce;
			if( !DecryptAuthenticated( encryptedNonce, currentEncryptingPrivateKey, blockNonce, nonce ) )
			{
				std::cerr << "Unable to decrypt identity file." << std::endl;
				return 4;
			}


		}
		else if( tCommand.compare( "register" ) == 0 )
		{
			/*if( connected )
			{
				SecureMemory<4096> username;
				SecureMemory<4096> password;
				SecureMemory<4096> passwordAgain;
	
				std::cout << "Enter username: ";
				SecureReadConsole( username, true );

				std::cout << "Enter password (hidden): ";
				SecureReadConsole( password, false );

				std::cout << "Re-enter password (hidden): ";
				SecureReadConsole( passwordAgain, false );

				SeedKeySecureMemory seedKey;

				SigningPrivateKeySecureMemory privateKey;
				SigningPublicKeySecureMemory publicKey;
				SaltSecureMemory salt;

				auto saltBytes = salt.lock( SecureMemoryBase::Write );

				//sodium_hex2bin( saltBytes, salt.getSize(), 

				//saltBytes

				if( !GenerateKeyPairSeedFromPassword( seedKey, salt, password ) )
				{
					std::cerr << "Unable to generate credentials seed." << std::endl;
					return 2;
				}

				if( !GenerateSigningKeyPairFromSeed( privateKey, publicKey, seedKey ) )
				{
					std::cerr << "Unable to generate credentials keypair." << std::endl;
					return 2;
				}

				//Test code to verify signature can be verified.
				{
					const char* testString = "hello world";
					size_t messageLength = strlen(testString);

					SecureMemory<> signedMessage( crypto_sign_BYTES + messageLength );
					auto signedMessageBytes = signedMessage.lock( SecureMemoryBase::Write );

					auto publicKeyBytes = publicKey.lock();
					auto privateKeyBytes = privateKey.lock();

					//Create a signed message
					unsigned long long signedMessageLength = 0;
					if( crypto_sign( signedMessageBytes, &signedMessageLength, (unsigned char*)testString, messageLength, privateKeyBytes ) != 0 )
					{
						std::cerr << "Unable to sign message." << std::endl;
						return 2;
					}

					//Verify the signed message and extract the original message
					unsigned long long unsignedMessageLength = 0;
					SecureMemory<> unsignedMessage( messageLength );
					auto unsignedMessageBytes = unsignedMessage.lock( SecureMemoryBase::Write );

					if( crypto_sign_open( unsignedMessageBytes, &unsignedMessageLength, signedMessageBytes, signedMessageLength, publicKeyBytes ) != 0 )
					{
						std::cerr << "Message forged." << std::endl;
						return 2;
					}

					if( memcmp( testString, unsignedMessageBytes, messageLength ) != 0 )
					{
						std::cerr << "Unsigned message does not match original." << std::endl;
						return 2;
					}
					else
					{
						std::cout << "Keys successfully verified." << std::endl;
					}
				}
			}
			else
			{
				std::cerr << "Not connected to a server." << std::endl;
			}*/
		}
	}

	return 0;
}

int main( int argc, char* argv[] )
{
	return mainInternal( argc, argv );
}