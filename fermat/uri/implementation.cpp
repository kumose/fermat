#include <string_view>

#include <fermat/uri.h>
#include <fermat/uri/common_defs.h>
#include <fermat/uri/parser.h>
#include <fermat/uri/url.h>
#include <fermat/uri/url_aggregator.h>

namespace fermat::uri {
    template<class result_type>
    [[nodiscard]] fermat::expected<result_type, fermat::uri::errors> parse(
        std::string_view input, const result_type *base_url) {
        result_type u =
                fermat::uri::parser::parse_url_impl<result_type, true>(input, base_url);
        if (!u.is_valid) {
            return fermat::unexpected(errors::generic_error);
        }
        return u;
    }

    template fermat::uri::result<Url> parse<Url>(std::string_view input,
                                                 const Url *base_url);

    template fermat::uri::result<UrlAggregator> parse<UrlAggregator>(
        std::string_view input, const UrlAggregator *base_url);

    fermat::KString href_from_file(std::string_view input) {
        // This is going to be much faster than constructing a URL.
        fermat::KString tmp_buffer;
        std::string_view internal_input;
        if (unicode::has_tabs_or_newline(input)) {
            tmp_buffer = input;
            helpers::remove_ascii_tab_or_newline(tmp_buffer);
            internal_input = tmp_buffer;
        } else {
            internal_input = input;
        }
        fermat::KString path;
        if (internal_input.empty()) {
            path = "/";
        } else if ((internal_input[0] == '/') || (internal_input[0] == '\\')) {
            helpers::parse_prepared_path(internal_input.substr(1),
                                         fermat::uri::scheme::type::FILE, path);
        } else {
            helpers::parse_prepared_path(internal_input, fermat::uri::scheme::type::FILE, path);
        }
        return "file://" + path;
    }

    bool can_parse(std::string_view input, const std::string_view *base_input) {
        fermat::uri::UrlAggregator base_aggregator;
        fermat::uri::UrlAggregator *base_pointer = nullptr;

        if (base_input != nullptr) {
            base_aggregator = fermat::uri::parser::parse_url_impl<fermat::uri::UrlAggregator, false>(
                *base_input, nullptr);
            if (!base_aggregator.is_valid) {
                return false;
            }
            base_pointer = &base_aggregator;
        }

        fermat::uri::UrlAggregator result =
                fermat::uri::parser::parse_url_impl<fermat::uri::UrlAggregator, false>(input,
                    base_pointer);
        return result.is_valid;
    }

    [[nodiscard]] fermat::KString to_string(fermat::uri::encoding_type type) {
        switch (type) {
            case fermat::uri::encoding_type::UTF8:
                return "UTF-8";
            case fermat::uri::encoding_type::UTF_16LE:
                return "UTF-16LE";
            case fermat::uri::encoding_type::UTF_16BE:
                return "UTF-16BE";
            default:
                TURBO_UNREACHABLE();
        }
    }
} // namespace fermat::uri
