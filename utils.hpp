#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

#include <string>
#include <map>
#include <vector>
#include <list>
#include <algorithm>
#include <utility>

using namespace eosio;
using namespace std;


#define N(X)            eosio::name(#X)
#define S(P,X)          eosio::symbol(#X,P)
#define EOS_SYMBOL      S(4, EOS)

#define TRANSFER_CODE   token       //eosio.token
#define FEE             1           //0.0001 eos
#define GAME_ACCOUNT    name("dev")

//发牌数组
int cards52[52] = {
        0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E, //方块 2 - K - A
        0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E, //梅花 2 - K - A
        0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E, //红桃 2 - K - A
        0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E, //黑桃 2 - K - A
};

enum flowertype {
    DIAMOND = 0,            // 方片 ♦
    CLUB,                   // 梅花 ♣
    HEART,                  // 红桃 ♥
    SPADE                   // 黑桃 ♠
};

enum cardnum { 
    card2 = 2, card3 = 3, card4 = 4, card5 = 5, card6 = 6, card7 = 7, card8 = 8, 
    card9 = 9, card10 = 10, cardJ = 11, cardQ = 12, cardK = 13, cardA = 14
};

enum playertype {
    NORMAL = 0,             //普通牌
    DOUBLE,                 //对子
    STRAIGHT,               //顺子
    FLUSH,                  //同花
    STRAIGHT_FLUSH,         //同花顺
    BOMB                    //炸弹
};

enum playertypemax {
    NORMAL_MAX_VALUE            = 3803,     //普通牌: 最大值AKJ=14*16*16+13*16+11=3803
    DOUBLE_MAX_VALUE            = 4040,     //对子牌: 最大值AAK=14*16+13=237,加上普通牌的基数3803=4040
    STRAIGHT_MAX_VALUE          = 4052,     //顺子牌: 最大值AKQ=12，加上对子的最大值基数4040=4052
    FLUSH_MAX_VALUE             = 7855,     //同花牌: 最大值AKJ，14*16*16+13*16+11=3803，加上顺子4052=7855
    STRAIGHT_FLUSH_MAX_VALUE    = 7867,     //同花顺: 最大值AKQ=12，加上同花7855=7867
    BOMB_MAX_VALUE              = 7881      //炸弹牌: 最大值AAA=14，加上同花顺7867=7881
};

struct card {
    uint8_t                     flower;     //花色: flowertype
    uint8_t                     number;     //牌点数: cardnum
};

struct playercard {
    vector<card>                cards;      
    uint8_t                     cardtype;  //牌类型：playtype
    bool                        a32;       //是否是A32
    int                         value;     //牌绝对值大小
};

size_t sub2sep( const std::string& input, std::string& output, const char& separator,
        size_t first_pos = 0, bool required = false ) {
    check(first_pos != std::string::npos, "invalid first pos");
    auto pos = input.find(separator, first_pos);
    if (pos == std::string::npos) {
        check(!required, "parse memo error");
        return std::string::npos;
    }
    output = input.substr(first_pos, pos - first_pos);
    return pos;
}

//获取玩家牌的绝对值大小
//cards已经按照牌值从大到小排序：888 | AKQ, 987, A32 | AKJ, 965 | AKQ, 987, A32 | AAJ 996
//a, 获取炸弹牌值绝对大小
int getBombValue(const playercard& player) {
    return player.cards[0].number + STRAIGHT_FLUSH_MAX_VALUE;
}

//b, 获取同花顺牌值绝对大小, 特殊：A32也是同花顺，是最小的同花顺
int getStraightFlushValue(const playercard& player) {
    if (player.a32) {
        return 1 + FLUSH_MAX_VALUE;  //此时A就等于是1
    }
    return player.cards[2].number + FLUSH_MAX_VALUE;
}

//c, 获取同花牌值绝对大小
int getFlushValue(const playercard& player) {
    return player.cards[0].number * 256 + player.cards[1].number * 16 + player.cards[2].number
            + STRAIGHT_MAX_VALUE;
}

//d, 获取顺子牌值绝对大小
int getStraightValue(const playercard& player) {
    if (player.a32) {
        return 1 + DOUBLE_MAX_VALUE;   //此时A就等于是1
    }
    return player.cards[2].number + DOUBLE_MAX_VALUE;
}

//e, 获取对子牌值绝对大小
// 在判断牌型时，如果是对子，则将对子放在数组前面两位
int getDoubleValue(const playercard&  player) {
    return player.cards[1].number * 16 + player.cards[2].number + NORMAL_MAX_VALUE;
}

//f, 获取普通牌值绝对大小
int getNormalValue(const playercard&  player) {
    return player.cards[0].number * 256 + player.cards[1].number * 16 + player.cards[2].number;
}

