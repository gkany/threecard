#pragma once

#include "utils.hpp"

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

#include <string>

using namespace eosio;
using namespace std;

CONTRACT threecard : public eosio::contract {
    public:
        using contract::contract;

        threecard(name receiver, name code, datastream<const char*> ds)
            : contract(receiver, code, ds),
              _global(receiver, receiver.value),
              _games(receiver, receiver.value),
              _deposits(receiver, receiver.value)
        {}

        ACTION hi(uint64_t id) {
            print("[action test]hi, ", id);
            //check(false, "this is a action test");

            //clean games table
            for(size_t i = 0; i <= id; i++) {
                auto iter = _games.find(id);
                if(iter != _games.end()) {
                    continue;
                }
               _games.erase(iter);
            }
        }

        void transfer(const name& from, const name& to, const asset& quantity, 
            const string& memo);

    private:
        tb_global                   _global;
        tb_games                    _games;
        tb_deposits                 _deposits;

    private:
        void parse_memo(string memo, string& action, map<string, string>& params);
        void parse_memo_start(string memo, uint64_t& id, uint64_t& volume, vector<string>& players);

        uint64_t next_id() {
            global g = _global.get_or_default(
                global{.current_id = _games.available_primary_key()});
            g.current_id += 1;
            _global.set(g, _self);
            return g.current_id;
        }

        // void save(const st_game& game) {
        //     _games.emplace(_self, [&](st_game& g) {
        //         g.id = game.id;
        //         g.players = game.players;
        //         g.cards = game.cards;
        //         g.amount = game.amount;
        //         g.created_at = game.created_at;
        //     });
        // }
        // void remove(const st_game& game) { 
        //     _games.erase(game); 
        // }

        //充值
        void deposit(const name& player, uint64_t quantity);

        //提币
        void withdraw(const name& player);

        //game start
        void start(uint64_t id, const vector<string>& players, uint64_t quantity);

        //加注
        void raise(uint64_t id, const string& player, uint64_t quantity);

        //弃牌
        void bust(uint64_t id, const string& player);

        //比牌
        void open(uint64_t id, const string& from, const string& to, uint64_t quantity);
};

