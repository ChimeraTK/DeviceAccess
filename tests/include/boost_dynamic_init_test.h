#ifndef BOOST_DYNAMIC_INIT_TEST
#define BOOST_DYNAMIC_INIT_TEST

/* Hack to use dynamic linking with a hand-written init_unit_test() function.
 * 1. Define BOOST_TEST_DYN_LINK and declare init_unit_test().
 * 2. Only include unit_test_suite.hpp. This will not re-define init_unit_test() because 
 *    BOOST_TEST_MAIN is not set yet.
 * 3. Define BOOST_TEST_MAIN
 * 4. Include unit_test.hpp, which will define the main function.
 */
#define BOOST_TEST_DYN_LINK
bool init_unit_test();
#include <boost/test/unit_test_suite.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>


#endif // BOOST_DYNAMIC_INIT_TEST
