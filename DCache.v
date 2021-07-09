`timescale 1ns / 1ps
`define STATE_IDLE 0
`define STATE_READ_REPLACE 1
`define STATE_FETCH 2
`define STATE_WRITE_REPLACE 3
`define STATE_FILL 4
`include"defines_cache.v"
module DCache(

    input wire                  clk,
    input wire rst,
    
    //cpu data request
    input wire                  cpu_rreq_i,
    input wire                  cpu_wreq_i,

    input wire [`DataAddrBus]   virtual_addr_i,
    input wire [`DataAddrBus]   physical_addr_i,
    input wire [`DataBus]       cpu_wdata_i,
    input wire [3:0]            cpu_wsel_i,

    output wire                 cpu_data_valid_o,//hit_o
    output wire [`DataBus]      cpu_data_final_o,
	
	//cache state
	output reg                  cpu_stall_o,
    
    //mem read
    input wire                  mem_rvalid_i,
    input wire [`WayBus]        mem_rdata_i,
    output wire                 mem_ren_o,
    output wire[`DataAddrBus]   mem_araddr_o,
	//mem write
    input wire                  mem_bvalid_i,
    output wire                 mem_wen_o,
    output wire[`WayBus]        mem_wdata_o,
    output wire [`DataAddrBus]  mem_awaddr_o
    
    );
	
//////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Initialization////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
    
	
	//mem_data_i in 2-dimen array
	wire [`DataBus]mem_rdata[`BlockNum-1:0];
    for(genvar i =0 ;i<`BlockNum; i=i+1)begin
		assign mem_rdata[i] = mem_rdata_i[32*(i+1)-1:32*i];
    end

    reg [`DataBus]mem_wdata[`BlockNum-1:0];
    for(genvar i =0 ;i<`BlockNum; i=i+1)begin
        assign mem_wdata_o[32*(i+1)-1:32*i] = mem_wdata[i];
    end 
	
	//keep input data
    reg [`DataAddrBus]	virtual_addr;
    reg [`DataAddrBus]	physical_addr;
    reg [`RegBus]		cpu_wdata;
	reg 				cpu_rreq_tmp;
	wire 				cpu_rreq = cpu_rreq_tmp ;
	reg 				cpu_wreq_tmp;
	wire 				cpu_wreq = cpu_wreq_tmp ;
    reg [3:0]			cpu_wsel;
    wire [`DataBus]		wsel_expand = {{8{cpu_wsel[3]}} , {8{cpu_wsel[2]}} , {8{cpu_wsel[1]}} , {8{cpu_wsel[0]}}};
    always@(posedge clk)begin
        if(rst)begin
            virtual_addr	<= `ZeroWord;
            physical_addr	<= `ZeroWord;
            cpu_wdata		<= `ZeroWord;
            cpu_rreq_tmp 		<= `Invalid;
            cpu_wreq_tmp 		<= `Invalid;
            cpu_wsel 		<= 4'h0;
        end
        else if(cpu_stall_o)begin
            virtual_addr 	<= virtual_addr;
            physical_addr	<= physical_addr;
            cpu_wdata 		<= cpu_wdata;
            cpu_rreq_tmp 	<= cpu_rreq;
            cpu_wreq_tmp 	<= cpu_wreq;
            cpu_wsel 		<= cpu_wsel;
        end
        else begin
            virtual_addr 	<= virtual_addr_i;
            physical_addr	<= physical_addr_i;
            cpu_wdata 		<= cpu_wdata_i;
            cpu_rreq_tmp 	<= cpu_rreq_i;
            cpu_wreq_tmp 	<= cpu_wreq_i;
            cpu_wsel 		<= cpu_wsel_i;
        end
    end
	
   
    
    //BANK 0~7 WAY 0~1

	reg [`DataBus]cache_wdata[`BlockNum-1:0];
	
    wire [3:0]wea_way0;
    wire [3:0]wea_way1;
    
	wire [`DataBus]way0_cache[`BlockNum-1:0];
    simple_dual_ram Bank0_way0 (.clka(clk),.ena(|wea_way0),.wea(wea_way0),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[0]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way0_cache[0]));
    simple_dual_ram Bank1_way0 (.clka(clk),.ena(|wea_way0),.wea(wea_way0),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[1]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way0_cache[1]));
    simple_dual_ram Bank2_way0 (.clka(clk),.ena(|wea_way0),.wea(wea_way0),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[2]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way0_cache[2]));
    simple_dual_ram Bank3_way0 (.clka(clk),.ena(|wea_way0),.wea(wea_way0),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[3]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way0_cache[3]));
    simple_dual_ram Bank4_way0 (.clka(clk),.ena(|wea_way0),.wea(wea_way0),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[4]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way0_cache[4]));
    simple_dual_ram Bank5_way0 (.clka(clk),.ena(|wea_way0),.wea(wea_way0),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[5]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way0_cache[5]));
    simple_dual_ram Bank6_way0 (.clka(clk),.ena(|wea_way0),.wea(wea_way0),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[6]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way0_cache[6]));
    simple_dual_ram Bank7_way0 (.clka(clk),.ena(|wea_way0),.wea(wea_way0),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[7]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way0_cache[7]));
   
	wire [`DataBus]way1_cache[`BlockNum-1:0]; 
    simple_dual_ram Bank0_way1 (.clka(clk),.ena(|wea_way1),.wea(wea_way1),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[0]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way1_cache[0]));
    simple_dual_ram Bank1_way1 (.clka(clk),.ena(|wea_way1),.wea(wea_way1),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[1]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way1_cache[1]));
    simple_dual_ram Bank2_way1 (.clka(clk),.ena(|wea_way1),.wea(wea_way1),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[2]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way1_cache[2]));
    simple_dual_ram Bank3_way1 (.clka(clk),.ena(|wea_way1),.wea(wea_way1),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[3]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way1_cache[3]));
    simple_dual_ram Bank4_way1 (.clka(clk),.ena(|wea_way1),.wea(wea_way1),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[4]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way1_cache[4]));
    simple_dual_ram Bank5_way1 (.clka(clk),.ena(|wea_way1),.wea(wea_way1),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[5]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way1_cache[5]));
    simple_dual_ram Bank6_way1 (.clka(clk),.ena(|wea_way1),.wea(wea_way1),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[6]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way1_cache[6]));
    simple_dual_ram Bank7_way1 (.clka(clk),.ena(|wea_way1),.wea(wea_way1),.addra(virtual_addr[`IndexBus]), .dina(cache_wdata[7]),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(way1_cache[7]));                        

    //Tag+Valid
    wire [`TagVBus]tagv_cache_w0;
    wire [`TagVBus]tagv_cache_w1;
    simple_dual_ram TagV0 (.clka(clk),.ena(|wea_way0),.wea(wea_way0),.addra(virtual_addr[`IndexBus]), .dina({1'b1,physical_addr[`TagBus]}),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(tagv_cache_w0));
    simple_dual_ram TagV1 (.clka(clk),.ena(|wea_way1),.wea(wea_way1),.addra(virtual_addr[`IndexBus]), .dina({1'b1,physical_addr[`TagBus]}),.clkb(clk),.enb(`Enable),.addrb(virtual_addr[`IndexBus]),.doutb(tagv_cache_w1));  
    
    //LRU
    reg [`SetBus]LRU;
    wire LRU_pick = LRU[virtual_addr[`IndexBus]];
    always@(posedge clk)begin
        if(rst)
            LRU <= 0;
        else if(hit_success == `HitSuccess)//hit: set LRU to bit that is not hit
            LRU[virtual_addr[`IndexBus]] <= hit_way0;
            //LRU_pick = 1, the way0 is used recently, the way1 is picked to replace
            //LRU_pick = 0, the way1 is used recently, the way0 is picked to replace
        else if(bus_read_success == `Success && hit_fail == `Valid)//not hit: set opposite LRU
            LRU[virtual_addr[`IndexBus]] <= ~LRU_pick;
        else
            LRU <= LRU;
    end
    
    //Dirty 
    reg [`DirtyBus] dirty;//2*SetNum, the WayNum
	wire write_dirty = dirty[{virtual_addr[`IndexBus],LRU_pick}]; //Should the way picked to be replaced be written back
    always@(posedge clk)begin
        if(rst)
            dirty<=0;
		else if(bus_read_success == `Success && cpu_rreq == `Valid)//Read not hit
            dirty[{virtual_addr[`IndexBus],LRU_pick}] <= `NotDirty;
		else if(bus_read_success == `Success && cpu_wreq == `Valid)//write not hit
            dirty[{virtual_addr[`IndexBus],LRU_pick}] <= `Dirty;
		else if((hit_way0|hit_way1) == `HitSuccess && cpu_wreq == `Valid)//write hit 
            dirty[{virtual_addr[`IndexBus],hit_way1}] <= `Dirty;
        else
            dirty <= dirty;
    end
	
	//Stall
	always@(*)begin 
		if(hit_fail == `Valid)
			cpu_stall_o <= ~bus_read_success;
		else 
			cpu_stall_o <= `Invalid;
	end
    
    

    
	wire bus_read_success = mem_rvalid_i;

    wire [`DataBus]data_way0 = way0_cache[virtual_addr[4:2]];
    wire [`DataBus]data_way1 = way1_cache[virtual_addr[4:2]];
								
//////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Main Operation////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
    ////////////////STATE_FETCH_DATA//////////////////
	//hit judgement
    wire hit_way0 = (tagv_cache_w0[20:0]==physical_addr[`TagBus] && tagv_cache_w0[21]==`Valid)? `HitSuccess : `HitFail;
    wire hit_way1 = (tagv_cache_w1[20:0]==physical_addr[`TagBus] && tagv_cache_w1[21]==`Valid)? `HitSuccess : `HitFail;
	wire hit_success = (hit_way0 | hit_way1) & (cpu_rreq | cpu_wreq);//hit & req valid
	wire hit_fail = ~(hit_success) & (cpu_rreq | cpu_wreq);      
   //Tag not hit
   //write to ram
    assign wea_way0 =(hit_fail==`Valid && bus_read_success == `Success && LRU_pick == 1'b0)? 4'b1111 : // Not Hit
                     (hit_way0 == `HitSuccess && cpu_wreq == `WriteEnable )? cpu_wsel: 4'h0;//Write Hit
    
    assign wea_way1 = (hit_fail==`Valid && bus_read_success == `Success && LRU_pick == 1'b1)? 4'b1111 ://not hit
                     (hit_way1 == `HitSuccess  && cpu_wreq == `WriteEnable )? cpu_wsel : 4'h0;//write hit
                     
                 


    reg [2:0]    state;
   //AXI read requirements
    reg     mem_ren;
	assign  mem_ren_o = mem_ren;
    reg     mem_wen;
    assign  mem_wen_o = mem_wen;
    reg     mem_awaddr;

    assign mem_awaddr_o = mem_awaddr;
	assign mem_araddr_o = physical_addr;

	//ram write data
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            // reset
            state  <=  `STATE_IDLE;

        end
        case(state)
        `STATE_IDLE:
        begin
            cache_wdata[0]  <= `ZeroWord;
            cache_wdata[1]  <= `ZeroWord;
            cache_wdata[2]  <= `ZeroWord;
            cache_wdata[3]  <= `ZeroWord;
            cache_wdata[4]  <= `ZeroWord;
            cache_wdata[5]  <= `ZeroWord;
            cache_wdata[6]  <= `ZeroWord;
            cache_wdata[7]  <= `ZeroWord;
            mem_ren         <= `Disable;
            mem_wen         <= `Disable;

            mem_awaddr      <= LRU_pick == 0 ?  {tagv_cache_w0[20:0], physical_addr[`IndexBus], 5'b00000}:
                                                {tagv_cache_w1[20:0], physical_addr[`IndexBus], 5'b00000};


            mem_wdata[0]    <= LRU_pick == 0 ?  way0_cache[0] : way1_cache[0];
            mem_wdata[1]    <= LRU_pick == 0 ?  way0_cache[1] : way1_cache[1];
            mem_wdata[2]    <= LRU_pick == 0 ?  way0_cache[2] : way1_cache[2];
            mem_wdata[3]    <= LRU_pick == 0 ?  way0_cache[3] : way1_cache[3];
            mem_wdata[4]    <= LRU_pick == 0 ?  way0_cache[4] : way1_cache[4];
            mem_wdata[5]    <= LRU_pick == 0 ?  way0_cache[5] : way1_cache[5];
            mem_wdata[6]    <= LRU_pick == 0 ?  way0_cache[6] : way1_cache[6];
            mem_wdata[7]    <= LRU_pick == 0 ?  way0_cache[7] : way1_cache[7];


            if (hit_success & cpu_rreq) state <= `STATE_FETCH; 
            if (hit_fail    & cpu_rreq) state <= `STATE_READ_REPLACE;
            if (hit_success & cpu_wreq) state <= `STATE_FILL;
            if (hit_fail    & cpu_wreq) state <= `STATE_WRITE_REPLACE;
        end

        `STATE_READ_REPLACE:
        begin
            mem_ren         <= `Enable;
            if(mem_rvalid_i)begin
                cache_wdata[0] <= mem_rdata[0];
                cache_wdata[1] <= mem_rdata[1];
                cache_wdata[2] <= mem_rdata[2];
                cache_wdata[3] <= mem_rdata[3];
                cache_wdata[4] <= mem_rdata[4];
                cache_wdata[5] <= mem_rdata[5];
                cache_wdata[6] <= mem_rdata[6];
                cache_wdata[7] <= mem_rdata[7];
            end
            if(write_dirty == `Dirty) begin
                mem_wen        <= `Enable;
            end
            
            state <= `STATE_IDLE;
        end
        `STATE_FETCH:
        begin
            state <= `STATE_IDLE;
        end

        `STATE_WRITE_REPLACE:
        begin
            mem_ren         <= `Enable;
            mem_wen         <= `Enable;
            if(mem_rvalid_i)begin
                cache_wdata[0] <= mem_rdata[0];
                cache_wdata[1] <= mem_rdata[1];
                cache_wdata[2] <= mem_rdata[2];
                cache_wdata[3] <= mem_rdata[3];
                cache_wdata[4] <= mem_rdata[4];
                cache_wdata[5] <= mem_rdata[5];
                cache_wdata[6] <= mem_rdata[6];
                cache_wdata[7] <= mem_rdata[7];
                if(cpu_wreq == `WriteEnable)//write
                    cache_wdata[virtual_addr[4:2]] <= (cpu_wdata & wsel_expand)|(mem_rdata_i[virtual_addr[4:2]] & ~wsel_expand);
            end
            if (write_dirty == `Dirty)  mem_wen <= `Enable;
            state <= `STATE_IDLE;
        end
        `STATE_FILL:
        begin
            if(hit_way0 == `HitSuccess)begin
                cache_wdata[virtual_addr[4:2]] <= (cpu_wdata & wsel_expand)|(way0_cache[virtual_addr[4:2]] & ~wsel_expand);
            end
            if(hit_way1 == `HitSuccess)begin
                cache_wdata[virtual_addr[4:2]] <= (cpu_wdata & wsel_expand)|(way1_cache[virtual_addr[4:2]] & ~wsel_expand);
            end
            state <= `STATE_IDLE;
        end
        endcase       
    end

/*	always@(*) begin 
        cache_wdata[0] <= `ZeroWord;
        cache_wdata[1] <= `ZeroWord;
        cache_wdata[2] <= `ZeroWord;
        cache_wdata[3] <= `ZeroWord;
        cache_wdata[4] <= `ZeroWord;
        cache_wdata[5] <= `ZeroWord;
        cache_wdata[6] <= `ZeroWord;
        cache_wdata[7] <= `ZeroWord;

		if(hit_fail == `Valid)begin//hit fail
			cache_wdata[0] <= mem_rdata[0];
			cache_wdata[1] <= mem_rdata[1];
			cache_wdata[2] <= mem_rdata[2];
			cache_wdata[3] <= mem_rdata[3];
			cache_wdata[4] <= mem_rdata[4];
			cache_wdata[5] <= mem_rdata[5];
			cache_wdata[6] <= mem_rdata[6];
			cache_wdata[7] <= mem_rdata[7];
			if(cpu_wreq == `WriteEnable)//write
				cache_wdata[virtual_addr[4:2]] <= (cpu_wdata & wsel_expand)|(mem_rdata_i[virtual_addr[4:2]] & ~wsel_expand);
		end
		if(hit_success == `HitSuccess)begin//hit success
			if(hit_way0 == `HitSuccess)begin

				cache_wdata[virtual_addr[4:2]] <= (cpu_wdata & wsel_expand)|(way0_cache[virtual_addr[4:2]] & ~wsel_expand);
			end
			if(hit_way1 == `HitSuccess)begin
				cache_wdata[virtual_addr[4:2]] <= (cpu_wdata & wsel_expand)|(way1_cache[virtual_addr[4:2]] & ~wsel_expand);
			end
		end
	end*/
   
    
//////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Output//////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
    reg [`DataBus] cpu_data_o;
    always@(*)begin
        cpu_data_o <= `ZeroWord;
        if(hit_way0 == `HitSuccess)
            cpu_data_o <= data_way0;
        if(hit_way1 == `HitSuccess)
            cpu_data_o <= data_way1;
        if(bus_read_success ==`Success)
			cpu_data_o <= mem_rdata[virtual_addr[4:2]];
    end

    assign cpu_data_valid_o = (hit_success == `HitSuccess && cpu_rreq == `Valid)? `Valid : `Invalid ;
							  
	assign cpu_data_final_o = cpu_data_o;

	

endmodule
