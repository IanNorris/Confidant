/* 
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch



   NOTE: This version has been modified to meet the needs of the library developer.

*/

#include "confidant/common/base64.h"
#include <iostream>
#include <string>

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

void base64_encode( SecureMemoryBase& encodedOut, const SecureMemoryBase& memoryIn )
{
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  auto bytes_to_encode_obj = memoryIn.lock();
  const char* bytes_to_encode = bytes_to_encode_obj;
  size_t in_len = memoryIn.getSize();
  
  size_t output_size = ((in_len / 3) + 1) * 4 + 1;

  encodedOut.allocate( output_size );
  auto encodedOutBytes = encodedOut.lock( SecureMemoryBase::Write );
  char* encodedOutBytesPtr = encodedOutBytes;

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        *(encodedOutBytesPtr++) = base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      *(encodedOutBytesPtr++) = base64_chars[char_array_4[j]];

    while((i++ < 3))
      *(encodedOutBytesPtr++) = '=';

  }

  *(encodedOutBytesPtr) = '\0';
}

void base64_decode( SecureMemoryBase& decodedOut, const SecureMemoryBase& encodedIn )
{
  int in_len = encodedIn.getSize();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];

  auto encodedInBytes = encodedIn.lock();
  const char* encodedInBytesPtr = encodedInBytes;

  decodedOut.allocate( in_len * 3 / 4 );
  auto decodedOutBytes = decodedOut.lock( SecureMemoryBase::Write );
  char* decodedOutBytesPtr = decodedOutBytes;

  while (in_len-- && ( encodedInBytesPtr[in_] != '=') && is_base64(encodedInBytesPtr[in_])) {
    char_array_4[i++] = encodedInBytesPtr[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        *(decodedOutBytesPtr++) = char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) *(decodedOutBytesPtr++) = char_array_3[j];
  }
}
