//
// Created by andy_yang on 2021/7/11.
//

#ifndef RISC_V_TOMASULO_HEADER_HPP
#define RISC_V_TOMASULO_HEADER_HPP

#include <iostream>

//#define efficiency_show
//#define debug_run
#define debug

using std::cin ;
using std::cout ;
using std::endl ;
using std::string ;

const int MEMORY_SIZE = 500000 ;

enum codeType{ R , I , S , B , U , J };

struct Memory{
    unsigned char all_data[MEMORY_SIZE] = {0} ;
    void write_word( int pos , unsigned int word ){ // 4 - byte
        *( unsigned int * )( all_data + pos ) = word ;
    }
    void write_ushort( int pos , unsigned short word ){ // 2 - byte
        *( unsigned short *)( all_data + pos ) = word ;
    }
    void write_short( int pos , unsigned char word ){ // 1 - byte
        all_data[pos] = word ;
    }
    unsigned int read_word( int pos ){
        return *( unsigned int * ) ( all_data + pos ) ;
    }
    unsigned short read_ushort( int pos ){
        return *( unsigned short *)( all_data + pos ) ;
    }
    unsigned char read_short( int pos ){
        return all_data[pos] ;
    }
};

enum Instruction_status{ Empty , Valid , Invalid };

struct Instruction{ // todo 增加 rs1_rely rs2_rely
    int npc = 0 , read_pc = 0 , ROB_ID = -1 , IR = 0 , opcode = 0 , func3 = 0 , func7 = 0 , rd = 0 , rs1 = 0 , rs2 = 0 , reg_rs1 = 0 , reg_rs2 = 0 , immediate = 0 , shamt = 0 , ALUOutput = 0 , LMD = 0 ;
    int rs1_rely = 0 , rs2_rely = 0 ;
    Instruction_status status = Empty ;
    codeType code_type = R ;

    Instruction &operator= ( const Instruction &other ){
        if ( this == &other ){
            return *this ;
        }
        status = other.status ; // 目前是直接拷贝状态
        read_pc = other.read_pc ;
        npc = other.npc ;
        ROB_ID = other.ROB_ID ;
        IR = other.IR ;
        opcode = other.opcode ;
        func3 = other.func3 ;
        func7 = other.func7 ;
        rd  = other.rd ;
        rs1 = other.rs1 ;
        rs2 = other.rs2 ;
        reg_rs1 = other.reg_rs1 ;
        reg_rs2 = other.reg_rs2 ;
        immediate = other.immediate ;
        shamt = other.shamt ;
        ALUOutput = other.ALUOutput ;
        code_type = other.code_type ;
        LMD = other.LMD ;
        return *this ;
    }

    void clear(){
        Instruction temp_I ;
        *this = temp_I ;
    }

    bool MemOP() const{
        return opcode == 0b0000011 || opcode == 0b0100011 ;
    }
};

struct Register{
    int q = -1 ; // ROB 指令 ID
    int v = 0 ;
};

template<class T, int size>
struct queue{
    T all_data[size] ;
    int front = 0 , rear = -1 , real_length = 0 ;
    int push_back( T &ele ) { all_data[(++rear)%size] = ele ; real_length++ ; return rear % size ; } // 返回 ROB_ID
    int pop(){ real_length-- ; return (front++) % size ; } // 返回 ROB_ID
    T top(){ return all_data[front%size] ; }
    T& operator[](int index){ return all_data[index%size] ; };
    bool empty(){ return real_length == 0 ; }
    bool full() { return real_length == size ; }
    int length() { return real_length ; }
    void clear(){
        front = 0 ;
        rear = -1 ;
        real_length = 0 ;
        T temp ;
        for ( int i = 0 ; i < size ; i++ ){
            all_data[i] = temp ;
        }
    }
};

class description
{
    // todo 保证 ROB SLB 队列连续

private:

    Register reg[32] ;
    Memory my_memory ;
    int pc = 0 ;

    Instruction RS[32] ;
    queue<Instruction,32> ROB ;
    queue<Instruction,32> SLB ;

public:

    int t_clock = 0 ;

    static int get_bits( int num , int low , int high ){
        int small_int = ( 1 << ( high - low + 1 ) ) - 1 ;
        return ( num >> low ) & small_int ;
    }

