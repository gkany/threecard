# 常用操作
### 查看账户充值表
* cleos get table card card balance

### 查看游戏表
* cleos get table card card games

### 查看action
* cleos get actions card -1 -j --console

### 查看transaction
* cleos get transaction 17f03952abcbd312ae54de8e5bb8680a589dac123ab44aee0e5e1f0128d2a372 

### 充值
* cleos push action token transfer '["alice", "card", "30.0000 EOS" "deposit-"]' -p alice@active

### 提币
* cleos push action token transfer '["alice", "card", "0.0001 EOS" "withdraw-"]' -p alice@active

### 开启一局游戏 
* cleos push action token transfer '[ "dev", "card", "0.0001 EOS" "start-2-5000-3-dev-bob-alice-" ]' -p dev@active

* 2 -- house id
  
* dev -- 游戏账号
 
* 玩家 dev bob alice, 实际dev是不参与游戏的，不想再创建账号, 这里直接使用了dev当做一个玩家

### 加注
* cleos push action token transfer '[ "alice", "card", "0.5000 EOS" "raise-2-" ]' -p alice@active

### 弃牌
* cleos push action token transfer '[ "alice", "card", "0.0001 EOS" "bust-2-" ]' -p alice@active

### 开牌
* cleos push action token transfer '[ "bob", "card", "0.0005 EOS" "open-2-dev-" ]' -p bob@active

### 删除games table 指定id的 record
* cleos push action card clean '{"id": 2}' -p card@active


# 计算牌型
使用开奖记录的种子“seed”进行 SHA512 运算，得到 A = sha512(seed)（64位长度的uint16数组），使用该数组即可完成发牌。

牌组的排序及相关值如下所示（52位长度的一维数组）：

* 0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E //方块 2 - K - A
* 0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E //梅花 2 - K - A
* 0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E //红桃 2 - K - A
* 0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E //黑桃 2 - K - A

发牌逻辑，最多总共需要发十五张牌，取牌按照如下公式计算得出，算出一张牌后，将该牌从牌组中移出以便计算下一张牌的位置：
假设有n个玩家：
* 前n张牌: A[i]*17+A[i+1]*13+A[i+2]*11; 
* 后2n张牌: A[i]*17+A[i+1]*13+A[i+2]

如果n=5, 五个玩家分别为: A B C D E, 15张牌计算如下：

(A[0]*17+A[1]*13+A[2]*11)%剩余牌的张数(52)      作为玩家A第一张牌位置，从牌组中取出。

(A[4]*17+A[5]*13+A[6]*11)%剩余牌的张数(51)      作为玩家B第一张牌位置，从牌组中取出。

(A[8]*17+A[9]*13+A[10]*11)%剩余牌的张数(50)     作为玩家C第一张牌位置，从牌组中取出。

(A[12]*17+A[13]*13+A[14]*11)%剩余牌的张数(49)   作为玩家D第一张牌位置，从牌组中取出。

(A[16]*17+A[17]*13+A[18]*11)%剩余牌的张数(48)   作为玩家E第一张牌位置，从牌组中取出。 

(A[20]*17+A[21]*13+A[22])%剩余牌的张数(47)      作为玩家A第二张牌位置，从牌组中取出。

(A[24]*17+A[25]*13+A[26])%剩余牌的张数(46)      作为玩家B第二张牌位置，从牌组中取出。

(A[28]*17+A[29]*13+A[30])%剩余牌的张数(45)      作为玩家C第二张牌位置，从牌组中取出。

(A[32]*17+A[33]*13+A[34])%剩余牌的张数(44)      作为玩家D第二张牌位置，从牌组中取出。

(A[36]*17+A[37]*13+A[38])%剩余牌的张数(43)      作为玩家E第二张牌位置，从牌组中取出。 

(A[40]*17+A[41]*13+A[42])%剩余牌的张数(42)      作为玩家A第三张牌位置，从牌组中取出。

(A[44]*17+A[45]*13+A[46])%剩余牌的张数(41)      作为玩家B第三张牌位置，从牌组中取出。

(A[48]*17+A[49]*13+A[50])%剩余牌的张数(40)      作为玩家C第三张牌位置，从牌组中取出。

(A[52]*17+A[53]*13+A[54])%剩余牌的张数(39)      作为玩家D第三张牌位置，从牌组中取出。

(A[56]17+A[57]*13+A[58])%剩余牌的张数(38)      作为玩家E第三张牌位置，从牌组中取出。




