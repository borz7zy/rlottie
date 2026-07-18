/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef LOTTIEFILTERMODEL_H
#define LOTTIEFILTERMODEL_H

#include <algorithm>
#include <bitset>
#include <cassert>
#include <variant>
#include "lottiemodel.h"
#include "rlottie.h"

using namespace rlottie::internal;
// Tagged union over the four property callback types.
// users should make sure proper combination
// of id and value are passed while creating the object.
class LOTVariant {
public:
    using ValueFunc = std::function<float(const rlottie::FrameInfo&)>;
    using ColorFunc = std::function<rlottie::Color(const rlottie::FrameInfo&)>;
    using PointFunc = std::function<rlottie::Point(const rlottie::FrameInfo&)>;
    using SizeFunc = std::function<rlottie::Size(const rlottie::FrameInfo&)>;

    LOTVariant(rlottie::Property prop, const ValueFunc& v)
        : mPropery(prop), mImpl(v)
    {
    }

    LOTVariant(rlottie::Property prop, ValueFunc&& v)
        : mPropery(prop), mImpl(std::move(v))
    {
    }

    LOTVariant(rlottie::Property prop, const ColorFunc& v)
        : mPropery(prop), mImpl(v)
    {
    }

    LOTVariant(rlottie::Property prop, ColorFunc&& v)
        : mPropery(prop), mImpl(std::move(v))
    {
    }

    LOTVariant(rlottie::Property prop, const PointFunc& v)
        : mPropery(prop), mImpl(v)
    {
    }

    LOTVariant(rlottie::Property prop, PointFunc&& v)
        : mPropery(prop), mImpl(std::move(v))
    {
    }

    LOTVariant(rlottie::Property prop, const SizeFunc& v)
        : mPropery(prop), mImpl(v)
    {
    }

    LOTVariant(rlottie::Property prop, SizeFunc&& v)
        : mPropery(prop), mImpl(std::move(v))
    {
    }

    rlottie::Property property() const { return mPropery; }

    /* std::get() would throw on a type mismatch, but the library is built with
     * -fno-exceptions; get_if() keeps the previous assert-in-debug,
     * undefined-in-release contract. */
    const ColorFunc& color() const { return get<ColorFunc>(); }
    const ValueFunc& value() const { return get<ValueFunc>(); }
    const PointFunc& point() const { return get<PointFunc>(); }
    const SizeFunc&  size() const { return get<SizeFunc>(); }

    LOTVariant() = default;
    ~LOTVariant() noexcept = default;
    LOTVariant(const LOTVariant&) = default;
    LOTVariant(LOTVariant&&) noexcept = default;
    LOTVariant& operator=(const LOTVariant&) = default;
    LOTVariant& operator=(LOTVariant&&) noexcept = default;

private:
    template <typename T>
    const T& get() const
    {
        const T* fn = std::get_if<T>(&mImpl);
        assert(fn);
        return *fn;
    }

    rlottie::Property mPropery{};
    std::variant<std::monostate, ValueFunc, ColorFunc, PointFunc, SizeFunc>
        mImpl;
};

namespace rlottie {

namespace internal {

namespace model {

class FilterData {
public:
    void addValue(LOTVariant& value)
    {
        uint32_t index = static_cast<uint32_t>(value.property());
        if (mBitset.test(index)) {
            std::replace_if(mFilters.begin(), mFilters.end(),
                            [&value](const LOTVariant& e) {
                                return e.property() == value.property();
                            },
                            value);
        } else {
            mBitset.set(index);
            mFilters.push_back(value);
        }
    }

    void removeValue(LOTVariant& value)
    {
        uint32_t index = static_cast<uint32_t>(value.property());
        if (mBitset.test(index)) {
            mBitset.reset(index);
            mFilters.erase(std::remove_if(mFilters.begin(), mFilters.end(),
                                          [&value](const LOTVariant& e) {
                                              return e.property() ==
                                                     value.property();
                                          }),
                           mFilters.end());
        }
    }
    bool hasFilter(rlottie::Property prop) const
    {
        return mBitset.test(static_cast<uint32_t>(prop));
    }
    model::Color color(rlottie::Property prop, int frame) const
    {
        rlottie::FrameInfo info(frame);
        rlottie::Color     col = data(prop).color()(info);
        return model::Color(col.r(), col.g(), col.b());
    }
    VPointF point(rlottie::Property prop, int frame) const
    {
        rlottie::FrameInfo info(frame);
        rlottie::Point     pt = data(prop).point()(info);
        return VPointF(pt.x(), pt.y());
    }
    VSize scale(rlottie::Property prop, int frame) const
    {
        rlottie::FrameInfo info(frame);
        rlottie::Size      sz = data(prop).size()(info);
        return VSize(sz.w(), sz.h());
    }
    float opacity(rlottie::Property prop, int frame) const
    {
        rlottie::FrameInfo info(frame);
        float              val = data(prop).value()(info);
        return val / 100;
    }
    float value(rlottie::Property prop, int frame) const
    {
        rlottie::FrameInfo info(frame);
        return data(prop).value()(info);
    }

private:
    const LOTVariant& data(rlottie::Property prop) const
    {
        auto result = std::find_if(
            mFilters.begin(), mFilters.end(),
            [prop](const LOTVariant& e) { return e.property() == prop; });
        return *result;
    }
    std::bitset<32>         mBitset{0};
    std::vector<LOTVariant> mFilters;
};

template <typename T>
struct FilterBase
{
    FilterBase(T *model): model_(model){}

