#include <iostream>
#include "all_header.hpp"

description riscv ;

int main()
{
    freopen("tak.data","r",stdin) ;
//    freopen("my_output.txt","w",stdout) ;
    riscv.LoadMemory() ;
    while ( true ){
        riscv.fetch_code() ;
        riscv.decode_code() ;
        riscv.execute_code() ;
        riscv.maintain_reg0() ;
#ifdef debug
        riscv.debug_show() ;
#endif
    }
}
