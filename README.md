# README



**文件结构**

- all_header / main -> 顺序执行
- pipeline_header / pipeline_main -> 五级流水
- tomasulo_header / tomasulo_main -> Tomasulo



**五级流水**

- [x] 五种操作：IF ID EX MEM WB
- [x] 同步1：synchronize 
  - [x] pc 判断：抹除中间层，等待输入标记，pc 修改在 MEM 阶段
  - [x] 读写判断：中间层拒绝被写入，所有操作等待 WB 写完
- [x] 中间层：前一层的输出，后一层的输入
  - [x] 状态：空 / 满 / 不可修改
  - [x] 若前一层为空，指令不执行；后一层为不可修改，指令不执行，同时前一层为不可修改
  - [x] WB 满指令执行完前一层变空，IF 拒绝写入不跳 pc
- [x] 中间夹层：数据元素与中间层相同
  - [x] 当前指令的结果输入到中间夹层，同步合法则将中间夹层结果给中间层
  - [x] bool status npc IR rs1 rs2 rd reg_rs1 reg_rs2 immediate ALUOutput 
- [x] 气泡停滞 stall
- [x] 分支预测：
  - [x] 4096 个 2 bit 饱和分支计数器 hashmap -> 下标
  - [x] 根据 pc 来获得下标，直接在 synchronize 时覆写 Layer
  - [x] 两个 bool 作为计数器



_以下内容就当笑话看吧，80%写假了_ :sweat_smile:



**Tomasulo**

- [x] 三种操作：issue execute commit
- [x] status: valid / invalid / empty
- [x] 类封装 ( 和 layer 差不多 )
  - [x] RS 元素：status npc read_pc rs1 rs2 reg_rs1 reg_rs2 immediate ALUOutput ROB_ID
  - [x] ROB 元素：同 RS RS_ID / SLB_ID
  - [x] SLB 元素：同 RS ROB_ID
  - [x] register -> Q / V 依赖关系 与 具体值
- [x] 循环队列处理 ROB / SLB 
- [x] 跳 pc 算出结果后清空 RS / SLB 其后的内容