    const char*  name() const { return model_->name(); }

    FilterData* filter() {
        if (!filterData_) filterData_ = std::make_unique<FilterData>();
        return filterData_.get();
    }

    const FilterData * filter() const { return filterData_.get(); }
    const T* model() const { return model_;}

    bool hasFilter(rlottie::Property prop) const {
        return filterData_ ? filterData_->hasFilter(prop)
                         : false;
    }

    T*                           model_{nullptr};
    std::unique_ptr<FilterData>  filterData_{nullptr};
};


template <typename T>
class Filter : public FilterBase<T> {
public:
    Filter(T* model): FilterBase<T>(model){}
    model::Color color(int frame) const
    {
        if (this->hasFilter(rlottie::Property::StrokeColor)) {
            return this->filter()->color(rlottie::Property::StrokeColor, frame);
        }
        return this->model()->color(frame);
    }
    float opacity(int frame) const
    {
        if (this->hasFilter(rlottie::Property::StrokeOpacity)) {
            return this->filter()->opacity(rlottie::Property::StrokeOpacity, frame);
        }
        return this->model()->opacity(frame);
    }

    float strokeWidth(int frame) const
    {
        if (this->hasFilter(rlottie::Property::StrokeWidth)) {
            return this->filter()->value(rlottie::Property::StrokeWidth, frame);
        }
        return this->model()->strokeWidth(frame);
    }

    float     miterLimit() const { return this->model()->miterLimit(); }
    CapStyle  capStyle() const { return this->model()->capStyle(); }
    JoinStyle joinStyle() const { return this->model()->joinStyle(); }
    bool      hasDashInfo() const { return this->model()->hasDashInfo(); }
    void      getDashInfo(int frameNo, std::vector<float>& result) const
    {
        return this->model()->getDashInfo(frameNo, result);
    }
};


template <>
class Filter<model::Fill>: public FilterBase<model::Fill>
{
public:
    Filter(model::Fill* model) : FilterBase<model::Fill>(model) {}

    model::Color color(int frame) const
    {
        if (this->hasFilter(rlottie::Property::FillColor)) {
            return this->filter()->color(rlottie::Property::FillColor, frame);
        }
        return this->model()->color(frame);
    }

    float opacity(int frame) const
    {
        if (this->hasFilter(rlottie::Property::FillOpacity)) {
            return this->filter()->opacity(rlottie::Property::FillOpacity, frame);
        }
        return this->model()->opacity(frame);
    }

    FillRule fillRule() const { return this->model()->fillRule(); }
};

template <>
class Filter<model::Group> : public FilterBase<model::Group>
{
public:
    Filter(model::Group* model = nullptr) : FilterBase<model::Group>(model) {}

    bool   hasModel() const { return this->model() ? true : false; }

    model::Transform* transform() const { return this->model() ? this->model()->mTransform : nullptr; }
    VMatrix           matrix(int frame) const
    {
        VMatrix mS, mR, mT;
        if (this->hasFilter(rlottie::Property::TrScale)) {
            VSize s = this->filter()->scale(rlottie::Property::TrScale, frame);
            mS.scale(s.width() / 100.0, s.height() / 100.0);
        }
        if (this->hasFilter(rlottie::Property::TrRotation)) {
            mR.rotate(this->filter()->value(rlottie::Property::TrRotation, frame));
        }
        if (this->hasFilter(rlottie::Property::TrPosition)) {
            mT.translate(this->filter()->point(rlottie::Property::TrPosition, frame));
        }

        return this->model()->mTransform->matrix(frame) * mS * mR * mT;
    }
};

template <>
class Filter<model::Trim>: public FilterBase<model::Trim>
{
public:
    Filter(model::Trim* model) : FilterBase<model::Trim>(model) {}

    model::Trim::Segment segment(int frameNo) const
    {
        if (this->hasFilter(rlottie::Property::TrimStart)) {
            this->model_->updateTrimStartValue(this->filter()->value(rlottie::Property::TrimStart, frameNo));
        }
        if (this->hasFilter(rlottie::Property::TrimEnd)) {
            this->model_->updateTrimEndValue(this->filter()->point(rlottie::Property::TrimEnd, frameNo));
        }
        model::Trim::Segment segment = this->model()->segment(frameNo);
        return segment;
    }
};


}  // namespace model

}  // namespace internal

}  // namespace rlottie

#endif  // LOTTIEFILTERMODEL_H
