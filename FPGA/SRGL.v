module SRGL(
    input wire clk,
    input wire reset,
    input wire mov, //flag de movimentacao

    input wire signed [31:0] mpu_valor, //30 valores do acelerometro -> 30x32(bits) = 960 bits
    input wire [7:0] letra_base, //letra base vindo do ldr
    input wire mpu_valid,           // Flag que indica que mpu_valor é válido neste ciclo

    output reg ready,
    output reg [7:0] letra_final
);

// ========================================================================
// 1. CONSTANTES E DEFINIÇÕES (PONTO FIXO: ESCALA x100)
// ========================================================================
// Tolerâncias multiplicadas por 100
localparam signed [31:0] TOLERANCIA_Z = 150; // 3.0 * 100
localparam signed [31:0] TOLERANCIA_J = 425; // 4.25 * 100
localparam signed [31:0] TOLERANCIA_H = 150; // 1.5 * 100
localparam signed [31:0] TOLERANCIA_V = 500; // 5.0 * 100
localparam signed [31:0] TOLERANCIA_T = 450; // 4.5 * 100
localparam signed [31:0] TOLERANCIA_K = 150; // 1.5 * 100
localparam signed [31:0] TOLERANCIA_G = 450; // 4.5 * 100


reg signed [31:0] mem_rom [29:0];
reg signed [31:0] mem_ram [29:0];

integer i;

//==============================================
//PREENCHE BUFFER
//==============================================
// Como não podemos receber 960 bits de uma vez, vamos encher a RAM sequencialmente.
reg [4:0] write_ptr; // Ponteiro para escrever na RAM (0 a 29)
reg buffer_full;     // Indica que recebemos as 30 amostras

always @(posedge clk or posedge reset) begin
    if (reset) begin
        write_ptr <= 0;
        buffer_full <= 0;
    end else begin
        if (mpu_valid && !buffer_full) begin
            mem_ram[write_ptr] <= mpu_valor;
            if (write_ptr == 29) begin
                buffer_full <= 1; // Buffer cheio, pode processar
            end else begin
                write_ptr <= write_ptr + 1;
            end
        end
        
        // Reset do buffer se o movimento acabar (opcional, depende do protocolo)
        if (!mov) begin 
            write_ptr <= 0;
            buffer_full <= 0;
        end
    end
end

reg signed [31:0] tolerancia_atual;
reg [7:0] letra_alvo_movimento; // Qual letra vira se o movimento for válido?
reg verificar_movimento;

