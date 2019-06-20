#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/singleton.hpp>

namespace data_exchange {

using namespace eosio;
using std::string;
using std::vector;

inline checksum256 string_to_sha256(const string& s) { 
    return sha256(s.c_str(), s.size()); 
}

struct account {
    asset balance;

    uint64_t primary_key()const { return balance.symbol.code().raw(); }
};
typedef eosio::multi_index<name("accounts"), account> accounts;

// 这里写明合约名，表示需要在合约的abi文件中编译生成表的结构声明
struct [[eosio::table,eosio::contract("exchange")]] matched_price{
    uint64_t id;            // 主键
    string commodity_name;  // 商品名
    asset price;            // 商品最近成交的价格，自动撮合时需要跟这个价格比对

    auto primary_key() const { return id; }
    checksum256 by_commodity_name() const { return string_to_sha256(commodity_name); };

    EOSLIB_SERIALIZE( matched_price, (id)(commodity_name)(price) )
};

struct [[eosio::table,eosio::contract("exchange")]] commodity_type {
    uint64_t id;    // 主键
    string name;    // 商品类别名，挂单的时候可以选择

    auto primary_key() const { return id; }
    checksum256 by_name() const { return string_to_sha256(name); }

    EOSLIB_SERIALIZE( commodity_type, (id)(name) )
};

struct [[eosio::table, eosio::contract("exchange")]] consignment_order {
    uint64_t id;            // 主键
    checksum256 oid;        // 委托单号
    name consigner;         // 挂单账号
    string commodity_type;  // 商品类别名称
    string commodity;       // 商品名称
    int64_t amount;         // 委托数量
    int64_t amount_done;    // 已成交量
    int64_t consign_time;   // 挂单时间
    asset price;            // 挂单价。比如 "23.1987 RMB"，支持4位小数，单位为大写字符串且不超过7个字母
    asset total;            // 挂单总额: price * amount 
    int8_t oper;            // 1:买, 0:卖
    int8_t stat;            // 0:撤销, 1:未全部成交, 3:全部成交

    auto primary_key() const { return id; }
    checksum256 by_oid() const { return oid; }
    uint64_t by_consigner() const { return consigner.value; }
    uint64_t by_consign_time() const { return consign_time; }
    uint64_t by_price() const { return price.amount; }
    uint64_t by_stat() const { return stat; }

    EOSLIB_SERIALIZE( consignment_order, (id)(oid)(consigner)(commodity_type)(commodity)(amount)(amount_done)(consign_time)(price)(total)(oper)(stat) )
};

struct [[eosio::table, eosio::contract("exchange")]] matched_order {
    uint64_t id;            // 主键
    checksum256 buyer_oid;  // 买家挂单号 
    checksum256 seller_oid; // 卖家挂单号
    name buyer;             // 买家
    name seller;            // 卖家
    string commodity_type;  // 商品类别名称
    string commodity;       // 商品名称
    int64_t amount;         // 成交数量
    int64_t matched_time;   // 成交时间
    asset price;            // 成交价，比如 "23.1987 RMB"，支持4位小数，单位为大写字符串且不超过7个字母

    auto primary_key() const { return id; }
    checksum256 by_buyer_oid() const { return buyer_oid; }
    checksum256 by_seller_oid() const { return seller_oid; }
    uint64_t by_buyer() const { return buyer.value; }
    uint64_t by_seller() const { return seller.value; }
    uint64_t by_matched_time() const { return matched_time; }

    EOSLIB_SERIALIZE( matched_order, (id)(buyer_oid)(seller_oid)(buyer)(seller)(commodity_type)(commodity)(amount)(matched_time)(price) )
};

struct [[eosio::table, eosio::contract("exchange")]] wallet_password {
    name account;   // 账号
    string wltpwd;  // 钱包和对应的密码,格式："wallet#password[|wallet#password]"

    uint64_t primary_key() const { return account.value; }

