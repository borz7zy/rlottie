#include <gtest/gtest.h>
#include <string>
#include "lottiemodel.h"

using rlottie::internal::model::Object;

/* Object stores names either inline (< 14 chars) or on the heap, switching
 * between the two through a union. A lottie file may repeat the "nm" key for
 * the same object, so setName() has to release the previous allocation before
 * overwriting mPtr. Run these under ASAN/LSAN to catch regressions. */

TEST(ObjectName, shortName)
{
    Object o(Object::Type::Layer);
    o.setName("abc");
    ASSERT_STREQ(o.name(), "abc");
}

TEST(ObjectName, longName)
{
    const char *n = "a-quite-long-layer-name-from-a-lottie-file";
    Object      o(Object::Type::Layer);
    o.setName(n);
    ASSERT_STREQ(o.name(), n);
}

TEST(ObjectName, inlineBoundary)
{
    Object a(Object::Type::Layer);
    a.setName("0123456789012");  // 13 chars, stays inline
    ASSERT_STREQ(a.name(), "0123456789012");

    Object b(Object::Type::Layer);
    b.setName("01234567890123");  // 14 chars, goes to the heap
    ASSERT_STREQ(b.name(), "01234567890123");
}

TEST(ObjectName, repeatedLongToLong)
{
    Object o(Object::Type::Layer);
    o.setName("first-long-name-that-heap-allocates");
    o.setName("second-long-name-that-heap-allocates");
    ASSERT_STREQ(o.name(), "second-long-name-that-heap-allocates");
}

TEST(ObjectName, repeatedLongToShort)
{
    Object o(Object::Type::Layer);
    o.setName("first-long-name-that-heap-allocates");
    o.setName("short");
    ASSERT_STREQ(o.name(), "short");
}

TEST(ObjectName, repeatedShortToLong)
{
    Object o(Object::Type::Layer);
    o.setName("short");
    o.setName("a-long-name-that-heap-allocates");
    ASSERT_STREQ(o.name(), "a-long-name-that-heap-allocates");
}

TEST(ObjectName, repeatedAlternating)
{
    Object o(Object::Type::Layer);
    for (int i = 0; i < 1000; ++i) {
        std::string s = (i % 2)
                            ? std::string("long-name-number-") + std::to_string(i)
                            : std::string("s") + std::to_string(i % 10);
        o.setName(s.c_str());
        ASSERT_STREQ(o.name(), s.c_str());
    }
}

TEST(ObjectName, nullKeepsPreviousName)
{
    const char *n = "a-long-name-that-heap-allocates";
    Object      o(Object::Type::Layer);
    o.setName(n);
    o.setName(nullptr);
    ASSERT_STREQ(o.name(), n);
}

TEST(ObjectName, emptyName)
{
    Object o(Object::Type::Layer);
    o.setName("a-long-name-that-heap-allocates");
    o.setName("");
    ASSERT_STREQ(o.name(), "");
}

TEST(ObjectName, unnamedObject)
{
    Object o(Object::Type::Layer);
    ASSERT_NE(o.name(), nullptr);
}

/* mPtr overlaps only the first bytes of the inline buffer; _type and the
 * bitfields live past it, so they must survive a heap-allocated name. */
TEST(ObjectName, flagsSurviveNameStorage)
{
    Object o(Object::Type::Fill);
    o.setStatic(false);
    o.setHidden(true);

    o.setName("a-long-name-that-heap-allocates");
    ASSERT_EQ(o.type(), Object::Type::Fill);
    ASSERT_FALSE(o.isStatic());
    ASSERT_TRUE(o.hidden());

    o.setName("short");
    ASSERT_EQ(o.type(), Object::Type::Fill);
    ASSERT_FALSE(o.isStatic());
    ASSERT_TRUE(o.hidden());
}
