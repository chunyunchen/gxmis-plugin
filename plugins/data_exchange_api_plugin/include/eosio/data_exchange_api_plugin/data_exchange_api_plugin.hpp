/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/data_exchange_plugin/data_exchange_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>

namespace eosio {

using namespace appbase;

/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class data_exchange_api_plugin : public appbase::plugin<data_exchange_api_plugin> {
public:
   data_exchange_api_plugin();
   virtual ~data_exchange_api_plugin();
 
   APPBASE_PLUGIN_REQUIRES((data_exchange_plugin)(http_plugin))
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
};

}
