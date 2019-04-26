#include "threecard.hpp"

//充值
void threecard::deposit(const name& player, uint64_t quantity) {
    check(is_account(player), "player account does not exist");
    auto iter = _deposits.find(player.value);
    if(iter == _deposits.end()) {
        asset amount = asset(0, EOS_SYMBOL);
        amount.amount = quantity;
        _deposits.emplace(_self, [&](struct deposit& d) {
            d.player = player;
            d.balance = amount;
        });
        //check(false, "test deposit, new player");
    } else {
        auto balance = iter->balance;
        balance.amount += quantity;
        _deposits.modify(iter, _self, [&](auto &d) {
            d.player = iter->player;
            d.balance = balance;
        });
        //check(false, "test deposit, modify player");
    }
}

//提币
void threecard::withdraw(const name& player) {
    //require_auth(_self);
    check(is_account(player), "player account does not exist");
    auto existing = _deposits.find(player.value);
    check(existing != _deposits.end(), "player not found");
    if(existing->balance.amount > 0) {
        action(permission_level{ _self, N(active) },
            N(token), N(transfer),
            std::make_tuple(_self, player, existing->balance, std::string("withdraw")))
        .send();
    }
    _deposits.erase(existing);
}

//game start
void threecard::start(uint64_t id, const vector<string>& players, uint64_t quantity) {
    size_t size = players.size();
    check(size >= 2 && size <= 5, "players number error");
    auto existing = _games.find(id);
    check(existing == _games.end(), "game id already exist");
    map<string, bool> mPlayers;

    for(size_t i = 0; i < size; i++) {
        auto it = _deposits.find(name{players[i]}.value);
        check(it != _deposits.end() && it->balance.amount >= quantity, players[i] + " need deposit");
        auto balance = it->balance;
        balance.amount -= quantity;
        _deposits.modify(it, _self, [&](auto& d) {
            d.player = it->player;
            d.balance = balance;
        });
        mPlayers.insert(std::pair<string, bool>(players[i], true)); 
    }

    asset bet = asset(0, EOS_SYMBOL);
    bet.amount = quantity * mPlayers.size();

    //random cards
    map<string, playercard> cards;
    // TODO: 发牌

    _games.emplace(_self, [&](game& g) {
        g.id = id;
        g.players = mPlayers;
        g.cards = cards;
        g.amount = bet;
        g.winner = "";
        g.size = mPlayers.size();
        g.created_at = time_point_sec(current_time_point());
    });
}

//加注
void threecard::raise(uint64_t id, const string& player, uint64_t quantity) {
    auto iter = _games.find(id);
    check(iter != _games.end(), "game id not found");
    auto existing = iter->players.find(player);
    check(existing != iter->players.end() && existing->second, 
        player + " not found or player has bust");

    auto it = _deposits.find(N(player).value);
    check(it != _deposits.end() && it->balance.amount >= quantity, 
        player + " need deposit");
    auto balance = it->balance;
    balance.amount -= quantity;
    _deposits.modify(it, _self, [&](auto& d) {
        d.player = it->player;
        d.balance = balance;
    });

    auto amount = iter->amount;
    amount.amount += quantity;
    _games.modify(iter, _self, [&](auto &g) {
        g.id = iter->id;
        g.players = iter->players;
        g.cards = iter->cards;
        g.amount = amount;
        g.winner = iter->winner;
        g.size = iter->size;
        g.created_at = iter->created_at;
    });
}

//弃牌
void threecard::bust(uint64_t id, const string& player) {
    auto iter = _games.find(id);
    check(iter != _games.end() && iter->size > 1, "game id not found");
    auto players = iter->players;
    auto existing = players.find(player);
    check(existing != players.end(), "player not found");
    players[player] = false;
    _games.modify(iter, _self, [&](auto &g) {
        g.id = iter->id;
        g.players = players;
        g.cards = iter->cards;
        g.amount = iter->amount;
        g.winner = (iter->winner == player ? "" : iter->winner);
        g.size = iter->size - 1;
        g.created_at = iter->created_at;
    });

    if (iter->size == 2) {
        // TODO: game over
    }
}

void threecard::open(uint64_t id, const string& player1, const string& player2, uint64_t quantity) {
    auto existing = _games.find(id);
    check(existing != _games.end() && existing->size >= 2, "game id not found or players not enough");
    auto players = existing->players;
    auto iter1 = players.find(player1);
    check(iter1 != players.end() && iter1->second, player1 + " player not found or already is false");
    auto iter2 = players.find(player2);
    check(iter2 != players.end() && iter2->second, player2 + " player not found or already is false");
    
    string loser;
    string winner;
    // TODO: 比牌逻辑
    players[loser] = false;

    auto amount = existing->amount;
    amount.amount += quantity;
    _games.modify(existing, _self, [&](auto &g) {
        g.id = existing->id;
        g.players = players;
        g.cards = existing->cards;
        g.amount = amount;
        g.winner = winner;
        g.size = existing->size - 1;
        g.created_at = existing->created_at;
    });

    if (existing->size == 2) {
        // TODO: game over
    }
}

