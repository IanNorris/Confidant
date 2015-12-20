#pragma 

#include <sodium.h>

//Define this if you need to be able to debug
//If this is not defined the memory will be
//unreadable in the debugger when the protected
//flag is set.
#if defined( _DEBUG )
	#define DISABLE_SECURE_MEMORY_PAGE_PROTECTION
#endif

class SecureMemoryBase
{
public:

	//! \brief The state of the memory in a SecureMemory buffer
	enum EAccess
	{
		Read		= 1 << 0,	//!< Memory is read only
		Write		= 1 << 1,	//!< Memory can be read and written
	};

	//! \brief Constructor
	//! \param allocationSize [IN] - The size of the allocation to be made
	SecureMemoryBase( size_t allocationSize );

	//! \brief Destructor
	~SecureMemoryBase();

	//! \brief Get the size of the allocation
	//! \return The size of the allocation in bytes
	size_t getSize() { return m_allocationSize; }

	//! \brief Securely erase the memory in the buffer
	void zeroMemory();

protected:

	//! \brief Get a pointer to the actual buffer.
	//! \return A pointer to the buffer
	void* get() { return m_allocation; }

	//! \brief Lock the memory for access with the requested permissions
	//! \param permissions [IN] - The permissions to request (see EAccess)
	//! \sa EAccess
	void lockAccess( unsigned int permissions );

	//! \brief Unlock the memory for access with the requested permissions
	//! \param permissions [IN] - The permissions to relinquish (see EAccess)
	//! \sa EAccess
	void unlockAccess( unsigned int permissions );

private:

	//! \brief Completely disable access to the memory until the state is changed again
	void disableAccess();

	//! \brief Enable read only access
	void enableReadAccess();
	
	//! \brief Enable read/write access
	void enableReadWriteAccess();

	void*	m_allocation;
	size_t	m_allocationSize;
	
	size_t	m_readLocks;
	size_t	m_writeLocks;
};

template< class T >
class SecureMemory : public SecureMemoryBase
{
public:

	class SecureMemoryLock
	{
	public:

		SecureMemoryLock( const SecureMemoryLock& lock )
		: m_locked( lock.m_locked )
		, m_permissions( lock.m_permissions )
		{
			m_locked.lockAccess( m_permissions );
		}

		SecureMemoryLock( SecureMemory<T>& bufferToLock, unsigned int permissions )
		: m_locked( bufferToLock )
		, m_permissions( permissions )
		{
			m_locked.lockAccess( m_permissions );
		}

		~SecureMemoryLock()
		{
			m_locked.unlockAccess( m_permissions );
		}

		//! \brief Get a pointer to the actual buffer.
		//! \return A pointer to the buffer
		operator T*()
		{
			return (T*)m_locked.get();
		}

	private:
		
		SecureMemory&	m_locked;
		unsigned int	m_permissions;
	};
	
	//! \brief Constructor
	//! \param allocationSize [IN] - The size of the allocation to be made
	SecureMemory( size_t allocationSize )
	: SecureMemoryBase( allocationSize )
	{}
	
	//! \brief Create a locked object for access to the buffer
	//! \param permissions [IN] - The permissions you require. See SecureMemoryBase::EAccess
	//! \sa SecureMemoryBase::EAccess
	//! \return A scope lock object that grants access to the memory in the buffer
	SecureMemoryLock lock( unsigned int permissions )
	{
		SecureMemoryLock lockObject( *this, permissions );

		return lockObject;
	}

private:

};
