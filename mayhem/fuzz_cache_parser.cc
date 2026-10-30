// Fuzz VW's binary cache-file example reader (read_example_from_cache).
#include "vw/cache_parser/parse_example_cache.h"
#include "vw/config/options_cli.h"
#include "vw/core/example.h"
#include "vw/core/io_buf.h"
#include "vw/core/memory.h"
#include "vw/core/vw.h"
#include "vw/io/io_adapter.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

static const std::vector<std::vector<std::string>> kConfigs = {
    {"--quiet", "--no_stdin", "-b", "12"},
    {"--quiet", "--no_stdin", "-b", "12", "--cb", "4"},
    {"--quiet", "--no_stdin", "-b", "12", "--oaa", "4"},
    {"--quiet", "--no_stdin", "-b", "12", "--csoaa", "4"},
    {"--quiet", "--no_stdin", "-b", "12", "--multilabel_oaa", "4"},
    {"--quiet", "--no_stdin", "-b", "12", "--ccb_explore_adf"},
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  if (size < 1) { return -1; }
  const auto& args = kConfigs[data[0] % kConfigs.size()];

  try
  {
    auto vw = VW::initialize(VW::make_unique<VW::config::options_cli>(args));
    VW::io_buf reader;
    reader.add_file(VW::io::create_buffer_view(reinterpret_cast<const char*>(data) + 1, size - 1));
    VW::multi_ex examples;
    VW::example ex;
    examples.push_back(&ex);
    VW::parsers::cache::read_example_from_cache(vw.get(), reader, examples);
  }
  catch (...)
  {
    return -1;
  }
  return 0;
}
