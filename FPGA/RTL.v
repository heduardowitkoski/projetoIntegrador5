module SRGL(
    input wire clk,
    input wire reset,
    input wire mov, //flag de movimentacao

    input wire signed [959:0] mpu, //30 valores do acelerometro -> 30x32(bits) = 960 bits
    input wire [4:0] ldr, //5 sinais dos ldrs

    output reg [7:0] letra_final
);

reg signed [31:0] mem_rom [9:0][29:0];
reg signed [31:0] mem_ram [29:0];

wire signed letra_base[7:0]; //8bits para letras em ASCII

integer i;
//===============
//DECODIFICADOR
//===============
always(*) begin
    case(ldr)
        5'b00001: letra_base = "A";

        5'b11110: letra_base = "B";

        5'b11111: letra_base = "C";

        5'b00010: letra_base = "D";

        5'b11101: letra_base = "F";

        5'b00110: letra_base = "U";

        5'b00101: letra_base = "H";

        5'b10000: letra_base = "I";
        
        5'b00011: letra_base = "L";

        5'b01110: letra_base = "W";

        5'b10001: letra_base = "Y";

        5'b00000: letra_base = "S";

        default:begin end
    endcase
end

wire signed [9:0] letra_encaminhada = mov  ?   ROM_addr    :
                                    letra_base;

wire signed [31:0] vetor_modelo [29:0];
wire signed [31:0] a, b;

//==============================================
//ESCOLHA NA ROM COM MODELOS E PREENCHE BUFFER
//==============================================
always @(*) begin
    a = 32;
    b = 32;
    if(mov) begin
            //PEGA O VETOR MODELO COM 30 ELEMENTOS CORRESPONDENTE DA LETRA
            vetor_modelo = mem_rom[letra_encaminhada];
            for (i = 0; i < 30; i = i + 1) begin
                //PREENCHE A RAM COM VALORES DO MPU
                mem_ram[i] = mpu[31+(a*i) : b*i];
            end
        end

end

reg signed [31:0] vetor_model [29:0];
reg signed [31:0] buff [29:0];
reg signed [7:0] letra_acel;

//==============================
//ADICIONA AOS REGISTRADORES
//==============================
always @(posedge clk or posedge reset) begin
    if(reset) begin
        vetor_model <= 0;
        buff        <= 0;
    end else begin
        if(mov) begin
            vetor_model <= vetor_modelo;
            buff        <= mem_ram;
            letra_acel  <= letra_base; //MODIFICAR DEPOIS PARA LETRA CORRETA DO ACELEROMETRO
        end

    end
end

wire signed [31:0] sub, soma_indice, media;


//==========================
//SUBTRATOR DA media
//==========================
always @(*) begin
    sub             = 0;
    soma_indice     = 0;
    media       = 0;
    if(mov) begin
        for(i=0; i < 30; i=i+1) begin
            sub = buff[i] - vetor_model[i];
            soma_indice = soma_indice + sub;
        end
        media = soma_indice / 30;
    end
end


reg signed [31:0] media_da_diferenca;

//=======================
//ADICIONA AO REGISTRADOR
//=======================
always @(posedge clk or posedge reset) begin
    if(reset) begin
        media_da_diferenca <= 0;
    end else begin
        if(mov) begin
            media_da_diferenca <= media;
        end
    end
end

//=====================
//TOLERAVEL
//=====================
wire toleravel = (media_da_diferenca < tolerancia);

wire AND = mov && toleravel;

wire signed [7:0] letra_f = AND     ?   letra_acel  :   letra_base;

//======================
//ADICIONA O RESULTADO
//======================
always @(posedge clk or posedge reset) begin
    if(reset) begin
        letra_final <= 0;
    end else begin
        if(mov) begin
            letra_final <= letra_f;
        end
    end
end




endmodule
