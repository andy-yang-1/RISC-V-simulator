//
// Created by andy_yang on 2021/7/9.
//

#include "Tomasulo_header.hpp"

description riscv ;

int main()
{
    freopen("queens.data","r",stdin) ;
//    freopen("my_output.txt","w",stdout) ;
    riscv.LoadMemory() ;
    while(true){
        riscv.t_clock++ ;
        riscv.commit() ;
        riscv.execute() ;
        riscv.MEM_process() ;
        riscv.issue() ;
#ifdef debug
        if ( riscv.t_clock == 10000 ){
            std::cerr << "check" << endl ;
        }
#endif
    }
}