// src/ort_eval.cpp

#include "euclid/ort_eval.hpp"

#include "euclid/encode.hpp"
#include "euclid/types.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>   // std::sscanf
#include <limits>
#include <memory>
#include <string>
#include <vector>

#if defined(EUCLID_USE_ORT)

// Support common include layouts (Homebrew and upstream).
#if __has_include(<onnxruntime/onnxruntime_cxx_api.h>)
  #include <onnxruntime/onnxruntime_cxx_api.h>
#elif __has_include(<onnxruntime_cxx_api.h>)
  #include <onnxruntime_cxx_api.h>
#elif __has_include(<onnxruntime/core/session/onnxruntime_cxx_api.h>)
  #include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#else
  #error "ONNX Runtime C++ headers not found. Expected onnxruntime/onnxruntime_cxx_api.h or equivalent."
#endif

#endif // EUCLID_USE_ORT

namespace euclid {
namespace {

#if defined(EUCLID_USE_ORT)

struct OrtState {
  std::unique_ptr<Ort::Session> session;

  std::string input_name;
  std::string output_name;

  int input_rank = 0; // 1 or 2
  bool loaded = false;
};

static OrtState g_ort;

// NOTE: We still intentionally keep a process-lifetime Env here.
// The crash you are seeing is an ORT macOS shutdown bug; the real mitigation
// is to avoid normal exit() teardown in executables/tests (use std::quick_exit)
// when ort_eval_macos_exit_workaround_needed() == true.
static Ort::Env& ort_env() {
  static Ort::Env* env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "euclid");
  return *env;
}

static int nn_output_to_cp(float y, int clip_cp) {
  const long v = std::lround(static_cast<double>(y));

  int cp = 0;
  if (v > static_cast<long>(std::numeric_limits<int>::max())) {
    cp = std::numeric_limits<int>::max();
  } else if (v < static_cast<long>(std::numeric_limits<int>::min())) {
    cp = std::numeric_limits<int>::min();
  } else {
    cp = static_cast<int>(v);
  }

  return std::clamp(cp, -clip_cp, +clip_cp);
}

static void ort_clear_internal() {
  g_ort.session.reset();
  g_ort.input_name.clear();
  g_ort.output_name.clear();
  g_ort.input_rank = 0;
  g_ort.loaded = false;
}

static bool validate_input_shape(const std::vector<int64_t>& shape, int& out_rank) {
  // Accept:
  // - rank 1: [781] or [-1]
  // - rank 2: [1,781] or [-1,781] (batch dim can be dynamic; feature dim can be dynamic)
  if (shape.size() == 1) {
    const int64_t d0 = shape[0];
    if (d0 == -1 || d0 == EncodedInputSpec::kTotal) {
      out_rank = 1;
      return true;
    }
    return false;
  }
  if (shape.size() == 2) {
    const int64_t d0 = shape[0];
    const int64_t d1 = shape[1];
    if (!(d0 == -1 || d0 == 1)) return false;
    if (!(d1 == -1 || d1 == EncodedInputSpec::kTotal)) return false;
    out_rank = 2;
    return true;
  }
  return false;
}

static bool validate_output_tensor(const Ort::Session& sess) {
  // Require at least 1 output and float type.
  const size_t outCount = sess.GetOutputCount();
  if (outCount < 1) return false;

  Ort::TypeInfo outTi = sess.GetOutputTypeInfo(0);
  auto outTsi = outTi.GetTensorTypeAndShapeInfo();
  const ONNXTensorElementDataType outType = outTsi.GetElementType();
  return outType == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
}

#endif // EUCLID_USE_ORT

} // namespace

