/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include "help_text.hpp"
#include "localize.hpp"
#include <iostream>
#include <regex>
#include <fc/io/json.hpp>

using namespace evt::client::localize;

const char* help_regex_error = _("Error locating help text: ${code} ${what}"); 

const std::vector<std::pair<const char*, std::vector<const char *>>> error_help_text {};

auto smatch_to_variant(const std::smatch& smatch) {
   auto result = fc::mutable_variant_object();
   for(size_t index = 0; index < smatch.size(); index++) {
      auto name = boost::lexical_cast<std::string>(index);
      if (smatch[index].matched) {
         result = result(name, smatch.str(index));
      } else {
         result = result(name, "");
      }
   }

   return result;
};


const std::map<int64_t, std::string> error_advice = {};


namespace evt { namespace client { namespace help {
bool print_recognized_errors(const fc::exception& e, const bool verbose_errors) {
   // evt recognized error code is from 3000000 to 3999999
   // refer to libraries/chain/include/evt/chain/exceptions.hpp
   if (e.code() >= 3000000 && e.code() <= 3999999) {
      std::string advice, explanation, stack_trace;

      // Get advice, if any
      const auto advice_itr = error_advice.find(e.code());
      if (advice_itr != error_advice.end()) advice = advice_itr->second;

      // Get explanation from log, if any
      for (auto &log : e.get_log()) {
         // Check if there's a log to display
         if (!log.get_format().empty()) {
            // Localize the message as needed
            explanation += "\n" + localized_with_variant(log.get_format().data(), log.get_data());
         } else if (log.get_data().size() > 0 && verbose_errors) {
            // Show data-only log only if verbose_errors option is enabled
            explanation += "\n" + fc::json::to_string(log.get_data());
         }
         // Check if there's stack trace to be added
         if (!log.get_context().get_method().empty() && verbose_errors) {
            stack_trace += "\n" +
                           log.get_context().get_file() +  ":" +
                           fc::to_string(log.get_context().get_line_number())  + " " +
                           log.get_context().get_method();
         }
      }
      // Append header
      if (!explanation.empty()) explanation = std::string("Error Details:") + explanation;
      if (!stack_trace.empty()) stack_trace = std::string("Stack Trace:") + stack_trace;

      std::cerr << "\033[31m" << "Error " << e.code() << ": " << e.what() << "\033[0m";
      if (!advice.empty()) std::cerr << "\n" << "\033[32m" << advice << "\033[0m";
      if (!explanation.empty()) std::cerr  << "\n" << "\033[33m" << explanation << "\033[0m";
      if (!stack_trace.empty()) std::cerr  << "\n" << stack_trace;
      std::cerr << std::endl;
      return true;
   }
   return false;
}

bool print_help_text(const fc::exception& e) {
   bool result = false;
   // Large input strings to std::regex can cause SIGSEGV, this is a known bug in libstdc++.
   // See https://stackoverflow.com/questions/36304204/%D0%A1-regex-segfault-on-long-sequences
   auto detail_str = e.to_detail_string();
   // 2048 nice round number. Picked for no particular reason. Bug above was reported for 22K+ strings.
   if (detail_str.size() > 2048) return result;
   try {
      for (const auto& candidate : error_help_text) {
         auto expr = std::regex {candidate.first};
         std::smatch matches;
         if (std::regex_search(detail_str, matches, expr)) {
            auto args = smatch_to_variant(matches);
            for (const auto& msg: candidate.second) {
               std::cerr << localized_with_variant(msg, args) << std::endl;
            }
            result = true;
            break;
         }
      }
   } catch (const std::regex_error& e ) {
      std::cerr << localized(help_regex_error, ("code", e.code())("what", e.what())) << std::endl;
   }

   return result;
}

}}}
