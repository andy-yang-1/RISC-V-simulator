//
// Created by 86137 on 2021/7/1.
//

#ifndef RISC_V_PIPELINE_HEADER_HPP
#define RISC_V_PIPELINE_HEADER_HPP

#include <iostream>

//#define pipeline_show
//#define debug_run
//#define debug

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

enum Layer_status{ Empty , Full , Invalid }; // invalid -> clear -> Full

struct Layer{
    Layer_status status = Empty ;
    int npc = 0 , IR = 0 , opcode = 0 , func3 = 0 , func7 = 0 , rd = 0 , rs1 = 0 , rs2 = 0 , reg_rs1 = 0 , reg_rs2 = 0 , immediate = 0 , shamt = 0 , ALUOutput = 0 , LMD = 0 ;
    codeType code_type = R ; // npc 为读取结束的后一位

    int debug_read = 0 ; // 判断读的哪一条指令

    Layer &operator= ( const Layer &other ) { // 层间信息传递
        if ( this == &other ) return *this ;
        if ( other.status == Invalid ){
            return *this ;
        }
        status = other.status ;
        debug_read = other.debug_read ;
        npc = other.npc ;
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
        if ( status == Invalid ){
            status = Full ;
            return ;
        }
        Layer temp_layer ;
        *this = temp_layer ;
        status = Empty ;
    }
};

struct predictor{
    bool first = false , second = false ; // 看 first: false -> invalid true -> valid  先改 second 再改 first
    bool JumpIsValid() const{ return first ; }
    void JumpSucceed( bool jumpsuccess ){
        if ( jumpsuccess ){
            if ( first ){ // true false / true true
                second = true ;
            }else if ( second ){ // false true
                first = true ;
                second = false ;
            }else{ second = true ; } // false false
        } else {
            if ( !first ){ // false true / false false
                second = false ;
            } else if ( !second ){ // true false
                first = false ;
                second = true ;
            }else{ second = false ; } // true true
        }
    }
};

class description
{
private:

    int reg[32] = {0} ;
    Memory my_memory ;
    int pc = 0 ;
    int t_clock = 0 ;

    Layer IF_ID_layer , ID_EX_layer , EX_MEM_layer , MEM_WB_layer ;
    Layer IF_ans , ID_ans , EX_ans , MEM_ans ;

    bool stall ;

    predictor predictor_map[4097] ;

public:

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

    // 规定 ans 为空才允许写入

    void IF(){ //  IF 阶段跳 pc
        if ( IF_ans.status == Full ){
            return ;
        }
#ifdef debug
        if ( pc == 4168 ){
            std::cerr << "debug" << endl ;
        }
#endif
        IF_ans.status = Full ;
        IF_ans.IR = 0 ;
        IF_ans.debug_read = pc ;
        for ( int i = 0 ; i < 4 ; i++ ){
            IF_ans.IR = IF_ans.IR << 8 ;
            IF_ans.IR += my_memory.all_data[pc+3-i] ;
        }
        pc += 4 ;
        IF_ans.npc = pc ;
#ifdef pipeline_show
        std::cerr << "IF Working: " << std::hex << IF_ans.debug_read << endl ;
#endif
    }

