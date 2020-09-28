#define XTENSOR_USE_XSIMD
// xtensor
#include <xtensor/xarray.hpp>
#include <xtensor/xio.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xslice.hpp>
#include <xtensor/xmanipulation.hpp>
#include <cstdint>
//#define OPTIMIZED
#ifdef OPTIMIZED
// xtensor-blas
#include <xtensor-blas/xlinalg.hpp>
#endif
#include <utility>

using namespace xt;
using placeholders::_;

// globals
const int INPUT_DIMENSIONS = 4;
const int HEIGHT_IDX = 2; // H
const int WIDTH_IDX = 3; // W
const int INPUT_CHANNELS_IDX = 1; // C
const int OUTPUT_CHANNELS_IDX = 0; // K
const int NUM_IMAGES_IDX = 0; // N

template <typename T>
auto pad(const xt::xexpression<T> &data,
         int padding = 1)
{
    using value_type = typename T::value_type;

    xt::xarray<value_type> _x = xt::eval(data.derived_cast());
    auto &&_x_shape = _x.shape();
    auto x_shape = _x_shape; // copy
    x_shape[HEIGHT_IDX] += 2 * padding;
    x_shape[WIDTH_IDX] += 2 * padding;

    xt::xarray<value_type, T::static_layout> x = xt::zeros<value_type>(x_shape);

xt::view(x,
             xt::all(),
             xt::all(),
             xt::range(padding, _x_shape[HEIGHT_IDX] + padding),
             xt::range(padding, _x_shape[WIDTH_IDX] + padding)) = _x;

    return x;
}

/// INTERMEDIATE VALUE DEF:
/// Input = (N, C, H, W)
/// Filter = (K, C, R, S)
/// Output = (N, K, P, Q)
template <typename T, typename O>
auto conv2d_forward(const xt::xexpression<T> &data,
                    const xt::xexpression<O> &filter,
                    int strides = 1)
{
    //From: https://github.com/OneRaynyDay/xtensor-convolution
    // modified to assume input is already padded.
    using value_type = std::common_type_t<typename T::value_type, typename O::value_type>;

    // Convention: _x for pre-resizing via padding.
    xt::xarray<value_type> x = xt::eval(data.derived_cast());
    xt::xarray<value_type> f = xt::eval(filter.derived_cast());

    auto &&x_shape = x.shape();
    auto &&f_shape = f.shape();

    if (x_shape.size() != INPUT_DIMENSIONS || f_shape.size() != INPUT_DIMENSIONS)
        throw std::runtime_error("conv2d: input and filter must both have four dimensions.");

    // Not necessary to perform convolution, but will lead to hard-to-debug
    // errors due to leftovers missing
    XTENSOR_ASSERT_MSG(x_shape[HEIGHT_IDX] % strides == 0, "Input height is not evenly divisible by stride size.")
    XTENSOR_ASSERT_MSG(x_shape[WIDTH_IDX] % strides == 0, "Input width is not evenly divisible by stride size.")
    XTENSOR_ASSERT_MSG(x_shape[INPUT_CHANNELS_IDX] == f_shape[INPUT_CHANNELS_IDX], "Number of channels in input does not match number channels expected by convolution.");

    auto N = x_shape[NUM_IMAGES_IDX];
    auto H = x_shape[HEIGHT_IDX];
    auto W = x_shape[WIDTH_IDX];
    auto K = f_shape[OUTPUT_CHANNELS_IDX];
    auto C = f_shape[INPUT_CHANNELS_IDX];
    auto R = f_shape[HEIGHT_IDX];
    auto S = f_shape[WIDTH_IDX];
    auto P = (x_shape[HEIGHT_IDX] - f_shape[HEIGHT_IDX]) / strides + 1; // output height
    auto Q = (x_shape[WIDTH_IDX] - f_shape[WIDTH_IDX]) / strides + 1; // output width

#ifdef OPTIMIZED
    std::conditional_t<(T::static_layout == O::static_layout) &&
                           (T::static_layout != layout_type::dynamic),
                       xarray<value_type, T::static_layout>,
                       xarray<value_type, XTENSOR_DEFAULT_LAYOUT>>
        im2col;

    im2col.resize({N, P, Q, C * R * S});

    // Perform flatten filter - cheap because already alligned.
    f.reshape({K, C * R * S});

    // TODO: Finish this
    // Perform im2col
    for (auto i = 0; i <= H - R; i += strides)
    {
        for (auto j = 0; j <= W - S; j += strides)
        {
            // A chunk of (N, C, R, S). Transpose to (C, R, S, N) then reshape into (C*R*S, N)
            auto v = xt::eval(xt::view(x, xt::all(), xt::all(), xt::range(i, i + R), xt::range(j, j + S)));
            // Perform flatten filter - cheap because already alligned.
            v.reshape({N, C * R * S});
            // Assign this to im2col:
            auto x = i / strides;
            auto y = j / strides;
            xt::view(im2col, xt::all(), x, y, xt::all()) = v;
        }
    }
    // Perform flatten im2col - cheap because already alligned.
    im2col.reshape({N * P * Q, C * R * S});
    // pre-shaped result
    auto _result = xt::linalg::dot(im2col, xt::transpose(f));          // Get out N*P*Q, K
    _result.reshape({N, P, Q, K});                                     // cheap reshape
    auto result = xt::transpose(std::move(_result), {N_IDX, 3, 1, 2}); // expensive transpose
#else
    std::conditional_t<(T::static_layout == O::static_layout) &&
                           (T::static_layout != layout_type::dynamic),
                       xarray<value_type, T::static_layout>,
                       xarray<value_type, XTENSOR_DEFAULT_LAYOUT>>
        result;

    result.resize({N, K, P, Q});
    for (uint32_t i = 0; i <= H - R; i += strides)
    {
        for (uint32_t j = 0; j <= W - S; j += strides)
        {
            // yields a N x C x R x S slice in the shape of the filter
            auto v = xt::view(x, xt::all(), xt::all(), xt::range(i, i + R), xt::range(j, j + S));
            for (uint32_t k = 0; k < K; k++)
            {
                // Yields a 1 x C x R x S slice of filter
                auto f_slice = xt::view(f, k, xt::all(), xt::all(), xt::all());
                // Reduction along height and width and channel to give us
                // A single N x C x R x S slice reduced into N
                // which is assigned to result(:, k, i', j')

                auto prod = xt::sum(v * f_slice, {INPUT_CHANNELS_IDX, HEIGHT_IDX, WIDTH_IDX});
                auto x = i / strides;
                auto y = j / strides;
                xt::view(result, xt::all(), k, x, y) = prod;
            }
        }
    }
#endif
    return result;
}

