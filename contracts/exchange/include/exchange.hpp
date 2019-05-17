#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

CONTRACT exchange : public contract {
   public:
      using contract::contract;

      ACTION buydata( name buyer, name seller, std::string orderid, std::string ordertime, std::string paytime, std::string payamount); 

      ACTION approvedata( name requester, name provider, std::string data, std::string time, bool approved );

      ACTION uploaddata( name uploader, std::string data, std::string datatype,std::string time );
	  
	  [[eosio::action("lmto")]]
      void limit_order( name consigner, std::string commodity_type, std::string commodity, double amount, asset price, int64_t consign_time, int8_t oper);

	  [[eosio::action("mlmto")]]
      void match_limit_order( name buyer, name seller, std::string commodity_type, std::string commodity, double amount, asset price, int64_t match_time );

      using buydata_action = action_wrapper<"buydata"_n, &exchange::buydata>;
      using approvedata_action = action_wrapper<"approvedata"_n, &exchange::approvedata>;
      using uploaddata_action = action_wrapper<"uploaddata"_n, &exchange::uploaddata>;
      using limit_order_action = action_wrapper<"lmto"_n, &exchange::limit_order>;
      using match_limit_order_action = action_wrapper<"mlmto"_n, &exchange::match_limit_order>;
};
