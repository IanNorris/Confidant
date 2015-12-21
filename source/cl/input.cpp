#include "input.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

//http://stackoverflow.com/questions/1413445/read-a-password-from-stdcin
void SetStdinEcho( bool enable )
{
#ifdef WIN32
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
	DWORD mode;
	GetConsoleMode(hStdin, &mode);

	if( !enable )
		mode &= ~ENABLE_ECHO_INPUT;
	else
		mode |= ENABLE_ECHO_INPUT;

	SetConsoleMode(hStdin, mode );

#else
	struct termios tty;
	tcgetattr(STDIN_FILENO, &tty);
	if( !enable )
		tty.c_lflag &= ~ECHO;
	else
		tty.c_lflag |= ECHO;

	(void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}

bool SecureReadConsole( SecureMemoryBase& targetMemory, bool printToScreen )
{
	//We need at least two characters including null terminator
	size_t bufferPos = 0;
	size_t bufferSize = targetMemory.getSize();
	if( bufferSize <= 1 )
	{
		return false;
	}

	if( !printToScreen )
	{
		SetStdinEcho( false );
	}

	//NOTE: This bit is NOT secure. The contents are not directly written to the secure
	//memory but to a buffer outside of our control. Short of writing our own input
	//system I'm not sure how we can fix this.
	{
		auto targetMemoryBuffer = targetMemory.lock( SecureMemoryBase::Write );
		targetMemory.zeroMemory();
		std::cin.getline( (char*)targetMemoryBuffer, bufferSize );
	}

	if( !printToScreen )
	{
		SetStdinEcho( true );
		std::cout << std::endl;
	}

	return true;
}