    void ID(){
        if ( IF_ID_layer.status == Empty ) {
            ID_ans.clear() ;
            return;
        }
        if ( ID_ans.status == Full ){
            return ;
        }
        ID_ans.status = Full ;
        ID_ans = IF_ID_layer ;
        ID_ans.opcode = get_bits(IF_ID_layer.IR,0,6) ;
        switch(ID_ans.opcode){
            case 0b0110111 :
            case 0b0010111 : ID_ans.code_type = U ; ID_ans.rd = get_bits( ID_ans.IR , 7 , 11 ) ; ID_ans.immediate = U_get_immediate(ID_ans.IR) ; break ;
            case 0b1101111 : ID_ans.code_type = J ; ID_ans.rd = get_bits( ID_ans.IR , 7, 11 ) ; ID_ans.immediate = J_get_immediate(ID_ans.IR) ; break ;
            case 0b1100111 :
            case 0b0000011 :
            case 0b0010011 : ID_ans.code_type = I ; ID_ans.rd = get_bits( ID_ans.IR , 7 , 11 ) ; ID_ans.rs1 = get_bits(ID_ans.IR,15,19) ; ID_ans.func3 = get_bits(ID_ans.IR,12,14) ; ID_ans.immediate = I_get_immediate(ID_ans.IR) ; ID_ans.shamt = get_bits(ID_ans.IR,20,24) ; ID_ans.func7 = get_bits(ID_ans.IR,25,31) ; break ;
            case 0b1100011 : ID_ans.code_type = B ; ID_ans.rs1 = get_bits( ID_ans.IR , 15 , 19 ) ; ID_ans.rs2 = get_bits( ID_ans.IR , 20 , 24 ) ; ID_ans.func3 = get_bits(ID_ans.IR,12,14) ; ID_ans.immediate = B_get_immediate(ID_ans.IR) ; break ;
            case 0b0100011 : ID_ans.code_type = S ; ID_ans.rs1 = get_bits( ID_ans.IR , 15 , 19 ) ; ID_ans.rs2 = get_bits( ID_ans.IR , 20 , 24 ) ; ID_ans.func3 = get_bits(ID_ans.IR,12,14) ; ID_ans.immediate = S_get_immediate(ID_ans.IR) ; break ;
            case 0b0110011 : ID_ans.code_type = R ; ID_ans.rd = get_bits(ID_ans.IR,7,11) ; ID_ans.rs1 = get_bits( ID_ans.IR , 15 , 19 ) ; ID_ans.rs2 = get_bits( ID_ans.IR , 20 , 24 ) ; ID_ans.func3 = get_bits(ID_ans.IR,12,14) ; ID_ans.func7 = get_bits(ID_ans.IR,25,31) ; break ;
        }
        ID_ans.reg_rs1 = reg[ID_ans.rs1] ;
        ID_ans.reg_rs2 = reg[ID_ans.rs2] ;
#ifdef pipeline_show
        std::cerr << "ID Working: " << std::hex << ID_ans.debug_read << endl ;
#endif
    }

    void PRINT_ANS(){
        cout << (unsigned int) ( reg[10] & 255u ) << endl ;
#ifdef pipeline_show
        cout << t_clock << endl ;
#endif
        exit(0) ;
    }

