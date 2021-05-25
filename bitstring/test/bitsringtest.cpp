#include <dynamicbitstring.h>
#include <staticbitstring.h>

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <sstream>

static bool allPass = true;

using Test = std::array<unsigned int, 2>;


static std::ofstream nullOut; // intentionally unopened

template < typename _ChkType1, typename _ChkType2 >
std::ostream &check_eq( const std::string &testname,  _ChkType1 was, _ChkType2 shouldbe ) {
  if( was == shouldbe ) {
    std::cout << "PASS: " << testname <<  std::endl;
    return nullOut;
  } else {
    std::cout << "FAIL: " << testname << "[actual: " << was << ";  expected: " << shouldbe << "]";
    allPass = false;
    return std::cout;
  }
}

std::ostream &check_true( const std::string &testname,  bool mustbeTrue ) {
  if( mustbeTrue ) {
    std::cout << "PASS: " << testname <<  std::endl;
    return nullOut;
  } else {
    allPass = false;
    std::cout << "FAIL: " << testname << "[actual: FALSE;  expected: TRUE]";
    return std::cout;
  }
}

std::ostream &check_false( const std::string &testname,  bool mustbeFalse ) {
  if( mustbeFalse ) {
    allPass = false;
    std::cout << "FAIL: " << testname << "[actual: TRUE;  expected: FALSE]";
    return std::cout;
  } else {
    std::cout << "PASS: " << testname <<  std::endl;
    return nullOut;
  }
}



template<typename _C, typename _Array> void runTest(_C &d, const _Array &test1 ) {
  std::cout << "----" << std::endl;
  for( auto &oneTest: test1 ) {
    d.addBits( oneTest[0], oneTest[1] );
  }

  int base = 0;
  for( auto &oneTest: test1 ) {
    check_eq(
        (std::stringstream() << oneTest[0] << ":" << oneTest[1]).str(),
        d.read( base, oneTest[1]), oneTest[0] ) << std::endl;
    base += oneTest[1];
  }
}



