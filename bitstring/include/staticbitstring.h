#pragma once

// FILE: staticbitstring.h
// PURPOSE: static version of bitstring, with std::array as storage

#include <bitstring_core.h>
#include <array>

namespace lxutil {

template< unsigned int _MaxBits, typename _BlockType = unsigned int> 
using BitSizedArray =
     std::array< _BlockType, (_MaxBits + sizeof(_BlockType) - 1 ) / sizeof(_BlockType)  >;


template<unsigned int _MaxBits, typename _StorageType = BitSizedArray<_MaxBits> >
    using staticbitstring = 
    bitstring<_MaxBits, false /*expandable*/, false /*auto-initialized*/,  _StorageType >;

} // namespace lxutil