    void EX(){
        if ( ID_EX_layer.status == Empty ) {
            EX_ans.clear() ;
            return;
        }
        if ( EX_ans.status == Full ){
            return ;
        }
        EX_ans.status = Full ;
        EX_ans = ID_EX_layer ;
        int temp = 0 ;
#ifdef pipeline_show
        std::cerr << "EX Working: " << std::hex << EX_ans.debug_read << endl ;
#endif
        switch (EX_ans.opcode) {
            case 0b0110111 : EX_ans.ALUOutput = EX_ans.immediate ; return ; // lui
            case 0b0010111 : EX_ans.ALUOutput = EX_ans.npc - 4 + EX_ans.immediate ; return ; // auipc
            case 0b1101111 : EX_ans.ALUOutput = EX_ans.npc  - 4 + 4 ; EX_ans.npc += EX_ans.immediate - 4 ; return ; // jal
            case 0b1100111 : temp = EX_ans.npc - 4 + 4 ; EX_ans.npc = ( EX_ans.reg_rs1 + EX_ans.immediate ) & ~1 ; EX_ans.ALUOutput = temp ; return ; // jalr
            case 0b1100011 :
                switch (EX_ans.func3) { // 跳转相关
                    case 0b000: if ( EX_ans.reg_rs1 == EX_ans.reg_rs2 ) EX_ans.npc += EX_ans.immediate - 4 ; return ; // beq
                    case 0b001: if ( EX_ans.reg_rs1 != EX_ans.reg_rs2 ) EX_ans.npc += EX_ans.immediate - 4 ; return ; // bne
                    case 0b100: if ( EX_ans.reg_rs1 < EX_ans.reg_rs2 ) EX_ans.npc += EX_ans.immediate - 4 ; return ; // blt
                    case 0b101: if ( EX_ans.reg_rs1 >= EX_ans.reg_rs2 ) EX_ans.npc += EX_ans.immediate - 4 ; return ; // bge
                    case 0b110: if ( EX_ans.reg_rs1 < unsigned ( EX_ans.reg_rs2 ) ) EX_ans.npc += EX_ans.immediate - 4 ; return ; // bltu
                    case 0b111: if ( EX_ans.reg_rs1 >= unsigned ( EX_ans.reg_rs2 )) EX_ans.npc += EX_ans.immediate - 4 ; return ; // bgeu
                }
            case 0b0000011 : // LOAD 相关
            case 0b0100011 : EX_ans.ALUOutput = EX_ans.reg_rs1 + EX_ans.immediate ; return ;

            case 0b0010011 :
                switch (EX_ans.func3) { // LOAD register 相关
                    case 0b000: EX_ans.ALUOutput = EX_ans.reg_rs1 + EX_ans.immediate ; return ; // addi
                    case 0b010: EX_ans.ALUOutput = EX_ans.reg_rs1 < EX_ans.immediate ; return ; // slti
                    case 0b011: EX_ans.ALUOutput = ( EX_ans.reg_rs1 < (unsigned ) EX_ans.immediate ) ; return ; // sltiu
                    case 0b100: EX_ans.ALUOutput = EX_ans.reg_rs1 xor EX_ans.immediate ; return ; // xori
                    case 0b110: EX_ans.ALUOutput = EX_ans.reg_rs1 | EX_ans.immediate ; return ; // ori
                    case 0b111: EX_ans.ALUOutput = EX_ans.reg_rs1 & EX_ans.immediate ; return ; // andi
                    case 0b001: EX_ans.ALUOutput = EX_ans.reg_rs1 << EX_ans.shamt ; return ; // slli
                    case 0b101:
                        if ( EX_ans.func7 == 0 ){ // srli
                            EX_ans.ALUOutput = ( (unsigned int) EX_ans.reg_rs1 >> EX_ans.shamt ) ;
                            return ;
                        }else{ // srai
                            EX_ans.ALUOutput = ( EX_ans.reg_rs1 >> EX_ans.shamt ) ;
                            return ;
                        }
                }
            case 0b0110011 :
                switch (EX_ans.func3) { // LOAD register 相关
                    case 0b000:
                        if ( EX_ans.func7 == 0 ){ // add
                            EX_ans.ALUOutput = EX_ans.reg_rs1 + EX_ans.reg_rs2 ;
                            return ;
                        }else{ // sub
                            EX_ans.ALUOutput = EX_ans.reg_rs1 - EX_ans.reg_rs2 ;
                            return ;
                        }
                    case 0b001: EX_ans.ALUOutput = EX_ans.reg_rs1 << EX_ans.reg_rs2 ; return ; // sll
                    case 0b010: EX_ans.ALUOutput = EX_ans.reg_rs1 < EX_ans.reg_rs2 ; return ; // slt
                    case 0b011: EX_ans.ALUOutput = EX_ans.reg_rs1 < (unsigned) EX_ans.reg_rs2 ; return ; // sltu
                    case 0b100: EX_ans.ALUOutput = EX_ans.reg_rs1 xor EX_ans.reg_rs2 ; return ; // xor
                    case 0b101:
                        if ( EX_ans.func7 == 0 ){ // srl
                            EX_ans.ALUOutput = EX_ans.reg_rs1 >> ((unsigned) EX_ans.reg_rs2) ;
                            return ;
                        }else{ // sra
                            EX_ans.ALUOutput = EX_ans.reg_rs1 >> EX_ans.reg_rs2 ;
                            return ;
                        }
                    case 0b110: EX_ans.ALUOutput = EX_ans.reg_rs1 | EX_ans.reg_rs2 ; return ; // or
                    case 0b111: EX_ans.ALUOutput = EX_ans.reg_rs1 & EX_ans.reg_rs2 ; return ; // and
                }
        }
    }

