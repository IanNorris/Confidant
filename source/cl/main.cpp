
#include <confidant/authentication.h>

#include "input.h"
#include <string>

#include "restclient-cpp/restclient.h"
#include "restclient-cpp/meta.h"
#include <json/json.h>
#include <string>

#if defined( _MSC_VER )
	#pragma comment(lib, "libconfidant.lib")
#endif

int main()
{
	bool connected = false;
	std::string serverName;

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

			std::string tTargetPath = "http://" + serverName + "/queryServer";

			Json::Value query;
			query[ "clientName" ] = "confidant-cl";

			Json::FastWriter writer;


			RestClient::response res = RestClient::post( tTargetPath, "text/json", writer.write( query ) );
			
			bool success = false;
			if( res.code == 200 )
			{
				Json::Reader reader;
				Json::Value responseValue;

				if( reader.parse( res.body, responseValue ) )
				{
					connected = true;
					std::cout << "Connected to server." << std::endl;
				}
				else
				{
					std::cerr << "Server did not respond correctly." << std::endl;
				}
			}
			else
			{
				std::cerr << "Server returned an error." << std::endl;
			}
		}
		else if( tCommand.compare( "register" ) == 0 )
		{
			if( connected )
			{
				SecureMemory<char> tUsername( 4096 );
				SecureMemory<char> tPassword( 4096 );
	
				std::cout << "Enter username: ";
				SecureReadConsole( tUsername, true );

				std::cout << "Enter password (hidden): ";
				SecureReadConsole( tPassword, false );
			}
			else
			{
				std::cerr << "Not connected to a server." << std::endl;
			}
		}
	}

	return 0;
}