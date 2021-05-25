#pragma once

// FILE: dynamicbitstring.h
// PURPOSE: dynamic version of bitstring, which can vary in size
//          during runtime.

#include <bitstring_core.h>
#include <vector>

namespace lxutil {



template<typename _StorageType = std::vector<unsigned int> >
    using dynamicbitstring = 
    bitstring<0, true /*expandable*/, true /*auto-initialized*/,  _StorageType>;

} // namespace lxutil