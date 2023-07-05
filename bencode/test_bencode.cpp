#include <iostream>
#include <cassert>
#include <fstream>
#include "bencode.h"

#define TEST_INT_PATH "test_files/test1_int"
#define TEST_STR_PATH "test_files/test2_str"
#define TEST_DECODE_PATH "test_files/test3_decode"
#define TEST_LIST_PATH "test_files/test4_list"
#define TEST_DICT_PATH "test_files/test5_dict"


void test_int() {
    std::cout << "Testing ints.\n";
    std::ifstream test_int(TEST_INT_PATH);
    // test 1
    Bencoding *benc = parseInt(test_int);
    assert(benc->toString() == "1");
    delete benc;
    // test 2
    benc = parseInt(test_int);
    assert(benc->toString() == "-42");
    delete benc;
    // test 3
    benc = parseInt(test_int);
    assert(benc->toString() == "9223372036854775807");
    delete benc;
    test_int.close();
    std::cout << "All int tests passed.\n";
}

void test_str() {
    const std::string aesop = "\"An Ox came down to a reedy pool to drink. As he splashed heavily int"
    "o the water, he crushed a young Frog into the mud.\n\nThe old Frog soon missed the little one "
    "and asked his brothers and sisters what had become of him.\"";


    std::cout << "Testing strings.\n";
    std::ifstream test_str(TEST_STR_PATH);
    // test 1
    Bencoding *benc = parseStr(test_str);
    assert(benc->toString() == "\"test\"");
    delete benc;
    // test 2
    benc = parseStr(test_str);
    assert(benc->toString() == aesop);
    delete benc;
    test_str.close();
    std::cout << "All str tests passed.\n";
}

void test_decode() {
    std::cout << "Testing decode on string and int.\n";
    std::ifstream test_decode(TEST_DECODE_PATH);
    Bencoding *benc = parse(test_decode);
    assert(benc->toString() == "-128");
    delete benc;
    benc = parse(test_decode);
    assert(benc->toString() == "\"negative one hundred twenty-eight\"");
    delete benc;
    test_decode.close();
    std::cout << "All decode tests passed.\n";
}

void test_list() {
    std::cout << "Testing lists.\n";
    std::ifstream test_list(TEST_LIST_PATH);
    Bencoding *benc = parseList(test_list);
    assert(benc->toString() == "[ 100, \"word\", ]");
    benc = parseList(test_list);
    delete benc;
    test_list.close();
    std::cout << "All list tests passed\n";
}

void test_dict() {
    std::cout << "Testing dicts.\n";
    std::ifstream test_dict(TEST_DICT_PATH);
    Bencoding *benc = parseDict(test_dict);
    assert(benc->toString() == "{ \"four\": 4, \"key\": \"value\", }");
    test_dict.close();
    std::cout << "All dict tests passed.\n";
}

int main() {
    test_int();
    test_str();
    test_decode();
    test_list();
    test_dict();
    return 0;
}