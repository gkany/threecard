#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

#include <string>
#include <map>
#include <algorithm>
#include <utility>

using namespace eosio;
using namespace std;


#define N(X)            eosio::name{#X}
#define S(P,X)          eosio::symbol(#X,P)
#define EOS_SYMBOL      S(4, EOS)

#define TRANSFER_CODE   token       //eosio.token
#define FEE             1           //0.0001 eos
#define GAME_ACCOUNT    name("dev")

// enum flowertype {
//     diamond = 0,            // 方片
//     club,                   // 梅花
//     heart,                  // 红桃
//     spade                   // 黑桃
// };

enum cardnum { 
    card2 = 2, card3, card4, card5, card6, card7, card8, card9, card10, cardJ, cardQ, cardK, cardA,
};

enum playertype {
    NORMAL = 0,        //普通牌
    DOUBLE,            //对子
    STRAIGHT,          //顺子
    FLUSH,             //同花
    STRAIGHT_FLUSH,    //同花顺
    BOMB               //炸弹
};

enum playertypemax {
    NORMAL_MAX_VALUE            = 3803,     //普通: 最大值AKJ=14*16*16+13*16+11=3803
    DOUBLE_MAX_VALUE            = 4040,     //对子: 最大值AAK=14*16+13=237,加上普通牌的基数3803=4040
    STRAIGHT_MAX_VALUE          = 4052,     //顺子: 最大值AKQ=12，加上对子的最大值基数4040=4052
    FLUSH_MAX_VALUE             = 7855,     //同花: 最大值AKJ，14*16*16+13*16+11=3803，加上顺子4052=7855
    STRAIGHT_FLUSH_MAX_VALUE    = 7867,     //同花顺: 最大值AKQ=12，加上同花7855=7867
    BOMB_MAX_VALUE              = 7881      //炸弹: 最大值AAA=14，加上同花顺7867=7881
};

struct card {
    uint8_t                     flower;     //花色: flowertype
    uint8_t                     number;     //牌点数: cardnum
};

struct playercard {
    card                        card0;     //牌1
    card                        card1;     //牌2
    card                        card2;     //牌3
    uint8_t                     cardtype;  //牌类型：playtype
    bool                        a32;       //是否是特殊牌
    int                         value;     //牌绝对值大小
};


//table: global, game, deposit
TABLE global {
    uint64_t                    current_id;
};

TABLE game {
    uint64_t                    id;             //house id
    map<string, bool>           players;        //player and player status
    map<string, playercard>     cards;          //cards
    asset                       amount;         //fund pool
    uint8_t                     size;           //surplus number
    string                      winner;         //winner
    time_point_sec              created_at;     //house create time

    uint64_t primary_key() const { return id; }
};

TABLE deposit {
    name                        player;
    asset                       balance;

    uint64_t primary_key() const { return player.value; }
};

typedef singleton<N(global), global>         tb_global;
typedef multi_index<N(games), game>          tb_games;
typedef multi_index<N(deposits), deposit>    tb_deposits;

//table end

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
//a, 获取炸弹牌值绝对大小
int getBombValue(const playercard& player) {
    return player.card0.number + STRAIGHT_FLUSH_MAX_VALUE;
}

//b, 获取同花顺牌值绝对大小,A32也是同花顺，是最小的同花顺(参考自百度百科)
int getStraightFlushValue(const playercard& player) {
    if (player.a32) {
        //此时A就等于是1
        return 1 + FLUSH_MAX_VALUE;
    }
    return player.card2.number + FLUSH_MAX_VALUE;
}

//c, 获取同花牌值绝对大小
int getFlushValue(const playercard& player) {
    return player.card0.number * 256 + player.card1.number * 16 + player.card2.number
            + STRAIGHT_MAX_VALUE;
}

//d, 获取顺子牌值绝对大小
int getStraightValue(const playercard& player) {
    if (player.a32) {
        //此时A就等于是1
        return 1 + DOUBLE_MAX_VALUE;
    }
    return player.card2.number + DOUBLE_MAX_VALUE;
}

//e, 获取对子牌值绝对大小
// 在判断牌型时，如果是对子，则将对子放在数组前面两位
int getDoubleValue(const playercard&  player) {
    return player.card1.number * 16 + player.card2.number + NORMAL_MAX_VALUE;
}