template <typename A, typename B, typename C, typename D, typename E>
void conv2d_backward(
    const xt::xexpression<A> &input_image,
    const xt::xexpression<B> &d_input_image,
    const xt::xexpression<C> &filter,
    const xt::xexpression<D> &d_filter,
    const xt::xexpression<E> &d_output_image,
    int strides = 1,
    int padding = 1
){
    /*
    Backwards convolutions done using xtensors library. For speed, enable SIMD or use BLAS.

    :param input_image: The input image to be convolved
    :param filter: The convolution kernel.
    :param d_output_image: error in the output from the forward pass
    :param stride: how many pixels in between each convolution
    :param padding: how much padding to expect (pre padded) on the border of the input image.
    */
    using value_type = std::common_type_t<typename A::value_type, typename B::value_type, typename C::value_type, typename D::value_type, typename E::value_type>;

    // Convention: _x for pre-resizing via padding.
    xt::xarray<value_type> x = xt::eval(input_image.derived_cast());
    xt::xarray<value_type> d_x = xt::eval(d_input_image.derived_cast());
    xt::xarray<value_type> f = xt::eval(filter.derived_cast());
    xt::xarray<value_type> fi = xt::eval(filter.derived_cast());
    xt::xarray<value_type> d_f = xt::eval(d_filter.derived_cast());
    xt::xarray<value_type> d_o = xt::eval(d_output_image.derived_cast());

    auto &&inp_shape = x.shape();
    auto &&f_shape = f.shape();
    auto &&d_out_shape = d_o.shape();

    if (inp_shape.size() != INPUT_DIMENSIONS || f_shape.size() != INPUT_DIMENSIONS || d_out_shape.size() != INPUT_DIMENSIONS)
        throw std::runtime_error("conv2d backward: input, filter, and output error must all have four dimensions.");

    fi = xt::flip(fi, 2);
    fi = xt::flip(fi, 3);

    auto N = inp_shape[NUM_IMAGES_IDX];
    auto H = inp_shape[HEIGHT_IDX];
    auto W = inp_shape[WIDTH_IDX];
    auto K = f_shape[OUTPUT_CHANNELS_IDX];
    auto C = f_shape[INPUT_CHANNELS_IDX];
    auto R = f_shape[HEIGHT_IDX];
    auto S = f_shape[WIDTH_IDX];
    auto P = d_out_shape[HEIGHT_IDX];
    auto Q = d_out_shape[WIDTH_IDX];

    d_x = 0;
    //d_input_image.resize({inp_shape}); //todo: use this instead of initializing with zeros
    d_f = 0;
    //d_filter.resize({f_shape}); //todo: use this instead of initializing with zeros

    for (uint32_t i=0; i <= uint32_t((H-R)/strides); i += 1){
        for (uint32_t j = 0; j <= uint32_t((W - S)/strides); j += 1)
        {
            auto out_select = xt::view(d_o, xt::all(), xt::all(), i, j);
            auto x_i = i * strides;
            auto y_i = j * strides;

            for (uint32_t k = 0; k < K; k++)
            {
                //auto f_slice = xt::view(f, k, xt::all(), xt::all(), xt::all());
                auto fi_slice = xt::view(fi, k, xt::all(), xt::all(), xt::all());
                auto inp_select = xt::view(x, k, xt::all(), xt::range(x_i, x_i + R), xt::range(y_i, y_i + S));

                auto prod_inp_img = xt::sum(out_select * fi_slice, {OUTPUT_CHANNELS_IDX, HEIGHT_IDX, WIDTH_IDX});
                auto prod_inp_filt = xt::sum(out_select * inp_select, {OUTPUT_CHANNELS_IDX, HEIGHT_IDX, WIDTH_IDX});

                xt::view(d_x, k, xt::all(), xt::range(x_i, x_i + R), xt::range(y_i, y_i + S)) += prod_inp_img;
                xt::view(d_f, k, xt::all(), xt::all(), xt::all()) += prod_inp_filt;
            }
        }
    }
}
