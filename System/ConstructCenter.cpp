//
// Created by zeal on 2022/12/2.
//

#include "ConstructCenter.h"
#include "F0nMessage.h"
#include "FnpMessage.h"
#include "FnMessage.h"

ConstructCenter::ConstructCenter(){
    printf("开始构造\n");
    initCenter();
    typeMap[0][7][10] = "秘钥滚动";
    typeMap[0][1][2] = "电子战 位置";
}


uint8_t *ConstructCenter::beginAssemble (const std::bitset<72> &b1, const std::bitset<72> &b2) {
    std::bitset<72> p[2];
    p[0] = b1;
    p[1] = b2;
    uint8_t *aesCode = beginAes(p, 2);
    uint8_t *crcCode = beginCrc(aesCode, 18);
    uint8_t *rsCode = beginRs(crcCode, aesCode,18);

    return rsCode;
}

/**
 * 解码过程
 * @param data 36个消息
 * @return 解码后的18个消息
 */
uint8_t *ConstructCenter::beginDisassemble(const uint8_t *data) {

    uint8_t *rsCode = beginDeRs(data,36);

    uint8_t crcCode[2];
    uint8_t aesCode[18];
    memcpy(crcCode, rsCode, 2 * sizeof(uint8_t));
    memcpy(aesCode, rsCode + 3 * sizeof(uint8_t), 18 * sizeof(uint8_t));

    bool crcFlag = beginDeCrc(crcCode, aesCode, 18);
    if (!crcFlag) {
        printf("CRC码校验错误！！！\n");
        return nullptr;
    }

    uint8_t *message = beginDeAes(aesCode, 18);

    return message;
}


void ConstructCenter::initCenter() {
    aes.set_key(key);
    aes.set_mode(MODE_OFB);
    aes.set_iv(iv);
}


/**
 * 进行aes
 *
 * @param data
 * @return 18个uint
 */
uint8_t *ConstructCenter::beginAes(const std::bitset<72> *data ,int arrayNum) {
    printf("-------------------------------------------begin AES-------------------------------- \n");

    uint8_t *input =  msgUtil.BitsetToCharArray(data,arrayNum);
    uint8_t temp[100];
    int inlen = sizeof(uint8_t) * 9 * arrayNum;
    printf("AES::before aes: ");
    print(input, inlen);
    int outlen = aes.Encrypt(input, inlen, temp);

    uint8_t *output = new uint8_t [outlen];
    memcpy(output, temp, outlen);
    printf("AES::after aes %d : ",outlen);
    print(output, outlen);

    printf("-------------------------------------------end AES-------------------------------------\n\n");
    return output;
}


/**
 * 生成16位crc码
 * @param msg 待编码的消息
 * @param arrayNum uint8个数
 * @return crc码
 */
uint8_t *ConstructCenter::beginCrc(const uint8_t *msg, int arrayNum) {
    printf("------------------------------------------begin CRC16----------------------------------\n");
    uint8_t *data;
    memcpy(data, msg, arrayNum * sizeof(uint8_t));
    uint8_t *crcCode = new uint8_t[2];
    crcUtil.get_crc16(data,arrayNum, crcCode);

    printf("Crc code is: ");
    print(crcCode,2 * sizeof(uint8_t));
    printf("------------------------------------------end CRC16------------------------------------\n\n");
    return crcCode;
}


/**
 * https://github.com/mersinvald/Reed-Solomon
 * RS<36,21>
 * @param crcCode
 * @param msgCode
 * @param arrayNum
 * @return 36个
 */
uint8_t *ConstructCenter::beginRs(const uint8_t *crcCode, const uint8_t *msgCode, int arrayNum) {
    printf("-------------------------------------------begin RS--------------------------------------\n");

    // 1.合并消息
    uint8_t data[21] = {0};
    memcpy(data, crcCode, 2 * sizeof(uint8_t));
    memcpy(data + 3, msgCode, 18 * sizeof(uint8_t));

    printf("RS::code before: ");
    print(data,21);
    // 2.rs编码
    uint8_t *output = new uint8_t[36];
    rs.Encode(data, output);
    printf("RS::code after: ");
    print(output,36);
    printf("-------------------------------------------end RS--------------------------------------\n\n");
    return output;
}

/**
 * rs解码
 * @param rsCode
 * @param arrayNum
 * @return 返回21个
 */
