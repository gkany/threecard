#include "threecard.hpp"

//充值
void threecard::deposit(const name& player, uint64_t quantity, bool flag) {
    check(is_account(player), player.to_string() + " deposit account does not exist");
    auto iter = _playerdata.find(player.value);
    playerdata pd;  //充值回执
    pd.player = player;
    if(iter == _playerdata.end()) {
        asset balance = asset(0, EOS_SYMBOL);
        balance.amount = quantity;
        pd.balance = balance;
        pd.count = 0;
        _playerdata.emplace(_self, [&](playerdata& d) {
            d.player = pd.player;
            d.balance = pd.balance;
            d.count = pd.count;
        });
        //check(false, "test deposit, new player");
    } else {
        auto balance = iter->balance;
        balance.amount += quantity;
        pd.balance = balance;
        pd.count = flag ? iter->count : iter->count - 1;
        _playerdata.modify(iter, _self, [&](auto &d) {
            d.player = pd.player;
            d.balance = pd.balance;
            d.count = pd.count;
        });
        //check(false, "test deposit, modify player");
    }
    depositrp(pd, flag ? "game over deposit" : "user deposit");  //充值回执
}

//提币
void threecard::withdraw(const name& player) {
    //require_auth(_self);
    check(is_account(player), player.to_string() + " withdraw account does not exist");
    auto existing = _playerdata.find(player.value);
    check(existing != _playerdata.end(), "player not found");
    check(existing->count <= 0, player.to_string() + " withdraw account in games, can't withdraw");
    if(existing->balance.amount > 0) {
        action(permission_level{ _self, N(active) },
            N(token), N(transfer),
            std::make_tuple(_self, player, existing->balance, std::string("withdraw")))
        .send();
    }
    _playerdata.erase(existing);
}

//game start
void threecard::start(uint64_t id, const vector<string>& players, uint64_t quantity, const string& seed) {
    size_t size = players.size();
    check(size >= 2 && size <= 5, "players number error");
    auto existing = _games.find(id);
    check(existing == _games.end(), "game id already exist");
    map<string, bool> mPlayers;

    for(size_t i = 0; i < size; i++) {
        auto it = _playerdata.find(name(players[i]).value);
        check(it != _playerdata.end() && it->balance.amount >= quantity, players[i] + " need deposit");
        auto balance = it->balance;
        balance.amount -= quantity;
        _playerdata.modify(it, _self, [&](auto& d) {
            d.player = it->player;
            d.balance = balance;
            d.count = it->count + 1;
        });
        mPlayers.insert(std::pair<string, bool>(players[i], true)); 
    }

    map<string, playercard> cards;
    asset baseamount = asset(0, EOS_SYMBOL);
    baseamount.amount = quantity * mPlayers.size();
    _games.emplace(_self, [&](game& g) {
        g.id = id;
        g.players = mPlayers;
        g.cards = cards; //init empty
        g.amount = baseamount;
        g.winner = "";
        g.size = mPlayers.size();
        g.created_at = time_point_sec(current_time_point());
    });

    //seed save seeds table
    //random cards and sort cards and calc value
    cards = getRandomCards(players, seed);
    auto dataiter = _gamedata.find(id);
    if(dataiter == _gamedata.end()) {
        _gamedata.emplace(_self, [&](gamedata& s) {
            s.id = id; 
            s.seed = seed;
            s.cards = cards;
        }); 
    } else {
        _gamedata.modify(dataiter, _self, [&](auto &s) {
            s.id = id; 
            s.seed = seed;
            s.cards = cards;
        });
    }
}

