# the location of the blocks directory (absolute path or relative to application data dir) (eosio::chain_plugin)
# blocks-dir = "blocks"

# the location of the protocol_features directory (absolute path or relative to application config dir) (eosio::chain_plugin)
# protocol-features-dir = "protocol_features"

# Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints. (eosio::chain_plugin)
# checkpoint = 

# Override default WASM runtime (eosio::chain_plugin)
# wasm-runtime = 

# Override default maximum ABI serialization time allowed in ms (eosio::chain_plugin)
#abi-serializer-max-time-ms = 15000

# Maximum size (in MiB) of the chain state database (eosio::chain_plugin)
#chain-state-db-size-mb = 1024

# Safely shut down node when free space remaining in the chain state database drops below this size (in MiB). (eosio::chain_plugin)
#chain-state-db-guard-size-mb = 128

# Maximum size (in MiB) of the reversible blocks database (eosio::chain_plugin)
#reversible-blocks-db-size-mb = 340

# Safely shut down node when free space remaining in the reverseible blocks database drops below this size (in MiB). (eosio::chain_plugin)
#reversible-blocks-db-guard-size-mb = 2

# Percentage of actual signature recovery cpu to bill. Whole number percentages, e.g. 50 for 50% (eosio::chain_plugin)
#signature-cpu-billable-pct = 50

# Number of worker threads in controller thread pool (eosio::chain_plugin)
#chain-threads = 2

# print contract's output to console (eosio::chain_plugin)
#contracts-console = false

# Account added to actor whitelist (may specify multiple times) (eosio::chain_plugin)
# actor-whitelist = 

# Account added to actor blacklist (may specify multiple times) (eosio::chain_plugin)
# actor-blacklist = 

# Contract account added to contract whitelist (may specify multiple times) (eosio::chain_plugin)
# contract-whitelist = 

# Contract account added to contract blacklist (may specify multiple times) (eosio::chain_plugin)
# contract-blacklist = 

# Action (in the form code::action) added to action blacklist (may specify multiple times) (eosio::chain_plugin)
# action-blacklist = 

# Public key added to blacklist of keys that should not be included in authorities (may specify multiple times) (eosio::chain_plugin)
# key-blacklist = 

# Deferred transactions sent by accounts in this list do not have any of the subjective whitelist/blacklist checks applied to them (may specify multiple times) (eosio::chain_plugin)
# sender-bypass-whiteblacklist = 

# Database read mode ("speculative", "head", "read-only", "irreversible").
# In "speculative" mode database contains changes done up to the head block plus changes made by transactions not yet included to the blockchain.
# In "head" mode database contains changes done up to the current head block.
# In "read-only" mode database contains changes done up to the current head block and transactions cannot be pushed to the chain API.
# In "irreversible" mode database contains changes done up to the last irreversible block and transactions cannot be pushed to the chain API.
#  (eosio::chain_plugin)
read-mode = speculative

# Chain validation mode ("full" or "light").
# In "full" mode all incoming blocks will be fully validated.
# In "light" mode all incoming blocks headers will be fully validated; transactions in those validated blocks will be trusted 
#  (eosio::chain_plugin)
validation-mode = full

# Disable the check which subjectively fails a transaction if a contract bills more RAM to another account within the context of a notification handler (i.e. when the receiver is not the code of the action). (eosio::chain_plugin)
#disable-ram-billing-notify-checks = false

# Indicate a producer whose blocks headers signed by it will be fully validated, but transactions in those validated blocks will be trusted. (eosio::chain_plugin)
# trusted-producer = 

# Database map mode ("mapped", "heap", or "locked").
# In "mapped" mode database is memory mapped as a file.
# In "heap" mode database is preloaded in to swappable memory.
# In "locked" mode database is preloaded and locked in to memory.
#  (eosio::chain_plugin)
# database-map-mode = mapped

# Track actions which match receiver:action:actor. Actor may be blank to include all. Action and Actor both blank allows all from Recieiver. Receiver may not be blank. (eosio::history_plugin)
# filter-on = 

# Do not track actions which match receiver:action:actor. Action and Actor both blank excludes all from Reciever. Actor blank excludes all from reciever:action. Receiver may not be blank. (eosio::history_plugin)
# filter-out = 

