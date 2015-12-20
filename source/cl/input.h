#pragma once

#include <iostream>
#include <confidant/common/secure_memory.h>

//! \brief Read the console into a secure memory block
//!
//! \param targetMemory [IN|OUT]	- Target memory that will receive the input
//! \param printToScreen [IN]		- Should the input be echoed to the console?
//! \return True if the input fit into the buffer, false if the input reached the buffer end
//! \sa SecureMemory
bool SecureReadConsole( SecureMemory<char>& targetMemory, bool printToScreen );