template<typename _C> void flexTest(const std::string &testname, _C &a ) {
  std::cout << "---- flexTest: " << testname << std::endl;
  a.addBits( 1, 30 );
  a.addBits( 9, 4 );
  check_eq( "pre1", a.read(0,30),  1 ) << std::endl;
  check_eq( "pre2", a.read(30,4),  9 ) << std::endl;
  check_eq( "pre3", a.sizeInBits(), 34 ) << std::endl;

  a.write( 8, 26, 4);
  check_eq( "t01", a.read(30,4),  9 ) << std::endl;
  check_eq( "t02", a.read(26,4),  8 ) << std::endl;
  check_eq( "t03", a.read(0,30),  8 ) << std::endl;
  check_eq( "t04", a.sizeInBits(), 34 ) << std::endl;

  a.write(255, 30, 8 );
  check_eq( "t05", a.sizeInBits(), 38 ) << std::endl;
  check_eq( "t06", a.read(30,8),  255) << std::endl;
  check_eq( "t08", a.read(30,4),  15 ) << std::endl;
  check_eq( "t09", a.read(34,4),  15 ) << std::endl;

  check_eq( "pre4", a.read(26,4),  8 ) << std::endl;
  auto save1 = a.read(0,32);
  check_eq( "pre5", save1,  0x23 ) << std::endl;

  a.write( (1<<31) + (1<<15), 32, 32 );
  check_eq( "t10", a.sizeInBits(), 64 ) << std::endl;
  auto save2 = a.read(32,32);
  check_eq( "t11", save2, (1<<31) + (1<<15) ) << std::endl;
  check_eq( "t12", a.read(0,32), save1 ) << std::endl;

  a.write( 10, 64, 4);
  check_eq( "pre5.a", a.sizeInBits(), 68 ) << std::endl;
  check_eq( "pre6", a.read(0,32), save1 ) << std::endl;
  check_eq( "pre7", a.read(32,32), save2 ) << std::endl;
  check_eq( "t13", a.read(64,4), 10 ) << std::endl;

  a.write( 11, 158, 4); // a boundary further away
  check_eq( "pre8", a.read(0,32), save1 ) << std::endl;
  check_eq( "pre9", a.read(32,32), save2 ) << std::endl;
  check_eq( "pre10", a.read(64,32), 10 << 28 ) << std::endl;
  check_eq( "pre11", a.read(96,32), 0 ) << std::endl;
  check_eq( "pre12", a.read(64,4), 10 ) << std::endl;
  check_eq( "t15", a.read(158,4), 11 ) << std::endl;
  check_eq( "t16", a.sizeInBits(), 162 ) << std::endl;
  check_eq( "t17", a.sizeInBlocks(), 6 ) << std::endl;

  a.write( 3, 68,2 );
  check_eq( "t17.a", a.read(68, 2), 3 ) << std::endl;

  a.resize(70);
  check_eq( "pre13", a.read(0,32), save1 ) << std::endl;
  check_eq( "pre14", a.read(32,32), save2 ) << std::endl;
  check_eq( "pre15", a.sizeInBits(), 70 ) << std::endl;
  check_eq( "t18", a.read(64,4), 10 ) << std::endl;
  check_eq( "t18.a", a.read(68,2), 3 ) << std::endl;

  a.resize(68); // back to an old size
  check_eq( "pre16.a", a.read(0,32), save1 ) << std::endl;
  check_eq( "pre16.b", a.read(32,32), save2 ) << std::endl;
  check_eq( "pre16.c", a.sizeInBits(), 68 ) << std::endl;
  check_eq( "t18", a.read(64,4), 10 ) << std::endl;

  a.resize(64); // remove last piece whole
  check_eq( "pre17.a", a.read(0,32), save1 ) << std::endl;
  check_eq( "pre17.b", a.read(32,32), save2 ) << std::endl;
  check_eq( "t19", a.sizeInBits(), 64 ) << std::endl;

  a.resize(0); // all gone
  check_eq( "t19.a", a.sizeInBits(), 0 ) << std::endl;

  a.write( 0xa000b000, 0, 32 );
  a.write( 0xc000d000, 32, 32 );
  a.write( 0xe000f000, 64, 32 );
  check_eq( "t19.b", a.sizeInBits(), 96 ) << std::endl;
  check_eq( "t19.c", a.read(0,32), 0xa000b000 ) << std::endl;
  check_eq( "t19.d", a.read(32,32), 0xc000d000 ) << std::endl;
  check_eq( "t19.e", a.read(64,32), 0xe000f000 ) << std::endl;

  a.resize( 96 - 16 );
  check_eq( "t19.f", a.sizeInBits(), 96 - 16 ) << std::endl;
  check_eq( "t19.g", a.read(0,32), 0xa000b000 ) << std::endl;
  check_eq( "t19.h", a.read(32,32), 0xc000d000 ) << std::endl;
  check_eq( "t19.i", a.read(64,16), 0xe000 ) << std::endl;

  a.resize( 32 );
  check_eq( "t19.j", a.sizeInBits(), 32 ) << std::endl;
  check_eq( "t19.k", a.read(0,32), 0xa000b000 ) << std::endl;
}


