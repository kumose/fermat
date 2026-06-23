#include <fermat/uri.h>
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;

struct parse_result {
  std::string result;
  std::string href;
  uint32_t type;
  fermat::uri::UrlComponents components;
};

parse_result parse(const std::string &input) {
  auto out = fermat::uri::parse<fermat::uri::UrlComponents>(input);
  parse_result result;
  if (!out.has_value()) {
    result.result = "fail";
  } else {
    result.result = "success";
    result.href = std::string(out->get_href());
    result.type = out->type;
    result.components = out->get_components();
  }
  return result;
}

EMSCRIPTEN_BINDINGS(url_components) {
  class_<parse_result>("Result")
      .property("result", &parse_result::result)
      .property("href", &parse_result::href)
      .property("type", &parse_result::type)
      .property("components", &parse_result::components);
  class_<fermat::uri::UrlComponents>("URLComponents")
      .property("protocol_end", &fermat::uri::UrlComponents::protocol_end)
      .property("username_end", &fermat::uri::UrlComponents::username_end)
      .property("host_start", &fermat::uri::UrlComponents::host_start)
      .property("host_end", &fermat::uri::UrlComponents::host_end)
      .property("port", &fermat::uri::UrlComponents::port)
      .property("pathname_start", &fermat::uri::UrlComponents::pathname_start)
      .property("search_start", &fermat::uri::UrlComponents::search_start)
      .property("hash_start", &fermat::uri::UrlComponents::hash_start);

  function("parse", &parse);
}