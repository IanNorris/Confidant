#pragma once

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

	class SecureMemoryLock
	{
	public:

		SecureMemoryLock( const SecureMemoryLock& lock )
		: m_locked( lock.m_locked )
		, m_permissions( lock.m_permissions )
		{
			m_locked->lockAccess( m_permissions );
		}

		SecureMemoryLock( SecureMemoryBase* bufferToLock, unsigned int permissions )
		: m_locked( bufferToLock )
		, m_permissions( permissions )
		{
			m_locked->lockAccess( m_permissions );
		}

		~SecureMemoryLock()
		{
			m_locked->unlockAccess( m_permissions );
		}

		//! \brief Get a pointer to the actual buffer.
		//! \return A pointer to the buffer
		operator void*()
		{
			return m_locked->get();
		}

		//! \brief Get a pointer to the actual buffer.
		//! \return A pointer to the buffer
		operator char*()
		{
			return (char*)m_locked->get();
		}

		//! \brief Get a pointer to the actual buffer.
		//! \return A pointer to the buffer
		operator const char*()
		{
			return (const char*)m_locked->get();
		}

		//! \brief Get a pointer to the actual buffer.
		//! \return A pointer to the buffer
		operator unsigned char*()
		{
			return (unsigned char*)m_locked->get();
		}

		//! \brief Get a pointer to the actual buffer.
		//! \return A pointer to the buffer
		operator const unsigned char*()
		{
			return (const unsigned char*)m_locked->get();
		}

	private:
		
		SecureMemoryBase*	m_locked;
		unsigned int		m_permissions;
	};

	class SecureMemoryLockConst
	{
	public:

		SecureMemoryLockConst( const SecureMemoryLockConst& lock )
		: m_locked( lock.m_locked )
		{
			m_locked->lockAccess( Read );
		}

		SecureMemoryLockConst( const SecureMemoryBase* bufferToLock )
		: m_locked( bufferToLock )
		{
			m_locked->lockAccess( Read );
		}

		~SecureMemoryLockConst()
		{
			m_locked->unlockAccess( Read );
		}

		//! \brief Get a pointer to the actual buffer.
		//! \return A pointer to the buffer
		operator char*()
		{
			return (char*)m_locked->get();
		}

		//! \brief Get a pointer to the actual buffer.
		//! \return A pointer to the buffer
		operator const char*()
		{
			return (const char*)m_locked->get();
		}

		//! \brief Get a pointer to the actual buffer.
		//! \return A pointer to the buffer
		operator unsigned char*()
		{
			return (unsigned char*)m_locked->get();
		}

		//! \brief Get a pointer to the actual buffer.
		//! \return A pointer to the buffer
		operator const unsigned char*()
		{
			return (const unsigned char*)m_locked->get();
		}

	private:
		
		const SecureMemoryBase* const	m_locked;
		unsigned int					m_permissions;
	};

	//! \brief Constructor
	//! \param allocationSize [IN] - The size of the allocation to be made
	SecureMemoryBase( size_t allocationSize );

	//! \brief Destructor
	~SecureMemoryBase();

	//! \brief Get the size of the allocation
	//! \return The size of the allocation in bytes
	size_t getSize() const { return m_allocationSize; }

	//! \brief Securely erase the memory in the buffer
	void zeroMemory();

	//! \brief Create a locked object for access to the buffer
	//! \param permissions [IN] - The permissions you require. See SecureMemoryBase::EAccess
	//! \sa SecureMemoryBase::EAccess
	//! \return A scope lock object that grants access to the memory in the buffer
	SecureMemoryLock lock( unsigned int permissions )
	{
		SecureMemoryLock lockObject( this, permissions );

		return lockObject;
	}

	//! \brief Create a locked object for read only access to the buffer
	//! \sa SecureMemoryBase::EAccess
	//! \return A scope lock object that grants access to the memory in the buffer
	SecureMemoryLockConst lock() const
	{
		SecureMemoryLockConst lockObject( this );

		return lockObject;
	}

protected:

	//! \brief Get a pointer to the actual buffer.
	//! \return A pointer to the buffer
	void* get() { return m_allocation; }

	//! \brief Get a pointer to the actual buffer.
	//! \return A pointer to the buffer
	const void* get() const { return m_allocation; }

	//! \brief Lock the memory for access with the requested permissions
	//! \param permissions [IN] - The permissions to request (see EAccess)
	//! \sa EAccess
	void lockAccess( unsigned int permissions ) const;

	//! \brief Unlock the memory for access with the requested permissions
	//! \param permissions [IN] - The permissions to relinquish (see EAccess)
	//! \sa EAccess
	void unlockAccess( unsigned int permissions ) const;

private:

	//! \brief Completely disable access to the memory until the state is changed again
	void disableAccess() const;

	//! \brief Enable read only access
	void enableReadAccess() const;
	
	//! \brief Enable read/write access
	void enableReadWriteAccess() const;

	void*			m_allocation;
	size_t			m_allocationSize;
	
	mutable size_t	m_readLocks;
	mutable size_t	m_writeLocks;
};

template< size_t Length = 0 >
class SecureMemory : public SecureMemoryBase
{
public:
	
	//! \brief Constructor
	//! \param allocationSize [IN] - The size of the allocation to be made
	SecureMemory( size_t allocationSize = Length )
	: SecureMemoryBase( allocationSize )
	{}
};