    void LoadMemory(){
        int memory_pointer = 0 ;
        string temp_input ;
        while( cin >> temp_input ){
            if ( temp_input[0] == '@' ){
                memory_pointer = strtol(temp_input.c_str()+1,NULL,16) ;
                continue;
            }
            my_memory.all_data[memory_pointer] = strtol(temp_input.c_str(),NULL,16) ;
            memory_pointer++ ;
        }
    }

    static int char_extend_sign( unsigned char word ){
        int sign_temp = get_bits(word,7,7) ;
        if ( sign_temp == 1 ){
            sign_temp = ( sign_temp << 24 ) - 1 ;
            sign_temp = sign_temp << 8 ;
        }
        sign_temp += word ;
        return sign_temp ;
    }

    static int short_extend_sign( unsigned char word ){
        int sign_temp = get_bits(word,15,15) ;
        if ( sign_temp == 1 ){
            sign_temp = ( sign_temp << 16 ) - 1 ;
            sign_temp = sign_temp << 16 ;
        }
        sign_temp += word ;
        return sign_temp ;
    }

    static int I_get_immediate( int real_code ){
        int temp = get_bits(real_code,31,31) ;
        if ( temp == 1 ) {
            temp = ( temp << 21 ) - 1 ;
            temp = temp << 11 ;
        }
        temp += get_bits(real_code,20,30) ;
        return temp ;
    }

    static int S_get_immediate( int real_code ){
        int temp = get_bits(real_code,31,31) ;
        if ( temp == 1 ) {
            temp = ( temp << 21 ) - 1 ;
            temp = temp << 6 ;
        }
        temp += get_bits(real_code,25,30) ;
        temp = temp << 5 ;
        temp += get_bits(real_code,7,11) ;
        return temp ;
    }

    static int B_get_immediate( int real_code ){
        int temp = get_bits(real_code,31,31) ;
        if ( temp == 1 ) {
            temp = ( temp << 20 ) - 1 ;
            temp = temp << 1 ;
        }
        temp += get_bits(real_code,7,7) ;
        temp = temp << 6 ;
        temp += get_bits(real_code,25,30) ;
        temp = temp << 4 ;
        temp += get_bits(real_code,8,11) ;
        temp = temp << 1 ;
        return temp ;
    }

    static int U_get_immediate( int real_code ){
        int temp = get_bits(real_code,12,31) ;
        temp = temp << 12 ;
        return temp ;
    }

    static int J_get_immediate( int real_code ){
        int temp = get_bits(real_code,31,31) ;
        if ( temp == 1 ) {
            temp = ( temp << 12 ) - 1 ;
            temp = temp << 8 ;
        }
        temp += get_bits(real_code,12,19) ;
        temp = temp << 1 ;
        temp += get_bits(real_code,20,20) ;
        temp = temp << 10 ;
        temp += get_bits(real_code,21,30) ;
        temp = temp << 1 ;
        return temp ;
    }

    // todo 满 ROB 停止读入

    void RS_clear(){
        Instruction temp ;
        for ( int i = 0 ; i < 32 ; i++ ){
            RS[i] = temp ;
        }
    }

    void REG_rely_clear(){
        for ( int i = 0 ; i < 32 ; i++ ){
            reg[i].q = -1 ;
        }
    }

    Instruction fetch(){ // todo 普通跳 pc 在 issue 实现
        Instruction temp_I ;
        temp_I.status = Invalid ;
        temp_I.read_pc = pc ;
        temp_I.npc = pc + 4 ;
        temp_I.IR = 0 ;
        for ( int i = 0 ; i < 4 ; i++ ){
            temp_I.IR = temp_I.IR << 8 ;
            temp_I.IR += my_memory.all_data[pc+3-i] ;
        }
        return temp_I ;
    }

