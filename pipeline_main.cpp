//
// Created by 86137 on 2021/7/1.
//

#include "PipeLine_header.hpp"

description riscv ;

int main()
{
    freopen("sample.data","r",stdin) ;
//    freopen("my_output.txt","w",stdout) ;
    riscv.LoadMemory() ;
    while(true){
        riscv.IF() ;
        riscv.ID() ;
        riscv.EX() ;
        riscv.MEM() ;
        riscv.WB() ;
        riscv.synchronize() ;
    }
}
