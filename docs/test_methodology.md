# Test Methodology

Unit tests are my primary method of verifying build correctness. As such, I've carefully considered **standard use and edge cases** to ensure that the following general invariants are held:

- **Order IDs are unique**
- **A particular level's volume is equal to the sum of each order's quantity currently resting on that level**
- **Best bid/ask are updated when the corresponding price level is emptied**
- **Executed/cancelled orders have their IDs removed from the ID-to-order map**
- **Memory pool expands with new volume and properly frees its owned memory**

The following is a comprehensive description of each unit test and each test family:

## Core Structure Tests:

### Memory Pool:

- Construction sanity checks
- Single allocation works
- Single deallocation works
- Multiple allocations construct distinct live objects
- Multiple deallocations return pool to empty live usage
- Freed blocks are reused without growing pool
- Pool grows by new slab when capacity is exceeded
- Reuses freed blocks before growing pool

### Price Ladder:

- Buy ladder constructs empty
- Sell ladder constructs empty
- Level lookup returns usable level at price
- Liquidity at price reflects underlying level state
- Counts non-empty levels
- Set best price stores configured price
- Buy ladder updates best price downward
- Sell ladder updates best price upward
- Buy ladder can find lowest configured price
- Sell ladder can find highest configured price
- Updating best price with no liquidity clears best price
- Buy ladder marketable liquidity returns true when enough liquidity exists
- Buy ladder marketable liquidity returns false when insufficient or not crossing
- Sell ladder marketable liquidity returns true when enough liquidity exists
- Sell ladder marketable liquidity returns false when insufficient or not crossing
- Empty ladder has no sufficient marketable liquidity
- Prune DAY orders removes only DAY orders and accumulates stats
- Prune DAY orders leaves best price alone when best level remains non-empty

### Price Level:

- Price level constructs empty
- Single push back works
- Multi-push back works
- Single pop front works
- Multi-pop front works
- Remove only order
- Remove head order
- Remove tail order
- Remove middle order
- Mixed order removals

## Book Operations Tests:

### Submit Good 'Till Cancelled or Day-Only Order

- Order book constructs
- Submit sell side order rests
- Submit buy side order rests
- Multiple sell orders at same level rest
- Multiple buy orders at same level rest
- Multiple sell orders at different levels rest
- Multiple buy orders at different levels rest
- Aggressive sell fully matches and does not rest
- Aggressive buy fully matches and does not rest
- Aggressive sell partially fills then rests remainder
- Aggressive buy partially fills then rests remainder
- Duplicate order id is rejected and does not mutate book
- Non-crossing order rests instead of executing
- Multi-level sweep produces multiple executions
- Same price fifo is preserved

### Submit Immediate or Cancel Order

- On empty book cancels
- Non-crossing buy cancels without resting
- Non-crossing sell cancels without resting
- Buy fully fills single ask
- Sell fully fills single bid
- Buy partially fills and cancels remainder
- Sell partially fills and cancels remainder
- Buy sweeps multiple levels and cancels remainder
- Sell sweeps multiple levels and cancels remainder
- Never rests residual on buy side
- Never rests residual on sell side
- Duplicate order id is rejected
- Buy exact touch crosses and fills
- Sell exact touch crosses and fills

### Submit Fill or Kill Order

- On empty book is killed
- Non-crossing buy is killed without mutation
- Non-crossing sell is killed without mutation
- Buy insufficient single level is killed without mutation
- Sell insufficient single level is killed without mutation
- Buy insufficient across multiple levels is killed without mutation
- Sell insufficient across multiple levels is killed without mutation
- Buy exact liquidity fully fills
- Sell exact liquidity fully fills
- Buy sufficient across multiple levels fully fills
- Sell sufficient across multiple levels fully fills
- Failed FOK never rests residual
- Duplicate order id is rejected before matching
- Buy exact touch crosses and fills
- Sell exact touch crosses and fills

### Submit Market Order

- Market order on empty book cancels
- Duplicate market order id is rejected
- Buy market order fully fills single ask
- Sell market order fully fills single bid
- Buy market order partially fills and cancels remainder
- Sell market order partially fills and cancels remainder
- Buy market order sweeps multiple ask levels and fills
- Sell market order sweeps multiple bid levels and fills
- Buy market order sweeps multiple ask levels and cancels remainder
- Sell market order sweeps multiple bid levels and cancels remainder
- Market order never rests residual on buy side
- Market order never rests residual on sell side

### Cancel Order

- Cancel missing order returns not found
- Cancel single bid order removes it
- Cancel single ask order removes it
- Cancel one of multiple orders at same bid level keeps level
- Cancel one of multiple orders at same ask level keeps level
- Cancel only order at bid level erases level
- Cancel only order at ask level erases level
- Cancel order at one bid level leaves other bid levels untouched
- Cancel order at one ask level leaves other ask levels untouched
- Cancel after partial execution cancels remaining open quantity
- Cancel does not touch opposite side book
- Re-cancelling same order returns not found
- Cancel after session end pruned DAY order returns not found

### Modify Order

- Modify missing order returns not found
- Modify to zero quantity becomes cancel
- Modify buy order to new non-crossing price resubmits and rests
- Modify sell order to new non-crossing price resubmits and rests
- Modify buy order can become aggressive and fully fill
- Modify sell order can become aggressive and fully fill
- Modify buy order can partially fill then rest remainder
- Modify sell order can partially fill then rest remainder
- Modify preserves side when resubmitting
- Modify preserves DAY lifetime for session end pruning
- Modify loses queue priority
- Modify preserves GTC lifetime through session end
- Modify after session end pruned DAY order returns not found
- Modify same price and same quantity still loses queue priority

### Mixed Operations

- Mixed operations leave consistent observable state
- Pool accounting tracks submit modify cancel and prune flows

### Session End

- Session end on empty book does nothing
- Session end prunes single DAY bid
- Session end prunes single DAY ask
- Session end leaves single GTC bid untouched
- Session end leaves single GTC ask untouched
- Session end prunes only DAY orders at same bid level
- Session end prunes only DAY orders at same ask level
- Session end erases level when all orders at bid level are DAY
- Session end erases level when all orders at ask level are DAY
- Session end prunes both sides and reports totals
- Session end does not remove anything when only GTC orders exist