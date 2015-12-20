#include <confidant/common/secure_memory.h>

SecureMemoryBase::SecureMemoryBase( size_t allocationSize )
: m_allocation( nullptr )
, m_allocationSize( allocationSize )
, m_readLocks( 0 )
, m_writeLocks( 0 )
{
	m_allocation = sodium_malloc( allocationSize );

	zeroMemory();
}

SecureMemoryBase::~SecureMemoryBase()
{
	sodium_free( m_allocation );
	m_allocation = nullptr;
}

void SecureMemoryBase::zeroMemory()
{
	lockAccess( Write );

	sodium_memzero( m_allocation, m_allocationSize );

	unlockAccess( Write );
}

void SecureMemoryBase::disableAccess() const
{
#if !defined( DISABLE_SECURE_MEMORY_PAGE_PROTECTION )
	sodium_mprotect_noaccess( m_allocation );
#endif
}

void SecureMemoryBase::enableReadAccess() const
{
#if !defined( DISABLE_SECURE_MEMORY_PAGE_PROTECTION )
	sodium_mprotect_readonly( m_allocation );
#endif
}

void SecureMemoryBase::enableReadWriteAccess() const
{
#if !defined( DISABLE_SECURE_MEMORY_PAGE_PROTECTION )
	sodium_mprotect_readwrite( m_allocation );
#endif
}

void SecureMemoryBase::lockAccess( unsigned int permissions ) const
{
	bool isReadEnabled = m_readLocks > 0;
	bool isWriteEnabled = m_writeLocks > 0;

	bool wantReadEnabled = (permissions & Read) != 0;
	bool wantWriteEnabled = (permissions & Write) != 0;

	int readLockInc = wantReadEnabled ? 1 : 0;
	int writeLockInc = wantWriteEnabled ? 1 : 0;

	m_readLocks += readLockInc;
	m_writeLocks += writeLockInc;

	if( wantWriteEnabled && !isWriteEnabled )
	{
		enableReadWriteAccess();
	}
	else if( wantReadEnabled && !isReadEnabled )
	{
		enableReadAccess();
	}
}

void SecureMemoryBase::unlockAccess( unsigned int permissions ) const
{
	bool isReadAt1 = m_readLocks == 1;
	bool isWriteAt1 = m_writeLocks == 1;

	bool wantReadDisabled = (permissions & Read) != 0;
	bool wantWriteDisabled = (permissions & Write) != 0;

	int readLockDec = wantReadDisabled ? 1 : 0;
	int writeLockDec = wantWriteDisabled ? 1 : 0;

	m_readLocks -= readLockDec;
	m_writeLocks -= writeLockDec;

	//Something needs to be changed
	if( isWriteAt1 || isReadAt1 )
	{
		if( m_writeLocks )
		{
			//Do nothing, still locked
		}
		else if( m_readLocks )
		{
			enableReadAccess();
		}
		else
		{
			disableAccess();
		}
	}
}
