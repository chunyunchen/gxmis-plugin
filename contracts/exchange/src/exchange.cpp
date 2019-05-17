#include "exchange.hpp"

ACTION exchange::buydata( name buyer, name seller, std::string orderid, std::string ordertime, std::string paytime, std::string payamount) {
   require_auth( buyer );
}   

ACTION exchange::approvedata( name requester, name provider, std::string data, std::string time, bool approved ) { 
   require_auth( requester );
}   

ACTION exchange::uploaddata( name uploader, std::string data, std::string datatype,std::string time ) { 
   require_auth( uploader );
}   

void exchange::limit_order( name consigner, std::string commodity_type, std::string commodity, double amount, asset price, int64_t consign_time, int8_t oper) {
	require_auth( consigner );
}

void exchange::match_limit_order( name buyer, name seller, std::string commodity_type, std::string commodity, double amount, asset price, int64_t match_time ) {
	require_auth( buyer );
	require_auth( seller );
}
