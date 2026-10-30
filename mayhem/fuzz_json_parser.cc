// Fuzz VW's JSON and DSJSON example parsers. The first input byte selects the parser/workspace
// configuration; the rest is the JSON document.
#include "vw/config/options_cli.h"
#include "vw/core/example.h"
#include "vw/core/memory.h"
#include "vw/core/multi_ex.h"
#include "vw/core/parser.h"
#include "vw/core/vw.h"
#include "vw/json_parser/parse_example_json.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

static const std::vector<std::vector<std::string>> kConfigs = {
    {"--quiet", "--no_stdin", "-b", "12", "--json"},
    {"--quiet", "--no_stdin", "-b", "12", "--json", "--chain_hash"},
    {"--quiet", "--no_stdin", "-b", "12", "--json", "--cb_explore_adf"},
    {"--quiet", "--no_stdin", "-b", "12", "--json", "--ccb_explore_adf"},
    {"--quiet", "--no_stdin", "-b", "12", "--json", "--slates"},
    {"--quiet", "--no_stdin", "-b", "12", "--json", "--oaa", "4"},
    {"--quiet", "--no_stdin", "-b", "12", "--json", "--multilabel_oaa", "4"},
    {"--quiet", "--no_stdin", "-b", "12", "--dsjson", "--cb_explore_adf"},
    {"--quiet", "--no_stdin", "-b", "12", "--dsjson", "--ccb_explore_adf"},
    {"--quiet", "--no_stdin", "-b", "12", "--dsjson", "--cb_explore_adf", "--chain_hash"},
    {"--quiet", "--no_stdin", "-b", "12", "--dsjson", "--cats", "4", "--min_value=185", "--max_value=23959",
        "--bandwidth", "1"},
    {"--quiet", "--no_stdin", "-b", "12", "--dsjson", "--cb", "4"},
};
static const size_t kFirstDsjson = 7;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  if (size < 1) { return -1; }
  const size_t cfg = data[0] % kConfigs.size();
  std::string line(reinterpret_cast<const char*>(data) + 1, size - 1);

  try
  {
    auto vw = VW::initialize(VW::make_unique<VW::config::options_cli>(kConfigs[cfg]));
    VW::multi_ex examples;
    examples.push_back(&VW::get_unused_example(vw.get()));
    auto factory = std::bind(VW::get_unused_example, vw.get());
    if (cfg < kFirstDsjson)
    {
      VW::parsers::json::read_line_json<true>(*vw, examples, &line[0], line.size(), factory);
    }
    else
    {
      VW::parsers::json::decision_service_interaction interaction;
      VW::parsers::json::read_line_decision_service_json<true>(
          *vw, examples, &line[0], line.size(), false, factory, &interaction);
    }
    VW::setup_examples(*vw, examples);
    VW::finish_example(*vw, examples);
  }
  catch (...)
  {
    return -1;
  }
  return 0;
}
