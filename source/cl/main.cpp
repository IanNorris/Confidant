
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
				SecureMemory<char> tUsername( 4096 );
				SecureMemory<char> tPassword( 4096 );
				SecureMemory<char> tPasswordAgain( 4096 );
	
				std::cout << "Enter username: ";
				SecureReadConsole( tUsername, true );

				std::cout << "Enter password (hidden): ";
				SecureReadConsole( tPassword, false );

				std::cout << "Re-enter password (hidden): ";
				SecureReadConsole( tPasswordAgain, false );
			}
			else
			{
				std::cerr << "Not connected to a server." << std::endl;
			}
		}
	}

	return 0;
}