//f, 获取普通牌值绝对大小
int getNormalValue(const playercard&  player) {
    return player.card0.number * 256 + player.card1.number * 16 + player.card2.number;
}

//玩家牌排序
//a, 将一副牌按牌面从大到小排序
void sortPlayerByNumber(playercard&  player) {
    vector<card> cards;
    cards.push_back(player.card0);
    cards.push_back(player.card1);
    cards.push_back(player.card2);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3 - i - 1; j++) {
            if (cards[j].number < cards[j + 1].number) {
                card tempCard =cards[j];
                cards[j] = cards[j + 1];
                cards[j + 1] = tempCard;
            }
        }
    }

    player.card0 = cards[0];
    player.card1 = cards[1];
    player.card2 = cards[2];
}

//b, 将一副牌按花色从大到小排序
void sortPlayerByFlower(playercard&  player) {
    vector<card> cards;
    cards.push_back(player.card0);
    cards.push_back(player.card1);
    cards.push_back(player.card2);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3 - i - 1; j++) {
            if (cards[j].flower < cards[j + 1].flower) {
                card tempCard = cards[j];
                cards[j] = cards[j + 1];
                cards[j + 1] = tempCard;
            }
        }
    }

    player.card0 = cards[0];
    player.card1 = cards[1];
    player.card2 = cards[2];
}

//c, 将对子移动到一幅牌的前两位
void moveDouble2Front(playercard&  player) {
    if (player.card1.number == player.card2.number) {
        card tempCard = player.card0;
        player.card0 = player.card2;
        player.card2 = tempCard;
    }
}

//d, 将对子已经处于一副牌的前两张的牌，移动对子中花色大的到最前面
void exchangeSortedDoubleFlower(playercard&  player) {
    if (player.card0.flower < player.card1.flower) {
        card tempCard = player.card0;
        player.card0 = player.card1;
        player.card1 = tempCard;
    }
}

//牌类型判断
// 是否同花
bool isFlush(const playercard& player) {
    return player.card0.flower == player.card1.flower
            && player.card1.flower == player.card2.flower;
}

bool isA32(const playercard& player) {
    return player.card0.number == 14 && player.card1.number == 3
        && player.card2.number == 2;
}

// 是否顺子, A32也是同花顺，是最小的同花顺
bool isStraight(const playercard& player) {
    bool isNormalStraight = player.card0.number == player.card1.number + 1
            && player.card1.number == player.card2.number + 1;
    return isNormalStraight || isA32(player);
}

// 是否炸弹
bool isBomb(const playercard& player) {
    return player.card0.number == player.card1.number
            && player.card1.number == player.card2.number;
}

// 是否对子
bool isDouble(const playercard& player) {
    return player.card0.number == player.card1.number
            || player.card1.number == player.card2.number;
}

// 是否特殊牌
bool isSpecial(const playercard& player) {
    return player.card0.number == 5 && player.card1.number == 3 && player.card2.number == 2;
}

//牌值计算
// 判断牌型、计算牌型绝对值大小
void regPlayerType(playercard& player) {
    int type;
    int value;
    if (isFlush(player)) {
        if (isStraight(player)) {// 同花顺
            player.cardtype = STRAIGHT_FLUSH;
            player.value = getStraightFlushValue(player);
        } else {// 同花
            player.cardtype = FLUSH;
            player.value = getFlushValue(player);
        }
    } else if (isStraight(player)) {// 顺子
        player.cardtype = STRAIGHT;
        player.value = getStraightValue(player);
    } else if (isDouble(player)) {
        if (isBomb(player)) {   // 炸弹
            player.cardtype = BOMB;
            player.value = getBombValue(player);
        } else {// 对子
            // 将对子放到玩家牌的前两张的位置,以便于之后的牌值计算
            moveDouble2Front(player);
            player.cardtype = DOUBLE;
            player.value = getDoubleValue(player);
        }
    } else {// 普通牌
        player.cardtype = NORMAL;
        player.value = getNormalValue(player);
        if (isSpecial(player)) {// 对于特殊牌，本算法不提供特殊大小计算，外部调用者自行判断是否有炸弹玩家产生
            player.a32 = true;
        }
    }
}

