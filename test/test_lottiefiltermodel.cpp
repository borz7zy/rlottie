#include <gtest/gtest.h>
#include <type_traits>
#include <vector>
#include "lottiefiltermodel.h"

using rlottie::FrameInfo;
using rlottie::Property;

namespace {

LOTVariant makeValue(float v)
{
    return LOTVariant(Property::FillOpacity,
                      LOTVariant::ValueFunc([v](const FrameInfo &) { return v; }));
}

LOTVariant makeColor(float r)
{
    return LOTVariant(Property::FillColor,
                      LOTVariant::ColorFunc([r](const FrameInfo &) {
                          return rlottie::Color(r, 0, 0);
                      }));
}

LOTVariant makePoint()
{
    return LOTVariant(Property::TrPosition, LOTVariant::PointFunc([](const FrameInfo &) {
                          return rlottie::Point(1, 2);
                      }));
}

LOTVariant makeSize()
{
    return LOTVariant(Property::TrScale, LOTVariant::SizeFunc([](const FrameInfo &) {
                          return rlottie::Size(3, 4);
                      }));
}

}  // namespace

/* LOTVariant is a hand-rolled tagged union over std::function (the project is
 * C++14, so std::variant is not available). Its assignment operators destroy
 * the active member before constructing the new one, which makes self
 * assignment and exception safety the interesting cases. Run under ASAN. */

TEST(LOTVariantTest, moveOperationsAreNoexcept)
{
    /* std::vector<LOTVariant> relies on these to move rather than copy on
     * reallocation. */
    ASSERT_TRUE(std::is_nothrow_move_constructible<LOTVariant>::value);
    ASSERT_TRUE(std::is_nothrow_move_assignable<LOTVariant>::value);
}

TEST(LOTVariantTest, moveAssignAcrossEveryTag)
{
    std::vector<LOTVariant> src;
    src.push_back(makeValue(1));
    src.push_back(makeColor(1));
    src.push_back(makePoint());
    src.push_back(makeSize());
    src.push_back(LOTVariant());

    for (auto &a : src) {
        for (auto &b : src) {
            LOTVariant lhs;
            lhs = a;
            LOTVariant rhs;
            rhs = b;
            lhs = std::move(rhs);
        }
    }
}

TEST(LOTVariantTest, selfMoveAssign)
{
    LOTVariant  v = makeValue(7);
    LOTVariant &alias = v;
    v = std::move(alias);
    ASSERT_FLOAT_EQ(v.value()(FrameInfo(0)), 7);
}

TEST(LOTVariantTest, selfCopyAssign)
{
    LOTVariant  v = makeColor(0.5f);
    LOTVariant &alias = v;
    v = alias;
    ASSERT_FLOAT_EQ(v.color()(FrameInfo(0)).r(), 0.5f);
}

TEST(LOTVariantTest, repeatedReassignmentChurnsTags)
{
    LOTVariant v;
    for (int i = 0; i < 500; ++i) {
        switch (i % 4) {
        case 0:
            v = makeValue(float(i));
            break;
        case 1:
            v = makeColor(0.25f);
            break;
        case 2:
            v = makePoint();
            break;
        case 3:
            v = makeSize();
            break;
        }
    }
    ASSERT_FLOAT_EQ(v.size()(FrameInfo(0)).w(), 3);
}

TEST(LOTVariantTest, copiesAreIndependent)
{
    LOTVariant a = makeValue(11);
    LOTVariant b = a;
    a = makeColor(1);
    ASSERT_FLOAT_EQ(b.value()(FrameInfo(0)), 11);
}

/* FilterData::addValue() goes through std::replace_if and removeValue()
 * through std::remove_if; both assign over live elements. */
TEST(LOTVariantTest, filterDataAddReplaceRemove)
{
    rlottie::internal::model::FilterData fd;
    for (int i = 0; i < 200; ++i) {
        LOTVariant a = makeValue(float(i));
        LOTVariant b = makeColor(0.1f);
        fd.addValue(a);
        fd.addValue(b);
    }
    ASSERT_TRUE(fd.hasFilter(Property::FillOpacity));
    ASSERT_FLOAT_EQ(fd.value(Property::FillOpacity, 0), 199.0f);

    LOTVariant a = makeValue(1);
    fd.removeValue(a);
    ASSERT_FALSE(fd.hasFilter(Property::FillOpacity));
}