//加注
void threecard::raise(uint64_t id, const name& player, const asset& quantity) {
    check(is_account(player), "player is not account");
    auto iter = _games.find(id);
    check(iter != _games.end(), "game id not found");
    auto existing = iter->players.find(player.to_string());
    check(existing != iter->players.end() && existing->second, 
        "player not found or player has bust");

    auto it = _playerdata.find(player.value);
    check(it != _playerdata.end(), player.to_string() +  " not found");
    check(it != _playerdata.end() && it->balance.amount >= quantity.amount, 
        player.to_string() + " need deposit.");
    auto balance = it->balance;
    balance.amount -= quantity.amount;
    _playerdata.modify(it, _self, [&](auto& d) {
        d.player = it->player;
        d.balance = balance;
        d.count = it->count;
    });

    auto amount = iter->amount;
    amount.amount += quantity.amount;
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

    auto dataiter = _gamedata.find(id);
    check(dataiter != _gamedata.end(), "game data not found. id: " + std::to_string(id));

    auto privatecards = dataiter->cards;
    auto carditer = privatecards.find(player);
    check(carditer != privatecards.end(), player + " private card not found");

    auto publiccards = iter->cards;
    publiccards.insert(std::pair<string, playercard>(player, carditer->second));
    privatecards.erase(carditer);

    if (iter->size > 2) {
        _games.modify(iter, _self, [&](auto &g) {
            g.id = iter->id;
            g.players = players;
            g.cards = publiccards;
            g.amount = iter->amount;
            g.winner = (iter->winner == player ? "" : iter->winner);
            g.size = iter->size - 1;
            g.created_at = iter->created_at;
        });

        _gamedata.modify(dataiter, _self, [&](auto &s) {
            s.id = dataiter->id; 
            s.seed = dataiter->seed;
            s.cards = privatecards;
        });
    } else {
        string memo;
        // TODO: winner memo, include balance, player and player cards
        memo = "winner";  //test


        //get winner
        string winner;
        if(iter->winner.empty()) {
            for(auto player = players.begin(); player != players.end(); player++) {
                if(player->second) {
                    winner = player->first;
                    break;
                }
            }
        } else {
            winner = iter->winner;
        }
        deposit(N(winner), iter->amount.amount, true);
        {
            game g;  //游戏结束回执
            g.id = id;
            g.players = players;
            g.cards = publiccards;
            g.amount = iter->amount;
            g.winner = winner;
            g.size = iter->size - 1;
            g.created_at = iter->created_at;
            receipt(g);
        }
        _games.erase(iter);
        _gamedata.erase(dataiter);
    }
}

