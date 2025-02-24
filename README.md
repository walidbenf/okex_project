# OKEX Connector Implementation

## Overview
Implementation of the CCAPI connector for OKX exchange, supporting:
- Order management with client identifiers (clOrdId)
- Batch order cancellation by client identifiers
- Market data streaming

## Features
- Create orders with custom client identifiers
- Cancel multiple orders using client identifiers
- Support for test accounts via simulated trading header

## Implementation Details
### Order Management
- Place orders with client identifiers (clOrdId)
- Cancel orders in batch using client identifiers
- Support for cash trading mode

### Authentication
- API Key authentication
- HMAC SHA256 request signing
- Simulated trading support for test accounts
## Build
```
bash clean_build.sh
```
## Testing
Run tests with:
```bash
bash test_unit.sh
```
```bash
bash test_integration.sh
```

## Dependencies
- Boost
- OpenSSL
- RapidJSON
