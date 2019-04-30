// #pragma once

#include "picosha2.h"

#include <iostream>
#include<sstream>
#include <string>
#include <vector>
#include <map>



using namespace std;

// //发牌数组
// int cards52[52] = {
//         0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E, //方块 2 - K - A
//         0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E, //梅花 2 - K - A
//         0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E, //红桃 2 - K - A
//         0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E, //黑桃 2 - K - A
// };

// enum flowertype {
//     DIAMOND = 0,            // 方片
//     CLUB,                   // 梅花
//     HEART,                  // 红桃
//     SPADE                   // 黑桃
// };

// enum cardnum { 
//     card2 = 2, card3 = 3, card4 = 4, card5 = 5, card6 = 6, card7 = 7, card8 = 8, 
//     card9 = 9, card10 = 10, cardJ = 11, cardQ = 12, cardK = 13, cardA = 14
// };

// enum playertype {
//     NORMAL = 0,             //普通牌
//     DOUBLE,                 //对子
//     STRAIGHT,               //顺子
//     FLUSH,                  //同花
//     STRAIGHT_FLUSH,         //同花顺
//     BOMB                    //炸弹
// };

// enum playertypemax {
//     NORMAL_MAX_VALUE            = 3803,     //普通牌: 最大值AKJ=14*16*16+13*16+11=3803
//     DOUBLE_MAX_VALUE            = 4040,     //对子牌: 最大值AAK=14*16+13=237,加上普通牌的基数3803=4040
//     STRAIGHT_MAX_VALUE          = 4052,     //顺子牌: 最大值AKQ=12，加上对子的最大值基数4040=4052
//     FLUSH_MAX_VALUE             = 7855,     //同花牌: 最大值AKJ，14*16*16+13*16+11=3803，加上顺子4052=7855
//     STRAIGHT_FLUSH_MAX_VALUE    = 7867,     //同花顺: 最大值AKQ=12，加上同花7855=7867
//     BOMB_MAX_VALUE              = 7881      //炸弹牌: 最大值AAA=14，加上同花顺7867=7881
// };

// struct card {
//     uint8_t                     flower;     //花色: flowertype
//     uint8_t                     number;     //牌点数: cardnum
// };

// struct playercard {
//     vector<card>                cards;      
//     uint8_t                     cardtype;  //牌类型：playtype
//     bool                        a32;       //是否是A32
//     int                         value;     //牌绝对值大小
// };

// vector<int> parsesha256(const string& seeds) {
//     vector<int> seed;
//     for(int i = 0; i < seeds.size(); i++) {
//         int n = seeds[i] - '0';
//         if(n >= 48) {
//             n = seeds[i] -'a' + 10;
//         }
//         seed.push_back(n);
//     }
//     return seed;
// }

// map<string, playercard> getRandomCards(const vector<string>& players, const string& seeds) {
//     vector<int> cards(cards52, cards52+52);
//     vector<int> seed = parsesha256(seeds);
//     int size = players.size();
//     int cardsize = cards.size();
//     map<string, playercard> playercards;
//     for(size_t i = 0; i < size*3; i++) {
//         int d = (i < size) ? 11 : 1;
//         int index = (seed[i]*17+seed[i+1]*13+seed[i+2]*d)%cardsize;
//         int random = cards[index];
//         card c;
//         c.flower = random/16;
//         c.number = random%16;
//         if(i < size) {
//             playercard pc;
//             pc.cards.push_back(c);
//             playercards.insert(std::pair<string, playercard>(players[i], pc));
//         } else {
//             string player = players[i%size];
//             playercards[player].cards.push_back(c);
//         }
//         std::remove(cards.begin(), cards.end(), random);
//         cardsize--;
//     }
//     return playercards;
// }

// string seedHash() {
//     string text = "this is a test. random hash text. ff3f4036a1164d1ddbad5b3edf9022addb3e1961a54a922708a6c1ffc49e5489 ";
//     time_t t = time(NULL);  //秒时间
//     char buf[128] = {0};
//     tm* gmt = gmtime(&t);  //转为格林威治时间  
//     strftime(buf, 64, "%Y-%m-%d %H:%M:%S", gmt);  
//     text += string(buf);
//     return picosha2::hash256_hex_string(text);
// }

int main()
{
    istringstream istream;

    // //string seed = "96f06aec039a44b9b053079476554fd6954ecd5a126692463cc3dab112b172199a3ea8a32ab998156ec47157b1bb595d3aa88277a89cb2a78302668d8560d50e";
    // string seed = "ff3f4036a1164d1ddbad5b3edf9022addb3e1961a54a922708a6c1ffc49e5489";
    // for(int i = 0; i < seed.size(); i++) {
    //     // istream.str(std::to_string(seed[i]));
    //     // int n;
    //     // istream >> n;
    //     // cout << "n = " << n << endl;

    //     int n = seed[i] - '0';
    //     if(n >= 48) {
    //         n = seed[i] -'a' + 10;
    //     }
    //     cout << n << " ";
    // }

    // cout << "seed size: " << seed.size() << endl;

    //string seed = "ff3f4036a1164d1ddbad5b3edf9022addb3e1961a54a922708a6c1ffc49e5489";
    string src_str = "this is a test. random hash text. ff3f4036a1164d1ddbad5b3edf9022addb3e1961a54a922708a6c1ffc49e5489 ";
    time_t t = time(NULL);  //秒时间
    char buf[128]= {0};
    tm* gmt = gmtime(&t);  //转为格林威治时间  
    strftime(buf, 64, "%Y-%m-%d %H:%M:%S", gmt);  
    std::cout << buf << std::endl; 
    src_str += string(buf);
    cout << "src_str: " << src_str << endl;
    string hash_hex_str = picosha2::hash256_hex_string(src_str);
    cout << "hash_hex_str: " << hash_hex_str << endl;
    string seed = hash_hex_str;
    for(int i = 0; i < seed.size(); i++) {
        int n = seed[i] - '0';
        if(n >= 48) {
            n = seed[i] -'a' + 10;
        }
        cout << n << " ";
    }
    cout << "seed size: " << seed.size() << endl;

    // vector<string> players;
    // players.push_back("Jane");
    // players.push_back("Lisa");
    // players.push_back("Peter");
    // map<string, playercard> cards = getRandomCards(players, seed);
    // for(auto iter = cards.begin(); iter != cards.end(); iter++) {
    //     cout << iter->first << ", cards:  " << endl;
    //     auto player = iter->second.cards;
    //     for(auto card = player.begin(); card != player.end(); card++) {
    //         cout << "flower: " << card->flower << " | card: " << card->number << endl;
    //     }
    //     cout << endl << endl;
    // }

    return 0;
}