bool ort_eval_load_file(const std::string& path) {
#if !defined(EUCLID_USE_ORT)
  (void)path;
  return false;
#else
  ort_clear_internal();

  try {
    Ort::SessionOptions opt;
    opt.SetIntraOpNumThreads(1);
    opt.SetInterOpNumThreads(1);
    opt.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

    g_ort.session = std::make_unique<Ort::Session>(ort_env(), path.c_str(), opt);

    Ort::AllocatorWithDefaultOptions alloc;

    const size_t inCount = g_ort.session->GetInputCount();
    const size_t outCount = g_ort.session->GetOutputCount();
    if (inCount != 1 || outCount < 1) {
      ort_clear_internal();
      return false;
    }

    // Names
    {
      auto inName = g_ort.session->GetInputNameAllocated(0, alloc);
      auto outName = g_ort.session->GetOutputNameAllocated(0, alloc);
      if (!inName || !outName) {
        ort_clear_internal();
        return false;
      }
      g_ort.input_name = std::string(inName.get());
      g_ort.output_name = std::string(outName.get());
    }

    // Input type/shape
    {
      Ort::TypeInfo inTi = g_ort.session->GetInputTypeInfo(0);
      auto inTsi = inTi.GetTensorTypeAndShapeInfo();
      const ONNXTensorElementDataType inType = inTsi.GetElementType();
      if (inType != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
        ort_clear_internal();
        return false;
      }

      std::vector<int64_t> inShape = inTsi.GetShape();
      int rank = 0;
      if (!validate_input_shape(inShape, rank)) {
        ort_clear_internal();
        return false;
      }
      g_ort.input_rank = rank;
    }

    // Output type
    if (!validate_output_tensor(*g_ort.session)) {
      ort_clear_internal();
      return false;
    }

    g_ort.loaded = true;
    return true;
  } catch (...) {
    ort_clear_internal();
    return false;
  }
#endif
}

void ort_eval_clear() {
#if defined(EUCLID_USE_ORT)
  ort_clear_internal();
#endif
}

bool ort_eval_enabled() {
#if !defined(EUCLID_USE_ORT)
  return false;
#else
  return g_ort.loaded && g_ort.session && (g_ort.input_rank == 1 || g_ort.input_rank == 2);
#endif
}

int ort_evaluate_white_pov(const Board& b) {
#if !defined(EUCLID_USE_ORT)
  (void)b;
  return 0;
#else
  if (!ort_eval_enabled()) return 0;

  const std::vector<float> feat = encode_12x64(b);
  assert(static_cast<int>(feat.size()) == EncodedInputSpec::kTotal);

  try {
    Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value input{nullptr};
    if (g_ort.input_rank == 1) {
      const int64_t shape[1] = { static_cast<int64_t>(EncodedInputSpec::kTotal) };
      input = Ort::Value::CreateTensor<float>(
        mem,
        const_cast<float*>(feat.data()),
        feat.size(),
        shape,
        1
      );
    } else {
      const int64_t shape[2] = { 1, static_cast<int64_t>(EncodedInputSpec::kTotal) };
      input = Ort::Value::CreateTensor<float>(
        mem,
        const_cast<float*>(feat.data()),
        feat.size(),
        shape,
        2
      );
    }

    const char* inNames[1]  = { g_ort.input_name.c_str() };
    const char* outNames[1] = { g_ort.output_name.c_str() };

    auto outputs = g_ort.session->Run(
      Ort::RunOptions{nullptr},
      inNames,
      &input,
      1,
      outNames,
      1
    );

    if (outputs.empty() || !outputs[0].IsTensor()) return 0;

    float* out = outputs[0].GetTensorMutableData<float>();
    const float y_stm = out[0];
    const int cp_stm = nn_output_to_cp(y_stm, ORT_CP_CLIP);

    // Contract: model output is side-to-move POV; evaluate(const Board&) is white-positive.
    if (b.side_to_move() == Color::White) return cp_stm;
    return -cp_stm;
  } catch (...) {
    return 0;
  }
#endif
}

bool ort_eval_macos_exit_workaround_needed() {
#if !defined(EUCLID_USE_ORT)
  return false;
#elif !defined(__APPLE__)
  return false;
#else
  // Known-bad range on macOS: ORT 1.21.x–1.22.x crashes at process exit.
  // Use version string from the ORT shared library.
  const OrtApiBase* base = OrtGetApiBase();
  if (!base) return false;

  const char* ver = base->GetVersionString();
  if (!ver) return false;

  int major = 0, minor = 0, patch = 0;
  const int n = std::sscanf(ver, "%d.%d.%d", &major, &minor, &patch);
  if (n < 2) return false;

  if (major != 1) return false;
  if (minor < 21) return false;
  if (minor > 22) return false;

  return true;
#endif
}

} // namespace euclid