    EOSLIB_SERIALIZE( wallet_password, (account)(wltpwd) )
};

typedef eosio::multi_index< "wltpwd"_n, wallet_password > wallet_password_table;

typedef eosio::multi_index< "matchedprice"_n, matched_price,
            indexed_by< "bycname"_n, const_mem_fun<matched_price, checksum256, &matched_price::by_commodity_name> >
            > matched_price_table;

typedef eosio::multi_index< "commoditype"_n, commodity_type,
            indexed_by< "byname"_n, const_mem_fun<commodity_type, checksum256, &commodity_type::by_name> >
            >commodity_type_table;

typedef eosio::multi_index<"buyer"_n, consignment_order,
            indexed_by< "byoid"_n, const_mem_fun<consignment_order, checksum256, &consignment_order::by_oid> >,
            indexed_by< "byconsigner"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_consigner> >,
            indexed_by< "bycsntime"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_consign_time> >,
            indexed_by< "byprice"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_price> >,
            indexed_by< "bystat"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_stat> >
            > buyer_table;

typedef eosio::multi_index<"buyerorder"_n, consignment_order,
            indexed_by< "byoid"_n, const_mem_fun<consignment_order, checksum256, &consignment_order::by_oid> >,
            indexed_by< "byconsigner"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_consigner> >,
            indexed_by< "bycsntime"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_consign_time> >,
            indexed_by< "byprice"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_price> >,
            indexed_by< "bystat"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_stat> >
            > buyer_order_table;

typedef eosio::multi_index<"seller"_n, consignment_order,
            indexed_by< "byoid"_n, const_mem_fun<consignment_order, checksum256, &consignment_order::by_oid> >,
            indexed_by< "byconsigner"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_consigner> >,
            indexed_by< "bycsntime"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_consign_time> >,
            indexed_by< "byprice"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_price> >,
            indexed_by< "bystat"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_stat> >
            > seller_table;

typedef eosio::multi_index<"sellerorder"_n, consignment_order,
            indexed_by< "byoid"_n, const_mem_fun<consignment_order, checksum256, &consignment_order::by_oid> >,
            indexed_by< "byconsigner"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_consigner> >,
            indexed_by< "bycsntime"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_consign_time> >,
            indexed_by< "byprice"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_price> >,
            indexed_by< "bystat"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_stat> >
            > seller_order_table;

typedef eosio::multi_index<"cancelorder"_n, consignment_order,
            indexed_by< "byoid"_n, const_mem_fun<consignment_order, checksum256, &consignment_order::by_oid> >,
            indexed_by< "byconsigner"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_consigner> >,
            indexed_by< "bycsntime"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_consign_time> >,
            indexed_by< "byprice"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_price> >,
            indexed_by< "bystat"_n, const_mem_fun<consignment_order, uint64_t, &consignment_order::by_stat> >
            > cancel_order_table;

typedef eosio::multi_index<"ctoc"_n, matched_order,
            indexed_by< "bybuyeroid"_n, const_mem_fun<matched_order, checksum256, &matched_order::by_buyer_oid> >,
            indexed_by< "byselleroid"_n, const_mem_fun<matched_order, checksum256, &matched_order::by_seller_oid> >,
            indexed_by< "bybuyer"_n, const_mem_fun<matched_order, uint64_t, &matched_order::by_buyer> >,
            indexed_by< "byseller"_n, const_mem_fun<matched_order, uint64_t, &matched_order::by_seller> >,
            indexed_by< "bymthtime"_n, const_mem_fun<matched_order, uint64_t, &matched_order::by_matched_time> >
            > ctoc_table;

class [[eosio::contract("exchange")]] mis_exchange : public contract {
    typedef singleton<"defersingle"_n, uint64_t> sendid;

    private:
        commodity_type_table    _commodity_type;
        buyer_table             _buyer;
        buyer_order_table       _buyer_order;
        seller_table            _seller;
        seller_order_table      _seller_order;
        ctoc_table              _matched_order;
        matched_price_table     _matched_price;
        sendid                  _sid;
        wallet_password_table   _wltpwd;
        cancel_order_table      _cancel_order;

    private:
        static constexpr name tb_scope{"dataex"_n};
        const std::string commodity_any = "*";

    public:
        using contract::contract;

        mis_exchange( name s, name code, datastream<const char*> ds );

        ACTION buydata( name buyer, name seller, const string& orderid, const string& ordertime, const string& paytime, const string& payamount); 

        ACTION approvedata( name requester, name provider, const string& data, const string& time, bool approved );

        ACTION uploaddata( name uploader, const string& data, const string& datatype,const string& time );
	  
        // 委托单
        [[eosio::action("lmto")]]
        void limit_order( name consigner, const string& commodity_type, const string& commodity, int64_t amount, asset price, int8_t oper);
        
        // 取消委托单
        [[eosio::action("clmto")]]
        void cancel_limit_order(const name consigner, const checksum256& oid);

        // 为自动撮合保存在链上的加密钱包名与对应的密码
        [[eosio::action("swlt")]]
        void surprise_wlt( name account, const string& wltpwd);

        // 自动撮合委托单
	    [[eosio::action("mlmto")]]
        void match_limit_order( const checksum256& buyer_oid, const checksum256& seller_oid, const string& commodity);

        using buydata_action = action_wrapper<"buydata"_n, &mis_exchange::buydata>;
        using approvedata_action = action_wrapper<"approvedata"_n, &mis_exchange::approvedata>;
        using uploaddata_action = action_wrapper<"uploaddata"_n, &mis_exchange::uploaddata>;
        using limit_order_action = action_wrapper<"lmto"_n, &mis_exchange::limit_order>;
        using cancel_limit_order_action = action_wrapper<"clmto"_n, &mis_exchange::cancel_limit_order>;
        using match_limit_order_action = action_wrapper<"mlmto"_n, &mis_exchange::match_limit_order>;
        using surprise_wlt_action = action_wrapper<"swlt"_n, &mis_exchange::surprise_wlt>;

    private:        
        void do_update_limit_order(const checksum256& oid, const uint64_t mlo_id);
        string to_hex(const char* d, uint32_t s);
        string sha256_to_hex(const checksum256& sha256);
        asset get_account_balance(name token_contract_account, name owner, symbol sym);
        uint64_t next_id();
};

} // namespace data_exchange
