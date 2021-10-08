<html>
<head>
<meta name="google-site-verification" content="vpnZZMkAK7Lv6js1xu1HDvgh-LL0LapwrMen5uk7doM" />
</head>
</html>

# bitstring

What: This class is a bitwise vector/container, that shares the same spirit
as std::bitset, but it has some specific design goals not covered by that
standard class:

1) Manipulation is done in "blocks", where each block is up to N bits,
   where N is the size of an integer (integer size template-configurable,
   e.g., could be a short, long long, or a character if needed)

2) Storage can be both static (max size set at compile time) to save CPU,
   or dynamic (expand as needed at run time), for flexibility.

3) Lexical comparison implemented, so the class can be used as a key
   to std::map or std::set

# Not implemented, may be some day will

1) shifting

2) More logical operations (~,  ^,  so on)

3) Combining/concatenation

4) Deleting arbitrary ranges of bits in the middle
