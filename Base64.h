#ifndef LIBWEBSOCKETS_BASE64_H
#define LIBWEBSOCKETS_BASE64_H


#include <string>

std::string Base64Encode(unsigned char const* , unsigned int len);
std::string Base64Decode(std::string const& s);


#endif //LIBWEBSOCKETS_BASE64_H