    void MEM(){
        if ( EX_MEM_layer.status == Empty ) {
            MEM_ans.clear() ;
            return;
        }
        if ( MEM_ans.status == Full ){
            return ;
        }
        MEM_ans.status = Full ;
        MEM_ans = EX_MEM_layer ;
#ifdef pipeline_show
        std::cerr << "MEM Working: " << std::hex << MEM_ans.debug_read  << endl ;
#endif
        switch (MEM_ans.opcode) {
            case 0b0110111 :  // lui
            case 0b0010111 :  // auipc
            case 0b1101111 :  // jal
            case 0b1100111 :  // jalr
            case 0b1100011 : return ; // todo 跳转相关，在 mem 阶段或 ex->synchronize 阶段跳转
            case 0b0000011 :
                switch (MEM_ans.func3) {
                    case 0b000: MEM_ans.LMD = char_extend_sign(my_memory.read_short(MEM_ans.ALUOutput)) ; return ; // lb
                    case 0b001: MEM_ans.LMD = short_extend_sign(my_memory.read_ushort(MEM_ans.ALUOutput)) ; return ; // lh
                    case 0b010: MEM_ans.LMD = my_memory.read_word(MEM_ans.ALUOutput) ; return ; // lw
                    case 0b100: MEM_ans.LMD = my_memory.read_short(MEM_ans.ALUOutput) ; return ; // lbu
                    case 0b101: MEM_ans.LMD = my_memory.read_ushort(MEM_ans.ALUOutput) ; return ; // lhu
                }
            case 0b0100011 :
                switch (MEM_ans.func3) {
                    case 0b000: my_memory.write_short(MEM_ans.ALUOutput,MEM_ans.reg_rs2) ; return ; // sb
                    case 0b001: my_memory.write_ushort(MEM_ans.ALUOutput,MEM_ans.reg_rs2) ; return ; // sh
                    case 0b010: my_memory.write_word(MEM_ans.ALUOutput,MEM_ans.reg_rs2) ;  return ; // sw
                }
            case 0b0010011 :
            case 0b0110011 : return ;
        }
    }

    void WB(){
        if ( MEM_WB_layer.status == Empty ) return ;
        if ( MEM_WB_layer.IR == 0x0ff00513 ){
            PRINT_ANS() ;
        }
#ifdef pipeline_show
        std::cerr << "WB Working: " << std::hex << MEM_WB_layer.debug_read << endl ;
#endif

#ifdef debug
        if ( MEM_WB_layer.npc == 4172 ){
            std::cerr << "debug" << endl ;
        }
#endif

        switch (MEM_WB_layer.opcode) {
            case 0b0110111 :  // lui
            case 0b0010111 :  // auipc
            case 0b1101111 :  // jal
            case 0b1100111 : reg[MEM_WB_layer.rd] = MEM_WB_layer.ALUOutput ; break ; // jalr
            case 0b1100011 : break ;
            case 0b0000011 : reg[MEM_WB_layer.rd] = MEM_WB_layer.LMD ; break ;
            case 0b0100011 : break ;
            case 0b0010011 :
            case 0b0110011 : reg[MEM_WB_layer.rd] = MEM_WB_layer.ALUOutput ; break ;
        }
        reg[0] = 0 ; // 归零
#ifdef debug_run
        debug_show() ;
#endif
    }

    // todo synchronize 时 branch_predict

    void branch_predict(){ // 主要是去改 pc
        if ( ID_ans.status == Empty ) return ;
        if ( ID_ans.code_type != J && ID_ans.code_type != B && ID_ans.opcode != 0b1100111 ) return ; // todo 未判断 jalr
        // todo
        if ( ID_ans.opcode == 0b1100111 ){
            pc = ( ID_ans.reg_rs1 + ID_ans.immediate ) & ~1 ;
            IF_ans.clear() ;
        }
        if ( ID_ans.code_type == J ){
            pc = ID_ans.npc - 4 + ID_ans.immediate ;
            IF_ans.clear() ;
        }else{
            if ( predictor_map[ID_ans.debug_read%4096].first ){
                pc = ID_ans.npc - 4 + ID_ans.immediate ;
                IF_ans.clear() ;
            }
        }
    }

