#include "currency_rate_provider.h"

#include <curl/curl.h>

#include <cctype>
#include <memory>
#include <regex>
#include <string>
#include <utility>

#include "currency_catalog.h"
#include "console_calc/value_format.h"

namespace console_calc {

namespace {

constexpr std::string_view k_rates_endpoint = "https://api.frankfurter.app/latest?from=NOK&to=";

class CurlHandle {
public:
    CurlHandle() : handle_(curl_easy_init()) {}
    ~CurlHandle() {
        if (handle_ != nullptr) {
            curl_easy_cleanup(handle_);
        }
    }

    CurlHandle(const CurlHandle&) = delete;
    CurlHandle& operator=(const CurlHandle&) = delete;

    [[nodiscard]] CURL* get() const { return handle_; }

private:
    CURL* handle_ = nullptr;
};

class CurlGlobalGuard {
public:
    CurlGlobalGuard() : initialized_(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK) {}
    ~CurlGlobalGuard() {
        if (initialized_) {
            curl_global_cleanup();
        }
    }

    CurlGlobalGuard(const CurlGlobalGuard&) = delete;
    CurlGlobalGuard& operator=(const CurlGlobalGuard&) = delete;

    [[nodiscard]] bool initialized() const { return initialized_; }

private:
    bool initialized_ = false;
};

size_t append_response(char* data, size_t size, size_t count, void* user_data) {
    auto* output = static_cast<std::string*>(user_data);
    output->append(data, size * count);
    return size * count;
}

[[nodiscard]] std::string build_request_url(std::span<const std::string_view> currencies) {
    std::string url(k_rates_endpoint);
    for (std::size_t index = 0; index < currencies.size(); ++index) {
        if (index != 0) {
            url += ',';
        }
        url += currencies[index];
    }
    return url;
}

[[nodiscard]] std::optional<double> parse_rate(std::string_view body, std::string_view currency) {
    const std::regex pattern(
        "\"" + std::string(currency) + "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)");
    std::cmatch match;
    if (!std::regex_search(body.begin(), body.end(), match, pattern)) {
        return std::nullopt;
    }

    try {
        return std::stod(match[1].str());
    } catch (...) {
        return std::nullopt;
    }
}

[[nodiscard]] std::string uppercase_code(std::string_view code) {
    std::string upper;
    upper.reserve(code.size());
    for (const char ch : code) {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }
    return upper;
}

class FrankfurterCurrencyRateProvider final : public CurrencyRateProvider {
public:
    CurrencyFetchResult fetch_nok_rates(std::span<const std::string_view> currencies,
                                        std::chrono::milliseconds timeout) override {
        static CurlGlobalGuard curl_guard;
        if (!curl_guard.initialized()) {
            return {.error = "currency provider initialization failed"};
        }

        CurlHandle curl;
        if (curl.get() == nullptr) {
            return {.error = "currency request initialization failed"};
        }

        std::string response;
        const std::string url = build_request_url(currencies);
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, append_response);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, static_cast<long>(timeout.count()));
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT_MS,
                         static_cast<long>(timeout.count()));
        curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "console_calc/0.1");

        const CURLcode curl_result = curl_easy_perform(curl.get());
        if (curl_result != CURLE_OK) {
            return {.error = curl_easy_strerror(curl_result)};
        }

        long response_code = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code < 200 || response_code >= 300) {
            return {.error = "currency request returned HTTP " + std::to_string(response_code)};
        }

        CurrencyRateTable rates;
        for (const std::string_view currency : currencies) {
            const std::string upper = uppercase_code(currency);
            const auto parsed_rate = parse_rate(response, upper);
            if (!parsed_rate.has_value()) {
                return {.error = "currency response did not include " + upper};
            }
            rates.emplace(std::string(currency), *parsed_rate);
        }

        return {.rates = std::move(rates)};
    }
};

}  // namespace

std::unique_ptr<CurrencyRateProvider> make_default_currency_rate_provider() {
    return std::make_unique<FrankfurterCurrencyRateProvider>();
}

void apply_currency_rate_definitions(DefinitionTable& definitions,
                                     const CurrencyRateTable& rates) {
    for (const auto& entry : k_console_currency_catalog) {
        const auto found = rates.find(std::string(entry.lower_code));
        if (found == rates.end()) {
            continue;
        }

        const std::string direct_name = "nok2" + std::string(entry.lower_code);
        const std::string inverse_name = std::string(entry.lower_code) + "2nok";
        definitions[direct_name] = UserDefinition{format_scalar(found->second)};
        definitions[inverse_name] = UserDefinition{format_scalar(1.0 / found->second)};
    }
}

}  // namespace console_calc
