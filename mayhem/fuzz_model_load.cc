// Fuzz VW model loading: the input is a saved model (the in-tree AFL fuzzer's input, test/tools/fuzzing),
// which VW::initialize deserializes, restoring the reduction stack its header describes.
#include "vw/config/options_cli.h"
#include "vw/core/memory.h"
#include "vw/core/vw.h"
#include "vw/io/io_adapter.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  try
  {
    auto reader = VW::io::create_buffer_view(reinterpret_cast<const char*>(data), size);
    auto vw = VW::initialize(VW::make_unique<VW::config::options_cli>(std::vector<std::string>{"--quiet", "--no_stdin"}),
        std::move(reader));
  }
  catch (...)
  {
    return -1;
  }
  return 0;
}
