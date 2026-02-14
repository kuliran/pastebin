#include "id_gen.hpp"

#include <userver/utest/utest.hpp>

using namespace paste_service::id_gen;

UTEST(IdGen, EncodeBase58) {
    EXPECT_EQ(EncodeBase58({0,0,0,0,0,0,0,0}), "1");
    EXPECT_EQ(EncodeBase58({1,1,1,1,1,1,1,1}), "Ajszg3RAw2");
    EXPECT_EQ(EncodeBase58({1,0,1,0,0,1,1,0}), "AhgWgzaSMV");
    EXPECT_EQ(EncodeBase58({0,0,1,0,0,1,1,0}), "VtB5q5y");
    EXPECT_EQ(EncodeBase58({0,0,0,0,0,1,1,0}), "LZM");
    EXPECT_NE(EncodeBase58({1,0,1,0,0,1,1,0}), EncodeBase58({1,0,1,0,0,1,1,1}));
    EXPECT_NE(EncodeBase58({1,0,1,0,0,1,1,0}), EncodeBase58({0,0,1,0,0,1,1,0}));
}