#include "base64.hpp"
#include <vector>
#include <sstream>
#include <map>
#include <bitset>
#include <logger.hpp>

namespace
{
    /**
     * For general Base64 encoding.
     * Convert one 6-bit input group(in the form of decimal number) 
     * to its corresponding Base64 character.
     */
    const char encoding_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    /**
     * For general Base64 decoding.
     * Map a encoded Base64 character to its corresponding 6-bit group(in the form of decimal number)
     */
    const std::map< char, uint16_t > decoding_map{
        {'A', 0},  {'B', 1},  {'C', 2},  {'D', 3},  {'E', 4},  {'F', 5},  {'G', 6},  {'H', 7},
        {'I', 8},  {'J', 9},  {'K', 10}, {'L', 11}, {'M', 12}, {'N', 13}, {'O', 14}, {'P', 15},
        {'Q', 16}, {'R', 17}, {'S', 18}, {'T', 19}, {'U', 20}, {'V', 21}, {'W', 22}, {'X', 23},
        {'Y', 24}, {'Z', 25}, {'a', 26}, {'b', 27}, {'c', 28}, {'d', 29}, {'e', 30}, {'f', 31},
        {'g', 32}, {'h', 33}, {'i', 34}, {'j', 35}, {'k', 36}, {'l', 37}, {'m', 38}, {'n', 39},
        {'o', 40}, {'p', 41}, {'q', 42}, {'r', 43}, {'s', 44}, {'t', 45}, {'u', 46}, {'v', 47},
        {'w', 48}, {'x', 49}, {'y', 50}, {'z', 51}, {'0', 52}, {'1', 53}, {'2', 54}, {'3', 55},
        {'4', 56}, {'5', 57}, {'6', 58}, {'7', 59}, {'8', 60}, {'9', 61}, {'+', 62}, {'/', 63},
    };

    /**
     * Only for URI Base64 encoding.
     * Convert one 6-bit input group(in the form of decimal number) 
     * to its corresponding Base64 character.
     */
    const char uri_encoding_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    /**
     * Only for URI Base64 decoding.
     * Map a encoded Base64 character to its corresponding 6-bit group(in the form of decimal number)
     */
    const std::map< char, uint16_t > uri_decoding_map{
        {'A', 0},  {'B', 1},  {'C', 2},  {'D', 3},  {'E', 4},  {'F', 5},  {'G', 6},  {'H', 7},
        {'I', 8},  {'J', 9},  {'K', 10}, {'L', 11}, {'M', 12}, {'N', 13}, {'O', 14}, {'P', 15},
        {'Q', 16}, {'R', 17}, {'S', 18}, {'T', 19}, {'U', 20}, {'V', 21}, {'W', 22}, {'X', 23},
        {'Y', 24}, {'Z', 25}, {'a', 26}, {'b', 27}, {'c', 28}, {'d', 29}, {'e', 30}, {'f', 31},
        {'g', 32}, {'h', 33}, {'i', 34}, {'j', 35}, {'k', 36}, {'l', 37}, {'m', 38}, {'n', 39},
        {'o', 40}, {'p', 41}, {'q', 42}, {'r', 43}, {'s', 44}, {'t', 45}, {'u', 46}, {'v', 47},
        {'w', 48}, {'x', 49}, {'y', 50}, {'z', 51}, {'0', 52}, {'1', 53}, {'2', 54}, {'3', 55},
        {'4', 56}, {'5', 57}, {'6', 58}, {'7', 59}, {'8', 60}, {'9', 61}, {'-', 62}, {'_', 63},
    };
}

base64::~base64() = default;
base64::base64() = default;

std::string base64::encode(const std::string& unencoded_string) const
{   
    size_t bits = 0;
    uint16_t buffer = 0;
    std::stringstream encoded_result;
    for(auto byte : unencoded_string)
    {
        buffer <<= 8;
        buffer += (uint16_t)byte;
        bits += 8;
        while ( bits >= 6)
        {
            /**
             * 0x3f is 0011 1111
             * 1101 0101 & 0x3f => 0001 0101 (reserve the most significant 6 bits of 1101 0101)
             * 1101 0101 >> 2 => 0011 0101 (drop the least significant 2 bits)
             */
            encoded_result << encoding_table[(buffer >> (bits-6)) & 0x3f];
            
            /**
             * clear most significant 6 bits of buffer.
             */
            buffer &= ~(0x3f << (bits-6));
            bits -= 6;
        }
    }

    /**
     * If after loop, there are still (1 byte - 6 bits = 2 bit)
     * Shift left 4 bits to form a new 6-bit character
     * and add padding(=) if needed.
     */
    if((unencoded_string.size() % 3) == 1)
    {
        buffer <<= 4;
        encoded_result << encoding_table[buffer & 0x3f] << "==";
    }

    /**
     * If after loop, there are still (2 bytes - 12 bits = 4 bit)
     * Shift left 2 bits to form a new 6-bit character.
     * and add padding if needed.
     */
    if((unencoded_string.size() % 3) == 2)
    {
        buffer <<= 2;
        encoded_result << encoding_table[buffer & 0x3f] << '=';
    }

    return encoded_result.str();
}

std::string base64::decode(const std::string& encoded_string) const
{

}