//玩家牌排序
//a, 将一副牌按牌面从大到小排序
void sortPlayerByNumber(playercard&  player) {
    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3 - i - 1; j++) {
            if (player.cards[j].number < player.cards[j + 1].number) {
                card tempCard =player.cards[j];
                player.cards[j] = player.cards[j + 1];
                player.cards[j + 1] = tempCard;
            }
        }
    }
}

//b, 将对子移动到一幅牌的前两位
void moveDouble2Front(playercard&  player) {
    if (player.cards[1].number == player.cards[2].number) {
        card tempCard = player.cards[0];
        player.cards[0] = player.cards[2];
        player.cards[2] = tempCard;
    }
}

//牌类型判断, cards已经按照牌值从大到小排序：888 | AKQ, 987, A32 | AKJ, 965 | AKQ, 987, A32 | AAJ 996
// a, 是否炸弹
bool isBomb(const playercard& player) {
    return player.cards[0].number == player.cards[1].number
            && player.cards[1].number == player.cards[2].number;
}

// b, 是否同花
bool isFlush(const playercard& player) {
    return player.cards[0].flower == player.cards[1].flower
            && player.cards[1].flower == player.cards[2].flower;
}

// c, 是否是A23
bool isA32(const playercard& player) {
    return player.cards[0].number == 14 && player.cards[1].number == 3
        && player.cards[2].number == 2;
}

// d, 是否顺子, A32也是同花顺，是最小的同花顺
bool isStraight(const playercard& player) {
    bool isNormalStraight = player.cards[0].number == player.cards[1].number + 1
            && player.cards[1].number == player.cards[2].number + 1;
    return isNormalStraight || isA32(player);
}

// e, 是否对子
bool isDouble(const playercard& player) {
    return player.cards[0].number == player.cards[1].number
            || player.cards[1].number == player.cards[2].number;
}

//牌值计算：判断玩家牌型、计算牌型绝对值大小
void calcCardsValueAndType(playercard& player) {
    if (isFlush(player)) {
        if (isStraight(player)) {
            // 同花顺
            player.cardtype = STRAIGHT_FLUSH;
            player.value = getStraightFlushValue(player);
        } else {
            // 同花
            player.cardtype = FLUSH;
            player.value = getFlushValue(player);
        }
    } else if (isStraight(player)) {
        // 顺子
        player.cardtype = STRAIGHT;
        player.value = getStraightValue(player);
    } else if (isDouble(player)) {
        if (isBomb(player)) {
            // 炸弹
            player.cardtype = BOMB;
            player.value = getBombValue(player);
        } else {
            // 对子 
            moveDouble2Front(player);  //将对子放到玩家牌的前两张的位置,以便于之后的牌值计算
            player.cardtype = DOUBLE;
            player.value = getDoubleValue(player);
        }
    } else {
        // 普通牌
        player.cardtype = NORMAL;
        player.value = getNormalValue(player);
    }
}

//玩家牌排序
void sortPlayersCards(map<string, playercard>& playerCards) {
    auto iter = playerCards.begin();
    for(; iter != playerCards.end(); iter++) {
        auto player = iter->second;
        sortPlayerByNumber(player);
        calcCardsValueAndType(player);
        playerCards[iter->first] = player;
    }
}

//玩家比牌: player1 compare player2
bool cardCompare(const playercard& player1, const playercard& player2) {
    return player1.value > player2.value;
}

vector<int> parsesha256(const string& seeds) {
    vector<int> seed;
    for(int i = 0; i < seeds.size(); i++) {
        int n = seeds[i] - '0';
        if(n >= 48) {
            n = seeds[i] -'a' + 10;
        }
        seed.push_back(n);
    }
    return seed;
}

//前n张牌:A[i]*17+A[i+1]*13+A[i+2]*11; 后2n张牌: A[i]*17+A[i+1]*13+A[i+2]
//seed sha256 64bytes 64个16进制数
map<string, playercard> getRandomCards(const vector<string>& players, const string& seeds) {
    vector<int> cards(cards52, cards52+52);
    vector<int> seed = parsesha256(seeds);
    int size = players.size();
    int cardsize = cards.size();
    map<string, playercard> playercards;
    for(size_t i = 0; i < size*3; i++) {
        int d = (i < size) ? 11 : 1;
        int index = (seed[i]*17+seed[i+1]*13+seed[i+2]*d)%cardsize;
        int random = cards[index];
        card c;
        c.flower = random/16;
        c.number = random%16;
        if(i < size) {
            playercard pc;
            pc.cards.push_back(c);
            playercards.insert(std::pair<string, playercard>(players[i], pc));
        } else {
            string player = players[i%size];
            playercards[player].cards.push_back(c);
        }
        std::remove(cards.begin(), cards.end(), random);
        cardsize--;
    }
    sortPlayersCards(playercards);
    return playercards;
}