uint8_t *ConstructCenter::beginDeRs(const uint8_t *rsCode, int arrayNum) {
    printf("-------------------------------------------begin DeRS--------------------------------------\n");
    printf("Code before deres:");
    print(rsCode, arrayNum);

    uint8_t *output = new uint8_t[21];
    rs.Decode(rsCode,output);

    printf("Code after deres:");
    print(output, 21);
    printf("-------------------------------------------end DeRS--------------------------------------\n\n");
    return output;
}

bool ConstructCenter::beginDeCrc(const uint8_t *crcCode, const uint8_t *data, int arrayNum) {
    printf("-------------------------------------------begin DeCrc--------------------------------------\n");
    printf("Original crcCode:: ");
    print(crcCode, 2);
    printf("Original data:: ");
    print(data, arrayNum);

    uint8_t *code = new uint8_t[2];
    crcUtil.get_crc16(const_cast<uint8_t *>(data), arrayNum,code);

    printf("Now crcCode:: ");
    print(code, 2);

    int res = memcmp(code, crcCode, 2 * sizeof(uint8_t));
    printf("-------------------------------------------end DeCrc--------------------------------------\n\n");
    if (res != 0) return false;
    else return true;
}

/**
 * 解aes
 * @param data
 * @return
 */
uint8_t *ConstructCenter::beginDeAes(const uint8_t *data, int arrayNum) {
    printf("-------------------------------------------begin DeAes--------------------------------------\n");
    printf("Code before deaes:: ");
    print(data, arrayNum);

    uint8_t *output = new uint8_t [18];
    aes.Decrypt(const_cast<unsigned char *>(data), 18, output);

    printf("Code after deaes:: ");
    print(output, 18);
    printf("-------------------------------------------end DeAes--------------------------------------\n\n");
    return output;
}

/**
 * ******
 * 系统入口
 * ******
 * @return
 */
uint8_t *ConstructCenter::constructMessage() {
    printf(" 请选择待填充的Link22消息类型：");
    printf("1. F0n.m-p    2. Fn-p     3. Fn\n");
    int n, m, p, type;
    uint8_t *res;
    std::cin >> type;
    if (type == 1) {
        printf("将生成 F0n.m-p 消息....\n");
        printf("请依次输入 n m p :\n");
        std::cin >> n >> m >> p;
        // 更改秘钥消息
        if (n == 0 && m == 7 && p == 10) {
            const uint8_t *msg;
            scanf("%s",msg);
            changeKey(msg);
            return nullptr;
        } else if (n == 0 && m == 1 && p == 2) {
            printf("请输入文本消息：\n");
            std::string message;
            std::cin >> message;
            res = beginConstruct(message, 1, n ,p ,m);
        }
    } else if (type == 2) {
        printf("请依次输入 n p \n");
        std::cin >> n >> p;
        printf("将构造消息,请输入消息内容：");
        std::string message;
        std::cin >> message;
        res = beginConstruct(message, 2, n ,p);

    } else if (type == 3) {
        printf("请依次输入 n\n");
        std::cin >> n;
        std::string message;
        std::cin >> message;
        res = beginConstruct(message, 3, n);
    }

    return res;
}