void threecard::open(uint64_t id, const string& player1, const string& player2, uint64_t quantity) {
    print("player1: ", player1, ", player2: ", player2, "\n");
    auto existing = _games.find(id);
    check(existing != _games.end() && existing->size >= 2, "game id not found or players not enough");
    auto players = existing->players;
    auto iter1 = players.find(player1);
    check(iter1 != players.end() && iter1->second, player1 + " player not found or already is false");
    auto iter2 = players.find(player2);
    check(iter2 != players.end() && iter2->second, player2 + " player not found or already is false");
    
    string winner, loser;
    auto cards = existing->cards;
    if(cardCompare(cards[player1], cards[player2])) {
        winner = player1;
        loser = player2;
    } else {
        winner = player2;
        loser = player1;
    }
    players[loser] = false;

    auto amount = existing->amount;
    amount.amount += quantity;

    auto dataiter = _gamedata.find(id);
    check(dataiter != _gamedata.end(), "game data not found. id: " + std::to_string(id));

    auto privatecards = dataiter->cards;
    auto carditer = privatecards.find(loser);
    check(carditer != privatecards.end(), loser + " private card not found");

    auto publiccards = existing->cards;
    publiccards.insert(std::pair<string, playercard>(loser, carditer->second));
    privatecards.erase(carditer);

    if (existing->size > 2) {
        _games.modify(existing, _self, [&](auto &g) {
            g.id = existing->id;
            g.players = players;
            g.cards = publiccards;
            g.amount = amount;
            g.winner = winner;
            g.size = existing->size - 1;
            g.created_at = existing->created_at;
        });

        _gamedata.modify(dataiter, _self, [&](auto &s) {
            s.id = dataiter->id; 
            s.seed = dataiter->seed;
            s.cards = privatecards;
        });
    } else {
        //string memo;
        // TODO: winner memo, include balance, player and player cards
        //memo = "winner";  //test
        //print("winner: ", winner, ", loser: ", loser, "\n");

        auto winiter = privatecards.find(winner);
        check(winiter != privatecards.end(), winner + " private card not found");
        publiccards.insert(std::pair<string, playercard>(winner, winiter->second));
        {
            game g;  //游戏结束回执
            g.id = id;
            g.players = players;
            g.cards = publiccards;
            g.amount = existing->amount;
            g.winner = winner;
            g.size = existing->size - 1;
            g.created_at = existing->created_at;
            receipt(g);
        }
        deposit(name(winner), amount.amount, true);
        _games.erase(existing);

        auto dataiter = _gamedata.find(id);
        check(dataiter != _gamedata.end(), "game data not found. id: " + std::to_string(id));
        _gamedata.erase(dataiter);
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

//raise: raise-id-; bust: bust-id-
void threecard::parse_memo_raise_or_bust(string memo, uint64_t& id) {
    // remove space
    memo.erase(std::remove_if(memo.begin(), memo.end(), [](unsigned char x) { 
            return std::isspace(x); }), memo.end());

    size_t pos;
    string container;
    pos = sub2sep(memo, container, '-', 0, true);
    check(!container.empty(), "no action");
    // action = container;
    //print("parse_memo_raise_or_bust, action: ", action);

    pos = sub2sep(memo, container, '-', ++pos, true);
    check(!container.empty(), "no game id");
    id = stoi(container);
    print("parse_memo_raise_or_bust, id: ", id, "\n");
}

//open-id-player2-
void threecard::parse_memo_open_card(string memo, uint64_t& id, string& player) {
    // remove space
    memo.erase(std::remove_if(memo.begin(), memo.end(), [](unsigned char x) { 
            return std::isspace(x); }), memo.end());

    size_t pos;
    string container;
    pos = sub2sep(memo, container, '-', 0, true);
    check(!container.empty(), "no action");
    // action = container;
    //print("parse_memo_open_card, action: ", action);

    pos = sub2sep(memo, container, '-', ++pos, true);
    check(!container.empty(), "no game id");
    id = stoi(container);
    print("parse_memo_open_card, id: ", id, "\n");

    pos = sub2sep(memo, container, '-', ++pos, true);
    check(!container.empty(), "no player2");
    player = container;
    print("parse_memo_open_card, player2: ", player, "\n");
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
        deposit(from, quantity.amount, false);
        return;
    } else if(action == "withdraw") {
        withdraw(from);
        return;
    } else if(action == "start") {
        check(from == GAME_ACCOUNT, "from account don't start game");
        uint64_t id;
        uint64_t volume;
        vector<string> players;
        //this is a test
        string seed = "52f80f5bd46bf55e4e6370750615535039b0f6e91ab2f21ac577460cd7c059c7";  //实际环境是从 memo 传过来的加密的seed
        parse_memo_start(memo, id, volume, players);
        start(id, players, volume, seed);
        return;
    } else if (action == "raise") {
        //check(from == GAME_ACCOUNT, "from account don't raise");
        uint64_t id;
        parse_memo_raise_or_bust(memo, id);
        raise(id, from, quantity);
        return;
    } else if (action == "bust") {
        //check(from == GAME_ACCOUNT, "from account don't bust");
        uint64_t id;
        parse_memo_raise_or_bust(memo, id);
        bust(id, from.to_string());
        return;
    } else if (action == "open") {
        //check(from == GAME_ACCOUNT, "from account don't open card");
        //open(uint64_t id, const string& player1, const string& player2, uint64_t quantity)
        uint64_t id;
        string player2;
        parse_memo_open_card(memo, id, player2);
        open(id, from.to_string(), player2, quantity.amount);
        return;
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

EOSIO_DISPATCH(threecard, (clean))

