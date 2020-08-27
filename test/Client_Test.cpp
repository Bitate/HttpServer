#include "Client.hpp"

#include <gtest/gtest.h>
#include <thread>

TEST(ClientTests, generateRequestTest)
{
    Client client;

    ASSERT_TRUE(client.parse_uri("http://localhost:2333/"));
    ASSERT_TRUE(client.generate_request());
    ASSERT_TRUE(client.connect_to());
    std::string generated_request = 
    {
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: Bitate\r\n"
        "\r\n"
    };

    ASSERT_EQ(client.get_generated_request(), generated_request);
    
}