    void issue(){
        if ( ROB.full() ) return ;
        Instruction temp_I = fetch() ;
        pc += 4 ;
        temp_I.opcode = get_bits(temp_I.IR,0,6) ;
        switch(temp_I.opcode){
            case 0b0110111 :
            case 0b0010111 : temp_I.code_type = U ; temp_I.rd = get_bits( temp_I.IR , 7 , 11 ) ; temp_I.immediate = U_get_immediate(temp_I.IR) ; break ;
            case 0b1101111 : temp_I.code_type = J ; temp_I.rd = get_bits( temp_I.IR , 7, 11 ) ; temp_I.immediate = J_get_immediate(temp_I.IR) ; break ;
            case 0b1100111 :
            case 0b0000011 :
            case 0b0010011 : temp_I.code_type = I ; temp_I.rd = get_bits( temp_I.IR , 7 , 11 ) ; temp_I.rs1 = get_bits(temp_I.IR,15,19) ; temp_I.func3 = get_bits(temp_I.IR,12,14) ; temp_I.immediate = I_get_immediate(temp_I.IR) ; temp_I.shamt = get_bits(temp_I.IR,20,24) ; temp_I.func7 = get_bits(temp_I.IR,25,31) ; break ;
            case 0b1100011 : temp_I.code_type = B ; temp_I.rs1 = get_bits( temp_I.IR , 15 , 19 ) ; temp_I.rs2 = get_bits( temp_I.IR , 20 , 24 ) ; temp_I.func3 = get_bits(temp_I.IR,12,14) ; temp_I.immediate = B_get_immediate(temp_I.IR) ; break ;
            case 0b0100011 : temp_I.code_type = S ; temp_I.rs1 = get_bits( temp_I.IR , 15 , 19 ) ; temp_I.rs2 = get_bits( temp_I.IR , 20 , 24 ) ; temp_I.func3 = get_bits(temp_I.IR,12,14) ; temp_I.immediate = S_get_immediate(temp_I.IR) ; break ;
            case 0b0110011 : temp_I.code_type = R ; temp_I.rd = get_bits(temp_I.IR,7,11) ; temp_I.rs1 = get_bits( temp_I.IR , 15 , 19 ) ; temp_I.rs2 = get_bits( temp_I.IR , 20 , 24 ) ; temp_I.func3 = get_bits(temp_I.IR,12,14) ; temp_I.func7 = get_bits(temp_I.IR,25,31) ; break ;
        }

        temp_I.reg_rs1 = reg[temp_I.rs1].v ;
        temp_I.reg_rs2 = reg[temp_I.rs2].v ; // 随意拷贝

        int real_ROB_ID = ROB.push_back(temp_I) ; // 不可能同一个周期内允许 issue commit
        temp_I.ROB_ID = real_ROB_ID ;
        if ( reg[temp_I.rs1].q == -1 && reg[temp_I.rs2].q == -1 ){ // 无依赖
            temp_I.status = Valid ;
        }else{
            temp_I.rs1_rely = (reg[temp_I.rs1].q != -1) ;
            temp_I.rs2_rely = (reg[temp_I.rs2].q != -1) ; // todo 通过看 rely 情况判断是否可执行
        }
        if ( temp_I.rd != 0 ){ // 在这之后的命令涉及 rd 的都需要等待
            reg[temp_I.rd].q = real_ROB_ID ;
        }

        if ( temp_I.MemOP() ){ // 互相之间信息补全
            int SLB_ID = SLB.push_back(temp_I) ;
            ROB[real_ROB_ID].ROB_ID = SLB_ID ;
        }else{
            for ( int i = 0 ; i < 32 ; i++ ){
                if ( RS[i].status == Empty ){
                    RS[i] = temp_I ;
                    ROB[real_ROB_ID].ROB_ID = i ;
                    break ;
                }
            }
        }

        // 发射完成
    }

    void PRINT_ANS(){
        cout << (unsigned int) ( reg[10].v & 255u ) << endl ;
#ifdef efficiency_show
        cout << "cycle: " << t_clock << endl ;
        cout << "right prediction rate: " << ((double )right_prediction_sum) / prediction_sum * 100 << "%" << endl ;
//        cout << ((double )right_prediction_sum) / prediction_sum * 100 << "%" << endl << right_prediction_sum << endl << prediction_sum ;
#endif
        exit(0) ;
    }

