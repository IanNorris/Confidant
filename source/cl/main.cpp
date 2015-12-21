
#include <confidant/authentication.h>
#include <confidant/common/random.h>

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

int main( int argc, char* argv[] )
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

	std::string identityPathRoot = identityPath + "confidant";

	SecureMemoryBase identityFileMemory;

	std::ifstream identityFile( identityPathRoot, std::ifstream::binary );
	if( identityFile )
	{
		identityFile.seekg( 0, std::ios::end );
		fpos_t fileSize = identityFile.tellg().seekpos();
		identityFile.seekg( 0, std::ios::beg );

		identityFileMemory.allocate( (size_t)fileSize + 1 );
		auto identityFileMemoryBytes = identityFileMemory.lock( SecureMemoryBase::Write );
		identityFile.read( identityFileMemoryBytes, fileSize );
		((char*)identityFileMemoryBytes)[ fileSize ] = 0;

		SecureMemory<4096> password;
		std::cout << "Enter password (hidden): ";
		SecureReadConsole( password, false );
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

		SaltSecureMemory passwordSalt;
		FillBufferWithRandomBytes( passwordSalt );
		
		SeedKeySecureMemory passwordKey;
		auto passwordKeyBytes = passwordKey.lock();
		if( !GenerateKeyPairSeedFromPassword( passwordKey, passwordSalt, password ) )
		{
			std::cerr << "Unable to generate keypair." << std::endl;
			return 3;
		}

		ToHexSecureMemory passwordSaltHex( passwordSalt );
		auto passwordSaltHexBytes = passwordSaltHex.lock();

		//TODO: Write salt to disc, we'll need it later

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

		SecureMemory<crypto_secretbox_NONCEBYTES> nonce;
		auto nonceBytes = nonce.lock();
		FillBufferWithRandomBytes( nonce );
		
		//TODO: How do we stop Json::Value leaving junk in memory? Custom allocator?

		Json::Value root;
		Json::Value identities;
		identities[ identityName ] = (const char*)identitySeedHexBytes;
		root[ "identities" ] = identities;

		Json::FastWriter writer;
		std::string identityJSON = writer.write( root );

		SecureMemoryBase cipherTextFile( crypto_secretbox_MACBYTES + identityJSON.size() );
		auto cipherTextFileBytes = cipherTextFile.lock( SecureMemoryBase::Write );
		if( crypto_secretbox_easy( cipherTextFileBytes, (const unsigned char*)identityJSON.c_str(), identityJSON.size(), nonceBytes, passwordKeyBytes ) != 0 )
		{
			std::cerr << "Unable to encrypt identity." << std::endl;
			return 4;
		}

		sodium_memzero( (void* const)identityJSON.c_str(), identityJSON.length() );

		std::ofstream identityFileOut( identityPathRoot, std::ofstream::binary );

		if( !identityFileOut )
		{
			std::cerr << "Unable to write to file " << identityPath << "." << std::endl;
			return 4;
		}

		identityFileOut.write( cipherTextFileBytes, cipherTextFile.getSize() );
		identityFileOut.close();

		std::ofstream passwordSaltFileOut( identityPath + "salt", std::ofstream::binary );

		if( !passwordSaltFileOut )
		{
			std::cerr << "Unable to write to file " << identityPath << "salt." << std::endl;
			return 4;
		}

		passwordSaltFileOut.write( passwordSaltHexBytes, passwordSaltHex.getSize() );
		passwordSaltFileOut.close();

		std::cout << "Identity successfully written to disc." << std::endl;
	}

	std::string currentIdentity = "Personal";

	bool connected = false;
	std::string serverName;
	Json::Value connectionSettings;

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
			query[ "clientName" ] = "confidant-cl";

			if( PostJsonQueryWithErrorHandling( false, serverName, "/queryServer", query, connectionSettings ) )
			{
				std::cout << "Connected to server." << std::endl;
				connected = true;
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

				if( !GenerateKeyPairSeedFromCredentials( seedKey, salt, username, password ) )
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
