
#include <confidant/authentication.h>

#include "input.h"
#include <string>

#include "restclient-cpp/restclient.h"
#include "restclient-cpp/meta.h"
#include <json/json.h>
#include <string.h> // string was already #include'd above, string.h is required for strlen and memcmp on linux

#if defined( _MSC_VER )
	#pragma comment(lib, "libconfidant.lib")
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

int main()
{
	bool connected = false;
	std::string serverName;
	Json::Value connectionSettings;

	if( sodium_init() == -1 )
	{
		std::cerr << "Failed to initialise libsodium." << std::endl;

		return 1;
	}

	std::cout << "--Confidant Client V0.01--" << std::endl;

	while( true )
	{
		std::string tCommand;
		std::cout << "# ";
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
			if( connected )
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
			}
		}
	}

	return 0;
}