# PEM encoded trusted root certificate (or path to file containing one) used to validate any TLS connections made.  (may specify multiple times)
#  (eosio::http_client_plugin)
# https-client-root-cert = 

# true: validate that the peer certificates are valid and trusted, false: ignore cert errors (eosio::http_client_plugin)
#https-client-validate-peers = true

# The local IP and port to listen for incoming http connections; set blank to disable. (eosio::http_plugin)
# http-server-address = 127.0.0.1:8888

# The local IP and port to listen for incoming https connections; leave blank to disable. (eosio::http_plugin)
# https-server-address = 

# Filename with the certificate chain to present on https connections. PEM format. Required for https. (eosio::http_plugin)
# https-certificate-chain-file = 

# Filename with https private key in PEM format. Required for https (eosio::http_plugin)
# https-private-key-file = 

# Configure https ECDH curve to use: secp384r1 or prime256v1 (eosio::http_plugin)
# https-ecdh-curve = secp384r1

# Specify the Access-Control-Allow-Origin to be returned on each request. (eosio::http_plugin)
# access-control-allow-origin = 

# Specify the Access-Control-Allow-Headers to be returned on each request. (eosio::http_plugin)
# access-control-allow-headers = 

# Specify the Access-Control-Max-Age to be returned on each request. (eosio::http_plugin)
# access-control-max-age = 

# Specify if Access-Control-Allow-Credentials: true should be returned on each request. (eosio::http_plugin)
#access-control-allow-credentials = false

# The maximum body size in bytes allowed for incoming RPC requests (eosio::http_plugin)
#max-body-size = 1048576

# Maximum size in megabytes http_plugin should use for processing http requests. 503 error response when exceeded. (eosio::http_plugin)
# http-max-bytes-in-flight-mb = 500

# Append the error log to HTTP responses (eosio::http_plugin)
# verbose-http-errors = false

# If set to false, then any incoming "Host" header is considered valid (eosio::http_plugin)
# http-validate-host = true

# Additionaly acceptable values for the "Host" header of incoming HTTP requests, can be specified multiple times.  Includes http/s_server_address by default. (eosio::http_plugin)
# http-alias = 

# Number of worker threads in http thread pool (eosio::http_plugin)
# http-threads = 2

# The maximum number of pending login requests (eosio::login_plugin)
# max-login-requests = 1000000

# The maximum timeout for pending login requests (in seconds) (eosio::login_plugin)
# max-login-timeout = 60

# The actual host:port used to listen for incoming p2p connections. (eosio::net_plugin)
# p2p-listen-endpoint = 0.0.0.0:9876

# An externally accessible host:port for identifying this node. Defaults to p2p-listen-endpoint. (eosio::net_plugin)
# p2p-server-address = 

# The public endpoint of a peer node to connect to. Use multiple p2p-peer-address options as needed to compose a network. (eosio::net_plugin)
# p2p-peer-address = 

# Maximum number of client nodes from any single IP address (eosio::net_plugin)
# p2p-max-nodes-per-host = 1

# The name supplied to identify this node amongst the peers. (eosio::net_plugin)
# agent-name = "EOS Test Agent"

# Can be 'any' or 'producers' or 'specified' or 'none'. If 'specified', peer-key must be specified at least once. If only 'producers', peer-key is not required. 'producers' and 'specified' may be combined. (eosio::net_plugin)
# allowed-connection = any

# Optional public key of peer allowed to connect.  May be used multiple times. (eosio::net_plugin)
# peer-key = 

# Tuple of [PublicKey, WIF private key] (may specify multiple times) (eosio::net_plugin)
# peer-private-key = 

# Maximum number of clients from which connections are accepted, use 0 for no limit (eosio::net_plugin)
# max-clients = 25

# number of seconds to wait before cleaning up dead connections (eosio::net_plugin)
# connection-cleanup-period = 30

# max connection cleanup time per cleanup call in millisec (eosio::net_plugin)
# max-cleanup-time-msec = 10

# True to require exact match of peer network version. (eosio::net_plugin)
# network-version-match = false

# Number of worker threads in net_plugin thread pool (eosio::net_plugin)
# net-threads = 1

# number of blocks to retrieve in a chunk from any individual peer during synchronization (eosio::net_plugin)
# sync-fetch-span = 100

# Enable expirimental socket read watermark optimization (eosio::net_plugin)
# use-socket-read-watermark = false