    void deal_time_hazard(){
//        if ( pc != EX_ans.npc + 8 && EX_ans.status != Empty ){ // todo 判 time hazard 方式需要改变 不可以用 pc 判 hazard 而是去特定模块 (fetch) 判 hazard
//#ifdef pipeline_show
//            std::cerr << "time hazard -> old pc: " << pc << " real pc: " << EX_ans.npc << endl ;
//#endif
//            pc = EX_ans.npc ;
//            IF_ans.clear() ;
//            ID_ans.clear() ;
//            IF_ID_layer.clear() ;
//        }
        if ( EX_ans.status != Empty && ( EX_ans.code_type == J || EX_ans.code_type == B || EX_ans.opcode == 0b1100111 ) ){
            if ( EX_ans.code_type == B ){
                if ( EX_ans.debug_read + 4 == EX_ans.npc ) predictor_map[EX_ans.debug_read%4096].JumpSucceed(false) ;
                else predictor_map[EX_ans.debug_read%4096].JumpSucceed(true) ;
            }
            if ( ID_ans.status != Empty && ID_ans.npc != EX_ans.npc + 4 ){
                pc = EX_ans.npc ;
                IF_ans.clear() ;
                ID_ans.clear() ;
                IF_ID_layer.clear() ;
                return ;
            }
            if ( ID_ans.status == Empty && IF_ans.status != Empty && IF_ans.npc != EX_ans.npc + 4 ){
                pc = EX_ans.npc ;
                IF_ans.clear() ;
                return ;
            }
        }
    }

    void deal_data_hazard(){
        if ( ID_ans.status != Empty && ( ( EX_ans.status != Empty && ( ID_ans.rs1 == EX_ans.rd || ID_ans.rs2 == EX_ans.rd ) && EX_ans.rd != 0 ) || ( MEM_ans.status != Empty && ( ID_ans.rs1 == MEM_ans.rd || ID_ans.rs2 == MEM_ans.rd ) && MEM_ans.rd != 0 ) || ( MEM_WB_layer.status != Empty && ( ID_ans.rs1 == MEM_WB_layer.rd || ID_ans.rs2 == MEM_WB_layer.rd ) && MEM_WB_layer.rd != 0 ) ) ){ // WB hazard 需要处理
#ifdef pipeline_show
            std::cerr << "data hazard -> ID rs1: " << ID_ans.rs1 << " ID rs2: " << ID_ans.rs2 << " EX rd: " << EX_ans.rd << " MEM rd: " << MEM_ans.rd << " WB rd: " << MEM_WB_layer.rd << endl ;
#endif
            ID_ans.clear() ; // 避免写入夹层
            IF_ans.status = Invalid ; // 不清空 IF_ans
        }
    }

    void synchronize(){ // status 自动
        t_clock++ ;
        // time hazard
        deal_time_hazard() ;
        // data hazard
        deal_data_hazard() ;
        branch_predict() ;
        MEM_WB_layer = MEM_ans ;
        EX_MEM_layer = EX_ans ;
        ID_EX_layer = ID_ans ;
        IF_ID_layer = IF_ans ; // 不能动 IF_ID_layer
        IF_ans.clear() ;
        ID_ans.clear() ;
        EX_ans.clear() ;
        MEM_ans.clear() ;
        reg[0] = 0 ;
#ifdef pipeline_show
        std::cerr << "finish_cycle: " << t_clock << endl ;
#endif
    }

    void debug_show(){
        cout << "<------------------------------->" << endl ;
        cout << "pc: " << MEM_WB_layer.npc << endl ;
        for ( int i = 0 ; i < 32 ; i++ ){
            cout << "register " << i << " : " << (unsigned int) reg[i] << endl ;
        }
        cout << "<------------------------------->" << endl ;
    }

};

#endif //RISC_V_PIPELINE_HEADER_HPP
