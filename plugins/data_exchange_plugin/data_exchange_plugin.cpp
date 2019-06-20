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


class data_exchange_plugin_impl
{
private:
   class wallet_guard
   {
   public:
      // @param wltpwd: 钱包名与解锁密码对，格式：wallet:password[,wallet:password]
      wallet_guard(data_exchange_plugin_impl &p, const std::string &wltpwd) : _parent(p)
      {
         parse_wltpwd(wltpwd);
         unlock();
      }
      ~wallet_guard()
      {
         lock();
      }

   private:
      void unlock()
      {
         for (auto &wp : vec_wltpwd)
         {
            _parent.call(client::http::wallet_open, fc::variant(wp.name));

            fc::variants vs = {fc::variant(wp.name), fc::variant(wp.pwd)};
            _parent.call(client::http::wallet_unlock, vs);
         }
      }

      void lock()
      {
         for (auto &wp : vec_wltpwd)
         {
            _parent.call(client::http::wallet_lock, fc::variant(wp.name));
         }
      }

      void parse_wltpwd(const std::string &wltpwd)
      {
         vector<string> vec_wpwd;
         boost::split(vec_wpwd, wltpwd, boost::is_any_of("#|"));
         auto wlt_num = vec_wpwd.size() / 2;
         for (auto i = 0; i < wlt_num; i++)
         {
            vec_wltpwd.push_back({fc::trim(vec_wpwd[2 * i]), fc::trim(vec_wpwd[2 * i + 1])});
         }
      }

   private:
      data_exchange_plugin_impl &_parent;
      std::vector<eosio::wallet_params> vec_wltpwd;
   };

   enum class trx_broadcast_method : uint8_t
   {
       trx_only_signed,
       trx_broadcast_sync,
   };

public:
   // 提交action
   fc::variant push_action(const push_action_params &params)
   {
      std::vector<chain::action> actions;
      create_push_action(actions, params);

      wallet_guard wg(*this, params.wltpwd);

      return send_actions(move(actions), trx_broadcast_method::trx_only_signed);
   }

   // 挂单/限价交易
   fc::variant limit_order(const limit_order_params &params)
   {
      cell_deal cdeal{
          .create_time = fc::time_point::now(),
          .commodity_type = params.commodity_type,
          .commodity = params.commodity,
          .account = account_name(params.account),
          .amount = params.amount,
          .price = asset::from_string(params.price),
          .oper = (params.oper == 0 ? deal_op::ask_op : deal_op::bid_op)};

      // TODO: 触发挂单交易合约，将委托交易入库

      if (cdeal.oper == deal_op::bid_op)
      {
         prio_queue_bid.push(cdeal);
      }
      else
      {
         prio_queue_ask.push(cdeal);
      }

      return fc::variant();
   }