template<typename _C> void compareTest(const std::string &testname, _C &b ) {
  std::cout << "---- compareTest: " << testname << std::endl;
  b.addBits( 10, 16 );
  b.addBits( 25, 16 );
  b.addBits( 35, 16 );
  {
    _C a(b);
    check_true( "equal", a == b ) << std::endl;
    check_true( "equalg1", a >= b ) << std::endl;
    check_true( "equall1", a <= b  ) << std::endl;
    a.write( a.read(16, 16) + 1, 16, 16 );
    check_eq( "bumped", a.read(16,16), 26 ) << std::endl;
    check_true( "gtrblock",  a > b ) << std::endl;
    check_true( "gtrblock.e",  a >= b ) << std::endl;
    check_false( "lssblock",  a < b ) << std::endl;
    check_false( "lssblock.e",  a <= b ) << std::endl;
    check_false( "diffblock",  a == b ) << std::endl;
  }

  {
    _C a(b);
    a.addBits( 1, 1 ); // just 1 bit difference
    check_true( "bitextra_gtr",  a > b ) << std::endl;
    check_true( "bitextra_lss",  b < a ) << std::endl;
    check_false( "bitextra_eq",  b == a ) << std::endl;
  }

  {
    _C a(b);
    a.addBits( 1, 32 ); // full block
    a.addBits( 2, 32 ); // full block
    check_true( "blkextra_gtr",  a > b ) << std::endl;
    check_true( "blkextra_lss",  b < a ) << std::endl;
    check_false( "blkextra_eq",  b == a ) << std::endl;
  }

  b.addBits(30, 16 ); // complete block
  {
    _C a(b);
    check_true( "precheck",  b == a ) << std::endl;
    a.addBits( 2, 32 );
    a.addBits( 3, 32 ); // couple blocks
    check_true( "exb_extra_gtr",  a > b ) << std::endl;
    check_true( "exb_extra_lss",  b < a ) << std::endl;
    check_false( "exb_extra_eq",  b == a ) << std::endl;

    b.resize( b.sizeInBits() - 1 ); // remove one bit
    check_eq( "resize", b.sizeInBits(),  63 ) << std::endl;

    check_true( "bdel_extra_gtr",  a > b ) << std::endl;
    check_true( "bdel_extra_lss",  b < a ) << std::endl;
    check_false( "bdel_extra_eq",  b == a ) << std::endl;

    a.resize( b.sizeInBits() );
    check_eq( "resize2", a.sizeInBits(),  63 ) << std::endl;
    check_true( "merge",  a == b) << std::endl;
  }

  {
    _C empty1;
    _C empty2;
    check_false( "eq_empty_b", b == empty1 ) << std::endl;
    check_false( "eq_empty_a", empty1 == b ) << std::endl;
    check_true( "gtr_empty_b", b > empty1 ) << std::endl;
    check_true( "lss_empty_a", empty1 < b ) << std::endl;
    check_true( "bothempty", empty1 == empty2) << std::endl;
    check_false( "bothempty.d", empty1 != empty2) << std::endl;
  }


}