void threecard::parse_memo(string memo, string& action, map<string, string>& params) {
    // remove space
    memo.erase(std::remove_if(memo.begin(), memo.end(), [](unsigned char x) { 
            return std::isspace(x); }), memo.end());

    size_t pos;
    std::string container;
    pos = sub2sep(memo, container, '-', 0, true);
    check(!container.empty(), "no action");
    action = container;
    if (container == "order") {
        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no owner");
        params["owner"] = container;

        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no nft id");
        params["nftid"] = container;

        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no price");
        params["price"] = container;

        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no side");
        check(container == "buy" || container == "sell", "side must be buy or sell");
        params["side"] = container;
    } else if (container == "trade") {
        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no from account");
        params["from"] = container;

        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no to account");
        params["to"] = container;

        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no order id");
        params["id"] = container;

        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no side");
        check(container == "buy" || container == "sell", "side must be buy or sell");
        params["side"] = container;
    } else if (container == "cancel") {
        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no from account");
        params["owner"] = container;

        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no order id");
        params["id"] = container;
    }
}

//memo: start-id-volume-playerSize-player1-player2-player3-...
void threecard::parse_memo_start(string memo, uint64_t& id, uint64_t& volume, vector<string>& players) {
    // remove space
    memo.erase(std::remove_if(memo.begin(), memo.end(), [](unsigned char x) { 
            return std::isspace(x); }), memo.end());

    size_t pos;
    string container;
    pos = sub2sep(memo, container, '-', 0, true);
    check(!container.empty(), "no action");
    // action = container;
    //print("parse_memo_start, action: ", action);

    pos = sub2sep(memo, container, '-', ++pos, true);
    check(!container.empty(), "no game id");
    id = stoi(container);
    print("parse_memo_start, id: ", id, "\n");

    pos = sub2sep(memo, container, '-', ++pos, true);
    check(!container.empty(), "no volume");
    volume = stoi(container);
    print("parse_memo_start, volume: ", volume, "\n");

    pos = sub2sep(memo, container, '-', ++pos, true);
    check(!container.empty(), "no players size");
    int size = stoi(container);
    print("parse_memo_start, size: ", size, "\n");
    //check(false, "this is parse_memo_start test");

    for(int i = 1; i <= size; i++) {
        pos = sub2sep(memo, container, '-', ++pos, true);
        check(!container.empty(), "no player");
        string player = container;
        players.push_back(player);
        print("parse_memo_start, player: ", player, "\n");
    }
}

void threecard::transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    check(is_account(from), "from account does not exist");
    check(is_account(to), "to account does not exist");
    check(quantity.symbol.is_valid(), "invalid symbol name");
    check(quantity.symbol.code().to_string() == "EOS", "currency must be EOS");
    check(memo.size() <= 256 && !memo.empty(), "memo has more than 256 bytes or is empty");
    if (from == _self || to != _self) {
        return;
    }

    if (memo.find('-') == std::string::npos) {
        return;
    }

    std::string action;
    size_t pos = sub2sep(memo, action, '-', 0, true);
    if(action == "deposit") {
        deposit(from, quantity.amount);
        return;
    } else if(action == "withdraw") {
        withdraw(from);
        return;
    } else if(action == "start") {
        check(from == GAME_ACCOUNT, "from account don't start game");
        uint64_t id;
        uint64_t volume;
        vector<string> players;
        parse_memo_start(memo, id, volume, players);
        start(id, players, volume);
        return;
    } else if (action == "raise") {
        check(from == GAME_ACCOUNT, "from account don't raise");

    } else if (action == "bust") {
        check(from == GAME_ACCOUNT, "from account don't bust");

    } else if (action == "open") {
        check(from == GAME_ACCOUNT, "from account don't open card");

    }
}

#ifdef EOSIO_DISPATCH
#undef EOSIO_DISPATCH
#endif
#define EOSIO_DISPATCH( TYPE, MEMBERS )                                         \
extern "C" {                                                                    \
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {             \
        if ( code == receiver ) {                                               \
            switch( action ) {                                                  \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS )                          \
            }                                                                   \
        }                                                                       \
        if (code == N(token).value && action == N(transfer).value) {            \
            execute_action(name(receiver), name(receiver), &threecard::transfer);    \
            return;                                                             \
        }                                                                       \
        if (action == N(transfer).value) {                                      \
            check(false, "only support EOS token");                             \
        }                                                                       \
    }                                                                           \
}

EOSIO_DISPATCH(threecard, (hi))