    void execute(){
        // todo 找到 RS 可执行命令 修改两个队列的依赖关系 ROB 依赖关系
        int target_instruction = 0 ;
        for(; target_instruction < 32 ; target_instruction++){
            if ( RS[target_instruction].status == Valid ) break ;
        }
        if ( target_instruction == 32 ){ // 空 RS
            return ;
        }
        int temp = 0 ;
        Instruction temp_I = RS[target_instruction] ;
        RS[target_instruction].clear() ; // 就地把自己处理掉
        switch (temp_I.opcode) {
            case 0b0110111 : temp_I.ALUOutput = temp_I.immediate ; break ; // lui
            case 0b0010111 : temp_I.ALUOutput = temp_I.npc - 4 + temp_I.immediate ; break ; // auipc
            case 0b1101111 : temp_I.ALUOutput = temp_I.npc  - 4 + 4 ; temp_I.npc += temp_I.immediate - 4 ; break ; // jal
            case 0b1100111 : temp = temp_I.npc - 4 + 4 ; temp_I.npc = ( temp_I.reg_rs1 + temp_I.immediate ) & ~1 ; temp_I.ALUOutput = temp ; break ; // jalr
            case 0b1100011 :
                switch (temp_I.func3) { // 跳转相关
                    case 0b000: if ( temp_I.reg_rs1 == temp_I.reg_rs2 ) temp_I.npc += temp_I.immediate - 4 ; break ; // beq
                    case 0b001: if ( temp_I.reg_rs1 != temp_I.reg_rs2 ) temp_I.npc += temp_I.immediate - 4 ; break ; // bne
                    case 0b100: if ( temp_I.reg_rs1 < temp_I.reg_rs2 ) temp_I.npc += temp_I.immediate - 4 ; break ; // blt
                    case 0b101: if ( temp_I.reg_rs1 >= temp_I.reg_rs2 ) temp_I.npc += temp_I.immediate - 4 ; break ; // bge
                    case 0b110: if ( temp_I.reg_rs1 < unsigned ( temp_I.reg_rs2 ) ) temp_I.npc += temp_I.immediate - 4 ; break ; // bltu
                    case 0b111: if ( temp_I.reg_rs1 >= unsigned ( temp_I.reg_rs2 )) temp_I.npc += temp_I.immediate - 4 ; break ; // bgeu
                } break ;
            case 0b0000011 : // LOAD 相关
            case 0b0100011 : temp_I.ALUOutput = temp_I.reg_rs1 + temp_I.immediate ; break ;

            case 0b0010011 :
                switch (temp_I.func3) { // LOAD register 相关
                    case 0b000: temp_I.ALUOutput = temp_I.reg_rs1 + temp_I.immediate ; break ; // addi
                    case 0b010: temp_I.ALUOutput = temp_I.reg_rs1 < temp_I.immediate ; break ; // slti
                    case 0b011: temp_I.ALUOutput = ( temp_I.reg_rs1 < (unsigned ) temp_I.immediate ) ; break ; // sltiu
                    case 0b100: temp_I.ALUOutput = temp_I.reg_rs1 xor temp_I.immediate ; break ; // xori
                    case 0b110: temp_I.ALUOutput = temp_I.reg_rs1 | temp_I.immediate ; break ; // ori
                    case 0b111: temp_I.ALUOutput = temp_I.reg_rs1 & temp_I.immediate ; break ; // andi
                    case 0b001: temp_I.ALUOutput = temp_I.reg_rs1 << temp_I.shamt ; break ; // slli
                    case 0b101:
                        if ( temp_I.func7 == 0 ){ // srli
                            temp_I.ALUOutput = ( (unsigned int) temp_I.reg_rs1 >> temp_I.shamt ) ;
                            break ;
                        }else{ // srai
                            temp_I.ALUOutput = ( temp_I.reg_rs1 >> temp_I.shamt ) ;
                            break ;
                        }
                }break ;
            case 0b0110011 :
                switch (temp_I.func3) { // LOAD register 相关
                    case 0b000:
                        if ( temp_I.func7 == 0 ){ // add
                            temp_I.ALUOutput = temp_I.reg_rs1 + temp_I.reg_rs2 ;
                            break ;
                        }else{ // sub
                            temp_I.ALUOutput = temp_I.reg_rs1 - temp_I.reg_rs2 ;
                            break ;
                        }
                    case 0b001: temp_I.ALUOutput = temp_I.reg_rs1 << temp_I.reg_rs2 ; break ; // sll
                    case 0b010: temp_I.ALUOutput = temp_I.reg_rs1 < temp_I.reg_rs2 ; break ; // slt
                    case 0b011: temp_I.ALUOutput = temp_I.reg_rs1 < (unsigned) temp_I.reg_rs2 ; break ; // sltu
                    case 0b100: temp_I.ALUOutput = temp_I.reg_rs1 xor temp_I.reg_rs2 ; break ; // xor
                    case 0b101:
                        if ( temp_I.func7 == 0 ){ // srl
                            temp_I.ALUOutput = temp_I.reg_rs1 >> ((unsigned) temp_I.reg_rs2) ;
                            break ;
                        }else{ // sra
                            temp_I.ALUOutput = temp_I.reg_rs1 >> temp_I.reg_rs2 ;
                            break ;
                        }
                    case 0b110: temp_I.ALUOutput = temp_I.reg_rs1 | temp_I.reg_rs2 ; break ; // or
                    case 0b111: temp_I.ALUOutput = temp_I.reg_rs1 & temp_I.reg_rs2 ; break ; // and
                } break ;
        }
        ROB[temp_I.ROB_ID] = temp_I ; // 拷贝过程中 Valid 了
    }

