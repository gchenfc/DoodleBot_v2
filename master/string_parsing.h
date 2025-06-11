#pragma once
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <string_view>
#include <type_traits>
#include <optional>

// Trim whitespace from the beginning and end of the string view
void trimFront(std::string_view& sv);
void trimBack(std::string_view& sv);
void trim(std::string_view& sv);
template<typename T>
bool starts_with(const std::string_view& sv, const std::string_view prefix);

// Parses a float from a string view, advancing the view. Returns nullopt on failure.
template<typename FloatingT>
std::optional<FloatingT> parseFloat(std::string_view& sv);

// Parses an integer from a string view, advancing the view. Returns nullopt on failure.
template<typename IntegralT>
std::optional<IntegralT> parseInt(std::string_view& sv);

// Parses a list of comma-separated numbers into output arguments only if there is an exact match.
// Returns false on failure and does not modify any outputs.
template<typename T1, typename... Ts>
bool parseNumbers(std::string_view& sv, T1& out1, Ts&... outs);

template<typename... Args>
bool parseNumbersNotInPlace(std::string_view line, Args&... args) {
  return parseNumbers(line, args...);
}

/**************************************************************************/

void trimFront(std::string_view& sv) {
  while (!sv.empty() && isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
}
void trimBack(std::string_view& sv) {
  while (!sv.empty() && isspace(static_cast<unsigned char>(sv.back()))) sv.remove_suffix(1);
  if (!sv.empty() && sv.back() == 0) sv.remove_suffix(1);
}
void trim(std::string_view& sv) {
  trimFront(sv);
  trimBack(sv);
}

bool starts_with(const std::string_view& sv, const std::string_view prefix) {
  return sv.size() >= prefix.size() && (sv.substr(0, prefix.size()) == prefix);
}

template<typename T>
std::optional<T> parseFloat(std::string_view& sv) {
  trimFront(sv);
  if (sv.empty()) return std::nullopt;

  bool negative = false;
  if (sv.front() == '-') {
    negative = true;
    sv.remove_prefix(1);
  } else if (sv.front() == '+') {
    sv.remove_prefix(1);
  }

  T value = 0.0;
  bool found_digit = false;

  while (!sv.empty() && sv.front() >= '0' && sv.front() <= '9') {
    found_digit = true;
    value = value * 10.0 + (sv.front() - '0');
    sv.remove_prefix(1);
  }

  if (!sv.empty() && sv.front() == '.') {
    sv.remove_prefix(1);
    T frac = 0.0;
    T base = 0.1;
    bool frac_digit = false;
    while (!sv.empty() && sv.front() >= '0' && sv.front() <= '9') {
      frac_digit = true;
      frac += (sv.front() - '0') * base;
      base *= 0.1;
      sv.remove_prefix(1);
    }
    value += frac;
    found_digit = found_digit || frac_digit;
  }

  return found_digit ? std::make_optional(negative ? -value : value) : std::nullopt;
}

template<typename T>
std::optional<T> parseInt(std::string_view& sv) {
  trimFront(sv);
  if (sv.empty()) return std::nullopt;

  bool negative = false;
  if constexpr (std::is_signed_v<T>) {
    if (sv.front() == '-') {
      negative = true;
      sv.remove_prefix(1);
    } else if (sv.front() == '+') {
      sv.remove_prefix(1);
    }
  }

  T value = 0;
  bool found_digit = false;
  while (!sv.empty() && sv.front() >= '0' && sv.front() <= '9') {
    found_digit = true;
    value = value * 10 + (sv.front() - '0');
    sv.remove_prefix(1);
  }

  return found_digit ? std::make_optional(negative ? -value : value) : std::nullopt;
}

template<typename T1, typename... Ts>
bool parseNumbers(std::string_view& sv, T1& out1, Ts&... outs) {
  using base_t = std::remove_cv_t<std::remove_reference_t<T1>>;
  std::optional<base_t> val;

  if constexpr (std::is_integral_v<base_t>) {
    val = parseInt<base_t>(sv);
  } else if constexpr (std::is_floating_point_v<base_t>) {
    val = parseFloat<base_t>(sv);
  } else {
    return false;
  }

  if (!val) return false;

  WebSerial.println(F("Checkpoint 1"));

  // Parse remaining
  if constexpr (sizeof...(outs) > 0) {
    trimFront(sv);
    if (sv.empty() || sv.front() != ',') return false;
    sv.remove_prefix(1);  // remove comma
    trimFront(sv);
    if (!parseNumbers(sv, outs...)) return false;
  } else {
    trim(sv);
    WebSerial.printf("CHekpoint 2: %u\n", sv.size());
    for (const auto& c : sv) {
      WebSerial.printf("checkpoint 3: %d\n", c);
    }
    WebSerial.write(reinterpret_cast<const uint8_t*>(sv.data()), sv.size());
    if (!sv.empty()) return false;
  }

  out1 = *val;
  return true;
}
