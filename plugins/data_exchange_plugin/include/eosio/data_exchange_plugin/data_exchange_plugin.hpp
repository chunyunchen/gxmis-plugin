/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
// #include <eosio/wallet_plugin/wallet_plugin.hpp>

namespace eosio {

using namespace appbase;

typedef std::unique_ptr<class data_exchange_plugin_impl> data_exchange_ptr;

struct wallet_params {
	std::string wallet_name;        // 钱包名称
    std::string wallet_pwd;         // 钱包密码
};
struct push_action_params {
    wallet_params wallet;			// 账户拥有的密钥对应的钱包名称和解锁密码
    std::string account;           	// 交易合约所在的账户，如：eosio.token
    std::string action_name;	   	// action的名称
    std::string action_args; 		// action的参数
    vector<std::string> permissions;// 用于交易签名的授权账号和许可级别，如eosio@active
};

class data_exchange_plugin : public appbase::plugin<data_exchange_plugin> {
public:
   data_exchange_plugin();
   virtual ~data_exchange_plugin();
 
   APPBASE_PLUGIN_REQUIRES((chain_plugin)(http_plugin))
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   // 提交action
   fc::variant push_action(const push_action_params& params);

private:
   data_exchange_ptr my;
};

} // namespace eosio

FC_REFLECT(eosio::wallet_params, (wallet_name)(wallet_pwd));
FC_REFLECT(eosio::push_action_params, (wallet)(account)(action_name)(action_args)(permissions));
