/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/data_exchange_api_plugin/data_exchange_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/io/json.hpp>

namespace eosio {namespace detail {
  struct data_exchange_api_plugin_empty {};
}}

FC_REFLECT(eosio::detail::data_exchange_api_plugin_empty, );

namespace eosio {
static appbase::abstract_plugin& _data_exchange_api_plugin = app().register_plugin<data_exchange_api_plugin>();

struct async_result_visitor : public fc::visitor<std::string>
{
   template <typename T>
   std::string operator()(const T &v) const
   {
      return fc::json::to_string(v);
   }
};

data_exchange_api_plugin::data_exchange_api_plugin(){}
data_exchange_api_plugin::~data_exchange_api_plugin(){}

void data_exchange_api_plugin::set_program_options(options_description&, options_description& cfg) { 
}
void data_exchange_api_plugin::plugin_initialize(const variables_map& options) { 
}

#define CALL(api_name, api_handle, call_name, INVOKE, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [&api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             INVOKE \
             cb(http_response_code, fc::json::to_string(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define CALL_ASYNC(api_name, api_handle, api_namespace, call_name, call_result, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [api_handle](string, string body, url_response_callback cb) mutable { \
      if (body.empty()) body = "{}"; \
      api_handle.validate(); \
      auto& dex_plugin = app().get_plugin<data_exchange_plugin>();\
      auto trx_signed = dex_plugin.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>());\
      api_handle.push_transaction(trx_signed.get_object(),\
         [cb, body](const fc::static_variant<fc::exception_ptr, call_result>& result){\
            if (result.contains<fc::exception_ptr>()) {\
               try {\
                  result.get<fc::exception_ptr>()->dynamic_rethrow_exception();\
               } catch (...) {\
                  http_plugin::handle_exception(#api_name, #call_name, body, cb);\
               }\
            } else {\
               cb(http_response_code, result.visit(async_result_visitor()));\
            }\
         });\
   }\
}

#define INVOKE_R_R(api_handle, call_name, in_param) \
     auto result = api_handle.call_name(fc::json::from_string(body).as<in_param>());

#define INVOKE_R_R_R_R(api_handle, call_name, in_param0, in_param1, in_param2) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     auto result = api_handle.call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>(), vs.at(2).as<in_param2>());

#define INVOKE_R_V(api_handle, call_name) \
     auto result = api_handle.call_name();

#define INVOKE_V_R(api_handle, call_name, in_param) \
     api_handle.call_name(fc::json::from_string(body).as<in_param>()); \
     eosio::detail::data_exchange_api_plugin_empty result;

#define TRX_CALL_ASYNC(call_name, call_result, http_response_code) CALL_ASYNC(exchange, rw_api, eosio, call_name, call_result, http_response_code)

void data_exchange_api_plugin::plugin_startup() {
   ilog( "starting data_exchange_api_plugin" );

   auto rw_api = app().get_plugin<chain_plugin>().get_read_write_api();\

   app().get_plugin<http_plugin>().add_api({
      TRX_CALL_ASYNC(push_action, chain_apis::read_write::push_transaction_results, 202)
   });	

}

void data_exchange_api_plugin::plugin_shutdown() {
}

}