// 填充，生成F系列消息
uint8_t *ConstructCenter::beginConstruct(const std::string &msg, int type, int n, int p, int m) {
    // 1. 将输入的string消息转换成01字符串
    printf("\n待转换的消息为：%s\n",msg.c_str());
    std::string str = msgUtil.StrToBitStr(msg);
    printf("补齐前的%lu比特二进制消息为：%s\n",str.length(),str.c_str());
    // 2. 计算切分的报文数
    int strLen = str.length();
    int MSGLEN;
    if (type == 1) MSGLEN = 55;
    else if (type == 2) MSGLEN = 61;
    else MSGLEN = 62;
    // while(str.length() % (2 * MSGLEN) != 0) str += '0';
    // printf("补齐后的%lu比特二进制消息为：%s\n\n",str.length(),str.c_str());
    // int num = str.length() / (MSGLEN * 2);
    // if (num % MSGLEN != 0) num++;
    int num = 0;
    while (num * 2 * MSGLEN < strLen) num++;
    printf("将生成%d个码元消息\n",num);
    uint8_t *temp = new uint8_t [num * 36 * sizeof(uint8_t)];
    uint8_t *res = temp;
    // 3. 分段填充
    int remainLen = str.length();
    int i = 0;
    // 3.1 处理整的部分
    while(remainLen > 2 * MSGLEN) {
        uint8_t *data;
        if (type == 1) {
            F0nMessage m1(str.substr(i,MSGLEN), n, m, p);
            F0nMessage m2(str.substr(i + MSGLEN,MSGLEN * 2), n, m, p);
            data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
        } else if (type == 2) {
            FnpMessage m1(str.substr(i,MSGLEN), n, p);
            FnpMessage m2(str.substr(i + MSGLEN,MSGLEN * 2), n, p);
            data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
        } else if (type == 3) {
            FnMessage m1(str.substr(i,MSGLEN), n);
            FnMessage m2(str.substr(i + MSGLEN,MSGLEN * 2), n);
            data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
        }
        remainLen -= 2 * MSGLEN;
        i += 2 * MSGLEN;
        memcpy(temp, data, 36);
        temp += 36 * sizeof(uint8_t);
    }
    // 3.2 处理不足的部分
    if(remainLen > 0 && remainLen <= 2 * MSGLEN) {
        uint8_t *data;
        if(remainLen > MSGLEN) {
            if (type == 1) {
                F0nMessage m1(str.substr(i,MSGLEN), n, m, p);
                F0nMessage m2(str.substr(i + MSGLEN), n, m, p);
                data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
            } else if (type == 2) {
                FnpMessage m1(str.substr(i,MSGLEN), n, p);
                FnpMessage m2(str.substr(i + MSGLEN), n, p);
                data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
            } else if (type == 3) {
                FnMessage m1(str.substr(i,MSGLEN), n);
                FnMessage m2(str.substr(i + MSGLEN), n);
                data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
            }
        } else {
            if (type == 1) {
                F0nMessage m1(str.substr(i), n, m, p);
                F0nMessage m2("", n, m, p);
                data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
            } else if (type == 2) {
                FnpMessage m1(str.substr(i), n, p);
                FnpMessage m2("", n, p);
                data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
            } else if (type == 3) {
                FnMessage m1(str.substr(i), n);
                FnMessage m2("", n);
                data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
            }
        }
        memcpy(temp, data, 36);
        temp += 36 * sizeof(uint8_t);
    }

//    for(int i = 0; i < (num - 1) * 2 * MSGLEN; i += MSGLEN * 2) {
//        uint8_t *data;
//        if (type == 1) {
//            F0nMessage m1(str.substr(i, i + MSGLEN), n, m, p);
//            F0nMessage m2(str.substr(i + MSGLEN, i + MSGLEN * 2), n, m, p);
//            data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
//        } else if (type == 2) {
//            FnpMessage m1(str.substr(i,i + MSGLEN), n, p);
//            FnpMessage m2(str.substr(i + MSGLEN, i + MSGLEN * 2), n, p);
//            data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
//        } else if (type == 3) {
//            FnMessage m1(str.substr(i,i + MSGLEN), n);
//            FnMessage m2(str.substr(i + MSGLEN, i + MSGLEN * 2), n);
//            data = beginAssemble(m1.getBitsetData(), m2.getBitsetData());
//        }
//        memcpy(temp, data, 36);
//        temp += 36 * sizeof(uint8_t);
//    }
    printf("最终结果为：");
    print(res, 36 * num);
    printf("\n\n\n");

    return res;
}

// 解密
void ConstructCenter::crackMessage(const uint8_t *data) {
    int arrayNum;

    printf("请输入码元个数：");
    std::cin >> arrayNum;
    printf("原数据：");
    print(data, 36 * arrayNum);

    std::string message;
    // 1.分为每 36 传入
    for (int i = 0; i < 36 * arrayNum; i += 36) {
        uint8_t tmp[36];
        memcpy(tmp, data + i, 36 * sizeof(uint8_t));
        // 2. 解码成 18，即两个 72bit 数据
        uint8_t *msg = beginDisassemble(tmp);
        std::string str = msgUtil.CharArrayToBitStr(msg, 18);
        // 3. 从中提取信息
        if (str.length() == 144) {
            message += msgUtil.getDataFromMessage(str.substr(0, 72));
            message += msgUtil.getDataFromMessage(str.substr(72,72));
        }
    }
    std::string res = msgUtil.BitStrToStr(message);
    printf("解密后的消息为：%s\n", res.c_str());
}


/**
 * 更新aes秘钥
 * @param newKey 新秘钥
 */
void ConstructCenter::changeKey(const uint8_t *newKey) {
    memcpy(this->key, newKey, 16 * sizeof(uint8_t));
    printf("新AES秘钥是：");
    print(newKey, 16);
}


void ConstructCenter::print(const uint8_t *state, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        printf("%x  ", state[i]);
    }
    printf("\n");
}


ConstructCenter::~ConstructCenter() {
    printf("构造结束\n");
}