    void MEM_process(){
        Instruction temp_I = SLB.top() ;
        if ( temp_I.status != Valid ) return ;
        SLB[SLB.front].clear() ;
        SLB.front++ ; // 就地处理已执行内容
        // todo take a short cut
        temp_I.ALUOutput = temp_I.reg_rs1 + temp_I.immediate ;
        switch (temp_I.opcode) {
            case 0b0110111 :  // lui
            case 0b0010111 :  // auipc
            case 0b1101111 :  // jal
            case 0b1100111 :  // jalr
            case 0b1100011 : break ; // 跳转相关，在 mem 阶段或 ex->synchronize 阶段跳转
            case 0b0000011 :
                switch (temp_I.func3) {
                    case 0b000: temp_I.LMD = char_extend_sign(my_memory.read_short(temp_I.ALUOutput)) ; break ; // lb
                    case 0b001: temp_I.LMD = short_extend_sign(my_memory.read_ushort(temp_I.ALUOutput)) ; break ; // lh
                    case 0b010: temp_I.LMD = my_memory.read_word(temp_I.ALUOutput) ; break ; // lw
                    case 0b100: temp_I.LMD = my_memory.read_short(temp_I.ALUOutput) ; break ; // lbu
                    case 0b101: temp_I.LMD = my_memory.read_ushort(temp_I.ALUOutput) ; break ; // lhu
                } break ;
            case 0b0100011 :
                switch (temp_I.func3) {
                    case 0b000: my_memory.write_short(temp_I.ALUOutput,temp_I.reg_rs2) ; break ; // sb
                    case 0b001: my_memory.write_ushort(temp_I.ALUOutput,temp_I.reg_rs2) ; break ; // sh
                    case 0b010: my_memory.write_word(temp_I.ALUOutput,temp_I.reg_rs2) ;  break ; // sw
                } break ;
            case 0b0010011 :
            case 0b0110011 : break ;
        }
        ROB[temp_I.ROB_ID] = temp_I ; // 拷贝过程 Valid
    }