// ========================================================================
// LÓGICA DE SELEÇÃO DE MODELO
// ========================================================================
always @(*) begin
        // Valores padrão (se não houver movimento associado)
        verificar_movimento = 0;
        tolerancia_atual = 0;
        letra_alvo_movimento = letra_base;
        
        // Zera o modelo
        for(i=0; i<30; i=i+1) mem_rom[i] = 0;

        case (letra_base)
            "D": begin // Se base é D, testamos se é Z
                verificar_movimento = 1;
                letra_alvo_movimento = "Z";
                tolerancia_atual = TOLERANCIA_Z;
                // Carrega MODELO_Z_X (Valores x100)
                mem_rom[0] = -865; mem_rom[1] = -854; mem_rom[2] = -685;
                mem_rom[3] = -813; mem_rom[4] = -809; mem_rom[5] = -784;
                mem_rom[6] = -836; mem_rom[7] = -781; mem_rom[8] = -598;
                mem_rom[9] = -341; mem_rom[10] = -313; mem_rom[11] = -347;
                mem_rom[12] = -270; mem_rom[13] = -225; mem_rom[14] = -209;
                mem_rom[15] = -283; mem_rom[16] = -472; mem_rom[17] = -886;
                mem_rom[18] = -1141; mem_rom[19] = -873; mem_rom[20] = -757;
                mem_rom[21] = -656; mem_rom[22] = -509; mem_rom[23] = -349;
                mem_rom[24] = -352; mem_rom[25] = -358; mem_rom[26] = -522;
                mem_rom[27] = -556; mem_rom[28] = -612; mem_rom[29] = -550;
            end

            "H": begin
                verificar_movimento = 1;
                letra_alvo_movimento = "K";
                tolerancia_atual = TOLERANCIA_K;
                // Carrega MODELO_K_X (Valores x100)
                mem_rom[0] = -674; mem_rom[1] = -601; mem_rom[2] = -628;
                mem_rom[3] = -549; mem_rom[4] = -586; mem_rom[5] = -576;
                mem_rom[6] = -615; mem_rom[7] = -594; mem_rom[8] = -637;
                mem_rom[9] = -592; mem_rom[10] = -685; mem_rom[11] = -577;
                mem_rom[12] = -470; mem_rom[13] = -329; mem_rom[14] = -176;
                mem_rom[15] = -332; mem_rom[16] = -116; mem_rom[17] = -313;
                mem_rom[18] = -470; mem_rom[19] = -513; mem_rom[20] = -469;
                mem_rom[21] = -567; mem_rom[22] = -563; mem_rom[23] = -540;
                mem_rom[24] = -502; mem_rom[25] = -448; mem_rom[26] = -470;
                mem_rom[27] = -381; mem_rom[28] = -353; mem_rom[29] = -380;
            end

            "F": begin
                verificar_movimento = 1;
                letra_alvo_movimento = "T";
                tolerancia_atual = TOLERANCIA_T;
                // Carrega MODELO_T_X (Valores x100)
                mem_rom[0] = -642; mem_rom[1] = -535; mem_rom[2] = -446;
                mem_rom[3] = -482; mem_rom[4] = -432; mem_rom[5] = -520;
                mem_rom[6] = -491; mem_rom[7] = -398; mem_rom[8] = -397;
                mem_rom[9] = -216; mem_rom[10] = -185; mem_rom[11] = -165;
                mem_rom[12] = -1; mem_rom[13] = 164; mem_rom[14] = 248;
                mem_rom[15] = 340; mem_rom[16] = 441; mem_rom[17] = 690;
                mem_rom[18] = 600; mem_rom[19] = 540; mem_rom[20] = 476;
                mem_rom[21] = 450; mem_rom[22] = 448; mem_rom[23] = 419;
                mem_rom[24] = 330; mem_rom[25] = 439; mem_rom[26] = 20;
                mem_rom[27] = -165; mem_rom[28] = -433; mem_rom[29] = -420;
            end
            
            "L": begin
                verificar_movimento = 1;
                letra_alvo_movimento = "G";
                tolerancia_atual = TOLERANCIA_G;
                // Carrega MODELO_T_X (Valores x100)
                mem_rom[0] = -503; mem_rom[1] = -376; mem_rom[2] = -276;
                mem_rom[3] = -275; mem_rom[4] = -311; mem_rom[5] = -617;
                mem_rom[6] = -2; mem_rom[7] = -482; mem_rom[8] = -28;
                mem_rom[9] = 199; mem_rom[10] = 489; mem_rom[11] = 540;
                mem_rom[12] = 682; mem_rom[13] = 743; mem_rom[14] = 846;
                mem_rom[15] = 787; mem_rom[16] = 808; mem_rom[17] = 627;
                mem_rom[18] = 491; mem_rom[19] = 397; mem_rom[20] = 253;
                mem_rom[21] = 206; mem_rom[22] = 32; mem_rom[23] = -64;
                mem_rom[24] = -234; mem_rom[25] = -178; mem_rom[26] = -192;
                mem_rom[27] = -361; mem_rom[28] = -354; mem_rom[29] = -381;
            end
        endcase
end

wire signed [31:0] sub, soma_indice, media;


// ========================================================================
// CÁLCULO MATEMÁTICO  (pensar em como fazer pipeline)
// ========================================================================

reg signed [31:0] soma_buffer;
reg signed [31:0] soma_modelo;
reg signed [31:0] media_buffer;
reg signed [31:0] media_modelo;
reg signed [31:0] diff_norm;
reg signed [31:0] soma_erro_abs;
reg signed [31:0] erro_final;

always @(*) begin
    soma_buffer = 0;
    soma_modelo = 0;
    soma_erro_abs = 0;

    // 1. Calcular Médias
    for (i = 0; i < 30; i = i + 1) begin
        soma_buffer = soma_buffer + mem_ram[i];
        soma_modelo = soma_modelo + mem_rom[i];
    end
    
    media_buffer = soma_buffer / 30;
    media_modelo = soma_modelo / 30;

    // 2. Calcular Diferença Absoluta Normalizada
    for (i = 0; i < 30; i = i + 1) begin
        // (Buffer - MediaB) - (Modelo - MediaM)
        diff_norm = (mem_ram[i] - media_buffer) - (mem_rom[i] - media_modelo);
        
        // Função ABS (Valor Absoluto)
        if (diff_norm < 0) 
            soma_erro_abs = soma_erro_abs - diff_norm;
        else
            soma_erro_abs = soma_erro_abs + diff_norm;
    end

    // 3. Média do Erro
    erro_final = soma_erro_abs / 30;
end


// ========================================================================
// SAÍDA REGISTRADA
// ========================================================================
always @(posedge clk or posedge reset) begin
    if (reset) begin
        letra_final <= "?";
        ready <= 0;
    end else begin
        if (buffer_full) begin
            ready <= 1; // Indica que o resultado é válido
            
            // Lógica de decisão
            if (mov && verificar_movimento && (erro_final < tolerancia_atual)) begin
                letra_final <= letra_alvo_movimento; // Troca (ex: D -> Z)
            end else begin
                letra_final <= letra_base; // Mantém original
            end
        end else begin
            ready <= 0; // Ainda recebendo dados
            letra_final <= letra_base; 
        end
    end
end




endmodule
