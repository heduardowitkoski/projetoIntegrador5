`timescale 1ns / 1ps

module tb_SRGL;

    // ========================================================================
    // 1. Sinais de Teste (Regs para entradas, Wires para saídas)
    // ========================================================================
    reg clk;
    reg reset;
    reg mov;
    reg signed [31:0] mpu_valor;
    reg mpu_valid;
    reg [7:0] letra_base;

    wire [7:0] letra_final;
    wire ready;

    // ========================================================================
    // 2. Instanciação do DUT (Device Under Test)
    // ========================================================================
    SRGL uut (
        .clk(clk), 
        .reset(reset), 
        .mov(mov), 
        .mpu_valor(mpu_valor), 
        .mpu_valid(mpu_valid), 
        .letra_base(letra_base), 
        .letra_final(letra_final), 
        .ready(ready)
    );

    // ========================================================================
    // 3. Geração de Clock (100MHz -> Período 10ns)
    // ========================================================================
    always #5 clk = ~clk;

    // ========================================================================
    // 4. Arrays de Dados para Teste
    // ========================================================================
    reg signed [31:0] padrao_Z [0:29];
    integer k;

    // Inicializa o array com o padrão Z correto (copiado do seu código)
    initial begin
        padrao_Z[0] = -865; padrao_Z[1] = -854; padrao_Z[2] = -685;
        padrao_Z[3] = -813; padrao_Z[4] = -809; padrao_Z[5] = -784;
        padrao_Z[6] = -836; padrao_Z[7] = -781; padrao_Z[8] = -598;
        padrao_Z[9] = -341; padrao_Z[10] = -313; padrao_Z[11] = -347;
        padrao_Z[12] = -270; padrao_Z[13] = -225; padrao_Z[14] = -209;
        padrao_Z[15] = -283; padrao_Z[16] = -472; padrao_Z[17] = -886;
        padrao_Z[18] = -1141; padrao_Z[19] = -873; padrao_Z[20] = -757;
        padrao_Z[21] = -656; padrao_Z[22] = -509; padrao_Z[23] = -349;
        padrao_Z[24] = -352; padrao_Z[25] = -358; padrao_Z[26] = -522;
        padrao_Z[27] = -556; padrao_Z[28] = -612; padrao_Z[29] = -550;
    end

    // ========================================================================
    // 5. Processo de Estímulo
    // ========================================================================
    initial begin
        // --- Inicialização ---
        clk = 0;
        reset = 1;
        mov = 0;
        mpu_valid = 0;
        mpu_valor = 0;
        letra_base = "?";
        
        $display("Iniciando Simulacao...");
        
        // Espera 100ns e solta o reset
        #100;
        reset = 0;
        #20;

        // --------------------------------------------------------------------
        // TESTE 1: Validar movimento Z (Entrada = Padrão Z, Base = D)
        // --------------------------------------------------------------------
        $display("\n--- TESTE 1: Injetando padrao Z (Esperado: D -> Z) ---");
        
        letra_base = "D"; // ASCII 0x44
        mov = 1;          // Movimento detectado
        #10;

        // Loop para enviar os 30 dados serialmente
        for (k = 0; k < 30; k = k + 1) begin
            @(posedge clk); 
            mpu_valor = padrao_Z[k]; // Coloca o dado na entrada
            mpu_valid = 1;        // Sinaliza que é válido
            
            @(posedge clk);       // Espera um clock para o DUT ler
            mpu_valid = 0;        // Baixa o valid (opcional, pode ser back-to-back)
        end

        // Aguarda o sinal de READY indicando fim do cálculo
        wait(ready == 1);
        #20; // Espera um pouco para estabilizar visualização

        // Verificação
        if (letra_final == "Z") 
            $display("SUCESSO: Letra final mudou para Z!");
        else 
            $display("FALHA: Letra final ficou %c (Esperado Z)", letra_final);

        // Reset para o próximo teste
        mov = 0;
        #100;
        reset = 1; // Pulso de reset para limpar RAM/Estados
        #20;
        reset = 0;
        #20;

        // --------------------------------------------------------------------
        // TESTE 2: Validar Ruído (Entrada = Ruído, Base = D)
        // --------------------------------------------------------------------
        $display("\n--- TESTE 2: Injetando Ruido (Esperado: D -> D) ---");
        
        letra_base = "D";
        mov = 1;
        #10;

        // Loop para enviar dados ruidosos (valores constantes altos, ex: 5000)
        // Isso deve gerar um erro grande e NÃO ativar a troca para Z
        for (k = 0; k < 30; k = k + 1) begin
            @(posedge clk); 
            mpu_valor = 32'd5000; // Valor muito diferente do modelo Z
            mpu_valid = 1;
            
            @(posedge clk);
            mpu_valid = 0;
        end

        wait(ready == 1);
        #20;

        // Verificação
        if (letra_final == "D") 
            $display("SUCESSO: Letra manteve-se D (Ruido rejeitado)!");
        else 
            $display("FALHA: Letra mudou para %c (Deveria manter D)", letra_final);

        $display("\nFim da Simulacao.");
        $finish;
    end

endmodule