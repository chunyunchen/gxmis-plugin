/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include <eosio/chain/action.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <regex>

#include <eosio/data_exchange_plugin/httpc.hpp>
#include <eosio/data_exchange_plugin/data_exchange_plugin.hpp>

namespace eosio
{
static appbase::abstract_plugin &_data_exchange_plugin = app().register_plugin<data_exchange_plugin>();

using namespace eosio::chain;

struct async_result_visitor : public fc::visitor<std::string>
{
   template <typename T>
   std::string operator()(const T &v) const
   {
      return fc::json::to_string(v);
   }
};

class data_exchange_plugin_impl
{
 private:
   class wallet_guard
   {
    public:
      wallet_guard(data_exchange_plugin_impl &p, const eosio::wallet_params &wp) : _parent(p), _wp(wp)
      {
         unlock();
      }
      ~wallet_guard()
      {
         lock();
      }

    private:
      void unlock()
      {
         _parent.call(client::http::wallet_open, fc::variant(_wp.wallet_name));

         fc::variants vs = {fc::variant(_wp.wallet_name), fc::variant(_wp.wallet_pwd)};
         _parent.call(client::http::wallet_unlock, vs);
      }
      void lock()
      {
         _parent.call(client::http::wallet_lock, fc::variant(_wp.wallet_name));
      }

    private:
      data_exchange_plugin_impl &_parent;
      const eosio::wallet_params &_wp;
   };

 public:
   // 提交action
   fc::variant push_action(const push_action_params &params)
   {
      wallet_guard wg(*this, params.wallet);
      auto accountPermissions = get_account_permissions(params.permissions);
      fc::variant action_args_var;
      if (!params.action_args.empty())
      {
         std::string str_action_args(params.action_args);
         combine_action_args(str_action_args);
         try
         {
            action_args_var = json_from_file_or_string(str_action_args, fc::json::relaxed_parser);
         }
         EOS_RETHROW_EXCEPTIONS(action_type_exception, "Fail to parse action JSON data='${action_args}'", ("action_args", str_action_args))
      }
      return send_actions({chain::action{accountPermissions, params.account, params.action_name, variant_to_bin(params.account, params.action_name, action_args_var)}});
   }

 public:
   data_exchange_plugin_impl()
   {
      context = eosio::client::http::create_http_context();
   }
   void set_wallet_url(const std::string &wurl)
   {
      _wallet_url = wurl;
   }

 private:
   const fc::microseconds abi_serializer_max_time = fc::milliseconds(500);
   std::string _wallet_url;
   client::http::http_context context;
   vector<string> headers;

 private:
   using get_abi_params = eosio::chain_apis::read_only::get_abi_params;
   using get_info_params = eosio::chain_apis::read_only::get_info_params;
   using get_block_params = eosio::chain_apis::read_only::get_block_params;
   using get_required_keys_params = eosio::chain_apis::read_only::get_required_keys_params;
   using push_transaction_params = eosio::chain_apis::read_write::push_transaction_params;
   using push_transaction_results = eosio::chain_apis::read_write::push_transaction_results;
   using get_required_keys_result = eosio::chain_apis::read_only::get_required_keys_result;

   fc::variant send_actions(std::vector<chain::action> &&actions, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none)
   {
      auto result = push_actions(move(actions), extra_kcpu, compression);

      return result;
   }

   vector<chain::permission_level> get_account_permissions(const vector<string> &permissions)
   {
      auto fixedPermissions = permissions | boost::adaptors::transformed([](const string &p) {
                                 vector<string> pieces;
                                 split(pieces, p, boost::algorithm::is_any_of("@"));
                                 if (pieces.size() == 1)
                                    pieces.push_back("active");
                                 return chain::permission_level{.actor = pieces[0], .permission = pieces[1]};
                              });
      vector<chain::permission_level> accountPermissions;
      boost::range::copy(fixedPermissions, back_inserter(accountPermissions));
      return accountPermissions;
   }

   fc::variant push_actions(std::vector<chain::action> &&actions, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none)
   {
      signed_transaction trx;
      trx.actions = std::forward<decltype(actions)>(actions);

      return may_push_transaction(trx, extra_kcpu, compression);
   }

