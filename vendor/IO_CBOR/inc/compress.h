#ifndef CBOR_COMPRESS_H
#define CBOR_COMPRESS_H

#include <iostream>

namespace cbor
{
    void compress(std::istream &is, std::ostream &os);
    void decompress(std::istream &is, std::ostream &os);
}

#endif