#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <memory>
#include <eosio/data_exchange_plugin/mysqlconn.hpp>

namespace eosio {
class connection_pool
{
    public:
        explicit connection_pool(
            const std::string host, const std::string user, const std::string passwd, const std::string database, 
            const uint16_t port, const uint16_t max_conn, const bool do_closeconn_on_unlock);
        ~connection_pool();
        
        shared_ptr<MysqlConnection> get_connection();
        void release_connection(MysqlConnection& con);

    private:
        MysqlConnPool m_pool;

        bool    _do_closeconn_on_unlock;
};
}

#endif