   // 自动撮合交易
   void match_limit_order()
   {
      deal_timer->expires_from_now(deal_interval);
      deal_timer->async_wait([this](boost::system::error_code ec) {
         match_limit_order();

         if (ec)
         {
            wlog("Peer deal ticked sooner than expected: ${m}", ("m", ec.message()));
         }

         do_match_limit_order();
      });
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

   void set_exchange_acct(const std::string &acct)
   {
     _exchange_contract_acct = acct;
   }

   void set_eosiosystem_acct(const std::string &acct)
   {
     _eosiosystem_contract_acct = acct;
   }

   void set_eosiotoken_acct(const std::string &acct)
   {
     _eosiotoken_contract_acct = acct;
   }

public:
   unique_ptr<boost::asio::steady_timer> deal_timer;

private:
   const fc::microseconds abi_serializer_max_time = fc::milliseconds(500);
   const boost::asio::steady_timer::duration deal_interval{std::chrono::milliseconds{50}};
   const std::string pwd_padding = "!";      // 密码填充
   const std::string commodity_any = "*";    // 表示任意物品
   const std::vector<uint8_t> pwd_seq_nums{1,2,3,5,6};         // 密码加密序列 
   std::string _wallet_url;
   std::string _exchange_contract_acct;     // 部署数据交易合约的账号,默认是"mis.exchange"
   std::string _eosiotoken_contract_acct;   // 部署eosio.token合约的账号,默认是"eosio.token"
   std::string _eosiosystem_contract_acct;  // 部署eosio.system合约的账号,默认是"eosio"
   client::http::http_context context;
   vector<string> headers;
   std::priority_queue<cell_deal> prio_queue_ask;
   std::priority_queue<cell_deal, std::vector<cell_deal>, cell_greater_cmp> prio_queue_bid;

private:
   using get_abi_params = eosio::chain_apis::read_only::get_abi_params;
   using get_info_params = eosio::chain_apis::read_only::get_info_params;
   using get_block_params = eosio::chain_apis::read_only::get_block_params;
   using get_required_keys_params = eosio::chain_apis::read_only::get_required_keys_params;
   using push_transaction_params = eosio::chain_apis::read_write::push_transaction_params;
   using push_transaction_results = eosio::chain_apis::read_write::push_transaction_results;
   using get_required_keys_result = eosio::chain_apis::read_only::get_required_keys_result;
   using get_table_rows_params = eosio::chain_apis::read_only::get_table_rows_params;
   using get_table_rows_result = eosio::chain_apis::read_only::get_table_rows_result;
   using auth_type = fc::static_variant<public_key_type, permission_level>;

   void do_match_limit_order()
   {
      get_table_rows_params price_query_param{
          .json = true,
          .code = _exchange_contract_acct,
          .scope = "dataex",
          .table = "matchedprice",
          .lower_bound = "", // 物品名先sha256，然后按字节翻转
          .upper_bound = "", // 物品名先sha256，然后按字节翻转
          .limit = 1,
          .key_type = "sha256",
          .index_position = "2" // 按物品名称排序
      };

      get_table_rows_params consigner_query_param{
          .json = true,
          .code = _exchange_contract_acct,
          .scope = "dataex",
          .limit = 100,
          .key_type = "i64",
          .index_position = "5" // 按价格排序
      };

      try
      {
         auto ro_chain = app().get_plugin<chain_plugin>().get_read_only_api();
         // 买方价高的优先撮合。价格相同，先挂单者先撮合。
         consigner_query_param.table = "buyer";
         consigner_query_param.reverse = true;
         auto buyer_query_result = ro_chain.get_table_rows(consigner_query_param);
         // 卖方价低的优先撮合。价格相同，先挂单者先撮合。
         consigner_query_param.table = "seller";
         consigner_query_param.reverse = false;
         auto seller_query_result = ro_chain.get_table_rows(consigner_query_param);

         auto br_size = buyer_query_result.rows.size();
         auto sr_size = seller_query_result.rows.size();
         if (br_size > 0 && 0 < sr_size)
         {
            auto buyer = buyer_query_result.rows[0].as<consignment_order>();
            auto seller = seller_query_result.rows[0].as<consignment_order>();
            for (int bi = 0, si = 0; sr_size > si || br_size > bi;)
            {
               // 如果最前部的买方和卖方都是同一账户，则依次匹配排位在后面的挂单
               if (buyer.consigner == seller.consigner)
               {
                  if (sr_size > si)
                  {
                     buyer.consigner = buyer_query_result.rows[0]["consigner"].get_string();
                     seller = seller_query_result.rows[si].as<consignment_order>();
                     seller.consigner = seller_query_result.rows[si]["consigner"].get_string();
                     ++si;
                  }
                  else
                  {
                     buyer = buyer_query_result.rows[bi].as<consignment_order>();
                     buyer.consigner = buyer_query_result.rows[bi]["consigner"].get_string();
                     seller.consigner = seller_query_result.rows[0]["consigner"].get_string();
                     ++bi;
                  }
               }
               else
               {
                  break;
               }
            }

            // 满足成交的条件
            if (seller.price <= buyer.price &&
                seller.consigner != buyer.consigner &&
                1 == seller.stat &&
                1 == buyer.stat)
            {
               auto str_sha256 = fc::sha256::hash(seller.commodity).str();
               price_query_param.lower_bound = reverse_string_by_sequence_step(str_sha256, {2, 32});
               price_query_param.upper_bound = price_query_param.lower_bound;
               auto price_query_result = ro_chain.get_table_rows(price_query_param);

               asset mprice;
               /* 成交价规则
               1）之前的成交价小于卖单价，本次成交价等于卖单价（相同挂单价，让时间早的优先成交）
               2）之前的成交价大于买单价，本次成交价等于买单价
               3）之前的成交价介于卖单价与买单价之间，本次成交价等于之前的成交价
            */
               if (price_query_result.rows.size() > 0)
               {
                  auto price_from_table = price_query_result.rows[0].as<matched_price>();
                  mprice = (price_from_table.price < seller.price ? seller.price
                                                                  : (price_from_table.price < buyer.price ? price_from_table.price
                                                                                                          : buyer.price));
               }
               else
               {
                  mprice = seller.price;
               }
               auto remaining_buyer = buyer.amount - buyer.amount_done;
               auto remaining_seller = seller.amount - seller.amount_done;
               auto matched_amount = (remaining_buyer < remaining_seller ? remaining_buyer : remaining_seller);

               //发起成交和转账action，如果是token交易，则需要加一次token转账action
               std::string memo = buyer.consigner + " buy " + buyer.commodity_type + " from " + seller.consigner;
               auto total = mprice;
               // 用循环加来防止溢出
               auto loop = matched_amount;
               while (--loop > 0)
               {
                  total += mprice;
               }

               std::vector<chain::action> vec_actions;

               vector<std::string> buyer_permission{buyer.consigner + "@active"};
               auto buyer_transfer = create_transfer(buyer_permission, buyer.consigner, seller.consigner, total, memo);
               vec_actions.push_back(buyer_transfer);

               // 卖token时，卖方需要转token到买方
    			uint8_t token_decimals = 1;
                std::string token = (commodity_any == buyer.commodity ? seller.commodity : buyer.commodity );

		        get_table_rows_result token_query_result;
                try 
                {
                    symbol s(1, token.c_str());
				    get_table_rows_params token_query_param{
				        .json = true,
				        .code = _eosiotoken_contract_acct,
				        .scope = token,
				        .table = "stat",
				        .limit = 1,
				    };
			        token_query_result = ro_chain.get_table_rows(token_query_param);
                }
                catch (...)
                {}

               std::string commodity_traded(token);

               if (token_query_result.rows.size() > 0)
               {
    			 token_decimals = asset::from_string(token_query_result.rows[0]["max_supply"].get_string()).decimals();

                  auto asset_str = combine_asset(token, token_decimals, matched_amount);
                  auto quantity = asset::from_string(asset_str);
                  vector<std::string> seller_permission{seller.consigner + "@active"};

                  auto seller_transfer = create_transfer(seller_permission, seller.consigner, buyer.consigner, quantity, memo);
                  vec_actions.push_back(seller_transfer);
               }
               else
               {
                // TODO：大数据手机号标签查询交易时，需要添加实际处理逻辑
                  commodity_traded = buyer.commodity;
               }

               vector<std::string> mlmto_permission{seller.consigner + "@active",
                                                    buyer.consigner + "@active",
                                                    _exchange_contract_acct + "@active"};
               auto mlmto = create_mlmto(mlmto_permission, buyer.oid, seller.oid, commodity_traded);
               vec_actions.push_back(mlmto);

               std::string decrypted_wltpwd("");
               decrypt_wltpwd(decrypted_wltpwd, ro_chain, _exchange_contract_acct, seller.consigner, buyer.consigner);

               wallet_guard wg(*this, decrypted_wltpwd);

               send_actions(move(vec_actions), trx_broadcast_method::trx_broadcast_sync);
            }
         }
      }
      catch (const account_query_exception &e)
      {
         //elog("${e}", ("e", e.to_detail_string()));
      }
      catch (const unsupported_abi_version_exception &e)
      {
         //elog("${e}", ("e", e.to_detail_string()));
      }
      catch (const wallet_locked_exception &e)
      {
         elog("${e}", ("e", e.to_detail_string()));
      }
      catch (const wallet_unlocked_exception &e)
      {
         //elog("${e}", ("e", e.to_detail_string()));
      }
      catch (const symbol_type_exception &e)
      {
        //elog("${e}", ("e", e.to_detail_string()));
      }
   }

   std::string reverse_string(const std::string &str_sha256, const uint8_t len = 2)
   {
      auto str_size = str_sha256.size();
      int slice_num = str_size / len;
      std::string reversed_str("");
      auto pos = 0;
      for (int i = 1; i <= slice_num; i++)
      {
         pos = str_size - len * i;
         reversed_str += str_sha256.substr(pos, len);
      }

      return reversed_str;
   }

   std::string combine_asset(const std::string& symbol_name, uint8_t decimals, float amount)
   {
       char token_asset_str[70]{'\0'};
       char format_str[17]{'\0'};

       snprintf(format_str, sizeof(format_str), "%%.%df %%s", decimals);
       snprintf(token_asset_str, sizeof(token_asset_str), format_str, amount, symbol_name.c_str());

       return token_asset_str;
   }

   std::string reverse_string_by_sequence_step(const std::string &src, std::vector<uint8_t> steps, bool reversed_steps = false)
   {
        if( reversed_steps )
        {
            std::reverse(steps.begin(), steps.end());
        }

        std::string reversed_str(src);
        for(auto sp: steps)
        {
            reversed_str = reverse_string(reversed_str, sp);
        }

        return reversed_str;
   }

   std::string encrypt_wltpwd(const std::string& src)
   {
      vector<string> vec_acct_wp;
      boost::split(vec_acct_wp, src, boost::is_any_of(","));

      vector<string> vec_wp;
      if(vec_acct_wp.size() >= 2)
      {
	      std::size_t found = vec_acct_wp[1].find(":");
	      if (found != std::string::npos)
	      {
	          boost::split(vec_wp, vec_acct_wp[1], boost::is_any_of(":"));
	      }
      }

      std::string padding = (vec_wp.size() == 2 ? vec_wp[1] : vec_acct_wp[1]);
      auto h = n_hcd(pwd_seq_nums);
      int ipadding_cunt = (h - padding.size() % h);

      while(ipadding_cunt--) padding += pwd_padding;

      auto crypto_pwd_str = reverse_string_by_sequence_step(padding, pwd_seq_nums);

      std::string res_str = vec_acct_wp[0] + ",";
      res_str += (vec_wp.size() == 2 ? (vec_wp[0] + ":" + crypto_pwd_str) : crypto_pwd_str);

      return res_str;
    }

   template<typename... ACCTS>
   void decrypt_wltpwd(string& de_wltpwd, chain_apis::read_only ro_chain, const string& acct, ACCTS... accts)
   {
       decrypt_wltpwd(de_wltpwd, ro_chain, acct);
       decrypt_wltpwd(de_wltpwd, ro_chain, accts...);
   }

   void decrypt_wltpwd(string& de_wltpwd, chain_apis::read_only ro_chain, const string& acct)
   {
	    get_table_rows_params wltpwd_query_param{
	        .json = true,
	        .code = _exchange_contract_acct,
	        .scope = "dataex",
	        .table = "wltpwd",
	        .lower_bound = acct,
	        .upper_bound = acct,
	        .limit = 1,
	        .index_position = "1"
	    };
        auto wltpwd_query_result = ro_chain.get_table_rows(wltpwd_query_param);

        if (wltpwd_query_result.rows.size() > 0)
        {
            auto encrypted_str = wltpwd_query_result.rows[0]["wltpwd"].get_string(); 
            auto decrypted_str = reverse_string_by_sequence_step(encrypted_str, pwd_seq_nums, true);

            regex rx(pwd_padding);
            decrypted_str = std::regex_replace(decrypted_str, rx, "");

            if (de_wltpwd.empty())
            {
                de_wltpwd = decrypted_str;
            } else {
                std::size_t found = de_wltpwd.find(decrypted_str);
	            if (found == std::string::npos)
                {
                    de_wltpwd += "|" + decrypted_str;
                }
            }
        }
   }
    
	// N个数的最小公倍数
	int n_hcd(const std::vector<uint8_t> nums)
	{
	    int h = 1;
	    for(auto o: nums)
	    {
	        h = hcd(o, h);
	    }
	
	    return h;
	}
	
	// 两个数的最大公约数
	int gcd(int x, int y)
	{
	    int r;
	    do
	    {
	        r = x % y;
	        x = y;
	        y = r;
	    } while(r != 0);
	
	    return x;
	}
	
	// 两个数的最小公倍数
	int hcd(int x, int y)
	{
	    return (x * y / gcd(x, y));
	}


   inline std::string to_upper(std::string str)
   {
      std::transform(std::begin(str), std::end(str), std::begin(str), [](const std::string::value_type &x) {
         return std::toupper(x, std::locale());
      });
      return str;
   }

   inline std::string to_lower(std::string str)
   {
      std::transform(std::begin(str), std::end(str), std::begin(str), [](const std::string::value_type &x) {
         return std::tolower(x, std::locale());
      });
      return str;
   }

   chain::action create_action(const vector<permission_level> &authorization, const account_name &code, const action_name &act, const fc::variant &args)
   {
      return chain::action{authorization, code, act, variant_to_bin(code, act, args)};
   }

   chain::permission_level to_permission_level(const std::string& s) {
        auto at_pos = s.find('@');
        return permission_level { s.substr(0, at_pos), s.substr(at_pos + 1) };
   }

   // 创建push_action接口action
   void create_push_action(std::vector<chain::action>& actions, const push_action_params &params)
   {
      if (!params.action_args.empty())
      { 
          auto accountPermissions = get_account_permissions(params.permissions);
          if("newaccount" == params.action_name)
          {
              vector<string> vec_acct_key;
              boost::split(vec_acct_key, params.action_args, boost::is_any_of(","));

              auto account_name = fc::trim(vec_acct_key[0]);
              auto owner_key_str = vec_acct_key.size() >= 2 ? fc::trim(vec_acct_key[1]) : "";
              auto active_key_str = vec_acct_key.size() >= 3 ? fc::trim(vec_acct_key[2]) : owner_key_str;

              auth_type owner, active;

              if( owner_key_str.find('@') != string::npos ) {
                 try {
                    owner = to_permission_level(owner_key_str);
                 } EOS_RETHROW_EXCEPTIONS( explained_exception, "Invalid owner permission level: ${permission}", ("permission", owner_key_str) )
              } else {
                 try {
                    owner = public_key_type(owner_key_str);
                 } EOS_RETHROW_EXCEPTIONS( public_key_type_exception, "Invalid owner public key: ${public_key}", ("public_key", owner_key_str) );
              }
  
              if( active_key_str.empty() ) {
                 active = owner;
              } else if( active_key_str.find('@') != string::npos ) {
                 try {
                    active = to_permission_level(active_key_str);
                 } EOS_RETHROW_EXCEPTIONS( explained_exception, "Invalid active permission level: ${permission}", ("permission", active_key_str) )
              } else {
                 try {
                    active = public_key_type(active_key_str);
                 } EOS_RETHROW_EXCEPTIONS( public_key_type_exception, "Invalid active public key: ${public_key}", ("public_key", active_key_str) );
              }

              auto create = create_newaccount(accountPermissions, params.account, account_name, owner, active);
              // 获取系统的核心token symbol
              symbol sc;
              auto asset_str = combine_asset(sc.name(), sc.decimals(), 30000);
              auto ram_asset_str = combine_asset(sc.name(), sc.decimals(), 100000);
              auto buyram = create_buyram(accountPermissions, params.account, account_name, asset::from_string(ram_asset_str));

              auto net = asset::from_string(asset_str);
              auto cpu = asset::from_string(asset_str);
              auto delegate = create_delegate(accountPermissions, params.account, account_name, net, cpu, false);

              actions.push_back(create);
              actions.push_back(buyram);
              actions.push_back(delegate);
          }
          else
          {
	         fc::variant act_payload;
	         std::string str_action_args(params.action_args);
	         if("swlt" == params.action_name)
	         {
	             str_action_args = encrypt_wltpwd(params.action_args);
	         }
	
	         combine_action_args(str_action_args);
	
	         try
	         {
	            act_payload = json_from_file_or_string(str_action_args, fc::json::relaxed_parser);
	         }
	         EOS_RETHROW_EXCEPTIONS(action_type_exception, "Fail to parse action JSON data='${action_args}'", ("action_args", str_action_args))
	
	         auto act = create_action(accountPermissions, params.account, params.action_name, act_payload);
	         actions.push_back(act);
         }
      }
   }

	chain::action create_newaccount(const vector<chain::permission_level> &permission, const name& creator, const name& newaccount, auth_type owner, auth_type active) {
	   return action {
	      permission,
	      eosio::chain::newaccount{
	         .creator      = creator,
	         .name         = newaccount,
	         .owner        = owner.contains<public_key_type>() ? authority(owner.get<public_key_type>()) : authority(owner.get<permission_level>()),
	         .active       = active.contains<public_key_type>() ? authority(active.get<public_key_type>()) : authority(active.get<permission_level>())
	      }
	   };
	}

   chain::action create_buyram(const vector<chain::permission_level> &permission, const name& creator, const name& newaccount, const asset& quantity) {
	   fc::variant act_payload = fc::mutable_variant_object()
	         ("payer", creator.to_string())
	         ("receiver", newaccount.to_string())
	         ("quant", quantity.to_string());
	   return create_action(permission, config::system_account_name, N(buyram), act_payload);
   }

   chain::action create_delegate(const vector<chain::permission_level> &permission, const name& from, const name& receiver, const asset& net, const asset& cpu, bool transfer) {
	   fc::variant act_payload = fc::mutable_variant_object()
	         ("from", from.to_string())
	         ("receiver", receiver.to_string())
	         ("stake_net_quantity", net.to_string())
	         ("stake_cpu_quantity", cpu.to_string())
	         ("transfer", transfer);
	   return create_action(permission, config::system_account_name, N(delegatebw), act_payload);
   }

   // 转账
   chain::action create_transfer(const vector<std::string> &permission, const name &from, const name &to, const asset &quantity, const std::string &memo = "memo")
   {
      fc::variant act_payload = fc::mutable_variant_object()("from", from.to_string())("to", to.to_string())("quantity", quantity.to_string())("memo", memo);
      return create_action(get_account_permissions(permission),
                           N(eosio.token), N(transfer), act_payload);
   }

   // 挂单成交
   chain::action create_mlmto(const vector<std::string> &permission, const fc::sha256 &buyer_oid, const fc::sha256 &seller_oid, const string &commodity)
   {
      fc::variant act_payload = fc::mutable_variant_object()("buyer_oid", buyer_oid.str())("seller_oid", seller_oid.str())("commodity", commodity);
      return create_action(get_account_permissions(permission),
                           N(mis.exchange), N(mlmto), act_payload);
   }

   fc::variant send_actions(std::vector<chain::action> &&actions,
                            trx_broadcast_method tbm = trx_broadcast_method::trx_only_signed,
                            packed_transaction::compression_type compression = packed_transaction::none)
   {
      auto result = push_actions(move(actions), tbm, compression );

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

   fc::variant push_actions(std::vector<chain::action> &&actions,
                            trx_broadcast_method tbm = trx_broadcast_method::trx_only_signed,
                            packed_transaction::compression_type compression = packed_transaction::none)
   {
      signed_transaction trx;
      trx.actions = std::forward<decltype(actions)>(actions);

      return may_push_transaction(trx, tbm, compression);
   }

   fc::variant may_push_transaction(signed_transaction &trx,
                            trx_broadcast_method tbm = trx_broadcast_method::trx_only_signed,
                            packed_transaction::compression_type compression = packed_transaction::none)
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
      bool tx_return_packed = true;

      if (!tx_skip_sign)
      {
         auto required_keys = determine_required_keys(trx);
         sign_transaction(trx, required_keys, info.chain_id);
      }

      switch (tbm)
      {  
          case trx_broadcast_method::trx_broadcast_sync:
          {
	        controller &chain = app().get_plugin<chain_plugin>().chain();
	
	        auto ptrx = std::make_shared<transaction_metadata>(trx, compression);
	        chain.push_transaction(ptrx, fc::time_point::maximum());
	
	        return fc::variant();
          }
          case trx_broadcast_method::trx_only_signed:
          {
	         if (!tx_return_packed)
	         {
	            return fc::variant(trx);
	         }
	         else
	         {
	            return fc::variant(packed_transaction(trx, compression));
	         }
             break;
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
         args = "[" + args + "]";
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

   fc::variant call(const std::string &path)
   {
      return call(path, fc::variant());
   }

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

fc::variant data_exchange_plugin::limit_order(const limit_order_params &params)
{
   return my->limit_order(params);
}

data_exchange_plugin::data_exchange_plugin() : my(new data_exchange_plugin_impl()) {}
data_exchange_plugin::~data_exchange_plugin() {}

void data_exchange_plugin::set_program_options(options_description &, options_description &cfg)
{
   cfg.add_options()("wallet-url", bpo::value<std::string>()->default_value("http://127.0.0.1:8900"),
                     "The unix socket path of wallet or http host for HTTP RPC (absolute file path or http(s)://wallet-host:port)");
   cfg.add_options()("exchange_contract_acct", bpo::value<std::string>()->default_value("mis.exchange"),
                     "The account which is deployed the data exchange contract");
   cfg.add_options()("eosiotoken_contract_acct", bpo::value<std::string>()->default_value("eosio.token"),
                     "The account which is deployed the eosio.token contract");
   cfg.add_options()("eosiosystem_contract_acct", bpo::value<std::string>()->default_value("eosio"),
                     "The account which is deployed the eosio.systemcontract");
}

void data_exchange_plugin::plugin_initialize(const variables_map &options)
{
   try
   {
      if (options.count("wallet-url"))
      {
         my->set_wallet_url(options.at("wallet-url").as<std::string>());
      }
      if (options.count("exchange_contract_acct"))
      {
         my->set_exchange_acct(options.at("exchange_contract_acct").as<std::string>());
      }
      if (options.count("eosiotoken_contract_acct"))
      {
         my->set_eosiotoken_acct(options.at("eosiotoken_contract_acct").as<std::string>());
      }
      if (options.count("eosiosystem_contract_acct"))
      {
         my->set_eosiosystem_acct(options.at("eosiosystem_contract_acct").as<std::string>());
      }
   }
   FC_LOG_AND_RETHROW()

   my->deal_timer.reset(new boost::asio::steady_timer(app().get_io_service()));
   my->match_limit_order();
}

void data_exchange_plugin::plugin_startup()
{
}

void data_exchange_plugin::plugin_shutdown()
{
}

} // namespace eosio
