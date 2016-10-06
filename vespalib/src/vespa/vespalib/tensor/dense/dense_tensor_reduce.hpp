// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include "dense_tensor_reduce.h"

namespace vespalib {
namespace tensor {
namespace dense {

using Cells = DenseTensor::Cells;
using DimensionsMeta = DenseTensor::DimensionsMeta;

namespace {

DimensionsMeta
removeDimension(const DimensionsMeta &dimensionsMeta,
                const string &dimensionToRemove)
{
    DimensionsMeta result = dimensionsMeta;
    auto itr = std::lower_bound(result.begin(), result.end(), dimensionToRemove,
                                [](const auto &dimMeta, const auto &dimension_in) {
                                    return dimMeta.dimension() < dimension_in;
                                });
    if ((itr != result.end()) && (itr->dimension() == dimensionToRemove)) {
        result.erase(itr);
    }
    return result;
}

size_t
calcCellsSize(const DimensionsMeta &dimensionsMeta)
{
    size_t cellsSize = 1;
    for (const auto &dimMeta : dimensionsMeta) {
        cellsSize *= dimMeta.size();
    }
    return cellsSize;
}


class DimensionReducer
{
private:
    DimensionsMeta _dimensionsResult;
    Cells _cellsResult;
    size_t _innerDimSize;
    size_t _sumDimSize;
    size_t _outerDimSize;

    void setup(const DimensionsMeta &dimensions,
               const vespalib::string &dimensionToRemove) {
        auto itr = std::lower_bound(dimensions.cbegin(), dimensions.cend(), dimensionToRemove,
                                    [](const auto &dimMeta, const auto &dimension) {
                                        return dimMeta.dimension() < dimension;
                                    });
        if ((itr != dimensions.end()) && (itr->dimension() == dimensionToRemove)) {
            for (auto outerItr = dimensions.cbegin(); outerItr != itr; ++outerItr) {
                _outerDimSize *= outerItr->size();
            }
            _sumDimSize = itr->size();
            for (++itr; itr != dimensions.cend(); ++itr) {
                _innerDimSize *= itr->size();
            }
        } else {
            _outerDimSize = calcCellsSize(dimensions);
        }
    }

public:
    DimensionReducer(const DimensionsMeta &dimensions,
                     const string &dimensionToRemove)
        : _dimensionsResult(removeDimension(dimensions, dimensionToRemove)),
          _cellsResult(calcCellsSize(_dimensionsResult)),
          _innerDimSize(1),
          _sumDimSize(1),
          _outerDimSize(1)
    {
        setup(dimensions, dimensionToRemove);
    }

    template <typename Function>
    DenseTensor::UP
    reduceCells(const Cells &cellsIn, Function &&func) {
        auto itr_in = cellsIn.cbegin();
        auto itr_out = _cellsResult.begin();
        for (size_t outerDim = 0; outerDim < _outerDimSize; ++outerDim) {
            auto saved_itr = itr_out;
            for (size_t innerDim = 0; innerDim < _innerDimSize; ++innerDim) {
                *itr_out = *itr_in;
                ++itr_out;
                ++itr_in;
            }
            for (size_t sumDim = 1; sumDim < _sumDimSize; ++sumDim) {
                itr_out = saved_itr;
                for (size_t innerDim = 0; innerDim < _innerDimSize; ++innerDim) {
                    *itr_out = func(*itr_out, *itr_in);
                    ++itr_out;
                    ++itr_in;
                }
            }
        }
        assert(itr_out == _cellsResult.end());
        assert(itr_in == cellsIn.cend());
        return std::make_unique<DenseTensor>(std::move(_dimensionsResult), std::move(_cellsResult));
    }
};

template <typename Function>
DenseTensor::UP
reduce(const DenseTensor &tensor, const vespalib::string &dimensionToRemove, Function &&func)
{
    DimensionReducer reducer(tensor.dimensionsMeta(), dimensionToRemove);
    return reducer.reduceCells(tensor.cells(), func);
}

}

template <typename Function>
std::unique_ptr<Tensor>
reduce(const DenseTensor &tensor, const std::vector<vespalib::string> &dimensions, Function &&func)
{
    if (dimensions.size() == 1) {
        return reduce(tensor, dimensions[0], func);
    } else if (dimensions.size() > 0) {
        DenseTensor::UP result = reduce(tensor, dimensions[0], func);
        for (size_t i = 1; i < dimensions.size(); ++i) {
            DenseTensor::UP tmpResult = reduce(*result, dimensions[i], func);
            result = std::move(tmpResult);
        }
        return result;
    } else {
        return std::unique_ptr<Tensor>();
    }
}

} // namespace dense
} // namespace tensor
} // namespace vespalib