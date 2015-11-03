/*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Product name: redemption, a FLOSS RDP proxy
*   Copyright (C) Wallix 2012-2015
*   Author(s): Christophe Grosjean, Javier Caverni, Xavier Dunat, Martin Potier,
*              Jonathan Poelen, Raphael Zhou, Meng Tan
*/

#ifndef REDEMPTION_CAPTURE_UTILS_MATCH_FINDER_HPP
#define REDEMPTION_CAPTURE_UTILS_MATCH_FINDER_HPP

#include "auth_api.hpp"
#include "regex.hpp"
#include "log.hpp"

#include <memory>
#include <cstring>

namespace utils {

using std::size_t;

struct MatchFinder
{
public:
    struct NamedRegex {
        re::Regex   regex;
        std::string name;
    };

    class NamedRegexArray {
        std::unique_ptr<NamedRegex[]> regexes;
        size_t                        len;

    public:
        typedef NamedRegex * iterator;

        NamedRegexArray()
        : len(0)
        {}

        void resize(size_t newlen)
        {
            this->regexes.reset(new NamedRegex[newlen]);
            this->len = newlen;
        }

        void shrink(size_t newlen)
        {
            if (!newlen) {
                this->regexes.reset();
            }
            this->len = newlen;
        }

        NamedRegex * begin() const
        { return this->regexes.get(); }

        NamedRegex * end() const
        { return this->regexes.get() + this->len; }
    };

    enum ConfigureRegexes {
          OCR         = 0
        , KBD_INPUT   = 1
    };

    static void configure_regexes(ConfigureRegexes conf_regex, const char * filters_list,
                                  NamedRegexArray & regexes_filter_ref, int verbose)
    {
        char * tmp_filters = new(std::nothrow) char[strlen(filters_list) + 1];
        if (!tmp_filters) {
            return ; // insufficient memory
        }

        std::unique_ptr<char[]> auto_free(tmp_filters);

        strcpy(tmp_filters, filters_list);

        char     * separator;
        char     * filters[64];
        unsigned   filter_number = 0;

        if (verbose) {
            LOG(LOG_INFO, "filters=\"%s\"", tmp_filters);
        }

        while (*tmp_filters) {
            if ((*tmp_filters == '\x01') || (*tmp_filters == '\t') || (*tmp_filters == ' ')) {
                tmp_filters++;
                continue;
            }

            separator = strchr(tmp_filters, '\x01');
            if (separator) {
                *separator = 0;
            }

            if (verbose) {
                LOG(LOG_INFO, "filter=\"%s\"", tmp_filters);
            }

            if (((conf_regex == ConfigureRegexes::OCR) && ((*tmp_filters != '$') ||
                                                           (strcasestr(tmp_filters, "$ocr:") == tmp_filters) ||
                                                           (strcasestr(tmp_filters, "$kbd-ocr:") == tmp_filters) ||
                                                           (strcasestr(tmp_filters, "$ocr-kbd:") == tmp_filters))) ||
                ((conf_regex == ConfigureRegexes::KBD_INPUT) && ((strcasestr(tmp_filters, "$kbd:") == tmp_filters) ||
                                                                 (strcasestr(tmp_filters, "$kbd-ocr:") == tmp_filters) ||
                                                                 (strcasestr(tmp_filters, "$ocr-kbd:") == tmp_filters)))) {
                if (((conf_regex == ConfigureRegexes::OCR) && (*tmp_filters == '$')) ||
                    (conf_regex == ConfigureRegexes::KBD_INPUT)) {
                    if (*(tmp_filters + 4) == ':') {
                        tmp_filters += 5;   // strlen("$ocr:") or strlen("$kdb:")
                    }
                    else {
                        REDASSERT(*(tmp_filters + 8) == ':');
                        tmp_filters += 9;   // strlen("$kbd-ocr:") or strlen("$ocr-kbd:")
                    }
                }

                filters[filter_number] = tmp_filters;
                filter_number++;
                if (filter_number >= (sizeof(filters) / sizeof(filters[0]))) {
                    break;
                }
            }

            if (!separator) {
                break;
            }

            tmp_filters = separator + 1;
        }

        if (verbose) {
            LOG(LOG_INFO, "filter number=%d", filter_number);
        }

        if (filter_number) {
            regexes_filter_ref.resize(filter_number);
            NamedRegex * pregex = regexes_filter_ref.begin();
            for (unsigned i = 0; i < filter_number; i++) {
                if (verbose) {
                    LOG(LOG_INFO, "Regex=\"%s\"", filters[i]);
                }
                pregex->name = filters[i];
                pregex->regex.reset(filters[i]);
                if (pregex->regex.message_error()) {
                    TODO("notification that the regex is too complex for us");
                    LOG(LOG_ERR, "Regex: %s err %s at position %u" , filters[i],
                        pregex->regex.message_error(), pregex->regex.position_error());
                }
                else {
                    ++pregex;
                }
            }
            regexes_filter_ref.shrink(pregex - regexes_filter_ref.begin());
        }
    }

    static void report(auth_api * authentifier, const char * reason,
        ConfigureRegexes conf_regex, const char * pattern, const char * data) {
        char message[4096];

        snprintf(message, sizeof(message), "$%s:%s|%s",
            ((conf_regex == ConfigureRegexes::OCR) ? "ocr" : "kbd" ), pattern, data);

        authentifier->report(reason, message);
    }
};

}

#endif