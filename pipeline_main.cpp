//
// Created by 86137 on 2021/7/1.
//

#include "PipeLine_header.hpp"
#include <algorithm>

description riscv ;

int main()
{
    freopen("array_test2.data","r",stdin) ;
//    freopen("my_output.txt","w",stdout) ;
    riscv.LoadMemory() ;
    while(true){
        int seq[5] = {0} ;
        for ( int j = 0 ; j < 5 ; j++ ) seq[j] = j ;
        std::random_shuffle(seq,seq+5) ;
        for ( int j = 0 ; j < 5 ; j++ ) {
            switch (seq[j]) {
                case 0 :  riscv.IF(); break ;
                case 1 :  riscv.ID(); break ;
                case 2 :  riscv.EX(); break ;
                case 3 :  riscv.MEM(); break ;
                case 4 :  riscv.WB(); break ;
            }
        }
        riscv.synchronize();
    }
}