    void commit(){
        reg[0].v = 0 ;
        reg[0].q = -1 ;
        if ( ROB.top().status != Valid ) return ; // 尚未处理完成
        Instruction temp_I = ROB.top() ;
        if ( temp_I.IR == 0x0ff00513 ){
            PRINT_ANS() ;
        }
        switch (temp_I.opcode) {
            case 0b0110111 :  // lui
            case 0b0010111 :  // auipc
            case 0b1101111 :  // jal
            case 0b1100111 : reg[temp_I.rd].v = temp_I.ALUOutput ; break ; // jalr
            case 0b1100011 : break ;
            case 0b0000011 : reg[temp_I.rd].v = temp_I.LMD ; break ;
            case 0b0100011 : break ;
            case 0b0010011 :
            case 0b0110011 : reg[temp_I.rd].v = temp_I.ALUOutput ; break ;
        }

        int real_Rob_ID = ROB.pop() ;
        ROB[real_Rob_ID].clear() ;

        if ( temp_I.rd != 0 ){ // 存在 rd 修改
            if ( reg[temp_I.rd].q == real_Rob_ID ){
                reg[temp_I.rd].q = -1 ;
                for ( int i = 0 ; i < 32 ; i++ ){ // RS 依赖清理
                    if ( RS[i].rs1 == temp_I.rd && temp_I.rd != 0 ){
                        RS[i].reg_rs1 = reg[temp_I.rd].v ;
                        RS[i].rs1_rely = 0 ;
                    }
                    if ( RS[i].rs2 == temp_I.rd && temp_I.rd != 0 ){
                        RS[i].reg_rs2 = reg[temp_I.rd].v ;
                        RS[i].rs2_rely = 0 ;
                    }
                    if ( RS[i].rs1_rely == 0 && RS[i].rs2_rely == 0 && RS[i].status == Invalid && temp_I.rd != 0 ){
                        RS[i].status = Valid ;
                    }
                }
                for ( int i = SLB.front ; i <= SLB.rear ; i++ ){ // SLB 依赖清理
                    if ( SLB[i].rs1 == temp_I.rd && temp_I.rd != 0 ){
                        SLB[i].reg_rs1 = reg[temp_I.rd].v ;
                        SLB[i].rs1_rely = 0 ;
                    }
                    if ( SLB[i].rs2 == temp_I.rd && temp_I.rd != 0 ){
                        SLB[i].reg_rs2 = reg[temp_I.rd].v ;
                        SLB[i].rs2_rely = 0 ;
                    }
                    if ( SLB[i].rs1_rely == 0 && SLB[i].rs2_rely == 0 && temp_I.rd != 0 ){
                        SLB[i].status = Valid ;
                    }
                }
            }else{ // 多重绑定 执行顺序 ROB 往后读
                for ( int i = ROB.front ; i <= ROB.rear ; i++ ){
                    if ( ROB[i].rd == temp_I.rd ){
                        reg[temp_I.rd].q = i % 32 ; // 修改寄存器依赖关系
                        // todo 自我依赖未实现
                        if ( ROB[i].rs1 == temp_I.rd || ROB[i].rs2 == temp_I.rd ){
                            if ( ROB[i].MemOP() ){ // MEM 型指令
                                if ( ROB[i].rs1 == temp_I.rd ){
                                    SLB[ROB[i].ROB_ID].reg_rs1 = reg[temp_I.rd].v ;
                                    SLB[ROB[i].ROB_ID].rs1_rely = 0 ;
                                }
                                if ( ROB[i].rs2 == temp_I.rd ){
                                    SLB[ROB[i].ROB_ID].reg_rs2 = reg[temp_I.rd].v ;
                                    SLB[ROB[i].ROB_ID].rs2_rely = 0 ;
                                }
                                if ( SLB[ROB[i].ROB_ID].rs1_rely == 0 && SLB[ROB[i].ROB_ID].rs2_rely == 0 ){
                                    SLB[ROB[i].ROB_ID].status = Valid ;
                                }
                            } else{ // EX 型指令
                                if ( ROB[i].rs1 == temp_I.rd ){
                                    RS[ROB[i].ROB_ID].reg_rs1 = reg[temp_I.rd].v ;
                                    RS[ROB[i].ROB_ID].rs1_rely = 0 ;
                                }
                                if ( ROB[i].rs2 == temp_I.rd ){
                                    RS[ROB[i].ROB_ID].reg_rs2 = reg[temp_I.rd].v ;
                                    RS[ROB[i].ROB_ID].rs2_rely = 0 ;
                                }
                                if ( RS[ROB[i].ROB_ID].rs1_rely == 0 && RS[ROB[i].ROB_ID].rs2_rely == 0 ){
                                    RS[ROB[i].ROB_ID].status = Valid ;
                                }
                            }
                        }
                    }
                }
            }
        }
        if ( temp_I.npc != temp_I.read_pc + 4 ){
            // todo 跳转
            pc = temp_I.npc ;
            SLB.clear() ;
            ROB.clear() ;
            RS_clear() ;
            REG_rely_clear() ;
        }
        reg[0].v = 0 ;
        reg[0].q = -1 ;
#ifdef debug
        if ( temp_I.npc == 4504 ){
            std::cerr << "debug" << endl ;
        }
#endif
#ifdef debug_run
        debug_show(temp_I) ;
#endif
    }

    // todo commit 跳 pc 该寄存器依赖状态

    void debug_show( Instruction &temp_I ){
        cout << "<------------------------------->" << endl ;
        cout << "pc: " << temp_I.npc << endl ;
        for ( int i = 0 ; i < 32 ; i++ ){
            cout << "register " << i << " : " << (unsigned int) reg[i].v << endl ;
        }
        cout << "<------------------------------->" << endl ;
    }

};
#endif //RISC_V_TOMASULO_HEADER_HPP
