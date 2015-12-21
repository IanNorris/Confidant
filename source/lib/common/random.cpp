#include <confidant/common/random.h>

void FillBufferWithRandomBytes( SecureMemoryBase& buffer )
{
	auto bufferBytes = buffer.lock( SecureMemoryBase::Write );

	randombytes_buf( bufferBytes, buffer.getSize() );
}