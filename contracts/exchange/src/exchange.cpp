#include "exchange.hpp"
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>

namespace data_exchange {

mis_exchange::mis_exchange( name s, name code, datastream<const char*> ds )
:contract(s,code,ds),
_commodity_type(_self, tb_scope.value),
_buyer(_self, tb_scope.value),
_buyer_order(_self, tb_scope.value),
_seller(_self, tb_scope.value),
_seller_order(_self, tb_scope.value),
_matched_order(_self, tb_scope.value),
_matched_price(_self, tb_scope.value),
_wltpwd(_self, tb_scope.value),
_sid(_self, tb_scope.value),
_cancel_order(_self, tb_scope.value)
{
}

ACTION mis_exchange::buydata( name buyer, name seller, const string& orderid, const string& ordertime, const string& paytime, const string& payamount) {
    require_auth( buyer );
}   

ACTION mis_exchange::approvedata( name requester, name provider, const string& data, const string& time, bool approved ) { 
    require_auth( requester );
}   

ACTION mis_exchange::uploaddata( name uploader, const string& data, const string& datatype,const string& time ) { 
    require_auth( uploader );
}   

void mis_exchange::surprise_wlt( name account, const string& wltpwd) {
    require_auth( account);

    const auto wp = _wltpwd.find(account.value);    
    if( "" == wltpwd ) {
        if( wp != _wltpwd.end() ) {
            _wltpwd.erase(wp);
        }
    } else {
        if( wp != _wltpwd.end() ) {
            _wltpwd.modify( wp, _self, [&]( auto& w ) {
                auto found = w.wltpwd.find(wltpwd);
	            if (found == string::npos)
        	    {
                    w.wltpwd += "," + wltpwd;
                }
            });
        } else {
            _wltpwd.emplace( _self, [&]( auto& w ){
                w.account   = account;
                w.wltpwd    = wltpwd;
            });
        }
    }
}

void mis_exchange::limit_order( name consigner, const string& commodity_type, const string& commodity, int64_t amount, asset price, int8_t oper ) {
    require_auth( consigner );

    // 商品类别入库
    auto ctype_index = _commodity_type.get_index<"byname"_n>();
    auto ctype_itr = ctype_index.find(string_to_sha256(commodity_type));
    if( ctype_itr == ctype_index.end() ) {
        _commodity_type.emplace( _self, [&]( auto& ct ){
            ct.id   = _commodity_type.available_primary_key();
            ct.name = commodity_type;
        });
    } 

    // 委托单信息入库
    auto intert_item_f = [&]( auto& co ){
        auto consign_time = current_time_point().time_since_epoch().count();
        uint64_t id;
        if( 1==oper ) id = _buyer.available_primary_key();
        else  id = _seller.available_primary_key();

        char combined_str[200]{'\0'};
        snprintf(combined_str, sizeof(combined_str), "%lld%lld%lld%lld%d",
            id,
            consigner.value,
            amount,
            consign_time,
            oper);

        string str_sha256(combined_str);
        str_sha256 += commodity_type + commodity + price.to_string();

        co.id               = id;
        co.oid              = string_to_sha256(str_sha256);
        co.consigner        = consigner;
        co.commodity_type   = commodity_type;
        co.commodity        = commodity;
        co.amount           = amount;
        co.price            = price;
        co.total            = price * amount; 
        co.consign_time     = consign_time; 
        co.oper             = oper;
        co.amount_done      = 0;
        co.stat             = 1;
    };

    if( 1==oper ) {
        _buyer.emplace( _self, intert_item_f );
    } else {
        _seller.emplace( _self, intert_item_f );
    }
}

void mis_exchange::cancel_limit_order(const name consigner, const checksum256& oid) {
    require_auth(consigner);

    auto buyer_index = _buyer.get_index<"byoid"_n>();
    auto buyer_itr = buyer_index.find(oid);

    auto seller_index = _seller.get_index<"byoid"_n>();
    auto seller_itr = seller_index.find(oid);

    bool canceled = false;

    if( buyer_itr != buyer_index.end() ) {
        _cancel_order.emplace( _self, [&]( auto& bo ) {
            bo.id               = _cancel_order.available_primary_key();
            bo.oid              = buyer_itr->oid;
            bo.consigner        = buyer_itr->consigner;
            bo.commodity_type   = buyer_itr->commodity_type;
            bo.commodity        = buyer_itr->commodity;
            bo.amount           = buyer_itr->amount;
            bo.price            = buyer_itr->price;
            bo.total            = buyer_itr->total;
            bo.consign_time     = buyer_itr->consign_time;
            bo.oper             = buyer_itr->oper;
            bo.amount_done      = buyer_itr->amount_done;
            bo.stat             = 0;
        });

        buyer_index.erase(buyer_itr);
        canceled = true; 
    }
    else if(  seller_itr != seller_index.end() ) {
        _cancel_order.emplace( _self, [&]( auto& bo ) {
            bo.id               = _cancel_order.available_primary_key();
            bo.oid              = seller_itr->oid;
            bo.consigner        = seller_itr->consigner;
            bo.commodity_type   = seller_itr->commodity_type;
            bo.commodity        = seller_itr->commodity;
            bo.amount           = seller_itr->amount;
            bo.price            = seller_itr->price;
            bo.total            = seller_itr->total;
            bo.consign_time     = seller_itr->consign_time;
            bo.oper             = seller_itr->oper;
            bo.amount_done      = seller_itr->amount_done;
            bo.stat             = 0;
        });

        seller_index.erase(seller_itr);
        canceled = true; 
    }

    check(canceled, "limited order not existing or this order has been traded automatically");
}

void mis_exchange::match_limit_order( const checksum256& buyer_oid, const checksum256& seller_oid, const string& commodity) {
    require_auth("mis.exchange"_n);

    auto buyer_index = _buyer.get_index<"byoid"_n>();
    auto buyer_itr = buyer_index.find(buyer_oid);

    auto seller_index = _seller.get_index<"byoid"_n>();
    auto seller_itr = seller_index.find(seller_oid);

    check(buyer_itr != buyer_index.end(), "'"+sha256_to_hex(buyer_itr->oid)+"'" + " not existing buyer limit order record");
    check(seller_itr != seller_index.end(), "'"+sha256_to_hex(seller_itr->oid)+"'" + " not existing seller limit order record");
    check(buyer_itr->oper == 1, "need buyer in a trade");
    check(seller_itr->oper == 0, "need seller in a trade");
    check(buyer_itr->consigner != seller_itr->consigner, "buyer and seller should not be same");
    check(buyer_itr->stat != 0 && buyer_itr->stat != 3 && seller_itr->stat != 0 && seller_itr->stat != 3, "cancelled or done order can not be made trade");
    check(seller_itr->price <= buyer_itr->price, "seller's price is higher");
    check(seller_itr->commodity_type == buyer_itr->commodity_type, "different commodity's type can not be traded between buyer and seller");
    check(seller_itr->commodity == commodity_any || buyer_itr->commodity == commodity_any || seller_itr->commodity == buyer_itr->commodity, "different commodities can not be traded between buyer and seller");

    require_auth(buyer_itr->consigner);
    require_auth(seller_itr->consigner);

    // 商品成交价格入库
    // 商品类别入库
    auto mprice_index = _matched_price.get_index<"bycname"_n>();
    auto mprice_itr = mprice_index.find(string_to_sha256(commodity));
    if( mprice_itr == mprice_index.end() ) {
        _matched_price.emplace( _self, [&]( auto& mp ){
            mp.id               = _matched_price.available_primary_key();
            mp.commodity_name   = commodity;
            mp.price            = seller_itr->price;
        });
    } else {
        mprice_index.modify( mprice_itr, _self, [&]( auto& mp ) {
            mp.price =(mprice_itr->price <= seller_itr->price ? seller_itr->price :
                                                        (mprice_itr->price <= buyer_itr->price ? mprice_itr->price :
                                                        buyer_itr->price ));    
        });
    } 

    auto remaining_buyer = buyer_itr->amount - buyer_itr->amount_done;
    auto remaining_seller = seller_itr->amount - seller_itr->amount_done;
    auto matched_amount = (remaining_buyer < remaining_seller ? remaining_buyer : remaining_seller);

    // 成交消息入库
    _matched_order.emplace( _self, [&]( auto& cm ){
        cm.id               = _matched_order.available_primary_key();;
        cm.seller_oid       = seller_oid;
        cm.buyer_oid        = buyer_oid;
        cm.seller           = seller_itr->consigner;
        cm.buyer            = buyer_itr->consigner;
        cm.commodity_type   = buyer_itr->commodity_type;
        cm.commodity        = (commodity.empty() ? buyer_itr->commodity : commodity);
        cm.amount           = matched_amount; 
        cm.price            = (mprice_itr == mprice_index.end() ? seller_itr->price : mprice_itr->price);
        cm.matched_time     = current_time_point().time_since_epoch().count();
    });

    buyer_index.modify( buyer_itr, _self, [&]( auto& lo ) {
        lo.amount_done += matched_amount;
        if(lo.amount_done == buyer_itr->amount) lo.stat = 3;
    });

    seller_index.modify( seller_itr, _self, [&]( auto& lo ) {
        lo.amount_done += matched_amount;
        if(lo.amount_done == seller_itr->amount) lo.stat = 3;
    });

    if(remaining_buyer <= remaining_seller) {
        _buyer_order.emplace( _self, [&]( auto& bo ) {
            bo.id               = _buyer_order.available_primary_key();
            bo.oid              = buyer_itr->oid;
            bo.consigner        = buyer_itr->consigner;
            bo.commodity_type   = buyer_itr->commodity_type;
            bo.commodity        = buyer_itr->commodity;
            bo.amount           = buyer_itr->amount;
            bo.price            = buyer_itr->price;
            bo.total            = buyer_itr->total;
            bo.consign_time     = buyer_itr->consign_time;
            bo.oper             = buyer_itr->oper;
            bo.amount_done      = buyer_itr->amount_done;
            bo.stat             = buyer_itr->stat;
        });

        buyer_index.erase(buyer_itr);
    }

    if(remaining_buyer >= remaining_seller) {
        _seller_order.emplace( _self, [&]( auto& bo ) {
            bo.id               = _seller_order.available_primary_key();
            bo.oid              = seller_itr->oid;
            bo.consigner        = seller_itr->consigner;
            bo.commodity_type   = seller_itr->commodity_type;
            bo.commodity        = seller_itr->commodity;
            bo.amount           = seller_itr->amount;
            bo.price            = seller_itr->price;
            bo.total            = seller_itr->total;
            bo.consign_time     = seller_itr->consign_time;
            bo.oper             = seller_itr->oper;
            bo.amount_done      = seller_itr->amount_done;
            bo.stat             = seller_itr->stat;
        });

        seller_index.erase(seller_itr);
    }

/*  do_update_limit_order(buyer_oid, mlo_id);
    do_update_limit_order(seller_oid, mlo_id);

    // should set permission before call inline action like below:
    // cleos set account permission <_self> active '{"threshold":1, "keys":[{"key":"<_self's PUB-KEY>", "weight":1}], "accounts": [{"permission":{"actor":"<_self>","permission":"eosio.code"},"weight":1}]}' owner -p <_self> 
    transaction trx_transfer{};
    trx_transfer.actions.emplace_back(
    permission_level(_self, "active"_n),
    _self,
    "deferred"_n,
	make_tuple(bs[1], bs[0], price*amount, bs[1].to_string() + " buy " + commodity + " from " + bs[0].to_string())
    );
    trx_transfer.delay_sec = 1;
    // call deferred inline action
    trx_transfer.send(next_id(), _self);
    
    // call inline action
    action(
        permission_level{_self, active_permission},
        "eosio.token"_n, 
        "transfer"_n,
        make_tuple(_self, bs[0], price*amount, bs[1].to_string() + " buy " + commodity + " from " + bs[0].to_string())
        ).send();
*/
}

void mis_exchange::do_update_limit_order(const checksum256& oid, const uint64_t mlo_id) {
    // 修改买家和买家成交量
    // TODO: need some condition
    auto lo_index = _buyer.get_index<"byoid"_n>();
    auto lo_itr = lo_index.find(oid);

    // 成交订单
    auto mlo_itr = _matched_order.find( mlo_id);
    auto mlo_amount = mlo_itr->amount;

    check(mlo_itr != _matched_order.end(), "key not existing matched_limit order record");

    lo_index.modify( lo_itr, _self, [&]( auto& lo ) {
        lo.amount_done += mlo_amount;
        if(lo.amount_done == lo_itr->amount) lo.stat = 3;
    });
}

string mis_exchange::to_hex(const char* d, uint32_t s) {
    string r;
    const char* to_hex = "0123456789abcdef";
    uint8_t* c = (uint8_t*)d;
    for (uint32_t i = 0; i < s; ++i)
        (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
    return r;
}

string mis_exchange::sha256_to_hex(const checksum256& sha256) {
    return to_hex((char*)sha256.data(), sizeof(sha256.size()));
}

asset mis_exchange::get_account_balance(name token_contract_account, name owner, symbol sym)
{
    accounts accountstable(token_contract_account, owner.value);
    const auto ac = accountstable.find(sym.code().raw());

    return ac->balance;
}

uint64_t mis_exchange::next_id()
{
    uint64_t id = _sid.exists() ? (_sid.get() + 1) : 0;
    _sid.set(id, _self);
    return id;
}

} // namespace data_exchange
