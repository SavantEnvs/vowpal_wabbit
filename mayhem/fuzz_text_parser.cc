// Fuzz VW's text-format example parser and the learners it feeds. The first input byte selects a
// workspace configuration (label type / reduction stack); the rest is newline-separated example text.
#include "vw/config/options_cli.h"
#include "vw/core/example.h"
#include "vw/core/learner.h"
#include "vw/core/memory.h"
#include "vw/core/multi_ex.h"
#include "vw/core/parser.h"
#include "vw/core/vw.h"
#include "vw/text_parser/parse_example_text.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

static const std::vector<std::vector<std::string>> kConfigs = {
    {"--quiet", "--no_stdin", "-b", "12"},
    {"--quiet", "--no_stdin", "-b", "12", "--oaa", "4"},
    {"--quiet", "--no_stdin", "-b", "12", "--csoaa", "4"},
    {"--quiet", "--no_stdin", "-b", "12", "--cb", "4"},
    {"--quiet", "--no_stdin", "-b", "12", "--cb_explore_adf"},
    {"--quiet", "--no_stdin", "-b", "12", "--ccb_explore_adf"},
    {"--quiet", "--no_stdin", "-b", "12", "--multilabel_oaa", "4"},
    {"--quiet", "--no_stdin", "-b", "12", "--cb_adf", "-q", "::"},
    {"--quiet", "--no_stdin", "-b", "12", "--nn", "2", "--ngram", "2", "--skips", "1"},
    {"--quiet", "--no_stdin", "-b", "12", "--lda", "3", "--lda_D", "10"},
    {"--quiet", "--no_stdin", "-b", "12", "--strict_parse", "--noconstant", "--interactions", "ab"},
    {"--quiet", "--no_stdin", "-b", "12", "--cb_explore_adf", "--squarecb"},
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  if (size < 1) { return -1; }
  const auto& args = kConfigs[data[0] % kConfigs.size()];
  const std::string text(reinterpret_cast<const char*>(data) + 1, size - 1);

  try
  {
    auto vw = VW::initialize(VW::make_unique<VW::config::options_cli>(args));
    VW::multi_ex examples;
    VW::parsers::text::read_lines(vw.get(), text, examples);
    VW::setup_examples(*vw, examples);
    // The driver never learns on an empty multi_ex.
    if (!examples.empty())
    {
      if (vw->l->is_multiline()) { vw->learn(examples); }
      else
      {
        for (auto* ex : examples) { vw->learn(*ex); }
      }
    }
    VW::finish_example(*vw, examples);
  }
  catch (...)
  {
    return -1;
  }
  return 0;
}
