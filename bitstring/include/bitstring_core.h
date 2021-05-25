#pragma once

// FILE: bitstring.h
// PURPOSE: implementation of the bitstring class, which is a
//          "bucket of bits" storing an array of bits,
//          up to a compile-time-defined maximum number of bits.

#include <string.h> // memset
#include <utility> // std::move
#include <type_traits> // std::conditional


namespace lxutil {


template<unsigned int _InitialBitCapacity, 
        bool _AllowExpand,
        bool _AutoZeroInit,
        typename _StorageType > class bitstring {

public:
    using BlockType = typename _StorageType::value_type;
public:
    bitstring( ): usedBlocks(0), totalUsedBits(0), usedBits(bitsInBlock) {
        if( _AllowExpand ) {
            resizer.reserve( storage, intialBlocks );
        } else {
            // final size decided early
            resizer.resize( storage, intialBlocks );
        }
    }

    // default assignment operator ok
    // default destructor ok
    // add special constructor - only useful for dynamic size
    bitstring( unsigned int rtInitBits ): usedBlocks(0), totalUsedBits(0), usedBits(bitsInBlock) {
        unsigned int iblocks = (rtInitBits - bitsInBlock - 1 )  / bitsInBlock;
        if( _AllowExpand ) {
            resizer.reserve( storage, iblocks );
        } else {
            // final size decided early
            resizer.resize( storage, iblocks );
        }
    };

    // copy constructor - plain vanilla
    bitstring(const bitstring &from ):
            usedBlocks(from.usedBlocks),
            usedBits(from.usedBits),
            totalUsedBits(from.totalUsedBits),
            storage(from.storage) {
    }

    // copy constructor - plain vanilla
    bitstring& operator=(const bitstring &from ) {
        usedBlocks = from.usedBlocks;
        usedBits = from.usedBits;
        totalUsedBits = from.totalUsedBits;
        storage = from.storage;
        return (*this);
    }


    // move constructor is just a little special
    bitstring(bitstring &&from ):
            usedBlocks(std::move(from.usedBlocks)),
            usedBits(from.usedBits),
            totalUsedBits(std::move(from.totalUsedBits)),
            storage(std::move(from.storage)) { // move constructor
        // empty state is special
        from.usedBits = bitsInBlock;
    }

    bool addBits( BlockType value, unsigned int nBits ) {
        if( (_AllowExpand) || ((totalUsedBits + nBits) <= capacityInBits()) ) {
            unsigned int remainingBits = (bitsInBlock - usedBits);
            
            // force storage to be what we need
            totalUsedBits += nBits;

            if( nBits < bitsInBlock ) {
                value &= ( (1 << nBits) - 1); // cut what we can't use
            }
            if( remainingBits >= nBits ) {
                // no extra blocks will be needed
                storage[usedBlocks - 1] <<= nBits;
                storage[usedBlocks - 1] |= value;
                usedBits += nBits; // remove what we used
                return true;
            } 

            // we will need more storage for sure
            ++usedBlocks;
            resizer.resize( storage, usedBlocks );

            if(remainingBits > 0) {
                unsigned int spilledBits = (nBits - remainingBits);
                storage[usedBlocks - 2] <<= remainingBits;
                storage[usedBlocks - 2] |= (value >> spilledBits); //  top
                storage[usedBlocks - 1] = (value & ((1 << spilledBits) - 1));  // bottom
                usedBits = spilledBits;
            } else {
                // no bits remaining in current int, get a new one
                storage[usedBlocks - 1] = value;
                usedBits = nBits;
            }
            return true;
        }
        return false;
    }
            
    BlockType read( unsigned int startingBit, unsigned int nBits ) {
        unsigned int startingBlock = startingBit / bitsInBlock;
        unsigned int firstBitInBlock = startingBit % bitsInBlock;
        unsigned int bitsPopulated = ( (startingBlock + 1) < sizeInBlocks() ) ?
                                        bitsInBlock : usedBits;
        unsigned int reverseStartBit = bitsPopulated - firstBitInBlock;
        BlockType p1;
        if( reverseStartBit >= bitsPopulated ) {
            // no need to mask anything, take all of it
            p1 = storage[startingBlock];
        } else {
            unsigned int mask1 = ( ( 1 << reverseStartBit ) - 1 );
            p1 = storage[startingBlock] & mask1;
        }

        if( reverseStartBit >= nBits ) {
            // one block has all necessary bits
            return p1 >> (reverseStartBit - nBits);
        }
        
        // must combine
        unsigned int bitsFromNextBlock = (nBits - reverseStartBit);
        p1 <<= bitsFromNextBlock; // make room for 2nd part

        // recompute info for next block
        ++startingBlock;
        bitsPopulated = ( (startingBlock + 1) < sizeInBlocks() ) ?
                                        bitsInBlock : usedBits;
        return p1 | 
                ( storage[startingBlock] >> (bitsPopulated - bitsFromNextBlock) );
    }

    bool resize( unsigned int newTotalBits ) {
        if( newTotalBits > totalUsedBits ) {
            unsigned int addedBits = newTotalBits - totalUsedBits;
            unsigned int newnblocks = (newTotalBits + bitsInBlock - 1 ) / bitsInBlock; // ceil
            if( newnblocks > storage.size() ) {
                if( !(_AllowExpand) ) {
                    return false;
                }
                resizer.resize( storage, newnblocks );
            }

            totalUsedBits = newTotalBits;

            unsigned int remainingBits = (bitsInBlock - usedBits);
            if( remainingBits >= addedBits ) {
                // no extra blocks will be needed
                storage[usedBlocks - 1] <<= addedBits;
                usedBits += addedBits; // remove what we used
                return true;
            }

            if( remainingBits > 0 ) {
                // take all we can from the last block
                // (this will not run if we are in empty state,
                // because there will be no remainingBits)
                storage[usedBlocks - 1] <<= remainingBits; // shifting is important
                                                        // effectively adds zeroes
            }

            usedBits = totalUsedBits % bitsInBlock;
            if( usedBits == 0 ) {
                usedBits = bitsInBlock; // meaning last block is full
            }
            if( _AutoZeroInit ) {
                // easy way
                usedBlocks = newnblocks;
            } else {
                // hard way
                while( usedBlocks < newnblocks ) {
                    storage[usedBlocks++] = 0;
                }
            }
        } else if( newTotalBits < totalUsedBits ) {
            unsigned int removedBits = totalUsedBits - newTotalBits;
            unsigned int bitsLastBlock = ( (usedBlocks > 0) ? usedBits : 0 );
            if( removedBits < bitsLastBlock ) {
                storage[usedBlocks - 1] >>= removedBits;
                usedBits -= removedBits;
            } else if( removedBits == bitsLastBlock ) {
                // just remove the last block entirely
                storage[usedBlocks - 1] = 0;
                --usedBlocks;
                usedBits = bitsInBlock; // prior block full, or empty state
                resizer.resize( storage, usedBlocks );
            } else {
                // remove a block or more
                usedBlocks = (newTotalBits + bitsInBlock - 1 ) / bitsInBlock;
                removedBits -= bitsLastBlock;
                removedBits = removedBits % bitsInBlock;

                if( usedBlocks > 0 ) {
                    // this is a full block we are changing
                    storage[usedBlocks - 1] >>= removedBits;
                }

                usedBits = bitsInBlock - removedBits;
                if( usedBits == 0 ) {
                    usedBits = bitsInBlock; // meaning last block is full
                }
                resizer.resize( storage, usedBlocks );
            }

            totalUsedBits = newTotalBits;
        } // else no change

        return true;
    }

    bool write( BlockType value, unsigned int startingBit, unsigned int nBits ) {
        // cap it
        if( nBits > bitsInBlock ) {
            nBits = bitsInBlock;
        }

        if( (startingBit + nBits ) > totalUsedBits ) {
            if( !resize(startingBit + nBits) ) {
                return false;
            }
        }


        unsigned int startingBlock = startingBit / bitsInBlock;
        unsigned int firstBitInBlock = startingBit % bitsInBlock;
        unsigned int bitsPopulated = ( (startingBlock + 1) < sizeInBlocks() ) ?
                                        bitsInBlock : usedBits;
        unsigned int reverseStartBit = bitsPopulated - firstBitInBlock;

        if( reverseStartBit >= nBits ) {
            BlockType pmask;
            if( nBits >= bitsInBlock ) {
                pmask = ~0;
            } else {
                pmask = ( (1 << nBits) - 1 );
                pmask <<= (reverseStartBit - nBits);
            }

            storage[startingBlock] &=  (~pmask); // zero all bits we will set
            storage[startingBlock] |=  ( (value << (reverseStartBit - nBits) ) & pmask );
            return true;
        }

        unsigned int bitsBottom = nBits - reverseStartBit;
        BlockType topmask = ( (1 << reverseStartBit) - 1 );
        storage[startingBlock] &= (~topmask); // clear bottom bits at the top
        storage[startingBlock] |= (value >> bitsBottom );

        // recompute info for next block
        ++startingBlock;
        bitsPopulated = ( (startingBlock + 1) < sizeInBlocks() ) ?
                                        bitsInBlock : usedBits;

        BlockType bottomMask = ( (1 << bitsBottom) - 1 );
        unsigned int bitsBehindBottom = (bitsPopulated - bitsBottom);
        BlockType containerBottomMask = (bottomMask << bitsBehindBottom);

        storage[startingBlock] &= (~containerBottomMask); // clear the top of it
        storage[startingBlock] |= ( (value & bottomMask) << bitsBehindBottom );

        return true;
    }




    bool operator>(const bitstring &comp) const {
        return compareWith(comp) == 1;
    }
    bool operator>=(const bitstring &comp) const {
        return !( (*this) < comp);
    }
    bool operator<=(const bitstring &comp) const {
        return !( (*this) > comp);
    }
    bool operator<(const bitstring &comp) const {
        return compareWith(comp) == -1;
    }
    bool operator==(const bitstring &comp) const {
        if( comp.totalUsedBits != totalUsedBits ) {
            return false;
        }
        return compareWith(comp) == 0;
    }

    bitstring &operator &=( const bitstring &rightop ) {
        andWith( rightop );
        return (*this);
    }

    bitstring &operator |=( const bitstring &rightop ) {
        orWith( rightop );
        return (*this);
    }


    unsigned int sizeInBits() const {
        return totalUsedBits;
    }
    unsigned int sizeInBlocks() const {
        return usedBlocks;
    }
    unsigned int capacityInBlocks() const {
        return resizer.capacity(storage);
    }
    unsigned int capacityInBits() const {
        return capacityInBlocks() * bitsInBlock;
    }
private:
    // -1:  this is less than comp
    // 1:  this is greater than comp
    // 0: both are equal
    int compareWith( const bitstring &comp ) const {
        unsigned int i = 0;
        if( usedBlocks < 1 ) {
            // no data here.  if the other side has any data, we are less
            return (comp.usedBlocks > 0) ? -1 : 0;
        }
        if( comp.usedBlocks < 1 ) {
            // we have data, the other side doesn't. other side is less.
            return 1;
        }
        
        // both sides have data            
        // get the shortest one
        unsigned int minBlocks = ( usedBlocks < comp.usedBlocks ? 
                                    usedBlocks : comp.usedBlocks );
        unsigned int localUsedBits = usedBits;
        unsigned int compUsedBits = comp.usedBits;
        while( (i + 1) < minBlocks) {
            if( storage[i] < comp.storage[i] ) {
                // look no further
                return -1;
            }
            if( storage[i] > comp.storage[i] ) {
                // look no further
                return 1;
            }
            ++i;
        }
        
        // getting here means at least one of them
        // is pointing at the LAST integer, which MAY be 
        // incomplete, and we didnt look at it yet
        if( usedBlocks > minBlocks ) {
            // local has more blocks than comp, the current block 
            // is complete
            if( comp.usedBits >= bitsInBlock ) {
                return (storage[i] < comp.storage[i]) ? -1 : 1;
                        // equal means 1 ("this" is longer)
            }
            localUsedBits = bitsInBlock;
        } else if( comp.usedBlocks > minBlocks ) {
            // other side has more blocks
            if( usedBits >= bitsInBlock ) {
                return (comp.storage[i] < storage[i] ) ? 1 : -1;
                        // equal means -1 ("this" is shorter)
            }
            compUsedBits = bitsInBlock;
        }
        
        // they are the same length (in ints, 
        //  so the comparison is down to bits)
        
        if( compUsedBits > localUsedBits ) {
            BlockType comppartial = ( comp.storage[i] >>
                (compUsedBits - localUsedBits) ); // shift the excess part
            return (comppartial < storage[i]) ? 1 : -1;
                // equal means -1, "this" is shorter                
        } else if(localUsedBits > compUsedBits) {
            // we are longer
            BlockType localpartial =  ( storage[i] >>
                    (localUsedBits - compUsedBits) ); // shift the excess part
            return localpartial < comp.storage[i] ? -1 : 1;      
                // equal means 1, "this" is longer
        }
        
        // exact same number of bits
        if( storage[i] < comp.storage[i] ) return -1;
        if( storage[i] > comp.storage[i] ) return 1;

        // exacly the same contents!
        return 0;
    }


    void andWith( const bitstring &comp )  {
        unsigned int i = 0;
        if( (usedBlocks < 1) || (comp.usedBlocks < 1) ) {
            return;
        }
        
        // both sides have data            
        // get the shortest one
        unsigned int minBlocks = ( usedBlocks < comp.usedBlocks ? 
                                    usedBlocks : comp.usedBlocks );
        unsigned int localUsedBits = usedBits;
        unsigned int compUsedBits = comp.usedBits;
        while( (i + 1) < minBlocks) {
            storage[i] &= comp.storage[i];
            ++i;
        }
        
        if( usedBlocks > minBlocks ) {
            // local has more blocks than comp, the current block 
            // is complete
            if( comp.usedBits >= bitsInBlock ) {
                storage[i] &= comp.storage[i];
                return;
            }
            localUsedBits = bitsInBlock;
        } else if( comp.usedBlocks > minBlocks ) {
            // other side has more blocks
            if( usedBits >= bitsInBlock ) {
                storage[i] &= comp.storage[i];
                return;
            }
            compUsedBits = bitsInBlock;
        }
        
        // they are the same length (in ints, 
        //  so the comparison is down to bits)
        BlockType comppartial = comp.storage[i];

        if( compUsedBits > localUsedBits ) {
            comppartial >>= (compUsedBits - localUsedBits); // shift the excess part
        } else if(localUsedBits > compUsedBits) {
            unsigned int trailing = (localUsedBits - compUsedBits);
            comppartial <<= trailing; // shift up to align with local
            comppartial |= ( (1 << trailing) - 1 ); // fill with 1 what we shifted
        }

        // fill the top part with 1s
        BlockType compmask;
        if( localUsedBits < bitsInBlock ) {
            compmask = (~0); // NULL and mask - all 1's
            compmask <<= localUsedBits; // create a block of trailing zeroes
            compmask |= comppartial; // set that block to comppartial
        } else {
            compmask = comppartial;
        }

        storage[i] &= compmask;
    }



    void orWith( const bitstring &comp )  {
        unsigned int i = 0;
        if( (usedBlocks < 1) || (comp.usedBlocks < 1) ) {
            return;
        }
        
        // both sides have data            
        // get the shortest one
        unsigned int minBlocks = ( usedBlocks < comp.usedBlocks ? 
                                    usedBlocks : comp.usedBlocks );
        unsigned int localUsedBits = usedBits;
        unsigned int compUsedBits = comp.usedBits;
        while( (i + 1) < minBlocks) {
            storage[i] |= comp.storage[i];
            ++i;
        }
        
        if( usedBlocks > minBlocks ) {
            // local has more blocks than comp, the current block 
            // is complete
            if( comp.usedBits >= bitsInBlock ) {
                storage[i] |= comp.storage[i];
                return;
            }
            localUsedBits = bitsInBlock;
        } else if( comp.usedBlocks > minBlocks ) {
            // other side has more blocks
            if( usedBits >= bitsInBlock ) {
                storage[i] |= comp.storage[i];
                return;
            }
            compUsedBits = bitsInBlock;
        }
        
        // they are the same length (in ints, 
        //  so the comparison is down to bits)
        BlockType comppartial = comp.storage[i];

        if( compUsedBits > localUsedBits ) {
            comppartial >>= (compUsedBits - localUsedBits); // shift the excess part
        } else if(localUsedBits > compUsedBits) {
            unsigned int trailing = (localUsedBits - compUsedBits);
            comppartial <<= trailing; // shift up to align with local
        }

        storage[i] |= comppartial;
    }


private: // ancillary types to abstract container differences
    template< typename _ContainerType> class VectorSizeManager {
    public:
        static void reserve( _ContainerType &c, size_t n ) {
            c.reserve(n);
        }
        static void resize( _ContainerType &c, size_t n ) {
            c.resize(n);
        }
        static size_t capacity( const _ContainerType &c) {
            return c.capacity();
        }
    };

    template<typename _ContainerType> class ArraySizeManager {
    public:
        static void reserve( _ContainerType &c, size_t n ) {
            // no support
        }
        static void resize( _ContainerType &c, size_t n ) {
            // no support
        }
        static size_t capacity( const _ContainerType &c)  {
            return c.size();
        }
    };
    using Resizer = typename std::conditional<_AllowExpand,
                             VectorSizeManager<_StorageType>,
                             ArraySizeManager<_StorageType> >::type;
private:
    Resizer resizer;

private:
    static constexpr unsigned int bitsInBlock = (sizeof(BlockType) * 8);
    static constexpr unsigned int intialBlocks = (_InitialBitCapacity - bitsInBlock - 1 )  / bitsInBlock;
    _StorageType storage;
    unsigned int usedBlocks;
    unsigned int usedBits; // on the last block
    unsigned int totalUsedBits;
};



} // namespace lxutil