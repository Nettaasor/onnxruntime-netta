#include "gtest/gtest.h"
#include "test/common/tensor_op_test_utils.h"
#include "test/providers/provider_test_utils.h"

#include <cfenv>

namespace onnxruntime {
namespace test {

TEST(QLinearLookupTableBasedOperatorTests, QLinearLeakyRelu_Int8) {
  OpTester test("QLinearLeakyRelu", 1, onnxruntime::kMSDomain);
  test.AddAttribute<float>("alpha", 0.1f);
  float X_scale = 0.25f;
  //int8_t X_zero_point = 0;
  float Y_scale = 0.1f;
  int8_t Y_zero_point = -100;

  std::vector<int64_t> dims = {16};
  test.AddInput<int8_t>("X", dims, {0, 16, 17, 18, 19, 90, 91, 127, -128, -110, -108, -100, -16, -17, -18, -1});
  test.AddInput<float>("X_scale", {}, {X_scale});
  test.AddOptionalInputEdge<int8_t>();  // optional "X_zero_point" using default value here
  test.AddInput<float>("Y_scale", {}, {Y_scale});
  test.AddInput<int8_t>("Y_zero_point", {}, {Y_zero_point});
  test.AddOutput<int8_t>("Y", dims, {-100, -60, -58, -55, -52, 125, 127, 127, -128, -128, -127, -125, -104, -104, -104, -100});
  auto origin_round_mode = std::fegetround();
  std::fesetround(FE_TONEAREST);
  test.Run();
  std::fesetround(origin_round_mode);
}

TEST(QLinearLookupTableBasedOperatorTests, QLinearLeakyRelu_UInt8) {
  OpTester test("QLinearLeakyRelu", 1, onnxruntime::kMSDomain);
  test.AddAttribute<float>("alpha", 0.1f);
  float X_scale = 0.25f;
  uint8_t X_zero_point = 128;
  float Y_scale = 0.1f;
  uint8_t Y_zero_point = 30;

  std::vector<int64_t> dims = {16};
  test.AddInput<uint8_t>("X", dims, {0, 16, 17, 18, 19, 90, 91, 127, 128, 136, 137, 138, 216, 217, 218, 255});
  test.AddInput<float>("X_scale", {}, {X_scale});
  test.AddInput<uint8_t>("X_zero_point", {}, {X_zero_point});
  test.AddInput<float>("Y_scale", {}, {Y_scale});
  test.AddInput<uint8_t>("Y_zero_point", {}, {Y_zero_point});
  test.AddOutput<uint8_t>("Y", dims, {0, 2, 2, 2, 3, 20, 21, 30, 30, 50, 52, 55, 250, 252, 255, 255});
  auto origin_round_mode = std::fegetround();
  std::fesetround(FE_TONEAREST);
  test.Run();
  std::fesetround(origin_round_mode);
}

TEST(QLinearLookupTableBasedOperatorTests, QLinearSigmoid_Int8) {
  OpTester test("QLinearSigmoid", 1, onnxruntime::kMSDomain);
  float X_scale = 0.025f;
  //int8_t X_zero_point = 0;
  float Y_scale = 1.0f / 256.0f;
  int8_t Y_zero_point = -120;

  std::vector<int64_t> dims = {16};
  test.AddInput<int8_t>("X", dims, {0, 16, 17, 18, 19, 90, 91, 127, -128, -110, -108, -100, -16, -17, -18, -1});
  test.AddInput<float>("X_scale", {}, {X_scale});
  test.AddOptionalInputEdge<int8_t>();  // optional "X_zero_point" using default value here
  test.AddInput<float>("Y_scale", {}, {Y_scale});
  test.AddInput<int8_t>("Y_zero_point", {}, {Y_zero_point});
  test.AddOutput<int8_t>("Y", dims, {8, 33, 35, 36, 38, 112, 112, 126, -110, -105, -104, -101, -17, -19, -20, 6});
  auto origin_round_mode = std::fegetround();
  std::fesetround(FE_TONEAREST);
  test.Run();
  std::fesetround(origin_round_mode);
}

TEST(QLinearLookupTableBasedOperatorTests, QLinearSigmoid_UInt8) {
  OpTester test("QLinearSigmoid", 1, onnxruntime::kMSDomain);
  float X_scale = 0.025f;
  uint8_t X_zero_point = 128;
  float Y_scale = 1.0f / 256.0f;
  uint8_t Y_zero_point = 8;

  std::vector<int64_t> dims = {16};
  test.AddInput<uint8_t>("X", dims, {0, 16, 17, 18, 19, 90, 91, 127, 128, 136, 137, 138, 216, 217, 218, 255});
  test.AddInput<float>("X_scale", {}, {X_scale});
  test.AddInput<uint8_t>("X_zero_point", {}, {X_zero_point});
  test.AddInput<float>("Y_scale", {}, {Y_scale});
  test.AddInput<uint8_t>("Y_zero_point", {}, {Y_zero_point});
  test.AddOutput<uint8_t>("Y", dims, {18, 23, 23, 23, 24, 79, 81, 134, 136, 149, 150, 152, 238, 239, 240, 254});
  auto origin_round_mode = std::fegetround();
  std::fesetround(FE_TONEAREST);
  test.Run();
  std::fesetround(origin_round_mode);
}

// NNAPI can only take 0 as Y_zero_point
TEST(QLinearLookupTableBasedOperatorTests, QLinearSigmoid_UInt8_0_Y_ZP) {
  auto run_test = [](bool scales_and_zp_are_initializers) {
    OpTester test("QLinearSigmoid", 1, onnxruntime::kMSDomain);
    float X_scale = 0.025f;
    uint8_t X_zero_point = 128;
    float Y_scale = 1.0f / 256.0f;
    uint8_t Y_zero_point = 0;

    std::vector<int64_t> dims = {16};
    test.AddInput<uint8_t>("X", dims, {0, 16, 17, 18, 19, 90, 91, 127, 128, 136, 137, 138, 216, 217, 218, 255});
    test.AddInput<float>("X_scale", {}, {X_scale}, scales_and_zp_are_initializers);
    test.AddInput<uint8_t>("X_zero_point", {}, {X_zero_point}, scales_and_zp_are_initializers);
    test.AddInput<float>("Y_scale", {}, {Y_scale}, scales_and_zp_are_initializers);
    test.AddInput<uint8_t>("Y_zero_point", {}, {Y_zero_point}, scales_and_zp_are_initializers);
    test.AddOutput<uint8_t>("Y", dims, {10, 15, 15, 15, 16, 71, 73, 126, 128, 141, 142, 144, 230, 231, 232, 246});
    auto origin_round_mode = std::fegetround();
    std::fesetround(FE_TONEAREST);
    test.Run();
    std::fesetround(origin_round_mode);
  };

  run_test(false);
  run_test(true);
}

TEST(QLinearLookupTableBasedOperatorTests, QLinearSoftmax_UInt8) {
  OpTester test("QLinearSoftmax", 1, onnxruntime::kMSDomain);
  test.AddAttribute<int64_t>("axis", -1);
  float X_scale = 0.166099221f;
  //
  uint8_t X_zero_point = 128;
  float Y_scale = 1.0f / 256.0f;
  uint8_t Y_zero_point = 0;
  //

  std::vector<int64_t> dims = {2, 20};
  auto x_in = std::vector<uint8_t>{50, 67, 58, 68, 46, 69, 77, 91, 62, 74, 67, 72, 71, 70, 83, 88, 75, 54, 74, 88};
  auto y_out = std::vector<uint8_t> { 0, 2, 0, 2, 0, 2, 8, 86, 1, 5, 2, 4, 3, 3, 23, 52, 6, 0, 5, 52 };
  for (int64_t i = 1; i < dims[0]; i++) {
    for (int64_t j = 0; j < dims[1]; j++) {
      x_in.push_back(x_in[j]);
      y_out.push_back(y_out[j]);
    }
  }

  test.AddInput<uint8_t>("X", dims, x_in);
  test.AddInput<float>("X_scale", {}, {X_scale});
  test.AddInput<uint8_t>("X_zero_point", {}, {X_zero_point});
  test.AddInput<float>("Y_scale", {}, {Y_scale});
  test.AddInput<uint8_t>("Y_zero_point", {}, {Y_zero_point});
  test.AddOutput<uint8_t>("Y", dims, y_out);
  auto origin_round_mode = std::fegetround();
  std::fesetround(FE_TONEAREST);
  test.Run();
  std::fesetround(origin_round_mode);
}

TEST(QLinearLookupTableBasedOperatorTests, QLinearSoftmax_Int8) {
  OpTester test("QLinearSoftmax", 1, onnxruntime::kMSDomain);
  test.AddAttribute<int64_t>("axis", -1);
  float X_scale = 0.166099221f;
  //
  int8_t X_zero_point = 0;
  float Y_scale = 1.0f / 128.0f;
  int8_t Y_zero_point = 0;
  //

  std::vector<int64_t> dims = {2, 20};
  auto x_in = std::vector<int8_t>{-28, -4, -4, -7, 3, -26, 4, -16, 23, 14, -7, 26, -8, 19, -16, -13, 7, 17, 27, 5};
  auto y_out = std::vector<int8_t>{0, 0, 0, 0, 1, 0, 1, 0, 22, 5, 0, 35, 0, 11, 0, 0, 2, 8, 42, 1};
  for (int64_t i = 1; i < dims[0]; i++) {
    for (int64_t j = 0; j < dims[1]; j++) {
      x_in.push_back(x_in[j]);
      y_out.push_back(y_out[j]);
    }
  }

  test.AddInput<int8_t>("X", dims, x_in);
  test.AddInput<float>("X_scale", {}, {X_scale});
  test.AddInput<int8_t>("X_zero_point", {}, {X_zero_point});
  test.AddInput<float>("Y_scale", {}, {Y_scale});
  test.AddInput<int8_t>("Y_zero_point", {}, {Y_zero_point});
  test.AddOutput<int8_t>("Y", dims, y_out);
  auto origin_round_mode = std::fegetround();
  std::fesetround(FE_TONEAREST);
  test.Run();
  std::fesetround(origin_round_mode);
}

}  // namespace test
}  // namespace onnxruntime
