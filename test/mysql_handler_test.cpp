#include <gtest/gtest.h>
#include "mysql_handler.hpp"

#include <vector>
// tmp header
#include <iostream>

TEST(mysql_handler_tests, connect_to_mysql_test)
{
    mysql_handler mysql;

    // port can not be negative
    ASSERT_FALSE(mysql.connect_to_mysql(-3306, "bitate", "qwer"));
    // successfully connect to mysql based on given port, username, and password
    ASSERT_TRUE(mysql.connect_to_mysql(3306, "bitate", "qwer"));
}

TEST(mysql_handler_tests, add_new_user_test)
{
    mysql_handler mysql;
    ASSERT_TRUE(mysql.connect_to_mysql(3306, "bitate", "qwer"));
    mysql.initialize_mysql_layout();
    
    struct single_user_record
    {
        std::string name;
        int age;
        std::string email;
    };

    const std::vector<single_user_record> user_info_records = 
    {
        { "Tom",  20, "tom@gmail.com" },
        { "Jane", 23, "Jane@gmail.com"},
        { "Michael", 25, "Happy@gmail.com"}
    };

    size_t index = 0;
    for( const single_user_record& user_record : user_info_records)
    {
        ASSERT_TRUE(mysql.add_user(user_record.name, user_record.age, user_record.email)) << index;
        ++index;
    }
}