   fc::variant may_push_transaction(signed_transaction &trx, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none)
   {
      auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();

      auto info = ro_api.get_info(get_info_params{});

      if (trx.signatures.size() == 0)
      { // #5445 can't change txn content if already signed
         const auto tx_expiration = fc::seconds(30);
         trx.expiration = info.head_block_time + tx_expiration;

         // Set tapos, default to last irreversible block if it's not specified by the user
         block_id_type ref_block_id = info.last_irreversible_block_id;

         string tx_ref_block_num_or_id("");
         try
         {
            fc::variant ref_block;

            if (!tx_ref_block_num_or_id.empty())
            {
               get_block_params gbp{tx_ref_block_num_or_id};
               ref_block = ro_api.get_block(gbp);
               ref_block_id = ref_block["id"].as<block_id_type>();
            }
         }
         EOS_RETHROW_EXCEPTIONS(invalid_ref_block_exception, "Invalid reference block num or id: ${block_num_or_id}", ("block_num_or_id", tx_ref_block_num_or_id));
         trx.set_reference_block(ref_block_id);

         bool tx_force_unique = false;
         if (tx_force_unique)
         {
            trx.context_free_actions.emplace_back(generate_nonce_action());
         }

         trx.max_cpu_usage_ms = 0;
         trx.max_net_usage_words = 0;
         trx.delay_sec = 0;
      }

      bool tx_skip_sign = false;
      bool tx_dont_broadcast = true;
      bool tx_return_packed = true;

      if (!tx_skip_sign)
      {
         auto required_keys = determine_required_keys(trx);
         sign_transaction(trx, required_keys, info.chain_id);
      }

      if (!tx_dont_broadcast)
      {
         fc::variant var_packed_tx(packed_transaction(trx, compression));
         std::string str_packed_tx(fc::json::to_string(var_packed_tx));

         push_transaction_params var_obj_params = fc::json::from_string(str_packed_tx).as<push_transaction_params>();
         fc::variant var_result;

         auto rw_api = app().get_plugin<chain_plugin>().get_read_write_api();
         // TODO: 解决异步发送交易到区块，并获得异步返回结果,
         //       现在的处理方式会导致程序魔幻退出
         rw_api.push_transaction(var_obj_params,
                                 [&var_result](const fc::static_variant<fc::exception_ptr, push_transaction_results> &result) {
                                    if (result.contains<fc::exception_ptr>())
                                    {
                                       try
                                       {
                                          result.get<fc::exception_ptr>()->dynamic_rethrow_exception();
                                       }
                                       catch (...)
                                       {
                                          ;
                                       }
                                    }
                                    else
                                    {
                                       result.visit(eosio::async_result_visitor());
                                       to_variant(result, var_result);
                                    }
                                 });
         // TODO
         return var_result[1];
      }
      else
      {
         if (!tx_return_packed)
         {
            return fc::variant(trx);
         }
         else
         {
            return fc::variant(packed_transaction(trx, compression));
         }
      }
   }

   chain::action generate_nonce_action()
   {
      return chain::action({}, config::null_account_name, "nonce", fc::raw::pack(fc::time_point::now().time_since_epoch().count()));
   }

   void combine_action_args(std::string &args)
   {
      regex rx("([:,])");
      std::string rfmt("\"$1\"");
      args = "\"" + args + "\"";
      args = std::regex_replace(args, rx, rfmt);

      std::regex rtrim_quotas("\" *(true|false|-?\\d{1,18}) *\"");
      args = std::regex_replace(args, rtrim_quotas, "$1");

      regex rcolon(":");
      if (regex_search(args, rcolon))
      {
         args = "{" + args + "}";
      }
      else
      {
         args = "[" +  args + "]";
      }

      // 需要用自定义的转义字符将输入参数值中包含的冒号和分号变换
      // 参数中的逗号转换为'%j',冒号转换为'%l'
      regex r_escape_comma("%j");
      args = std::regex_replace(args, r_escape_comma, ","); 
      regex r_escape_colon("%l");
      args = std::regex_replace(args, r_escape_colon, ":");
   }

   void sign_transaction(signed_transaction &trx, get_required_keys_result &required_keys, const chain_id_type &chain_id)
   {
      fc::variants sign_args = {fc::variant(trx), fc::variant(required_keys.required_keys), fc::variant(chain_id)};
      const auto &signed_trx = call(client::http::wallet_sign_trx, sign_args);
      trx = signed_trx.as<signed_transaction>();
   }