# The string used to format peers when logging messages about them.  Variables are escaped with ${<variable name>}.
# Available Variables:
#    _name  	self-reported name
# 
#    _id    	self-reported ID (64 hex characters)
# 
#    _sid   	first 8 characters of _peer.id
# 
#    _ip    	remote IP address of peer
# 
#    _port  	remote port number of peer
# 
#    _lip   	local IP address connected to peer
# 
#    _lport 	local port number connected to peer
# 
#  (eosio::net_plugin)
# peer-log-format = ["${_name}" ${_ip}:${_port}]

# Enable block production, even if the chain is stale. (eosio::producer_plugin)
enable-stale-production = true

# Start this node in a state where production is paused (eosio::producer_plugin)
# pause-on-startup = false

# Limits the maximum time (in milliseconds) that is allowed a pushed transaction's code to execute before being considered invalid (eosio::producer_plugin)
# max-transaction-time = 30

# Limits the maximum age (in seconds) of the DPOS Irreversible Block for a chain this node will produce blocks on (use negative value to indicate unlimited) (eosio::producer_plugin)
# max-irreversible-block-age = -1

# ID of producer controlled by this node (e.g. inita; may specify multiple times) (eosio::producer_plugin)
producer-name = eosio

# (DEPRECATED - Use signature-provider instead) Tuple of [public key, WIF private key] (may specify multiple times) (eosio::producer_plugin)
# private-key = 

# Key=Value pairs in the form <public-key>=<provider-spec>
# Where:
#    <public-key>    	is a string form of a vaild EOSIO public key
# 
#    <provider-spec> 	is a string in the form <provider-type>:<data>
# 
#    <provider-type> 	is KEY, or KEOSD
# 
#    KEY:<data>      	is a string form of a valid EOSIO private key which maps to the provided public key
# 
#    KEOSD:<data>    	is the URL where keosd is available and the approptiate wallet(s) are unlocked (eosio::producer_plugin)
signature-provider = EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV=KEY:5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3

# Limits the maximum time (in milliseconds) that is allowed for sending blocks to a keosd provider for signing (eosio::producer_plugin)
# keosd-provider-timeout = 5

# account that can not access to extended CPU/NET virtual resources (eosio::producer_plugin)
# greylist-account = 

# offset of non last block producing time in microseconds. Negative number results in blocks to go out sooner, and positive number results in blocks to go out later (eosio::producer_plugin)
# produce-time-offset-us = 0

# offset of last block producing time in microseconds. Negative number results in blocks to go out sooner, and positive number results in blocks to go out later (eosio::producer_plugin)
# last-block-time-offset-us = 0

# Maximum wall-clock time, in milliseconds, spent retiring scheduled transactions in any block before returning to normal transaction processing. (eosio::producer_plugin)
# max-scheduled-transaction-time-per-block-ms = 100

# ratio between incoming transations and deferred transactions when both are exhausted (eosio::producer_plugin)
# incoming-defer-ratio = 1

# Number of worker threads in producer thread pool (eosio::producer_plugin)
# producer-threads = 2

# the location of the snapshots directory (absolute path or relative to application data dir) (eosio::producer_plugin)
# snapshots-dir = "snapshots"

# the location of the state-history directory (absolute path or relative to application data dir) (eosio::state_history_plugin)
# state-history-dir = "state-history"

# enable trace history (eosio::state_history_plugin)
# trace-history = false

# enable chain state history (eosio::state_history_plugin)
# chain-state-history = false

# the endpoint upon which to listen for incoming connections. Caution: only expose this port to your internal network. (eosio::state_history_plugin)
# state-history-endpoint = 127.0.0.1:8080

# enable debug mode for trace history (eosio::state_history_plugin)
# trace-history-debug-mode = false

# Lag in number of blocks from the head block when selecting the reference block for transactions (-1 means Last Irreversible Block) (eosio::txn_test_gen_plugin)
# txn-reference-block-lag = 0

# Number of worker threads in txn_test_gen thread pool (eosio::txn_test_gen_plugin)
# txn-test-gen-threads = 2

# Prefix to use for accounts generated and used by this plugin (eosio::txn_test_gen_plugin)
# txn-test-gen-account-prefix = txn.test.

# Plugin(s) to enable, may be specified multiple times
# plugin = 