template<typename _C> void logicTest(const std::string &testname, _C &a ) {
  std::cout << "---- logicTest: " << testname << std::endl;

    a.addBits( 5, 32 );
    a.addBits( 0x50000, 32 ); // couple blocks

    {
      _C b;
      b.addBits(3, 32 );
      b.addBits(0x30000, 32);

      check_eq( "prea.a", a.read(0,32),  5 ) << std::endl;
      check_eq( "prea.b", a.read(32,32),  0x50000 ) << std::endl;
      check_eq( "preb.a", b.read(0,32),  3 ) << std::endl;
      check_eq( "preb.b", b.read(32,32),  0x30000 ) << std::endl;

      b &= a;
      check_eq( "and1.a", b.read(0,32),  1 ) << std::endl;
      check_eq( "and1.b", b.read(32,32),  0x10000 ) << std::endl;
 
      b.write(3, 0, 32);
      b.write(0x30000, 32, 32);
      b |= a;
      check_eq( "or1.a", b.read(0,32),  7 ) << std::endl;
      check_eq( "or1.b", b.read(32,32),  0x70000 ) << std::endl;

    }

    a.addBits( 9, 4 );
    {
      _C b;
      b.addBits(3, 32 );
      b.addBits(0x30000, 32);
      b.addBits(11, 4);

      b &=a;
      check_eq( "and2.a", b.read(0,32),  1 ) << std::endl;
      check_eq( "and2.b", b.read(32,32),  0x10000 ) << std::endl;
      check_eq( "and2.c", b.read(64,4),  9 ) << std::endl;

      b.write(3, 0, 32);
      b.write(0x30000, 32, 32);
      b.write( 11, 64, 4 );
      b |=a;
      check_eq( "or2.a", b.read(0,32),  7 ) << std::endl;
      check_eq( "or2.b", b.read(32,32),  0x70000 ) << std::endl;
      check_eq( "or2.c", b.read(64,4),  11 ) << std::endl;



      b.resize(67); // only 3 bits at the end
      b.write(3, 0, 32);
      b.write(0x30000, 32, 32);
      b.write( 7, 64, 3 );
      a.write( 8, 64, 4 );
      _C saveb(b);

      // left side short, right only 1 bit long
      b &= a;
      check_eq( "and3.a", b.read(0,32),  1 ) << std::endl;
      check_eq( "and3.b", b.read(32,32),  0x10000 ) << std::endl;
      check_eq( "and3.c", b.read(64,3),  4 ) << std::endl;

      // add a lot of bits, cross over to next block, then repeat the test above
      a.addBits( 0,  32 );
      b = saveb;
      b &= a;
      check_eq( "and3.wa", b.read(0,32),  1 ) << std::endl;
      check_eq( "and3.wb", b.read(32,32),  0x10000 ) << std::endl;
      check_eq( "and3.wc", b.read(64,3),  4 ) << std::endl;
      a.resize( a.sizeInBits() - 32 ); // remove what we added

      b.write(3, 0, 32);
      b.write(0x30000, 32, 32);
      b.write( 1, 64, 3 );
      a.write( 8, 64, 4 );
      saveb = b;

      // left side short
      b |= a;
      check_eq( "or3.a", b.read(0,32),  7 ) << std::endl;
      check_eq( "or3.b", b.read(32,32),  0x70000 ) << std::endl;
      check_eq( "or3.c", b.read(64,3),  5 ) << std::endl;


      // add a lot of bits, cross over to next block, then repeat the test above
      a.addBits( 0,  32 );
      b = saveb;
      b |= a;
      check_eq( "or3.wa", b.read(0,32),  7 ) << std::endl;
      check_eq( "or3.wb", b.read(32,32),  0x70000 ) << std::endl;
      check_eq( "or3.wc", b.read(64,3),  5 ) << std::endl;
      a.resize( a.sizeInBits() - 32 ); // remove what we added

      b.write(3, 0, 32);
      b.write(0x30000, 32, 32);
      b.write( 3, 64, 3 );

      _C c(a);
      c.write( 9, 64, 4 ); // last bit is SET, should remain set

      // left side long
      c &= b;
      check_eq( "and4.a", c.read(0,32),  1 ) << std::endl;
      check_eq( "and4.b", c.read(32,32),  0x10000 ) << std::endl;
      check_eq( "and4.c", c.read(64,4),  1 ) << std::endl;

      // reapeat with MUCH longer left side
      c = a;
      c.write( 9, 64, 4 );
      c.addBits( 0xFFFFFFFF, 32 ); // lots of 1s, should ALL stay set
      c &= b;
      check_eq( "and4.wa", c.read(0,32),  1 ) << std::endl;
      check_eq( "and4.wb", c.read(32,32),  0x10000 ) << std::endl;
      check_eq( "and4.wc", c.read(64,32),  0x1FFFFFFF ) << std::endl;
      check_eq( "and4.wd", c.read(96,4),  0xF ) << std::endl;


      c = a;
      c.write( 8, 64, 4); // last bit is UNSET, don't want it to be set
      b.write( 3, 64, 3 );

      // left side long
      c |= b;
      check_eq( "or4.a", c.read(0,32),  7 ) << std::endl;
      check_eq( "or4.b", c.read(32,32),  0x70000 ) << std::endl;
      check_eq( "or4.c", c.read(64,4),  0xE ) << std::endl;

      // reapeat with MUCH longer left side
      c = a;
      c.write( 8, 64, 4); // last bit is UNSET, don't want it to be set
      c.addBits( 0, 32 ); // lots of 0s, should ALL stay unset
      c |= b;
      check_eq( "or4.wa", c.read(0,32),  7 ) << std::endl;
      check_eq( "or4.wb", c.read(32,32),  0x70000 ) << std::endl;
      check_eq( "or4.wc", c.read(64,32),  0xE0000000  ) << std::endl;
      check_eq( "or4.wd", c.read(96,4),  0 ) << std::endl;
    }

    {
      a.resize(64);
      _C b;
      a.write( 5, 0, 32 );
      a.write( 0x50000, 32, 32 );
      _C savea(a);

      // build b from scratch with writes

      b.write(3,        0, 32);
      check_eq("pre5.s1",  b.sizeInBits(), 32 ) << std::endl;
      check_eq("pre5.s1a", b.read(0,32), 3  ) << std::endl;


      b.write(0x30000, 32, 32);
      check_eq("pre5.s2",  b.sizeInBits(), 64 ) << std::endl;
      check_eq("pre5.s2a", b.read(0,32), 3  ) << std::endl;
      check_eq("pre5.s2b", b.read(32,32), 0x30000  ) << std::endl;

      b.write(0x300,   64, 32); // longer by an entire block
      check_eq("pre5",  b.sizeInBits(), 96 ) << std::endl;
      check_eq("pre5.a", b.read(0,32), 3  ) << std::endl;
      check_eq("pre5.b", b.read(32,32), 0x30000  ) << std::endl;
      check_eq("pre5.c", b.read(64,32), 0x300  ) << std::endl;


      _C saveb(b);


      a&=b;
      check_eq( "and5.a", a.read(0,32),  1 ) << std::endl;
      check_eq( "and5.b", a.read(32,32),  0x10000 ) << std::endl;

      a = savea;
      a|=b;
      check_eq( "or5.a", a.read(0,32),  7 ) << std::endl;
      check_eq( "or5.b", a.read(32,32),  0x70000 ) << std::endl;

      a = savea;
      b&=a;
      check_eq( "and6.a", b.read(0,32),  1 ) << std::endl;
      check_eq( "and6.b", b.read(32,32),  0x10000 ) << std::endl;
      check_eq( "and6.c", b.read(64,32),  0x300 ) << std::endl; // unchanged

      b = saveb;
      b|=a;
      check_eq( "or6.a", b.read(0,32),  7 ) << std::endl;
      check_eq( "or6.b", b.read(32,32),  0x70000 ) << std::endl;
      check_eq( "or6.c", b.read(64,32),  0x300 ) << std::endl; // unchanged
    }
}

int main() {
  std::cout << "newest version" << std::endl;
  std::array test1 {
    //    value    #bits
    Test {    105,    7},
    Test {    234,    8},
    Test {   1024,   14},
    Test {     20,    5},
    Test { 500005,   20},
    Test {      3,    2}
  };

  {
    lxutil::dynamicbitstring<> a;
    runTest( a, test1 );
  }

  {
    lxutil::staticbitstring<300> a;
    runTest( a, test1 );
  }

  {
    lxutil::staticbitstring<300> a;
    flexTest("fixed", a );
  }

  {
    lxutil::dynamicbitstring<> a(300);
    flexTest("short", a );
  }

  {
    lxutil::dynamicbitstring<> a;
    flexTest("long", a );
  }

  {
    lxutil::staticbitstring<300> a;
    compareTest("comparisons", a );
  }

  {
    lxutil::staticbitstring<300> a;
    logicTest("A", a );
  }

  if( allPass ) {
    std::cout << "### OVERALL: PASS ###" << std::endl;
    return 0;
  } else {
    std::cout << "### OVERALL: !!!FAIL!!! ###" << std::endl;
    return 1;
  }
}
