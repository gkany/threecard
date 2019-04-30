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
              //_balance(receiver, receiver.value),
              _gamedata(receiver, receiver.value),
              _playerdata(receiver, receiver.value)
        {}

        //table: global, game, deposit
        TABLE global {
            uint64_t                    current_id;
        };

        TABLE game {
            uint64_t                    id;             //house id
            uint8_t                     size;           //surplus player number
            map<string, bool>           players;        //player and player status
            map<string, playercard>     cards;          //public cards
            asset                       amount;         //fund pool
            string                      winner;         //current winner
            time_point_sec              created_at;     //house create time

            uint64_t primary_key() const { return id; }
        };

        // TABLE balance {
        //     name                        player;
        //     asset                       balance;

        //     uint64_t primary_key() const { return player.value; }
        // };

        TABLE playerdata {
            name                        player;
            asset                       balance;
            uint64_t                    count;

            uint64_t primary_key() const { return player.value; }
        };

        //private table
        struct gamedata {
            uint64_t                    id;             //game id
            string                      seed;           //random cards use
            map<string, playercard>     cards;          //surplus cards

            uint64_t primary_key() const { return id; }
        };

        typedef singleton<N(global), global>         tb_global;
        typedef multi_index<N(games), game>          tb_games;
        //typedef multi_index<N(balance), balance>     tb_balance;
        typedef multi_index<N(gamedata), gamedata>   tb_gamedata;
        typedef multi_index<N(players), playerdata>  tb_playerdata;
        //table end

    private:
        tb_global                   _global;
        tb_games                    _games;
        //tb_balance                  _balance;
        tb_gamedata                 _gamedata;
        tb_playerdata               _playerdata;

    private:
        void parse_memo(string memo, string& action, map<string, string>& params);
        void parse_memo_start(string memo, uint64_t& id, uint64_t& volume, vector<string>& players);
        void parse_memo_raise_or_bust(string memo, uint64_t& id);
        void parse_memo_open_card(string memo, uint64_t& id, string& player);

        // uint64_t next_id() {
        //     global g = _global.get_or_default(
        //         global{.current_id = _games.available_primary_key()});
        //     g.current_id += 1;
        //     _global.set(g, _self);
        //     return g.current_id;
        // }

        //充值
        void deposit(const name& player, uint64_t quantity, bool flag = false);

        //提币
        void withdraw(const name& player);

        //game start
        void start(uint64_t id, const vector<string>& players, uint64_t quantity, const string& seed);

        //加注
        void raise(uint64_t id, const name& player, const asset& quantity);

        //弃牌
        void bust(uint64_t id, const string& player);

        //比牌
        void open(uint64_t id, const string& from, const string& to, uint64_t quantity);

public:

        ACTION receipt(const game& data) {
            //require(_self, "game over receipt error");
        }

        ACTION depositrp(const playerdata& data, const string& memo) {
            //require(_self, "deposit receipt error");
            //require_auth(_self);
        }  //deposit receipt

        //clean games, balance table //for test
        ACTION clean(const uint64_t& id) {
            auto existing = _games.find(id);
            if(existing != _games.end()) {
                _games.erase(existing);
            }
        }

        void transfer(const name& from, const name& to, const asset& quantity, 
            const string& memo);
};

