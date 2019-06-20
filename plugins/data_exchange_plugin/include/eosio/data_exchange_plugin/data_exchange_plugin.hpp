/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain/asset.hpp>
// #include <eosio/wallet_plugin/wallet_plugin.hpp>

namespace eosio
{

using namespace appbase;

typedef std::unique_ptr<class data_exchange_plugin_impl> data_exchange_ptr;

FC_DECLARE_EXCEPTION( explained_exception, 9000000, "explained exception, see error log" );

enum class deal_op : uint8_t
{
    ask_op = 0, // 委托挂单卖
    bid_op  // 委托挂单买
};

// 挂单（暫未使用）
struct cell_deal
{
    fc::time_point create_time; // 挂单时间
    std::string commodity_type; // 商品类型，如token，phone_query
    std::string commodity;      // 商品，比如EOS
    account_name account;       // 账号
    int64_t amount;             // 数量
    asset price;                // 价格，如23.1329 USDT
    deal_op oper;               // 买或卖
    
    // 挂单交易id
    chain::digest_type id() const
    {
        return chain::digest_type::hash(*this);
    }

    bool operator< (const cell_deal &r) const {
        // 1. 价格小的优先在前面；2. 价格相同，则时间小的在前面
        if(price == r.price) return create_time > r.create_time;
        return price < r.price;
    }
};

// 卖方序列顺序
// struct cell_less_cmp {
 
//     bool operator ()(const cell_deal &l, const cell_deal &r)
//     {
//         // 1. 价格小的优先在前面；2. 价格相同，则时间小的在前面
//         if(l.price == r.price) return l.create_time < r.create_time;
//         return l.price < r.price;
//     }
// };

// 买方序列顺序（暂未使用）
struct cell_greater_cmp {
 
    bool operator ()(const cell_deal &l, const cell_deal &r)
    {
        // 1. 价格大的优先在前面；2. 价格相同，则时间小的在前面
        if(l.price == r.price) return l.create_time < r.create_time;
        return l.price > r.price;
    }
};

// 交易的挂单（暂未使用）
struct exchange_deal
{
    cell_deal cdeal;
    int64_t filled_amount;   // 已成交量
    int64_t unfilled_amount; // 未成交量
    int8_t  state;            // 挂单状态，-1：撤销；0：创建；1：全部成交；2：部分成交；

    bool operator< (const exchange_deal &r) {
        // 按照value从大到小排列
        if(cdeal.price == r.cdeal.price) return cdeal.create_time > r.cdeal.create_time;
        return cdeal.price < r.cdeal.price;
    }
};

// 卖方序列顺序（暂未使用）
struct exchange_greater_cmp {
 
    bool operator ()(const exchange_deal &l, const exchange_deal &r)
    {
        // 1. 价格大的优先在前面；2. 价格相同则时间小的在前面
        if(l.cdeal.price == r.cdeal.price) return l.cdeal.create_time < r.cdeal.create_time;
        return l.cdeal.price > r.cdeal.price;
    }
};

// 挂单/限价交易参数（暂未使用）
struct limit_order_params
{
    std::string commodity_type; // 商品类型，如token，phone_query
    std::string commodity;      // 商品，比如EOS
    std::string account;        // 账号
    int64_t amount;             // 数量
    std::string price;          // 价格，如23.1329 USDT
    int8_t oper;                // 1:买, 0:卖
};

struct wallet_params
{
    std::string name; // 钱包名称
    std::string pwd;  // 钱包密码
};

struct push_action_params
{
    std::string wltpwd;              // 钱包名与解锁密码对，格式：wallet#password[|wallet#password]
    std::string account;             // 交易合约所在的账户，如：eosio.token
    std::string action_name;         // action的名称
    std::string action_args;         // action的参数
    vector<std::string> permissions; // 用于交易签名的授权账号和许可级别，如eosio@active
};

struct matched_price{
    uint64_t id;            // 主键
    string commodity_name;  // 商品名
    asset price;            // 商品最近成交的价格，自动撮合时需要跟这个价格比对
};

struct consignment_order {
    uint64_t id;            // 主键
    fc::sha256 oid;         // 委托单号
    string consigner;       // 挂单账号
    string commodity_type;  // 商品类别名称
    string commodity;       // 商品名称
    int64_t amount;         // 委托数量
    int64_t amount_done;    // 已成交量
    int64_t consign_time;   // 挂单时间
    asset price;            // 挂单价。比如 "23.1987 RMB"，支持4位小数，单位为大写字符串且不超过7个字母
    asset total;            // 挂单总额: price * amount 
    int8_t oper;            // 1:买, 0:卖
    int8_t stat;            // 0:撤销, 1:未全部成交, 3:全部成交
};

struct account_balance {
    asset    balance;       // 账户余额
};

class data_exchange_plugin : public appbase::plugin<data_exchange_plugin>
{
  public:
    data_exchange_plugin();
    virtual ~data_exchange_plugin();

    APPBASE_PLUGIN_REQUIRES((chain_plugin)(http_plugin))
    virtual void set_program_options(options_description &, options_description &cfg) override;

    void plugin_initialize(const variables_map &options);
    void plugin_startup();
    void plugin_shutdown();

    // 提交action
    fc::variant push_action(const push_action_params &params);
    // 挂单/限价交易
    fc::variant limit_order(const limit_order_params &params);

  private:
    data_exchange_ptr my;
};

} // namespace eosio

FC_REFLECT(eosio::account_balance, (balance));
FC_REFLECT(eosio::wallet_params, (name)(pwd));
FC_REFLECT(eosio::push_action_params, (wltpwd)(account)(action_name)(action_args)(permissions));
FC_REFLECT(eosio::matched_price, (id)(commodity_name)(price));
FC_REFLECT(eosio::consignment_order, (id)(oid)(commodity_type)(commodity)(amount)(amount_done)(consign_time)(price)(total)(oper)(stat));
FC_REFLECT(eosio::cell_deal, (create_time)(commodity_type)(commodity)(account)(amount)(price)(oper));
FC_REFLECT(eosio::exchange_deal, (cdeal)(filled_amount)(unfilled_amount)(state));
FC_REFLECT(eosio::limit_order_params, (commodity_type)(commodity)(account)(amount)(price)(oper));