   template <typename T>
   fc::variant call(const std::string &path, const T &v)
   {
      try
      {
         auto sp = std::make_unique<eosio::client::http::connection_param>(context, client::http::parse_url(_wallet_url) + path, true, headers);
         return eosio::client::http::do_http_call(*sp, fc::variant(v), false, false);
      }
      catch (boost::system::system_error &e)
      {
         throw client::http::connection_exception(fc::log_messages{FC_LOG_MESSAGE(error, e.what())});
      }
   }

   fc::variant call(const std::string &path) { return call(path, fc::variant()); }

   optional<abi_serializer> abi_serializer_resolver(const name &account)
   {
      static unordered_map<account_name, optional<abi_serializer>> abi_cache;
      auto it = abi_cache.find(account);
      if (it == abi_cache.end())
      {
         //auto result = call(get_abi_func, fc::mutable_variant_object("account_name", account));
         //auto abi_results = result.as<eosio::chain_apis::read_only::get_abi_results>();
         auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
         auto abi_results = ro_api.get_abi(get_abi_params{account});

         optional<abi_serializer> abis;
         if (abi_results.abi.valid())
         {
            abis.emplace(*abi_results.abi, abi_serializer_max_time);
         }
         else
         {
            std::cerr << "ABI for contract " << account.to_string() << " not found. Action data will be shown in hex only." << std::endl;
         }
         abi_cache.emplace(account, abis);

         return abis;
      }

      return it->second;
   };

   vector<chain::permission_level> get_account_permissions(const vector<string> &permissions, const chain::permission_level &default_permission)
   {
      if (permissions.empty())
         return vector<chain::permission_level>{default_permission};
      else
         return get_account_permissions(permissions);
   }

   bytes variant_to_bin(const account_name &account, const action_name &action, const fc::variant &action_args_var)
   {
      auto abis = abi_serializer_resolver(account);
      FC_ASSERT(abis.valid(), "No ABI found for ${contract}", ("contract", account));

      auto action_type = abis->get_action_type(action);
      FC_ASSERT(!action_type.empty(), "Unknown action ${action} in contract ${contract}", ("action", action)("contract", account));
      return abis->variant_to_binary(action_type, action_args_var, abi_serializer_max_time);
   }

   fc::variant json_from_file_or_string(const string &file_or_str, fc::json::parse_type ptype = fc::json::legacy_parser)
   {
      regex r("^[ \t]*[\{\[]");
      if (!regex_search(file_or_str, r) && fc::is_regular_file(file_or_str))
      {
         return fc::json::from_file(file_or_str, ptype);
      }
      else
      {
         return fc::json::from_string(file_or_str, ptype);
      }
   }

   get_required_keys_result determine_required_keys(const signed_transaction &trx)
   {
      const auto &public_keys = call(client::http::wallet_public_keys);

      auto public_keys_set = public_keys.as<flat_set<public_key_type>>();
      const get_required_keys_params args{fc::variant((transaction)trx), public_keys_set};

      auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
      auto required_keys = ro_api.get_required_keys(args);

      return required_keys;
   }
};

fc::variant data_exchange_plugin::push_action(const push_action_params &params)
{
   return my->push_action(params);
}

data_exchange_plugin::data_exchange_plugin() : my(new data_exchange_plugin_impl()) {}
data_exchange_plugin::~data_exchange_plugin() {}

void data_exchange_plugin::set_program_options(options_description &, options_description &cfg)
{
   cfg.add_options()("wallet-url", bpo::value<std::string>()->default_value("http://127.0.0.1"),
                     "The unix socket path of wallet or http host for HTTP RPC (absolute file path or http(s)://wallet-host:port)");
}

void data_exchange_plugin::plugin_initialize(const variables_map &options)
{
   try
   {
      if (options.count("wallet-url"))
      {
         my->set_wallet_url(options.at("wallet-url").as<std::string>());
      }
   }
   FC_LOG_AND_RETHROW()
}

void data_exchange_plugin::plugin_startup()
{
}

void data_exchange_plugin::plugin_shutdown()
{
}

} // namespace eosio
